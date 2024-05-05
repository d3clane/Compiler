section .text

global StdFOut

NumberIsNegative equ 1

StdFOut:
        push rbp
        mov rbp, rsp

        pxor xmm1, xmm1
        comisd xmm0, xmm1
        ja PRINT_VALUE

        push '-'        ; write '-'
        mov rax, 1
        mov rdi, 1
        mov rsi, rsp
        mov rdx, 1
        syscall 
        pop rax

        movsd xmm1, [rel XMM_MAKE_NEGATIVE] ;
        pxor xmm0, xmm1                     ; xmm0 := -xmm0

PRINT_VALUE:
        cvttsd2si rdi, xmm0
        call PrintDecimalInt

        push '.'
        mov rax, 1      ; write
        mov rdi, 1      ; stdout
        mov rsi, rsp    ; rsp == '.'
        mov rdx, 1      ; len = 1
        syscall 
        pop rax

        cvttsd2si rax, xmm0
        mov rdx, 1000
        mul rdx
        mulsd xmm0, [rel XMM_VAL_1000]
        cvttsd2si rdi, xmm0
        sub rdi, rax

        test rdi, rdi
        jns PRINT_AFTER_POINT
        neg rdi
PRINT_AFTER_POINT:        
        call PrintDecimalInt

        mov rsp, rbp
        pop rbp
        ret



PrintDecimalInt:
            push rbp
            mov  rbp, rsp
            
PrintDecimalIntArrSize equ 0x10

            sub rsp,  PrintDecimalIntArrSize    ; saving 10 bytes for number
            mov rax, rdi                        ; saving value in rax for dividing
            mov rdi,  -1                        ; counter for array
            mov r8,  10d                        ; saving for dividing

            xor r9, r9
            test eax, eax
            jns PrintDecimalIntLoop
            neg eax
            mov r9, NumberIsNegative     ; setting that it was negative
PrintDecimalIntLoop:
            xor rdx, rdx
            div r8
            
            add rdx, '0'    ; creating ASCII

            mov [rbp + rdi], dl    ; pushing ASCII
            dec rdi

            cmp rax, 0x0    ; stop pushing bits in case val is 0
            je PrintDecimalIntLoopEnd
            jmp PrintDecimalIntLoop

PrintDecimalIntLoopEnd:
            cmp r9, NumberIsNegative
            jne PrintDecimalIntWrite
            mov byte [rbp + rdi], '-'
            dec rdi

PrintDecimalIntWrite:
            sub rsp, 0x8    ; aligning
            push rdi        ; saving len

            ;preparing for syscall write 
            mov rdx, rdi
            not rdx
            ; rdx = -rax = ~rax + 1. No inc because actual size is rdx - 1

            lea rsi, [rbp + rdi + 1]  ; string

            mov rax, 0x1        ; preparing for syscall write
            mov rdi, 1          ; stdout file descriptor

            syscall

            pop rax
            not rax 

            mov rsp, rbp
            pop rbp
            ret

section .rodata

XMM_VAL_1000:
        dd 0
        dd 1083129856

XMM_MAKE_NEGATIVE:
        dd 0
        dd -2147483648
        dd 0
        dd 0
