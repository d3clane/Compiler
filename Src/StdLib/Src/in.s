section .text

global StdIn

StdIn:  push rbp
        mov  rbp, rsp
        
        sub rsp, 0x10

        pxor xmm0, xmm0                 ; result
        pxor xmm1, xmm1                 ; tmp storage for read value
        movsd xmm2, [rel XMM_VAL_10]    ; multiplier1
        movsd xmm3, [rel XMM_VAL_1]     ; multiplier2
        movsd xmm4, [rel XMM_VAL_1]     ; divider

        xor rcx, rcx                    ; isNegative
        mov rdx, 1                      ; isFirstChar
STDIN_READ_WHILE:
        sub rsp, 0x50               ; for five xmm registers
        movsd [rsp + 0x00], xmm0
        movsd [rsp + 0x10], xmm1
        movsd [rsp + 0x20], xmm2
        movsd [rsp + 0x30], xmm3
        movsd [rsp + 0x40], xmm4
        push rcx
        push rdx

        mov rdi, 0                  ; stdin descriptor
        lea rsi, [rbp - 0x10]       ; rsi = rbp - 0x10 buffer 1 char len
        mov rdx, 1                  ; buff len
        mov rax, 0                  ; id syscall read
        syscall

        pop rdx
        pop rcx
        movsd xmm0, [rsp + 0x00]    ; restoring xmm registers
        movsd xmm1, [rsp + 0x10]
        movsd xmm2, [rsp + 0x20]
        movsd xmm3, [rsp + 0x30]
        movsd xmm4, [rsp + 0x40]

        cmp rax, 1
        jne STDIN_RET

        cmp BYTE [rbp - 0x10], '-'
        jne NOT_NEG
        cmp rdx, 1
        jne STDIN_RET
        mov rcx, 1
        jmp AFTER_IF_ELSE

NOT_NEG:
        cmp BYTE [rbp - 0x10], '.'
        jne CHECK_IS_NUM
        movsd xmm2, [rel XMM_VAL_1]
        movsd xmm4, [rel XMM_VAL_10]
        jmp AFTER_IF_ELSE


CHECK_IS_NUM:
        cmp BYTE [rbp - 0x10], '0'
        jb STDIN_RET
        cmp BYTE [rbp - 0x10], '9'
        ja STDIN_RET
        mulsd xmm0, xmm2

        xor rax, rax
        mov al, BYTE [rbp - 0x10]
        sub rax, '0'
        cvtsi2sd xmm1, rax
        mulsd xmm1, xmm3
        addsd xmm0, xmm1

AFTER_IF_ELSE:
        mov rdx, 0
        divsd xmm3, xmm4
        jmp STDIN_READ_WHILE

STDIN_RET:
        cmp rcx, 1
        jne NOT_NEG_RET
        movsd xmm1, [rel XMM_MAKE_NEGATIVE]
        pxor xmm0, xmm1

NOT_NEG_RET:
        mov rsp, rbp
        pop rbp
        ret


section .rodata

XMM_VAL_1:
        dd   0
        dd   1072693248

XMM_VAL_10:
        dd   0
        dd   1076101120

XMM_MAKE_NEGATIVE:
        dd 0
        dd -2147483648
        dd 0
        dd 0