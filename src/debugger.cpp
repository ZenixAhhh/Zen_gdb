#include "debugger.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <iomanip>

namespace zen {

Debugger::Debugger() = default;

Debugger::~Debugger() {
    terminate();
}

bool Debugger::spawn(const std::wstring& application_path, const std::wstring& cmdline_args) {
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::wstring full_command = L"\"" + application_path + L"\" " + cmdline_args;
    std::vector<wchar_t> cmd_buffer(full_command.begin(), full_command.end());
    cmd_buffer.push_back(L'\0');

    std::wcout << L"[*] Launching target process: " << application_path << L"\n";

    BOOL success = CreateProcessW(
        nullptr,
        cmd_buffer.data(),
        nullptr,
        nullptr,
        FALSE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success) {
        std::cerr << "[!] CreateProcessW failed with error code: " << GetLastError() << "\n";
        return false;
    }

    m_target_pid = pi.dwProcessId;
    m_process_handle = pi.hProcess;
    m_thread_handles[pi.dwThreadId] = pi.hThread;
    m_current_tid = pi.dwThreadId;
    m_state = DebuggerState::Running;
    m_is_initial_breakpoint = true;

    std::cout << "[+] Process launched successfully. Target PID: " << m_target_pid 
              << " (Main TID: " << pi.dwThreadId << ")\n";

    return true;
}

bool Debugger::attach(DWORD pid) {
    std::cout << "[*] Attaching to PID: " << pid << "...\n";

    if (!DebugActiveProcess(pid)) {
        std::cerr << "[!] DebugActiveProcess failed with error code: " << GetLastError() << "\n";
        return false;
    }

    m_target_pid = pid;
    m_process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    m_state = DebuggerState::Running;
    m_is_initial_breakpoint = true;

    std::cout << "[+] Attached to process " << pid << " successfully.\n";
    return true;
}

void Debugger::detach() {
    if (m_target_pid != 0) {
        DebugActiveProcessStop(m_target_pid);
        std::cout << "[*] Detached from target PID: " << m_target_pid << "\n";
    }
}

void Debugger::terminate() {
    if (m_process_handle) {
        TerminateProcess(m_process_handle, 0);
        CloseHandle(m_process_handle);
        m_process_handle = nullptr;
    }

    for (auto& [tid, hThread] : m_thread_handles) {
        if (hThread && hThread != INVALID_HANDLE_VALUE) {
            CloseHandle(hThread);
        }
    }
    m_thread_handles.clear();
    m_state = DebuggerState::Terminated;
}

HANDLE Debugger::get_thread_handle(DWORD tid) const {
    auto it = m_thread_handles.find(tid);
    if (it != m_thread_handles.end()) {
        return it->second;
    }
    return nullptr;
}

bool Debugger::read_current_context(CONTEXT& ctx) {
    HANDLE hThread = get_thread_handle(m_current_tid);
    if (!hThread) {
        std::cerr << "[!] Invalid thread handle for TID: " << m_current_tid << "\n";
        return false;
    }
    return get_thread_context(hThread, ctx);
}

bool Debugger::write_current_context(const CONTEXT& ctx) {
    HANDLE hThread = get_thread_handle(m_current_tid);
    if (!hThread) {
        std::cerr << "[!] Invalid thread handle for TID: " << m_current_tid << "\n";
        return false;
    }
    return set_thread_context(hThread, ctx);
}

void Debugger::print_current_registers() {
    CONTEXT ctx{};
    if (read_current_context(ctx)) {
        print_registers(ctx);
    }
}

void Debugger::single_step() {
    CONTEXT ctx{};
    if (read_current_context(ctx)) {
        set_trap_flag(ctx, true);
        write_current_context(ctx);
        m_single_stepping = true;
    }
}

void Debugger::run() {
    DEBUG_EVENT debug_event{};

    std::cout << "[*] Starting Win32 debug event loop...\n";

    while (m_state != DebuggerState::Terminated) {
        if (!WaitForDebugEvent(&debug_event, INFINITE)) {
            std::cerr << "[!] WaitForDebugEvent failed (Error: " << GetLastError() << ")\n";
            break;
        }

        m_current_tid = debug_event.dwThreadId;
        m_last_continue_status = DBG_CONTINUE;

        dispatch_event(debug_event);

        if (m_state == DebuggerState::Terminated) {
            ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, m_last_continue_status);
            break;
        }

        ContinueDebugEvent(debug_event.dwProcessId, debug_event.dwThreadId, m_last_continue_status);
    }

    std::cout << "[*] Debug session ended.\n";
}

void Debugger::dispatch_event(const DEBUG_EVENT& event) {
    switch (event.dwDebugEventCode) {
    case CREATE_PROCESS_DEBUG_EVENT:
        handle_create_process(event.u.CreateProcessInfo, event.dwThreadId);
        break;

    case EXIT_PROCESS_DEBUG_EVENT:
        handle_exit_process(event.u.ExitProcess);
        break;

    case CREATE_THREAD_DEBUG_EVENT:
        handle_create_thread(event.u.CreateThread, event.dwThreadId);
        break;

    case EXIT_THREAD_DEBUG_EVENT:
        handle_exit_thread(event.u.ExitThread, event.dwThreadId);
        break;

    case LOAD_DLL_DEBUG_EVENT:
        handle_load_dll(event.u.LoadDll);
        break;

    case UNLOAD_DLL_DEBUG_EVENT:
        handle_unload_dll(event.u.UnloadDll);
        break;

    case EXCEPTION_DEBUG_EVENT:
        handle_exception(event.u.Exception, event.dwThreadId);
        break;

    case RIP_EVENT:
        std::cerr << "[!] System RIP event received.\n";
        break;

    default:
        break;
    }
}

