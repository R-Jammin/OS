# Overview
This code implements a simple two-pass assembler for a custom operating system called ikanOS. It processes assembly source code, resolves labels and external references, generates x86 machine code, and outputs an ELF-formatted executable. The assembler targets 32-bit x86 architecture and integrates with a shared library (libc.o) for external functions. It runs as a user-space program in ikanOS, using system calls for file operations and memory management. The code is written in C and uses custom libraries like libc-main.h for string and memory utilities.
## Main Features
1. Two-Pass Assembly Process

First Pass: Scans the source code to collect labels (e.g., start:) and calculate instruction sizes and section offsets (.text and .data). It builds a label table with names, addresses, and sections without generating code.
Second Pass: Generates machine code by emitting opcodes and operands, resolving label addresses and external library functions. It handles forward references using the label table from the first pass.

2. Supported Instructions and Operands

Arithmetic and Data Movement:
mov: Supports register-register, register-memory, memory-register, register-immediate, and memory-immediate.
add and sub: Similar operand support as mov, including immediates and memory.
cmp: For comparisons, supporting register-register, register-immediate, and memory-immediate.

Control Flow:
jmp: Unconditional jump to labels (32-bit relative).
Conditional jumps: jne (not equal), jle (less or equal), jge (greater or equal) to labels (32-bit relative).
call: To labels, registers, or external library functions (relative or via register).
ret: Simple return from subroutine.

Stack Operations:
push and pop: For registers.
pusha and popa: Push/pop all general-purpose registers.

Interrupts:
int: Software interrupt with an immediate value (e.g., int 0x80).

Data Definition:
db: Defines byte data, primarily for strings in the .data section (e.g., db 'hello').

Operand Types:
Registers: 32-bit general-purpose (eax, ecx, edx, ebx, esp, ebp, esi, edi).
Immediates: Decimal or hex (e.g., 42, 0x2A, -10), with sign support.
Memory: Simple base+offset like [ebp-4] or [eax+8] (limited to ebp or eax as base).
Labels: Resolved to addresses in code or data sections.


3. Section Handling

Supports .text (or .txt) for code and .data for data.
Switches sections with section .data or section .text.
Code is emitted to a buffer for .text, data to a separate buffer for .data.
Aligns sections to page boundaries (4096 bytes) in the output ELF.

4. Label and Symbol Resolution

Labels are defined with label: and can be local to sections.
Supports up to 256 labels.
Resolves internal labels to relative or absolute addresses (base: 0x00402000 for code, adjusted for data).
External references: Looks up functions in a shared jump table from libc.o (e.g., via findLibFunctionAddr).

5. ELF Output Generation

Creates a 32-bit ELF executable with three loadable segments:
Header segment (ELF header + program headers, read-only).
Text segment (code, read-execute).
Data segment (data/BSS, read-write).

Virtual addresses start at 0x00401000.
Entry point defaults to start label or offset 0 in code.
Flags for dynamic loading if external labels are used (loads libc.o at 0x700000).
Pads segments to page size and handles file/memory sizes.

6. Input/Output and Environment Integration

Reads assembly source from a file specified in COMMAND_BUFFER.
Maps libc.o into memory for symbol resolution.
Outputs to a temporary file (.al.tmp), then creates the final binary.
Cleans up temporary files and closes descriptors using system calls.
Prints debug/status messages to the screen (e.g., label additions, pass execution).

7. Error Handling and Limits

Basic error checks: File open failures, buffer overflows (max 4096 bytes per section, 256 labels, 256-byte lines).
Exits with custom codes (e.g., PROCESS_EXIT_CODE_INPUT_FILE_FAILED).
Detects malformed output (e.g., if ELF header overwrites code).

## Capabilities

What It Can Do:
Assemble simple programs with basic control flow, data manipulation, and system calls (via int).
Link with libc.o for OS-provided functions (e.g., print, file ops).
Produce standalone ELF executables runnable in ikanOS.
Handle strings and constants in data sections for programs like "hello world".
Support stack-based operations for functions and local variables.
Resolve forward/backward jumps for loops and conditionals.

Use Cases:
Compiling small utilities or test programs for ikanOS.
Educational tool for learning x86 assembly basics.
Bootstrapping simple applications in a minimal OS environment.