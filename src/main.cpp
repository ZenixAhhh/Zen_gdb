#include <iostream>
#include <string>
#include <vector>
#include "debugger.hpp"

void print_banner() {
    std::cout << R"(
  ______               _____     _ _     
 |___  /              / ____|   | | |    
    / / ___ _ __     | |  __  __| | |__  
   / / / _ \ '_ \    | | |_ |/ _` | '_ \ 
  / /_|  __/ | | |   | |__| | (_| | |_) |
 /_____\___|_| |_|    \_____|\__,_|_.__/  
  Malware Dynamic Analysis Debugger (x86_64)
    )" << "\n";
}

void print_usage(const wchar_t* prog_name) {
    std::wcout << L"Usage:\n";
    std::wcout << L"  " << prog_name << L" <path_to_executable> [target_args...]\n";
    std::wcout << L"  " << prog_name << L" -p <pid>\n\n";
    std::wcout << L"Commands inside debug prompt:\n";
    std::wcout << L"  c, continue  - Continue target execution\n";
    std::wcout << L"  s, step      - Single step one instruction (TF)\n";
    std::wcout << L"  r, regs      - Dump all registers and condition flags\n";
    std::wcout << L"  q, quit      - Kill target and exit\n";
}

int wmain(int argc, wchar_t* argv[]) {
    print_banner();

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    zen::Debugger dbg;
    std::wstring arg1 = argv[1];

    if (arg1 == L"-p" || arg1 == L"--pid") {
        if (argc < 3) {
            std::cerr << "[!] Error: PID required with -p option.\n";
            return 1;
        }
        DWORD pid = std::stoul(argv[2]);
        if (!dbg.attach(pid)) {
            return 1;
        }
    } else {
        std::wstring target_path = arg1;
        std::wstring cmdline_args = L"";
        for (int i = 2; i < argc; ++i) {
            if (!cmdline_args.empty()) cmdline_args += L" ";
            cmdline_args += argv[i];
        }

        if (!dbg.spawn(target_path, cmdline_args)) {
            return 1;
        }
    }

    dbg.run();
    return 0;
}
