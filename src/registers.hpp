#pragma once

#include <windows.h>
#include <cstdint>
#include <string>

namespace zen {

// Read/write CPU register context for an active thread
bool get_thread_context(HANDLE hThread, CONTEXT& ctx);
bool set_thread_context(HANDLE hThread, const CONTEXT& ctx);

// Formatted display of 64-bit general purpose registers & condition flags
void print_registers(const CONTEXT& ctx);

// Register accessors & modifiers
uint64_t get_rip(const CONTEXT& ctx);
void set_rip(CONTEXT& ctx, uint64_t rip);

uint64_t get_rsp(const CONTEXT& ctx);
uint64_t get_rbp(const CONTEXT& ctx);
uint64_t get_rflags(const CONTEXT& ctx);

// Flag manipulation
bool is_flag_set(uint64_t rflags, uint64_t mask);
void set_trap_flag(CONTEXT& ctx, bool enable);

// x86_64 RFLAGS bit masks
namespace flags {
    constexpr uint64_t CF = 1ULL << 0;  // Carry
    constexpr uint64_t PF = 1ULL << 2;  // Parity
    constexpr uint64_t AF = 1ULL << 4;  // Auxiliary Carry
    constexpr uint64_t ZF = 1ULL << 6;  // Zero
    constexpr uint64_t SF = 1ULL << 7;  // Sign
    constexpr uint64_t TF = 1ULL << 8;  // Trap (Single Step)
    constexpr uint64_t IF = 1ULL << 9;  // Interrupt Enable
    constexpr uint64_t DF = 1ULL << 10; // Direction
    constexpr uint64_t OF = 1ULL << 11; // Overflow
}

} // namespace zen
