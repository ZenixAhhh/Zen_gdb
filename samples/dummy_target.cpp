#include <windows.h>
#include <iostream>

int main() {
    std::cout << "[TARGET] Dummy Target process running. PID: " << GetCurrentProcessId() << "\n";

    // Anti-debug detection check
    if (IsDebuggerPresent()) {
        std::cout << "[TARGET] [!] Debugger detected via IsDebuggerPresent()!\n";
    } else {
        std::cout << "[TARGET] [*] No debugger detected (IsDebuggerPresent returned false).\n";
    }

    std::cout << "[TARGET] Executing simulated payload loop...\n";
    volatile int counter = 0;
    for (int i = 0; i < 5; ++i) {
        counter += (i * 2 + 1);
        std::cout << "[TARGET] Step " << i << " counter: " << counter << "\n";
        Sleep(500);
    }

    std::cout << "[TARGET] Target execution completed.\n";
    return 0;
}
