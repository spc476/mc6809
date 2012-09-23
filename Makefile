
.PHONY = all clean

CC     = gcc -std=c99
CFLAGS = -g -Wall -Wextra -pedantic
LFLAGS = -g

all : mc09disasm 

mc09disasm : mc09disasm.o mc6809dis.o
	$(CC) -o $@ $^ $(LFLAGS)

mc09disasm.o : mc09disasm.c mc6809dis.h
	$(CC) $(CFLAGS) -c -o $@ $<

mc6809.o : mc6809.c mc6809.h
	$(CC) $(CFLAGS) -c -o $@ $<

mc6809dis.o : mc6809dis.c mc6809.h mc6809dis.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	/bin/rm -rf *.o mc09disasm *~
