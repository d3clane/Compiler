section .text

global StdIn
global StdStrOut
global StdFOut

StdIn:  push rbp
        mov  rbp, rsp
        
        sub rsp, 0x10

        pxor xmm0, xmm0                 ; result
        pxor xmm1, xmm1                 ; tmp storage for read value
        movsd xmm2, [XMM_VAL_10]    ; multiplier1
        movsd xmm3, [XMM_VAL_1]     ; multiplier2
        movsd xmm4, [XMM_VAL_1]     ; divider

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
        movsd xmm2, [XMM_VAL_1]
        movsd xmm4, [XMM_VAL_10]
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
        movsd xmm1, [XMM_MAKE_NEGATIVE]
        pxor xmm0, xmm1

NOT_NEG_RET:
        mov rsp, rbp
        pop rbp
        ret

StdStrOut:
        push rbp
        mov rbp, rsp
        
        mov rdi, [rbp + 0x10]   ; getting in val

        sub rsp, 0x8    ; aligning

        mov rsi, rdi    ; saving my string ptr
        call StrLen     

        sub rsp, 0x8    ; aligning
        push rax        ; saving len
        ; preparing for write syscall
        mov rdi, 0x1    ; stdout
        mov rdx, rax    ; length
        mov rax, 0x1    ; call number
        ; rsi is already saved as a string
        syscall

        pop rax
        add rsp, 0x10
        
        mov rsp, rbp
        pop rbp
        ret 8

StrLen:     cld 

            xor rcx, rcx
            dec rcx
            mov al, 0x0     ; null terminating 
            repne scasb
            not rcx
            dec rcx

            mov rax, rcx
            ret


NumberIsNegative equ 1

StdFOut:
        push rbp
        mov rbp, rsp

        ; TODO: think about receiving on stack vs on rdi
        movsd xmm0, [rbp + 0x10]    ; value on stack

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

        movsd xmm1, [XMM_MAKE_NEGATIVE] ;
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
        mulsd xmm0, [XMM_VAL_1000]
        cvttsd2si rdi, xmm0
        sub rdi, rax

        test rdi, rdi
        jns PRINT_AFTER_POINT
        neg rdi
PRINT_AFTER_POINT:        
        call PrintThreeCharsInt

        mov rax, 1
        mov rdi, 1
        push 10 ; '\n'
        mov rsi, rsp
        mov rdx, 1
        sub rsp, 0x8
        syscall 
        add rsp, 0x10

        mov rsp, rbp
        pop rbp
        ret 16



PrintDecimalInt:
            push rbp
            mov  rbp, rsp
            
PrintDecimalIntArrSize equ 0x10

            sub rsp,  PrintDecimalIntArrSize    ; saving 16 bytes for number
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


PrintThreeCharsInt:
push rbp
            mov  rbp, rsp
            
PrintThreeCharsIntArrSize equ 0x10

            sub rsp,  PrintThreeCharsIntArrSize    ; saving 16 bytes for number
            mov rax, rdi                        ; saving value in rax for dividing
            mov rdi,  -1                        ; counter for array
            mov r8,  10d                        ; saving for dividing

            xor r9, r9
            test eax, eax
            jns PrintThreeCharsIntLoop
            neg eax
            mov r9, NumberIsNegative     ; setting that it was negative
PrintThreeCharsIntLoop:
            xor rdx, rdx
            div r8
            
            add rdx, '0'    ; creating ASCII

            mov [rbp + rdi], dl    ; pushing ASCII
            dec rdi

            cmp rdi, -4    ; stop pushing bits in case val is 0
            je PrintThreeCharsIntLoopEnd
            jmp PrintThreeCharsIntLoop

PrintThreeCharsIntLoopEnd:
            cmp r9, NumberIsNegative
            jne PrintThreeCharsIntWrite
            mov byte [rbp + rdi], '-'
            dec rdi

PrintThreeCharsIntWrite:
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

StdHlt:
        mov rax, 0x3c
        xor rdi, rdi
        syscall
        ret

section .rodata

XMM_VAL_1:
        dd   0
        dd   1072693248

XMM_VAL_10:
        dd   0
        dd   1076101120

XMM_VAL_1000:
        dd 0
        dd 1083129856
        
XMM_MAKE_NEGATIVE:
        dd 0
        dd -2147483648
        dd 0
        dd 0
