
#ifndef MC6809DIS_H
#define MC6809DIS_H

#include "mc6809.h"

#define MC6809_DIS_DONE	MC6809_FAULT_user

/************************************************************************/

typedef struct mc6809dis
{
  char addr    [ 5];
  char opcode  [ 5];
  char operand [ 7];
  char topcode [ 6];
  char toperand[19];
  char data    [32];

  mc6809addr__t pc;
  mc6809byte__t inst;
  mc6809addr__t next;
  jmp_buf       err;

  void           *user;
  mc6809byte__t (*read) (struct mc6809dis *,mc6809addr__t);
  void          (*fault)(struct mc6809dis *,mc6809fault__t);

} mc6809dis__t;

/************************************************************************/

int mc6809dis_format	(mc6809dis__t *const,char *,size_t) __attribute__((nonnull));
int mc6809dis_registers	(mc6809__t    *const,char *,size_t) __attribute__((nonnull));

int mc6809dis_run	(mc6809dis__t *const,mc6809__t *const) __attribute__((nonnull(1)));
int mc6809dis_step	(mc6809dis__t *const,mc6809__t *const) __attribute__((nonnull(1)));

#endif
