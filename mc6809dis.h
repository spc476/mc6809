/********************************************
*
* Copyright 2012 by Sean Conner.  All Rights Reserved.
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or (at your
* option) any later version.
*
* This library is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
* License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; if not, see <http://www.gnu.org/licenses/>.
*
* Comments, questions and criticisms can be sent to: sean@conman.org
*
*********************************************/

#ifndef MC6809DIS_H
#define MC6809DIS_H

#include "mc6809.h"

#define MC6809_DIS_DONE MC6809_FAULT_user

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
  mc6809addr__t next;
  jmp_buf       err;
  
  void           *user;
  mc6809byte__t (*read) (struct mc6809dis *,mc6809addr__t);
  void          (*fault)(struct mc6809dis *,mc6809fault__t);
  
} mc6809dis__t;

/************************************************************************/

int mc6809dis_format    (mc6809dis__t *const,char *,size_t) __attribute__((nonnull));
int mc6809dis_registers (mc6809__t    *const,char *,size_t) __attribute__((nonnull));

int mc6809dis_run       (mc6809dis__t *const,mc6809__t *const) __attribute__((nonnull(1)));
int mc6809dis_step      (mc6809dis__t *const,mc6809__t *const) __attribute__((nonnull(1)));

void mc6809dis_indexed  (mc6809dis__t *const,mc6809__t  *const,const char *const,const bool) __attribute__((nonnull(1,3)));
void mc6809dis_immediate(mc6809dis__t *const,const char *const,const bool)                   __attribute__((nonnull(1)));
void mc6809dis_direct   (mc6809dis__t *const,mc6809__t  *const,const char *const,const bool) __attribute__((nonnull(1,3)));
void mc6809dis_extended (mc6809dis__t *const,mc6809__t  *const,const char *const,const bool) __attribute__((nonnull(1,3)));
void mc6809dis_relative (mc6809dis__t *const,const char *const,const char *const)            __attribute__((nonnull));
void mc6809dis_lrelative(mc6809dis__t *const,const char *const,const char *const)            __attribute__((nonnull));

void mc6809dis_pshregs  (char *,size_t,mc6809byte__t,bool);
void mc6809dis_pulregs  (char *,size_t,mc6809byte__t,bool);
void mc6809dis_cc       (char *,size_t,mc6809byte__t);

#endif
