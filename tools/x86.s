
BITS 32

_start:
    push ebp
    call proc
    mov eax, dword [ebp-4] ; return code
    pop ebp
    ret

proc:
    mov ebp, esp
    mov dword [ebp-4],4
    ret

