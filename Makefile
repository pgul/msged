# Makefile for the Husky build environment

# include Husky-Makefile-Config
ifeq ($(DEBIAN), 1)
include debian/huskymak.cfg
else
include ../huskymak.cfg
endif

ifeq ($(DEBUG), 1)
  CFLAGS=$(WARNFLAGS) $(DEBCFLAGS) -I$(INCDIR)
  LFLAGS=$(DEBLFLAGS)
else
  CFLAGS=$(WARNFLAGS) $(OPTCFLAGS) -I$(INCDIR)
  LFLAGS=$(OPTLFLAGS)
endif

ifndef MSGEDCFG
  MSGEDCFG=\"$(CFGDIR)/msged.cfg\"
endif

CDEFS=-D$(OSTYPE) -DUSE_MSGAPI -DUSE_FIDOCONFIG -DUNAME=\"$(UNAME)\" \
      $(ADDCDEFS) -DREADMAPSDAT=\"$(CFGDIR)/readmaps.dat\" \
      -DWRITMAPSDAT=\"$(CFGDIR)/writmaps.dat\" \
      -DDEFAULT_CONFIG_FILE=$(MSGEDCFG)

ifeq ($(SHORTNAME), 1)
  LIBS= -L$(LIBDIR) -lfidoconf -lsmapi
else
  LIBS= -L$(LIBDIR) -lfidoconfig -lsmapi
endif

TARGET=	msged$(EXE)

ifeq ($(OSTYPE), UNIX)
  osobjs=	ansi$(OBJ) \
		readtc$(OBJ)
  OSLIBS=-l$(TERMCAP)
endif
ifeq ($(OSTYPE), OS2)
  osobjs=	os2scr$(OBJ) \
		malloc16$(OBJ)
endif
ifeq ($(OSTYPE), WINNT)
  osobjs=	winntscr$(OBJ)
endif


objs=   addr$(OBJ)     \
	areas$(OBJ)    \
	bmg$(OBJ)      \
	charset$(OBJ)  \
	config$(OBJ)   \
	control$(OBJ)  \
	curses$(OBJ)   \
	date$(OBJ)     \
	dialogs$(OBJ)  \
	dirute$(OBJ)   \
	dlgbox$(OBJ)   \
	dlist$(OBJ)    \
	echotoss$(OBJ) \
	environ$(OBJ)  \
	fconf$(OBJ)    \
	fecfg145$(OBJ) \
	fido$(OBJ)     \
	filedlg$(OBJ)  \
	flags$(OBJ)    \
	freq$(OBJ)     \
	gestr120$(OBJ) \
	getopts$(OBJ)  \
	group$(OBJ)    \
	help$(OBJ)     \
	helpcmp$(OBJ)  \
	helpinfo$(OBJ) \
	init$(OBJ)     \
	keycode$(OBJ)  \
	list$(OBJ)     \
	maintmsg$(OBJ) \
	makemsgn$(OBJ) \
	memextra$(OBJ) \
	menu$(OBJ)     \
	misc$(OBJ)     \
	mnu$(OBJ)      \
	msg$(OBJ)      \
	msged$(OBJ)    \
	mxbt$(OBJ)     \
	normalc$(OBJ)  \
	nshow$(OBJ)    \
	quick$(OBJ)    \
	quote$(OBJ)    \
	readmail$(OBJ) \
	screen$(OBJ)   \
	strextra$(OBJ) \
	system$(OBJ)   \
	template$(OBJ) \
	textfile$(OBJ) \
	timezone$(OBJ) \
	userlist$(OBJ) \
	vsev$(OBJ)     \
	vsevops$(OBJ)  \
	win$(OBJ)      \
	wrap$(OBJ)


ifeq ($(OSTYPE), UNIX)
   all: $(TARGET) testcons do-maps msghelp.dat
else
   all: $(TARGET) do-maps msghelp.dat

endif

do-maps:
	(cd maps && $(MAKE) -f makefile.husky)
	(cd doc && cd manual && $(MAKE) -f makefile.husky)


%$(OBJ): %.c
	$(CC) $(CFLAGS) $(CDEFS) -c $*.c

$(TARGET): $(objs) $(osobjs)
	$(CC) $(LFLAGS) -o $(TARGET) $(objs) $(osobjs) $(LIBS) $(OSLIBS) -ltmalloc

ifeq ($(OSTYPE), UNIX)
testcons: testcons$(OBJ)
	$(CC) $(LFLAGS) -o testcons$(EXE) testcons$(OBJ) $(LIBS) $(OSLIBS)
endif

msghelp.dat: msghelp.src
	.$(DIRSEP)$(TARGET) -hc msghelp.src msghelp.dat

clean:
	-$(RM) $(RMOPT) *$(OBJ)
	-$(RM) $(RMOPT) *~
	(cd maps && $(MAKE) -f makefile.husky clean)
	(cd doc && cd manual && $(MAKE) -f makefile.husky clean)

distclean: clean
	-$(RM) $(RMOPT) $(TARGET)
	-$(RM) $(RMOPT) msghelp.dat
	-$(RM) $(RMOPT) testcons$(EXE)
	(cd maps && $(MAKE) -f makefile.husky distclean)
	(cd doc && cd manual && $(MAKE) -f makefile.husky distclean)

ifeq ($(OSTYPE), UNIX)

install: $(TARGET) msghelp.dat testcons$(EXE)
	-$(MKDIR) $(MKDIROPT) $(BINDIR)
	$(INSTALL) $(IBOPT) $(TARGET) $(BINDIR)
	-$(MKDIR) $(MKDIROPT) $(CFGDIR)
	$(INSTALL) $(IIOPT) msghelp.dat $(CFGDIR)
	(cd maps && $(MAKE) -f makefile.husky install)
	(cd doc && cd manual && $(MAKE) -f makefile.husky install)
	$(INSTALL) $(IBOPT) testcons$(EXE) $(BINDIR)

else

install: $(TARGET) msghelp.dat
	-$(MKDIR) $(MKDIROPT) $(BINDIR)
	$(INSTALL) $(IBOPT) $(TARGET) $(BINDIR)
	-$(MKDIR) $(MKDIROPT) $(CFGDIR)
	$(INSTALL) $(IIOPT) msghelp.dat $(CFGDIR)
	(cd maps && $(MAKE) -f makefile.husky install)
	(cd doc && cd manual && $(MAKE) -f makefile.husky install)

endif

uninstall:
	-$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)$(TARGET)
	-$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)testcons$(EXE) $(BINDIR)
	-$(RM) $(RMOPT) $(CFGDIR)$(DIRSEP)msghelp.dat
	(cd maps && $(MAKE) -f makefile.husky uninstall)
	(cd doc && cd manual && $(MAKE) -f makefile.husky uninstall)

