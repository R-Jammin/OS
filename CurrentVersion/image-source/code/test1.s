section .data

message:
    db 'hello from ikanOS assembler'

section .text

_start:

    mov eax, 2
    push eax
    call systemShowInodeDetail
    
    mov eax, 3
    push eax
    call wait

    mov eax, 519
    push eax
    call systemBlockViewer

    mov eax, 3
    push eax
    call wait

    call clearScreen

    mov eax, 0xb8690
    mov ebx, 0x07690768
    mov [eax], ebx
    mov eax, 5
    push eax
    call wait

    call clearScreen

    mov eax, message
    push eax
    mov eax, 5
    push eax
    mov eax, 5
    push eax
    mov eax, 3
    push eax
    call printString

    mov eax, 3
    push wax
    call wait

    mov eax, 1
    push eax
    call systemExit