void Debugger::handle_create_process(const CREATE_PROCESS_DEBUG_INFO& info, DWORD tid) {
    m_process_handle = info.hProcess;
    m_thread_handles[tid] = info.hThread;
    m_image_base = info.lpBaseOfImage;
    m_entry_point = info.lpStartAddress;

    std::cout << "[+] Event: CREATE_PROCESS\n";
    std::cout << "    Base Address: 0x" << std::hex << (uintptr_t)m_image_base << "\n";
    std::cout << "    Entry Point : 0x" << std::hex << (uintptr_t)m_entry_point << std::dec << "\n";

    // Per Win32 API specification: close hFile handle immediately
    if (info.hFile) {
        CloseHandle(info.hFile);
    }
}

void Debugger::handle_exit_process(const EXIT_PROCESS_DEBUG_INFO& info) {
    std::cout << "[+] Event: EXIT_PROCESS (Exit Code: " << info.dwExitCode << ")\n";
    m_state = DebuggerState::Terminated;
}

void Debugger::handle_create_thread(const CREATE_THREAD_DEBUG_INFO& info, DWORD tid) {
    std::cout << "[+] Event: CREATE_THREAD (TID: " << tid << ")\n";
    m_thread_handles[tid] = info.hThread;
}

void Debugger::handle_exit_thread(const EXIT_THREAD_DEBUG_INFO& info, DWORD tid) {
    std::cout << "[+] Event: EXIT_THREAD (TID: " << tid << ", Exit Code: " << info.dwExitCode << ")\n";
    auto it = m_thread_handles.find(tid);
    if (it != m_thread_handles.end()) {
        CloseHandle(it->second);
        m_thread_handles.erase(it);
    }
}

void Debugger::handle_load_dll(const LOAD_DLL_DEBUG_INFO& info) {
    if (info.hFile) {
        CloseHandle(info.hFile);
    }
}

void Debugger::handle_unload_dll(const UNLOAD_DLL_DEBUG_INFO& info) {
    (void)info;
}

void Debugger::handle_exception(const EXCEPTION_DEBUG_INFO& info, DWORD tid) {
    DWORD code = info.ExceptionRecord.ExceptionCode;
    void* addr = info.ExceptionRecord.ExceptionAddress;
    bool first_chance = (info.dwFirstChance != 0);

    // Initial system loader breakpoint triggered by Windows kernel
    if (code == EXCEPTION_BREAKPOINT && m_is_initial_breakpoint) {
        m_is_initial_breakpoint = false;
        std::cout << "\n[+] Initial system breakpoint hit at 0x" 
                  << std::hex << (uintptr_t)addr << std::dec << " (TID: " << tid << ")\n";
        m_state = DebuggerState::Paused;
        print_current_registers();
        enter_cli();
        return;
    }

    // Single step exception (Trap Flag TF or Hardware Breakpoint)
    if (code == EXCEPTION_SINGLE_STEP) {
        m_single_stepping = false;
        std::cout << "\n[+] Single step trap at 0x" 
                  << std::hex << (uintptr_t)addr << std::dec << " (TID: " << tid << ")\n";
        m_state = DebuggerState::Paused;
        print_current_registers();
        enter_cli();
        return;
    }

    // Software Breakpoint (0xCC / INT 3)
    if (code == EXCEPTION_BREAKPOINT) {
        std::cout << "\n[+] Software Breakpoint hit at 0x" 
                  << std::hex << (uintptr_t)addr << std::dec << " (TID: " << tid << ")\n";
        
        // x86 CPU advances RIP past the 0xCC byte before firing interrupt; rewind by 1
        CONTEXT ctx{};
        if (read_current_context(ctx)) {
            set_rip(ctx, get_rip(ctx) - 1);
            write_current_context(ctx);
        }

        m_state = DebuggerState::Paused;
        print_current_registers();
        enter_cli();
        return;
    }

    std::cout << "\n[!] Exception 0x" << std::hex << code << " at 0x" 
              << (uintptr_t)addr << std::dec << " (First chance: " << first_chance << ")\n";

    if (!first_chance) {
        std::cerr << "[!] Unhandled exception in target process!\n";
        m_last_continue_status = DBG_EXCEPTION_NOT_HANDLED;
    }

    m_state = DebuggerState::Paused;
    print_current_registers();
    enter_cli();
}

void Debugger::enter_cli() {
    std::string line;
    while (m_state == DebuggerState::Paused) {
        std::cout << "(zen) > ";
        if (!std::getline(std::cin, line)) {
            terminate();
            break;
        }

        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        line = line.substr(first);

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "c" || cmd == "continue") {
            m_state = DebuggerState::Running;
            break;
        } else if (cmd == "s" || cmd == "step") {
            single_step();
            m_state = DebuggerState::Running;
            break;
        } else if (cmd == "r" || cmd == "regs") {
            print_current_registers();
        } else if (cmd == "q" || cmd == "quit" || cmd == "kill") {
            std::cout << "[*] Terminating debuggee and exiting...\n";
            terminate();
            break;
        } else if (cmd == "help" || cmd == "?") {
            std::cout << "\nAvailable Commands:\n";
            std::cout << "  c, continue    - Resume execution\n";
            std::cout << "  s, step        - Single step instruction (TF)\n";
            std::cout << "  r, regs        - Display current registers and flags\n";
            std::cout << "  q, quit, kill  - Terminate target process and exit debugger\n";
            std::cout << "  help, ?        - Show this help message\n\n";
        } else {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for available commands.\n";
        }
    }
}

} // namespace zen
