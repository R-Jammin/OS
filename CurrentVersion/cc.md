# Overview
This is a simple, single-pass compiler written in C++ (targeting x86 assembly output), designed for a custom operating system called "ikanOS". It compiles a highly restricted subset of C-like code into NASM-style x86 assembly (using int 0x80 for syscalls, suggesting a Linux-like syscall interface adapted for ikanOS). The compiler is self-contained, with lexical analysis, parsing, and code generation integrated. It processes input from a file (e.g., via command-line argument), generates assembly in a temporary buffer, and outputs to a .s file. Error handling is rudimentary: it prints colored messages to the screen (using custom print functions) with line numbers and exits on failures.
The compiler's primary goal appears to be educational or for a minimal OS environment, as it lacks many features of a full C compiler (e.g., no complex expressions, limited control flow, no function parameters for user-defined functions). It implicitly declares variables as int if not explicitly declared and supports basic stack management for locals.
Supported Language Features (Input Language)

## Capabilities
1. Variable Declarations and Types

Basic types: int, char, void.
Pointers: Limited to int* (e.g., int* ptr;). char is often used for string pointers.
Declarations can include initialization:
Numeric: int x = 5;
String literals for char: char str = "hello"; (places string in data section).
Syscall-based: int fd = syscall(5, "file.txt", 0); (e.g., open file).

Implicit declaration: If a variable is used without declaration, it's treated as int and added to the symbol table automatically.
Scope: Function-local only (no globals). Up to 64 symbols per function (hard limit).
No arrays, structs, enums, or unions.

2. Expressions

Very limited: Only literals (decimal/hex numbers, string literals), identifiers, and chains of additions like id + number + number (e.g., x + 5 + 3).
No operator precedence, no subtraction/multiplication/division, no logical operators except in specific control structures.
Dereferencing: Limited to assignments like *ptr = expr;.
String literals: Only in char initializations (e.g., generates db 'string' in data section).

3. Assignments

Simple: id = expr; (e.g., x = y + 5;).
Dereference: *ptr = expr; (stores to the address held in ptr).
Syscall-based: id = syscall(number, expr1, expr2); (e.g., fd = syscall(5, "file.txt", 0); for file open).
No compound assignments (e.g., no +=).

4. Control Flow

If-Else: if (id != id) { ... } else { ... }
Condition limited to two identifiers compared with !=.
Generates cmp and je (jump if equal) assembly.

While: while (id < number) { ... }
Condition limited to identifier < literal number.
Generates cmp and jge (jump if greater or equal) assembly.

No for, switch, do-while, or other loops.
No break, continue, or goto.

5. Functions

Definitions:
Void functions: void func() { ... } (no parameters, no return value).
Main: int main() { ... } (entry point, labeled _start in assembly).

Calls:
To defined functions: func(); (no args).
To external/undefined functions: external( expr1, expr2, ... ); (up to 5 args, pushes args in reverse order, cleans stack).

Up to 64 functions total (hard limit).
No recursion (not explicitly prevented, but stack management is basic).
Return: return expr; (only in main or void functions; sets exit code via syscall 1).
Prologue/Epilogue: Automatically added for functions with locals (stack allocation via sub esp, N).

6. Syscalls

Explicit: syscall(number, expr1, expr2); (sets EAX=number, EBX=expr1, ECX=expr2, invokes int 0x80).
Limited to exactly two arguments (plus syscall number).
Used for OS interactions (e.g., file I/O, exit). The compiler itself uses syscalls for file operations in its main().

7. Other Features

Comments: Line comments with //.
Literals: Decimal/hex numbers (e.g., 0xFF), strings (e.g., "text").
Program Structure: Top-level consists of void functions and one int main().
No includes, macros, or preprocessor.

8. Limitations in Parsing

No nested expressions beyond simple additions.
Conditions are hardcoded (e.g., no >, ==, or complex booleans).
No function parameters or return types beyond void and int for main.
Error recovery is poor: On parse errors, it prints a message and continues (may produce invalid output).
Hard limits: 1000 tokens, 4096 bytes asm output, 500-byte strings, etc.

## Output Capabilities (Generated Assembly)

Format: NASM-compatible x86 assembly (32-bit, using registers like EAX, EBX, ECX).
Sections:
.data: For string literals (e.g., str: db 'hello').
.text: Code, with global labels for functions (e.g., _start: for main).

Code Generation:
Stack-based locals: Offsets from EBP (e.g., mov [ebp-4], eax).
Expressions: Evaluated into EAX (e.g., mov eax, 5; add eax, 3).
Calls: Pushes caller-saved registers (EAX, ECX, EDX) for external calls.
Syscalls: Preserves registers with pusha/popa.
Optimizations: None (straightforward translation).

Entry Point: _start (no standard C runtime).
File Output: Writes to <input>.s (without extension), using a temp file for buffering.

## Runtime/Environment Assumptions

Targets ikanOS: Uses custom libc functions (e.g., printString, systemOpenFile) and memory locations (e.g., RUNNING_PID_LOC).
Syscalls: Assumes int 0x80 interface (e.g., exit=1, open=5).
No floating-point, no advanced instructions.
Debugging: Prints compile-time messages to screen (colored, with line numbers).

Compiler's Own Capabilities (as a Program)

File Handling: Reads input file, creates temp file, outputs .s file, deletes temp.
Error Reporting: Screen-based (e.g., "Unexpected token" at line X).
Performance: Single-pass, in-memory buffers (up to 4KB asm output).
Extensibility: Code is modular (e.g., separate parsers for expressions, statements), but tightly coupled.