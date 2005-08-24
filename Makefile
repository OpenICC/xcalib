# Makefile for xcalib
#
# (c) 2004-2005 Stefan Doehla <stefan AT doehla DOT de>
#
# This program is GPL-ed postcardware! please see README
#
# It is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA.
#

#
# the following targets are defined:
# - lo_xcalib
#   xcalib in its standard version (internal parser)
# - icclib_xcalib
#   xcalib using Graeme Gill's icclib
# - lcms_xcalib
#   xcalib using a patched version of Marti Maria's LCMS
# - win_xcalib
#   version for MS-Windows systems with MinGW (internal parser)
# - fglrx_xcalib
#   version for ATI's proprietary fglrx driver (internal parser)
#
# - clean
#   delete all objects and binaries
# - dist
#   create .tar.gz archive
#
# if it doesn't compile right-out-of-the-box, it may be sufficient
# to change the following variables

XCALIB_VERSION = 0.6
CFLAGS = -Os
XINCLUDEDIR = /usr/X11R6/include
XLIBDIR = /usr/X11R6/lib
LCMSINCLUDEDIR = /usr/local/include
LCMSLIBDIR = /usr/local/lib
# for ATI's proprietary driver (must contain the header file fglrx_gamma.h)
FGLRXINCLUDEDIR = ./fglrx
FGLRXLIBDIR = ./fglrx

# default make target
all: lo_xcalib
	

# low overhead version (internal parser)
lo_xcalib: xcalib.c
	$(CC) $(CFLAGS) -c xcalib.c -I$(XINCLUDEDIR) -DXCALIB_VERSION=\"$(XCALIB_VERSION)\"
	$(CC) $(CFLAGS) -L$(XLIBDIR) -lm -o xcalib xcalib.o -lX11 -lXxf86vm -lXext

fglrx_xcalib: xcalib.c
	$(CC) $(CFLAGS) -c xcalib.c -I$(XINCLUDEDIR) -DXCALIB_VERSION=\"$(XCALIB_VERSION)\" -I$(FGLRXINCLUDEDIR) -DFGLRX
	$(CC) $(CFLAGS) -L$(XLIBDIR) -L$(FGLRXLIBDIR) -lm -o xcalib xcalib.o -lX11 -lXxf86vm -lXext -lfglrx_gamma

win_xcalib: xcalib.c
	$(CC) $(CFLAGS) -c xcalib.c -DXCALIB_VERSION=\"$(XCALIB_VERSION)\" -DWIN32GDI
	windres.exe resource.rc resource.o
	$(CC) $(CFLAGS) -mwindows -lm resource.o -o xcalib xcalib.o

# there is a bool conflict between lcms and X that can be handled by
# undeffing it at the beginning of the code - therefore use -DBOOL_CONFLICT
# as defined in the lcms patch
lcms_xcalib: xcalib.c
	$(CC) $(CFLAGS) -DPATCHED_LCMS -DBOOL_CONFLICT -c xcalib.c -I$(XINCLUDEDIR) \
	      -I$(LCMSINCLUDEDIR) -DXCALIB_VERSION=\"$(XCALIB_VERSION)\"
	$(CC) $(CFLAGS) -L$(XLIBDIR) -L$(LCMSLIBDIR) -lm -o xcalib xcalib.o -llcms -lX11 -lXxf86vm -lXext

# icclib version
icclib_xcalib: xcalib.c
	$(CC) $(CFLAGS) -DICCLIB -c xcalib.c -I./icclib -I$(XINCLUDEDIR) -DXCALIB_VERSION=\"$(XCALIB_VERSION)\"
	$(MAKE) -C icclib libicc.a
	$(CC) $(CFLAGS) -L$(XLIBDIR) -lm -o xcalib xcalib.o icclib/libicc.a -lX11 -lXxf86vm -lXext

install:
	cp ./xcalib $(DESTDIR)/usr/local/bin/
	chmod 0644 $(DESTDIR)/usr/local/bin/xcalib

clean:
	rm -f xcalib.o
	rm -f resource.o
	rm -f xcalib
	rm -f xcalib.exe
	$(MAKE) -C icclib clean

dist:
	cd ..
	tar czf xcalib-source-$(XCALIB_VERSION).tar.gz xcalib-$(XCALIB_VERSION)/
	cd xcalib-$(XCALIB_VERSION)/
	
