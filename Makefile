# Makefile for xcalib
#
# (c) 2004 Stefan Doehla <stefan AT doehla DOT de>
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

# there is a bool conflict between lcms and X that can be handled by
# undeffing it at the beginning of the code
CFLAGS = -DBOOL_CONFLICT -Os

all: lo_xcalib
	

lo_xcalib: xcalib.c
	$(CC) $(CFLAGS) -c xcalib.c -I./icclib -I/usr/X11R6/include
	$(CC) $(CFLAGS) -L/usr/X11R6/lib -lm -o xcalib xcalib.o -lX11 -lXxf86vm -lXext

lcms_xcalib: xcalib.c
	$(CC) $(CFLAGS) -DPATCHED_LCMS -c xcalib.c -I./icclib -I/usr/X11R6/include
	$(CC) $(CFLAGS) -L/usr/X11R6/lib -L/usr/local/lib -lm -o xcalib xcalib.o -llcms -lX11 -lXxf86vm -lXext

icclib_xcalib: xcalib.c
	$(CC) $(CFLAGS) -DICCLIB -c xcalib.c -I./icclib -I/usr/X11R6/include
	$(MAKE) -C icclib libicc.a
	$(CC) $(CFLAGS) -L/usr/X11R6/lib -lm -o xcalib xcalib.o icclib/libicc.a -lX11 -lXxf86vm -lXext

install:
	cp ./xcalib $(DESTDIR)/usr/local/bin/
	chmod 0644 $(DESTDIR)/usr/local/bin/xcalib

clean:
	rm xcalib.o
	rm xcalib
	$(MAKE) -C icclib clean-lib


