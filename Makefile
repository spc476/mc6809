#########################################################################
#
# Copyright 2012 by Sean Conner.  All Rights Reserved.
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 3 of the License, or (at your
# option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
# License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, see <http://www.gnu.org/licenses/>.
#
# Comments, questions and criticisms can be sent to: sean@conman.org
#
########################################################################

INSTALL         = /usr/bin/install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA    = $(INSTALL) -m 644

prefix      = /usr/local
includedir  = $(prefix)/include
exec_prefix = $(prefix)
libdir      = $(exec_prefix)/lib

CC      = gcc -std=c99
CFLAGS  = -g -Wall -Wextra -pedantic
LDFLAGS = -g
LDLIBS  = 

%.a :
	$(AR) $(ARFLAGS) $@ $?

.PHONY = all clean install uninstall

all : mc09emulator mc09disasm libmc6809.a

mc09emulator : mc09emulator.o mc6809.o mc6809dis.o
mc09disasm   : mc09disasm.o mc6809dis.o
libmc6809.a  : mc6809.o mc6809dis.o

mc09emulator.o : mc09emulator.c mc6809.h mc6809dis.h
mc09disasm.o   : mc09disasm.c   mc6809.h mc6809dis.h
mc6809.o       : mc6809.c       mc6809.h
mc6809dis.o    : mc6809dis.c    mc6809.h mc6809dis.h

clean:
	/bin/rm -rf *.o mc09emulator mc09disasm *~ libmc6809.a

install: libmc6809.a
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -d $(DESTDIR)$(includedir)
	$(INSTALL_DATA) libmc6809.a $(DESTDIR)$(libdir)
	$(INSTALL_DATA) mc6809.h    $(DESTDIR)$(includedir)
	$(INSTALL_DATA) mc6809dis.h $(DESTDIR)$(includedir)

uninstall:
	$(RM) $(DESTDIR)$(libdir)/libmc6809.a
	$(RM) $(DESTDIR)$(includedir)/mc6809.h 
	$(RM) $(DESTDIR)$(includedir)/mc6809dis.h
