; startupa.nas  Startup routine for Borland C

bits 32
global ..start

        extern WinMain
        extern GetModuleHandleA
        extern GetCommandLineA
        extern ExitProcess

segment text use32 public

..start:
        call GetCommandLineA
        cmp eax, 0
        je clend
        mov dl, 0
clcmp:
        cmp byte [eax], 0
        je clend
        cmp byte [eax], '"'     ; "
        je cldq
        cmp byte [eax], ' '
        je clspace
clnext:
        add eax, 1
        jmp clcmp
cldq:
        not dl
        jmp clnext
clspace:
        cmp dl, 0
        jne clnext
        add eax, 1
clend:
        mov ebx, eax

        push dword 0
        call GetModuleHandleA

        push dword 0
        push ebx
        push dword 0
        push eax
        call WinMain

        push eax
        call ExitProcess
        ret
