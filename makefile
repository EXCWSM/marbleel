# marbleel for Borland C++ Compiler & Netwide Assembler | Tiny C Compiler

CC=bcc32
COPT=-c -6 -O2 -Oi -d -DNODEFAULTLIB

LINK=ilink32
LOPT=/c /C /Gn /x /Tpe /aa
LIBS=kernel32.lib user32.lib shell32.lib

AS=nasm
ASOPT=-f obj

RM=del /f /s /q
CP=copy /y

rel : marbleel.exe marbleel64.exe
rel32 : marbleel.exe
ansi : marbleela.exe
debug : marbleel_dbga.exe
debugw : marbleel_dbgw.exe
rel64 : marbleel64.exe

marbleel.exe : marbleelw.obj startupw.obj
	$(LINK) $(LOPT) marbleelw.obj startupw.obj,$@,,$(LIBS),,

marbleelw.obj : marbleel.c
	$(CC) $(COPT) -DUNICODE -D_UNICODE -o$@ marbleel.c

startupw.obj : startupw.nas
	$(AS) $(ASOPT) -o $@ startupw.nas

marbleela.exe : marbleela.obj startupa.obj
	$(LINK) $(LOPT) marbleela.obj startupa.obj,$@,,$(LIBS),,

marbleela.obj : marbleel.c
	$(CC) $(COPT) -D_MBCS -o$@ marbleel.c

startupa.obj : startupa.nas
	$(AS) $(ASOPT) -o $@ startupa.nas

marbleel_dbga.exe : marbleel_dbga.obj startupa.obj
	$(LINK) $(LOPT) marbleel_dbga.obj startupa.obj,$@,,$(LIBS),,

marbleel_dbga.obj : marbleel.c
	$(CC) $(COPT) -DDEBUG -D_MBCS -o$@ marbleel.c

marbleel_dbgw.exe : startupw.obj marbleel_dbgw.obj
	$(LINK) $(LOPT) marbleel_dbgw.obj startupw.obj,$@,,$(LIBS),,

marbleel_dbgw.obj : marbleel.c
	$(CC) $(COPT) -DDEBUG -DUNICODE -D_UNICODE -o$@ marbleel.c

marbleel64.exe : marbleel.c
	tcc -m64 -o $@ marbleel.c -DUNICODE -D_UNICODE -nostdlib -lkernel32 -luser32 -lshell32

clean :
	- ${RM} *.obj
	- ${RM} *.map
	- ${RM} *.tds

cleanall : clean
	- ${RM} *.exe
