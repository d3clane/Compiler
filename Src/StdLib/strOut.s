section .text

global StdStrOut

StdStrOut:
        push rbp
        mov rbp, rsp
        
        // TODO: think about receiving on stack vs on rdi
        
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
        ret 1

StrLen:     cld 

            xor rcx, rcx
            dec rcx
            mov al, 0x0     ; null terminating 
            repne scasb
            not rcx
            dec rcx

            mov rax, rcx
            ret
            