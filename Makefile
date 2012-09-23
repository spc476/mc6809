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

.PHONY = all clean

CC     = gcc -std=c99
CFLAGS = -g -Wall -Wextra -pedantic
LFLAGS = -g

all : mc09emulator mc09disasm 

mc09emulator : mc09emulator.o mc6809.o mc6809dis.o
	$(CC) -o $@ $^ $(LFLAGS)

mc09disasm : mc09disasm.o mc6809dis.o
	$(CC) -o $@ $^ $(LFLAGS)

mc09emulator.o : mc09emulator.c mc6809.h mc6809dis.h
	$(CC) $(CFLAGS) -c -o $@ $<

mc09disasm.o : mc09disasm.c mc6809dis.h
	$(CC) $(CFLAGS) -c -o $@ $<

mc6809.o : mc6809.c mc6809.h
	$(CC) $(CFLAGS) -c -o $@ $<

mc6809dis.o : mc6809dis.c mc6809.h mc6809dis.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	/bin/rm -rf *.o mc09emulator mc09disasm *~
