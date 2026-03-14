;; Copyright (c) 2023-2026 Dan O’Malley
;; This file is licensed under the MIT License. See LICENSE for details.


bits 16

    org 0x7c00


;; load bootloader-stage2 (first 64 KB) to this location in memory

    ;; ASSIGNMENT 1 TO DO

    mov ah, 02h
mov al, 128
mov ch, 0
mov cl, 02
mov dh, 0
xor bx, bx
mov ds, bx
mov es, bx
mov bx, 0x8000
int 13h



;; load bootloader-stage2 (second 64 KB) to this location in memory

    ;; ASSIGNMENT 1 TO DO

    ;;es is necessary for extra segment registers such as video memory. Very possibly neccesary for this assignment
;;Memory at 0x18000
    mov ah, 02h
    mov al, 128
    mov ch, 0
    mov cl, 0x82
    mov dh, 0
    xor bx, bx
    mov es, bx

    mov bx, 0x1800
    mov es, bx
    mov bx, 0x0000 
    int 13h



;; load bootloader-stage2 (third 64 KB) to this location in memory

    ;; ASSIGNMENT 1 TO DO
    mov ah, 02h
    mov al, 128
    mov ch, 0
    mov cl, 0x82
    mov dh, 0
    xor bx, bx
    mov es, bx

    mov bx, 0x2800
    mov es, bx
    mov bx, 0x0000 
    int 13h



;; switch to protected mode
    cli
    lgdt [gdtr]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

;; far jump
    jmp 0x8:protected_mode_code ;code segment descriptor offset; clears instruction queue


gdt:
    dq 0x0 ; null segment

ring_0_code_segment_descriptor:
    dw 0xffff  ;; limit 0-15
    dw 0x0     ;; base 0-15
    db 0x0     ;; base 16-23
    db 0x9a    ;; present=1, DPL=00 (or 0), code/data seg=1, code=1, conform=0, r/w=1, accessed =0
    db 0xcf    ;; avail=1, long=1, big=0 (32bit), gran=0(4kb page), limit=0xf
    db 0x0     ;; base 24-31

ring_0_data_segment_descriptor:
    dw 0xffff  ;; limit 0-15
    dw 0x0     ;; base 0-15
    db 0x0     ;; base 16-23
    db 0x92    ;; present=1, DPL=00 (or 0), code/data seg=1, code=0, conform=0, r/w=1, accessed =0
    db 0xcf    ;; avail=1, long=1, big=0 (32bit), gran=0(4kb page), limit=0xf
    db 0x0     ;; base 24-31

ring_3_code_segment_descriptor:
    dw 0xffff  ;; limit 0-15
    dw 0x0     ;; base 0-15
    db 0x0     ;; base 16-23
    db 0xfa    ;; present=1, DPL=11 (or 3), code/data seg=1, code=1, conform=0, r/w=1, accessed =0
    db 0xcf    ;; avail=1, long=1, big=0 (32bit), gran=0(4kb page), limit=0xf
    db 0x0

ring_3_data_segment_descriptor:
    dw 0xffff  ;; limit 0-15
    dw 0x0     ;; base 0-15
    db 0x0     ;; base 16-23
    db 0xf2    ;; present=1, DPL=11 (or 3), code/data seg=1, code=0, conform=0, r/w=1, accessed =0
    db 0xcf    ;; avail=1, long=1, big=0 (32bit), gran=0(4kb page), limit=0xf
    db 0x0

tss_kernel_descriptor:
    dw task_struct_kernel + 0x68 ;; limit 0-15
    dw task_struct_kernel     ;; base 0-15
    db 0x0     ;; base 16-23
    db 0xE9    ;; present=1, DPL=00 (or 0), code/data seg=0, code=1, conform=0, r/w=0, accessed=1
    db 0x0     ;; avail=0, long=0, big=0, gran=0, limit=0x0
    db 0x0


gdtr:
    dw 0x30 ;; 48 bytes in size (or 0x30)
    dd gdt


data_segment equ ring_0_data_segment_descriptor - gdt

bits 32

protected_mode_code:
    
    mov ax, data_segment
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov ax, 0x0
    mov fs, ax
    mov gs, ax

    mov ebp, 0x99FFFF
    mov esp, ebp

    call [0x8018] ;; ELF Header Entry Address


task_struct_kernel:
	dd 0x0      ;; uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	dd 0x99FFFF      ;; uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	dd 0x10     ;; uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	dd 0x0      ;; uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	dd 0x0      ;; uint32_t ss1;
	dd 0x0      ;; uint32_t esp2;
	dd 0x0      ;; uint32_t ss2;
	dd 0x0      ;; uint32_t cr3;
	dd 0x0      ;; uint32_t eip;
	dd 0x0      ;; uint32_t eflags;
	dd 0x0      ;; uint32_t eax;
	dd 0x0      ;; uint32_t ecx;
	dd 0x0      ;; uint32_t edx;
	dd 0x0      ;; uint32_t ebx;
	dd 0x7FEFF0      ;; uint32_t esp;
	dd 0x0      ;; uint32_t ebp;
	dd 0x0      ;; uint32_t esi;
	dd 0x0      ;; uint32_t edi;
	dd 0x13     ;; uint32_t es; Normal segments with RPL=3
	dd 0x0b     ;; uint32_t cs; Normal segments with RPL=3
	dd 0x13     ;; uint32_t ss; Normal segments with RPL=3
	dd 0x13     ;; uint32_t ds; Normal segments with RPL=3
	dd 0x13      ;; uint32_t fs;
	dd 0x13      ;; uint32_t gs;
	dd 0x0      ;; uint32_t ldt;
	dw 0x0      ;; uint16_t trap;
	dw 0x0      ;; uint16_t iomap_base;

times 510-($-$$) db 0
dw 0xaa55
