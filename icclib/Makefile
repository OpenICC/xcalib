# Dumbo UNIX style makefile, for icclib and friends.

# Microsoft C++, WinNT setup
#include Makefile.WNT

# IBM C++, WinNT setup
#include Makefile.IBMNT

# Generic UNIX setup
include Makefile.UNIX

# Apple OSX
#include Makefile.OSX

###############################

#Compile with separate stdio
#CCDEFINES = $(DEFFLAG)SEPARATE_STD

#Set optimisation on
CCFLAGS = $(CCFLAGSDEF) $(CCOPTFLAG) $(CCDEFINES)

#Set debugging on
#CCFLAGS = $(CCFLAGSDEF) $(CCDEBUGFLAG) $(CCDEFINES)

STDHDRS = $(STDHDRSDEF)
LINKFLAGS = $(LINKFLAGSDEF) $(LINKDEBUGFLAG)

all:: libicc$(SUFLIB) icctest$(SUFEXE) lutest$(SUFEXE) icclu$(SUFEXE) iccdump$(SUFEXE) iccrw$(SUFEXE) fbtest$(SUFEXE)


icc$(SUFOBJ): icc.c icc.h
	$(CC) $(CCOF)icc$(SUFOBJ) icc.c

libicc$(SUFLIB): icc$(SUFOBJ)
	$(LIBU) $(LIBOF)libicc$(SUFLIB) icc$(SUFOBJ)
	$(RANLIB) libicc$(SUFLIB)


# Separate for executables
iccstd$(SUFOBJ): iccstd.c icc.h
	$(CC) $(CCOF)iccstd$(SUFOBJ) iccstd.c


icctest$(SUFOBJ): icctest.c icc.h
	$(CC) $(CCOF)icctest$(SUFOBJ) icctest.c

icctest$(SUFEXE): icctest$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)
	$(LINK) $(LINKOF)icctest$(SUFEXE) icctest$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)


lutest$(SUFOBJ): lutest.c icc.h
	$(CC) $(CCOF)lutest$(SUFOBJ) lutest.c

lutest$(SUFEXE): lutest$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)
	$(LINK) $(LINKOF)lutest$(SUFEXE) lutest$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)


icclu$(SUFOBJ): icclu.c icc.h
	$(CC) $(CCOF)icclu$(SUFOBJ) icclu.c

icclu$(SUFEXE): icclu$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)
	$(LINK) $(LINKOF)icclu$(SUFEXE) icclu$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)


iccdump$(SUFOBJ): iccdump.c icc.h
	$(CC) $(CCOF)iccdump$(SUFOBJ) iccdump.c

iccdump$(SUFEXE): iccdump$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)
	$(LINK) $(LINKOF)iccdump$(SUFEXE) iccdump$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)


iccrw$(SUFOBJ): iccrw.c icc.h
	$(CC) $(CCOF)iccrw$(SUFOBJ) iccrw.c

iccrw$(SUFEXE): iccrw$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)
	$(LINK) $(LINKOF)iccrw$(SUFEXE) iccrw$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)


fbtest$(SUFOBJ): fbtest.c icc.h
	$(CC) $(CCOF)fbtest$(SUFOBJ) fbtest.c

fbtest$(SUFEXE): fbtest$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)
	$(LINK) $(LINKOF)fbtest$(SUFEXE) fbtest$(SUFOBJ) iccstd$(SUFOBJ) libicc$(SUFLIB)

clean: clean-lib clean-progs
	

clean-lib:
	rm -f libicc.a
	rm -f icc.o

clean-progs:
	rm -f fbtest
	rm -f iccdump
	rm -f icclu
	rm -f iccrw
	rm -f icctest
	rm -f lutest
	rm -f *.o




