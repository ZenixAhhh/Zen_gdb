#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>
#include "registers.hpp"

namespace zen {

enum class DebuggerState {
    Uninitialized,
    Running,
    Paused,
    Terminated
};

class Debugger {
public:
    Debugger();
    ~Debugger();

    Debugger(const Debugger&) = delete;
    Debugger& operator=(const Debugger&) = delete;

    // Process spawning & attachment
    bool spawn(const std::wstring& application_path, const std::wstring& cmdline_args = L"");
    bool attach(DWORD pid);
    void detach();
    void terminate();

    // Main Win32 debug event dispatch loop
    void run();

    // Context & stepping operations
    bool read_current_context(CONTEXT& ctx);
    bool write_current_context(const CONTEXT& ctx);
    void print_current_registers();
    void single_step();
    void continue_execution();

    // Accessors
    DebuggerState get_state() const { return m_state; }
    DWORD get_target_pid() const { return m_target_pid; }
    HANDLE get_process_handle() const { return m_process_handle; }
    HANDLE get_thread_handle(DWORD tid) const;

private:
    // Event loop dispatchers
    void dispatch_event(const DEBUG_EVENT& event);
    void handle_create_process(const CREATE_PROCESS_DEBUG_INFO& info, DWORD tid);
    void handle_exit_process(const EXIT_PROCESS_DEBUG_INFO& info);
    void handle_create_thread(const CREATE_THREAD_DEBUG_INFO& info, DWORD tid);
    void handle_exit_thread(const EXIT_THREAD_DEBUG_INFO& info, DWORD tid);
    void handle_load_dll(const LOAD_DLL_DEBUG_INFO& info);
    void handle_unload_dll(const UNLOAD_DLL_DEBUG_INFO& info);
    void handle_exception(const EXCEPTION_DEBUG_INFO& info, DWORD tid);

    // Interactive CLI shell when execution pauses
    void enter_cli();

private:
    DebuggerState m_state{DebuggerState::Uninitialized};
    DWORD m_target_pid{0};
    DWORD m_current_tid{0};
    HANDLE m_process_handle{nullptr};
    std::unordered_map<DWORD, HANDLE> m_thread_handles;

    void* m_image_base{nullptr};
    void* m_entry_point{nullptr};

    bool m_is_initial_breakpoint{true};
    bool m_single_stepping{false};
    DWORD m_last_continue_status{DBG_CONTINUE};
};

} // namespace zen
