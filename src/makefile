#
# Makefile for POP3 client
#
# Bob Eager   December 2003
#
# Product names
#
PRODUCT		= pop3
TEST1		= test1
TEST2		= test2
#
# Target directory
#
TARGET		= f:\binp
#
# Compiler setup
#
CC		= icc
DEFS		= -DTCPV40HDRS
!IFDEF DEBUG
DBUG		= -DDEBUG
!ELSE
DBUG		=
!ENDIF
!IFDEF	NOLOG
SLOG		= -DNOLOG
!ENDIF
!IFDEF	PROD
CFLAGS		= $(DEFS) $(DBUG) $(SLOG) -G4 -Gm -Gn -O -Q -Se -Tl30
CLIB		= cppom30o.lib
!ELSE
CFLAGS		= $(DEFS) $(DBUG) $(SLOG) -G4 -Gm -Gn -Q -Se -Ti -Tl30 -Tm -Tx
CLIB		= cppom30.lib
!ENDIF
#
# Names of library files
#
NETLIB		= ..\netlib\netlib.lib
LIBS		= so32dll.lib tcp32dll.lib $(CLIB) \
		  $(NETLIB) os2386.lib
#
# Names of object files
#
OBJ		= pop3.obj client.obj store.obj lock.obj netio.obj log.obj
#
# Other files
#
DEF		= $(PRODUCT).def
LNK		= $(PRODUCT).lnk
README		= readme.txt
OTHER		= todo.txt
#
# Final executable file
#
EXE		= $(PRODUCT).exe
#
# Distribution
#
DIST		= dist.zip
NETLIBDLL	= ..\netlib\netlib.dll
#
# Archive
#
ARCH		= arch.zip
#
#-----------------------------------------------------------------------------
#
$(EXE):		$(OBJ) $(LNK) $(DEF)
!IFDEF	PROD
		ilink /nodefaultlibrarysearch /nologo /exepack:2 @$(LNK)
		lxlite $(EXE)
		eautil $(EXE) nul /s
!ELSE
		ilink /nodefaultlibrarysearch /debug /nobrowse /nologo @$(LNK)
!ENDIF
#
# Object files
#
pop3.obj:	pop3.c pop3.h log.h
#
client.obj:	client.c pop3.h store.h netio.h log.h
#
lock.obj:	lock.c lock.h log.h
#
netio.obj:	netio.c netio.h
#
log.obj:	log.c log.h
#
store.obj:	store.c store.h log.h
#
# Linker response file. Rebuild if makefile changes
#
$(LNK):		makefile
		@if exist $(LNK) erase $(LNK)
		@echo /map:$(PRODUCT) >> $(LNK)
		@echo /out:$(PRODUCT) >> $(LNK)
		@echo $(OBJ) >> $(LNK)
		@echo $(LIBS) >> $(LNK)
		@echo $(DEF) >> $(LNK)
#
test:		$(TEST1).exe $(TEST2).exe
#
$(TEST1).exe:	$(TEST1).obj lock.obj log.obj
		ilink /debug /nobrowse /nologo /pmtype:vio $(TEST1).obj lock.obj log.obj
#
$(TEST1).obj:	$(TEST1).c lock.h log.h
#
$(TEST2).exe:	$(TEST2).obj lock.obj log.obj
		ilink /debug /nobrowse /nologo /pmtype:vio $(TEST2).obj lock.obj log.obj
#
$(TEST2).obj:	$(TEST2).c lock.h log.h
#
clean:		
		-erase $(OBJ) $(LNK) $(PRODUCT).map csetc.pch
		-erase $(TEST1).obj $(TEST2).obj $(TEST1).exe $(TEST2).exe
#
install:	$(EXE)
		@copy $(EXE) $(TARGET) > nul
#
dist:		$(EXE) $(NETLIBDLL) $(README)
		zip -9 -j $(DIST) $**
#
arch:		$(EXE) $(README) $(OTHER) $(DEF) *.c *.h makefile
		zip -9 -j $(ARCH) $**
#
# End of makefile for POP3 client
#
