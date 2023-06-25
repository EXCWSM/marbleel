; startupw.nas  Startup routine for Borland C

bits 32
global ..start

        extern wWinMain
        extern GetModuleHandleW
        extern GetCommandLineW
        extern ExitProcess

segment text use32 public

..start:
        call GetCommandLineW
        cmp eax, 0
        je clend
        mov dl, 0
clcmp:
        cmp word [eax], 0
        je clend
        cmp word [eax], 0022h  ; double quotation mark
        je cldq
        cmp word [eax], 0020h  ; space
        je clspace
clnext:
        add eax, 2
        jmp clcmp
cldq:
        not dl
        jmp clnext
clspace:
        cmp dl, 0
        jne clnext
        add eax, 2
clend:
        mov ebx, eax

        push dword 0
        call GetModuleHandleW

        push dword 0
        push ebx
        push dword 0
        push eax
        call wWinMain

        push eax
        call ExitProcess
        ret
