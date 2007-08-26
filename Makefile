# Makefile for xcalib
#
# (c) 2004-2007 Stefan Doehla <stefan AT doehla DOT de>
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
# - xcalib
#   default
# - win_xcalib
#   version for MS-Windows systems with MinGW (internal parser)
# - fglrx_xcalib
#   version for ATI's proprietary fglrx driver (internal parser)
#
# - clean
#   delete all objects and binaries
#
# if it doesn't compile right-out-of-the-box, it may be sufficient
# to change the following variables

XCALIB_VERSION = 0.8
CFLAGS = -O2
XINCLUDEDIR = /usr/X11R6/include
XLIBDIR = /usr/X11R6/lib
# for ATI's proprietary driver (must contain the header file fglrx_gamma.h)
FGLRXINCLUDEDIR = ./fglrx
FGLRXLIBDIR = ./fglrx

# default make target
all: xcalib
	

# low overhead version (internal parser)
xcalib: xcalib.c
	$(CC) $(CFLAGS) -c xcalib.c -I$(XINCLUDEDIR) -DXCALIB_VERSION=\"$(XCALIB_VERSION)\"
	$(CC) $(CFLAGS) -L$(XLIBDIR) -lm -o xcalib xcalib.o -lX11 -lXxf86vm -lXext

fglrx_xcalib: xcalib.c
	$(CC) $(CFLAGS) -c xcalib.c -I$(XINCLUDEDIR) -DXCALIB_VERSION=\"$(XCALIB_VERSION)\" -I$(FGLRXINCLUDEDIR) -DFGLRX
	$(CC) $(CFLAGS) -L$(XLIBDIR) -L$(FGLRXLIBDIR) -lm -o xcalib xcalib.o -lX11 -lXxf86vm -lXext -lfglrx_gamma

win_xcalib: xcalib.c
	$(CC) $(CFLAGS) -c xcalib.c -DXCALIB_VERSION=\"$(XCALIB_VERSION)\" -DWIN32GDI
	windres.exe resource.rc resource.o
	$(CC) $(CFLAGS) -mwindows -lm resource.o -o xcalib xcalib.o

install:
	cp ./xcalib $(DESTDIR)/usr/local/bin/
	chmod 0644 $(DESTDIR)/usr/local/bin/xcalib

clean:
	rm -f xcalib.o
	rm -f resource.o
	rm -f xcalib
	rm -f xcalib.exe

