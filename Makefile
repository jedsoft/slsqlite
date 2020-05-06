#---------------------------------------------------------------------------
# Installation Directories
#---------------------------------------------------------------------------
prefix = /usr/local
exec_prefix = ${prefix}
MODULE_INSTALL_DIR = ${exec_prefix}/lib/slang/v2/modules
SL_FILES_INSTALL_DIR = ${prefix}/share/slsh/local-packages
HLP_FILES_INSTALL_DIR = $(SL_FILES_INSTALL_DIR)/help

#---------------------------------------------------------------------------
# DESTDIR is designed to facilitate making packages.  Normally it is empty
#---------------------------------------------------------------------------
DESTDIR =
DEST_MODULE_INSTALL_DIR = $(DESTDIR)$(MODULE_INSTALL_DIR)
DEST_SL_FILES_INSTALL_DIR = $(DESTDIR)$(SL_FILES_INSTALL_DIR)
DEST_HLP_FILES_INSTALL_DIR = $(DESTDIR)$(HLP_FILES_INSTALL_DIR)

#---------------------------------------------------------------------------
# Location of the S-Lang library and its include file
#---------------------------------------------------------------------------
SLANG_INC	= -I/usr/local/include
SLANG_LIB	= -L/usr/local/lib -lslang

#---------------------------------------------------------------------------
# C Compiler to create a shared library
#---------------------------------------------------------------------------
CC_SHARED 	= $(CC) $(CFLAGS) -shared -Wall -fPIC

#---------------------------------------------------------------------------
# Misc Programs required for installation
#---------------------------------------------------------------------------
INSTALL		= /usr/bin/install -m 644
MKINSDIR        = /usr/bin/install -d
#---------------------------------------------------------------------------
# You shouldn't need to edit anything below this line
#---------------------------------------------------------------------------

LIBS = $(SLANG_LIB) -lm -lsqlite3
INCS = $(SLANG_INC)

all: sqlite-module.so

sqlite-module.so: sqlite-module.c
	$(CC_SHARED) $(INCS) -g sqlite-module.c -o sqlite-module.so $(LIBS)

sqlite.o: sqlite-module.c
	gcc $(CFLAGS) $(INCS) -O2 -c -g sqlite-module.c -o sqlite.o

sqlite.hlp: sqlitefuns.tm
	tmexpand -Mslhlp sqlitefuns.tm sqlite.hlp


clean:
	rm -f sqlite-module.so *.o

#---------------------------------------------------------------------------
# Installation Rules
#---------------------------------------------------------------------------
install_directories:
	$(MKINSDIR) $(DEST_MODULE_INSTALL_DIR)
	$(MKINSDIR) $(DEST_SL_FILES_INSTALL_DIR)
	$(MKINSDIR) $(DEST_HLP_FILES_INSTALL_DIR)

install_modules:
	$(INSTALL) sqlite-module.so $(DEST_MODULE_INSTALL_DIR)
install_slfiles:
	$(INSTALL) sqlite.sl $(DEST_SL_FILES_INSTALL_DIR)
install_hlpfiles:
	$(INSTALL) sqlite.hlp $(DEST_HLP_FILES_INSTALL_DIR)

install: all install_directories install_modules install_slfiles install_hlpfiles
