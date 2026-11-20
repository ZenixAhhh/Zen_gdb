#include "registers.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace zen {

bool get_thread_context(HANDLE hThread, CONTEXT& ctx) {
    std::memset(&ctx, 0, sizeof(CONTEXT));
    ctx.ContextFlags = CONTEXT_ALL;
    if (!GetThreadContext(hThread, &ctx)) {
        std::cerr << "[!] GetThreadContext failed (Error: " << GetLastError() << ")\n";
        return false;
    }
    return true;
}

bool set_thread_context(HANDLE hThread, const CONTEXT& ctx) {
    if (!SetThreadContext(hThread, &ctx)) {
        std::cerr << "[!] SetThreadContext failed (Error: " << GetLastError() << ")\n";
        return false;
    }
    return true;
}

uint64_t get_rip(const CONTEXT& ctx) {
    return ctx.Rip;
}

void set_rip(CONTEXT& ctx, uint64_t rip) {
    ctx.Rip = rip;
}

uint64_t get_rsp(const CONTEXT& ctx) {
    return ctx.Rsp;
}

uint64_t get_rbp(const CONTEXT& ctx) {
    return ctx.Rbp;
}

uint64_t get_rflags(const CONTEXT& ctx) {
    return ctx.EFlags;
}

bool is_flag_set(uint64_t rflags, uint64_t mask) {
    return (rflags & mask) != 0;
}

void set_trap_flag(CONTEXT& ctx, bool enable) {
    if (enable) {
        ctx.EFlags |= flags::TF;
    } else {
        ctx.EFlags &= ~flags::TF;
    }
}

void print_registers(const CONTEXT& ctx) {
    auto hex16 = [](uint64_t val) {
        std::ostringstream ss;
        ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << val;
        return ss.str();
    };

    std::cout << "\n============================== [ REGISTERS ] ==============================\n";
    std::cout << "  RAX: " << hex16(ctx.Rax) << "  RBX: " << hex16(ctx.Rbx) << "\n";
    std::cout << "  RCX: " << hex16(ctx.Rcx) << "  RDX: " << hex16(ctx.Rdx) << "\n";
    std::cout << "  RSI: " << hex16(ctx.Rsi) << "  RDI: " << hex16(ctx.Rdi) << "\n";
    std::cout << "  RBP: " << hex16(ctx.Rbp) << "  RSP: " << hex16(ctx.Rsp) << "\n";
    std::cout << "  R8 : " << hex16(ctx.R8)  << "  R9 : " << hex16(ctx.R9)  << "\n";
    std::cout << "  R10: " << hex16(ctx.R10) << "  R11: " << hex16(ctx.R11) << "\n";
    std::cout << "  R12: " << hex16(ctx.R12) << "  R13: " << hex16(ctx.R13) << "\n";
    std::cout << "  R14: " << hex16(ctx.R14) << "  R15: " << hex16(ctx.R15) << "\n";
    std::cout << "  RIP: " << hex16(ctx.Rip) << "  RFL: " << hex16(ctx.EFlags) << "\n";

    std::cout << "  Flags: [";
    if (is_flag_set(ctx.EFlags, flags::CF)) std::cout << " CF";
    if (is_flag_set(ctx.EFlags, flags::ZF)) std::cout << " ZF";
    if (is_flag_set(ctx.EFlags, flags::SF)) std::cout << " SF";
    if (is_flag_set(ctx.EFlags, flags::OF)) std::cout << " OF";
    if (is_flag_set(ctx.EFlags, flags::AF)) std::cout << " AF";
    if (is_flag_set(ctx.EFlags, flags::PF)) std::cout << " PF";
    if (is_flag_set(ctx.EFlags, flags::TF)) std::cout << " TF";
    if (is_flag_set(ctx.EFlags, flags::IF)) std::cout << " IF";
    if (is_flag_set(ctx.EFlags, flags::DF)) std::cout << " DF";
    std::cout << " ]\n";
    std::cout << "===========================================================================\n\n";
}

} // namespace zen
