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

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include "mc6809dis.h"

/**************************************************************************/

static int		page2		(mc6809dis__t *const,mc6809__t *const)             __attribute__((nonnull(1)));
static int		page3		(mc6809dis__t *const,mc6809__t *const)             __attribute__((nonnull(1)));
static void		psh		(mc6809dis__t *const,const char *const,const bool) __attribute__((nonnull));
static void		pul		(mc6809dis__t *const,const char *const,const bool) __attribute__((nonnull));
static void		exgtfr		(mc6809dis__t *const,const char *const)            __attribute__((nonnull));
static void		ccimmediate	(mc6809dis__t *const,const char *const)            __attribute__((nonnull));
static mc6809byte__t	cctobyte	(mc6809__t *const)                                 __attribute__((nonnull));

/***************************************************************************/

int mc6809dis_format(mc6809dis__t *const dis,char *dest,size_t size)
{
  assert(dis  != NULL);
  assert(dest != NULL);
  assert(size >= (sizeof(dis->addr) + sizeof(dis->opcode) + sizeof(dis->operand) + sizeof(dis->topcode) + sizeof(dis->toperand) + sizeof(dis->data)));

  snprintf(
	dest,
	size,
	"%-4.4s %-4.4s %-6.6s - %-5.5s %-18.18s ; %s",
	dis->addr,
	dis->opcode,
	dis->operand,
	dis->topcode,
	dis->toperand,
	dis->data
  );
  if (size < (sizeof(dis->addr) + sizeof(dis->opcode) + sizeof(dis->operand) + sizeof(dis->topcode) + sizeof(dis->toperand) + sizeof(dis->data)))
    return EINVAL;

  return 0;    
}

/**************************************************************************/

int mc6809dis_registers(mc6809__t *const cpu,char *dest,size_t size)
{
  char flags[9];
  
  assert(cpu  != NULL);
  assert(dest != NULL);
  assert(size >= 64);

  mc6809dis_cc(flags,sizeof(flags),cctobyte(cpu));    
  snprintf(
  	dest,
  	size,
  	"PC=%04X X=%04X Y=%04X U=%04X S=%04X DP=%02X A=%02X B=%02X CC=%s",
  	cpu->pc.w,
  	cpu->X.w,
  	cpu->Y.w,
  	cpu->U.w,
  	cpu->S.w,
  	cpu->dp,
  	cpu->A,
  	cpu->B,
  	flags
  );
  
  return (size >= 64) ? 0 : EINVAL;
}

/*************************************************************************/  	

int mc6809dis_run(mc6809dis__t *const dis,mc6809__t *const cpu)
{
  char inst[128];
  char regs[128];
  int  rc;
  
  assert(dis != NULL);
  
  while(true)
  {
    rc = mc6809dis_step(dis,cpu);
    if (rc != 0) break;
    mc6809dis_format(dis,inst,sizeof(inst));
    if (cpu != NULL)
    {
      mc6809dis_registers(cpu,regs,sizeof(regs));
      printf("%s | ",regs);
    }
    printf("%s\n",inst);
    dis->pc = dis->next;
  }
  
  return rc;
}

/*************************************************************************/

int mc6809dis_step(mc6809dis__t *dis,mc6809__t *const cpu)
{
  volatile int rc;
  
  assert(dis != NULL);

  rc = setjmp(dis->err);
  if (rc != 0) return rc;
  
  dis->next   = dis->pc;
  dis->inst   = (*dis->read)(dis,dis->next++);
  
  dis->operand[0]  = '\0';
  dis->topcode[0]  = '\0';
  dis->toperand[0] = '\0';
  dis->data[0]     = '\0';
  
  snprintf(dis->addr,  sizeof(dis->addr),  "%04X",dis->pc);
  snprintf(dis->opcode,sizeof(dis->opcode),"%02X",dis->inst);
  
  switch(dis->inst)
  {
    case 0x00:
         mc6809dis_direct(dis,cpu,"NEG",false);
         break;
    
    case 0x01:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x02:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x03:
         mc6809dis_direct(dis,cpu,"COM",false);
         break;
         
    case 0x04:
         mc6809dis_direct(dis,cpu,"LSR",false);
         break;
         
    case 0x05:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x06:
         mc6809dis_direct(dis,cpu,"ROR",false);
         break;
         
    case 0x07:
         mc6809dis_direct(dis,cpu,"ASR",false);
         break;
         
    case 0x08:
         mc6809dis_direct(dis,cpu,"LSL",false);
         break;
    
    case 0x09:
         mc6809dis_direct(dis,cpu,"ROL",false);
         break;
         
    case 0x0A:
         mc6809dis_direct(dis,cpu,"DEC",false);
         break;
         
    case 0x0B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x0C:
         mc6809dis_direct(dis,cpu,"INC",false);
         break;
         
    case 0x0D:
         mc6809dis_direct(dis,cpu,"TST",false);
         break;
         
    case 0x0E:
         mc6809dis_direct(dis,cpu,"JMP",false);
         break;
         
    case 0x0F:
         mc6809dis_direct(dis,cpu,"CLR",false);
         break;
         
    case 0x10:
         return page2(dis,cpu);
    
    case 0x11:
         return page3(dis,cpu);
    
    case 0x12:
         snprintf(dis->topcode,sizeof(dis->topcode),"NOP");
         break;
    
    case 0x13:
         snprintf(dis->topcode,sizeof(dis->topcode),"SYNC");
         break;
    
    case 0x14:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x15:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x16:
         mc6809dis_lrelative(dis,"LBRA","");
         break;
         
    case 0x17:
         mc6809dis_lrelative(dis,"LBSR","");
         break;
    
    case 0x18:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x19:
         snprintf(dis->topcode,sizeof(dis->topcode),"DAA");
         break;
         
    case 0x1A:
         ccimmediate(dis,"ORCC");
         break;
         
    case 0x1B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x1C:
         ccimmediate(dis,"ANDCC");
         break;
    
    case 0x1D:
         snprintf(dis->topcode,sizeof(dis->topcode),"SEX");
         break;
    
    case 0x1E:
         exgtfr(dis,"EXG");
         break;
    
    case 0x1F:
         exgtfr(dis,"TFR");
         break;
         
    case 0x20:
         mc6809dis_relative(dis,"BRA","");
         break;
    
    case 0x21:
         mc6809dis_relative(dis,"BRN","");
         break;
    
    case 0x22:
         mc6809dis_relative(dis,"BHI","unsigned");
         break;
    
    case 0x23:
         mc6809dis_relative(dis,"BLS","unsigned");
         break;
         
    case 0x24:
         mc6809dis_relative(dis,"BHS","unsigned");
         break;
         
    case 0x25:
         mc6809dis_relative(dis,"BLO","unsigned");
         break;
           
    case 0x26:
         mc6809dis_relative(dis,"BNE","");
         break;
         
    case 0x27:
         mc6809dis_relative(dis,"BEQ","");
         break;
         
    case 0x28:
         mc6809dis_relative(dis,"BVC","");
         break;
         
    case 0x29:
         mc6809dis_relative(dis,"BVS","");
         break;
         
    case 0x2A:
         mc6809dis_relative(dis,"BPL","");
         break;
         
    case 0x2B:
         mc6809dis_relative(dis,"BMI","");
         break;
         
    case 0x2C:
         mc6809dis_relative(dis,"BGE","signed");
         break;
         
    case 0x2D:
         mc6809dis_relative(dis,"BLT","signed");
         break;
         
    case 0x2E:
         mc6809dis_relative(dis,"BGT","signed");
         break;
         
    case 0x2F:
         mc6809dis_relative(dis,"BLE","signed");
         break;
    
    case 0x30:
         mc6809dis_indexed(dis,cpu,"LEAX",false);
         break;
    
    case 0x31:
         mc6809dis_indexed(dis,cpu,"LEAY",false);
         break;
         
    case 0x32:
         mc6809dis_indexed(dis,cpu,"LEAS",false);
         break;
    
    case 0x33:
         mc6809dis_indexed(dis,cpu,"LEAU",false);
         break;
         
    case 0x34:
         psh(dis,"PSHS",true);
         break;
    
    case 0x35:
         pul(dis,"PULS",true);
         break;
         
    case 0x36:
         psh(dis,"PSHU",false);
         break;
         
    case 0x37:
         pul(dis,"PULU",false);
         break;
         
    case 0x38:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x39:
         snprintf(dis->topcode,sizeof(dis->topcode),"RTS");
         break;
         
    case 0x3A:
         snprintf(dis->topcode,sizeof(dis->topcode),"ABX");
         break;
         
    case 0x3B:
         snprintf(dis->topcode,sizeof(dis->topcode),"RTI");
         break;
    
    case 0x3C:
         ccimmediate(dis,"CWAI");
         break;
         
    case 0x3D:
         snprintf(dis->topcode,sizeof(dis->topcode),"MUL");
         break;
         
    case 0x3E:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x3F:
         snprintf(dis->topcode,sizeof(dis->topcode),"SWI");
         break;
         
    case 0x40:
         snprintf(dis->topcode,sizeof(dis->topcode),"NEGA");
         break;
         
    case 0x41:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x42:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x43:
         snprintf(dis->topcode,sizeof(dis->topcode),"COMA");
         break;
         
    case 0x44:
         snprintf(dis->topcode,sizeof(dis->topcode),"LSRA");
         break;
         
    case 0x45:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x46:
         snprintf(dis->topcode,sizeof(dis->topcode),"RORA");
         break;
         
    case 0x47:
         snprintf(dis->topcode,sizeof(dis->topcode),"ASRA");
         break;
         
    case 0x48:
         snprintf(dis->topcode,sizeof(dis->topcode),"LSLA");
         break;
         
    case 0x49:
         snprintf(dis->topcode,sizeof(dis->topcode),"ROLA");
         break;
         
    case 0x4A:
         snprintf(dis->topcode,sizeof(dis->topcode),"DECA");
         break;
         
    case 0x4B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x4C:
         snprintf(dis->topcode,sizeof(dis->topcode),"INCA");
         break;
         
    case 0x4D:
         snprintf(dis->topcode,sizeof(dis->topcode),"TSTA");
         break;
         
    case 0x4E:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x4F:
         snprintf(dis->topcode,sizeof(dis->topcode),"CLRA");
         break;

    case 0x50:
         snprintf(dis->topcode,sizeof(dis->topcode),"NEGB");
         break;
         
    case 0x51:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x52:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x53:
         snprintf(dis->topcode,sizeof(dis->topcode),"COMB");
         break;
         
    case 0x54:
         snprintf(dis->topcode,sizeof(dis->topcode),"LSRB");
         break;
         
    case 0x55:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x56:
         snprintf(dis->topcode,sizeof(dis->topcode),"RORB");
         break;
         
    case 0x57:
         snprintf(dis->topcode,sizeof(dis->topcode),"ASRB");
         break;
         
    case 0x58:
         snprintf(dis->topcode,sizeof(dis->topcode),"LSLB");
         break;
         
    case 0x59:
         snprintf(dis->topcode,sizeof(dis->topcode),"ROLB");
         break;
         
    case 0x5A:
         snprintf(dis->topcode,sizeof(dis->topcode),"DECB");
         break;
         
    case 0x5B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x5C:
         snprintf(dis->topcode,sizeof(dis->topcode),"INCB");
         break;
         
    case 0x5D:
         snprintf(dis->topcode,sizeof(dis->topcode),"TSTB");
         break;
         
    case 0x5E:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x5F:
         snprintf(dis->topcode,sizeof(dis->topcode),"CLRB");
         break;

    case 0x60:
         mc6809dis_indexed(dis,cpu,"NEG",false);
         break;
    
    case 0x61:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x62:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x63:
         mc6809dis_indexed(dis,cpu,"COM",false);
         break;
         
    case 0x64:
         mc6809dis_indexed(dis,cpu,"LSR",false);
         break;
         
    case 0x65:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x66:
         mc6809dis_indexed(dis,cpu,"ROR",false);
         break;
         
    case 0x67:
         mc6809dis_indexed(dis,cpu,"ASR",false);
         break;
         
    case 0x68:
         mc6809dis_indexed(dis,cpu,"LSL",false);
         break;
    
    case 0x69:
         mc6809dis_indexed(dis,cpu,"ROL",false);
         break;
         
    case 0x6A:
         mc6809dis_indexed(dis,cpu,"DEC",false);
         break;
         
    case 0x6B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x6C:
         mc6809dis_indexed(dis,cpu,"INC",false);
         break;
         
    case 0x6D:
         mc6809dis_indexed(dis,cpu,"TST",false);
         break;
         
    case 0x6E:
         mc6809dis_indexed(dis,cpu,"JMP",false);
         break;
         
    case 0x6F:
         mc6809dis_indexed(dis,cpu,"CLR",false);
         break;

    case 0x70:
         mc6809dis_extended(dis,cpu,"NEG",false);
         break;
    
    case 0x71:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x72:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x73:
         mc6809dis_extended(dis,cpu,"COM",false);
         break;
         
    case 0x74:
         mc6809dis_extended(dis,cpu,"LSR",false);
         break;
         
    case 0x75:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x76:
         mc6809dis_extended(dis,cpu,"ROR",false);
         break;
         
    case 0x77:
         mc6809dis_extended(dis,cpu,"ASR",false);
         break;
         
    case 0x78:
         mc6809dis_extended(dis,cpu,"LSL",false);
         break;
    
    case 0x79:
         mc6809dis_extended(dis,cpu,"ROL",false);
         break;
         
    case 0x7A:
         mc6809dis_extended(dis,cpu,"DEC",false);
         break;
         
    case 0x7B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x7C:
         mc6809dis_extended(dis,cpu,"INC",false);
         break;
         
    case 0x7D:
         mc6809dis_extended(dis,cpu,"TST",false);
         break;
         
    case 0x7E:
         mc6809dis_extended(dis,cpu,"JMP",false);
         break;
         
    case 0x7F:
         mc6809dis_extended(dis,cpu,"CLR",false);
         break;

    case 0x80:
         mc6809dis_immediate(dis,"SUBA",false);
         break;
    
    case 0x81:
         mc6809dis_immediate(dis,"CMPA",false);
         break;
    
    case 0x82:
         mc6809dis_immediate(dis,"SBCA",false);
         break;
    
    case 0x83:
         mc6809dis_immediate(dis,"SUBD",true);
         break;
    
    case 0x84:
         mc6809dis_immediate(dis,"ANDA",false);
         break;
    
    case 0x85:
         mc6809dis_immediate(dis,"BITA",false);
         break;
    
    case 0x86:
         mc6809dis_immediate(dis,"LDA",false);
         break;
    
    case 0x87:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x88:
         mc6809dis_immediate(dis,"EORA",false);
         break;
    
    case 0x89:
         mc6809dis_immediate(dis,"ADCA",false);
         break;
    
    case 0x8A:
         mc6809dis_immediate(dis,"ORA",false);
         break;
    
    case 0x8B:
         mc6809dis_immediate(dis,"ADDA",false);
         break;
    
    case 0x8C:
         mc6809dis_immediate(dis,"CMPX",true);
         break;
    
    case 0x8D:
         mc6809dis_relative(dis,"BSR","");
         break;
    
    case 0x8E:
         mc6809dis_immediate(dis,"LDX",true);
         break;
    
    case 0x8F:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x90:
         mc6809dis_direct(dis,cpu,"SUBA",false);
         break;
    
    case 0x91:
         mc6809dis_direct(dis,cpu,"CMPA",false);
         break;
    
    case 0x92:
         mc6809dis_direct(dis,cpu,"SBCA",false);
         break;
    
    case 0x93:
         mc6809dis_direct(dis,cpu,"SUBD",true);
         break;
    
    case 0x94:
         mc6809dis_direct(dis,cpu,"ANDA",false);
         break;
    
    case 0x95:
         mc6809dis_direct(dis,cpu,"BITA",false);
         break;
    
    case 0x96:
         mc6809dis_direct(dis,cpu,"LDA",false);
         break;
    
    case 0x97:
         mc6809dis_direct(dis,cpu,"STA",false);
         break;
    
    case 0x98:
         mc6809dis_direct(dis,cpu,"EORA",false);
         break;
    
    case 0x99:
         mc6809dis_direct(dis,cpu,"ADCA",false);
         break;
    
    case 0x9A:
         mc6809dis_direct(dis,cpu,"ORA",false);
         break;
    
    case 0x9B:
         mc6809dis_direct(dis,cpu,"ADDA",false);
         break;
    
    case 0x9C:
         mc6809dis_direct(dis,cpu,"CMPX",true);
         break;
    
    case 0x9D:
         mc6809dis_direct(dis,cpu,"JSR",false);
         break;
    
    case 0x9E:
         mc6809dis_direct(dis,cpu,"LDX",true);
         break;
    
    case 0x9F:
         mc6809dis_direct(dis,cpu,"STX",true);
         break;

    case 0xA0:
         mc6809dis_indexed(dis,cpu,"SUBA",false);
         break;
    
    case 0xA1:
         mc6809dis_indexed(dis,cpu,"CMPA",false);
         break;
    
    case 0xA2:
         mc6809dis_indexed(dis,cpu,"SBCA",false);
         break;
    
    case 0xA3:
         mc6809dis_indexed(dis,cpu,"SUBD",true);
         break;
    
    case 0xA4:
         mc6809dis_indexed(dis,cpu,"ANDA",false);
         break;
    
    case 0xA5:
         mc6809dis_indexed(dis,cpu,"BITA",false);
         break;
    
    case 0xA6:
         mc6809dis_indexed(dis,cpu,"LDA",false);
         break;
    
    case 0xA7:
         mc6809dis_indexed(dis,cpu,"STA",false);
         break;
    
    case 0xA8:
         mc6809dis_indexed(dis,cpu,"EORA",false);
         break;
    
    case 0xA9:
         mc6809dis_indexed(dis,cpu,"ADCA",false);
         break;
    
    case 0xAA:
         mc6809dis_indexed(dis,cpu,"ORA",false);
         break;
    
    case 0xAB:
         mc6809dis_indexed(dis,cpu,"ADDA",false);
         break;
    
    case 0xAC:
         mc6809dis_indexed(dis,cpu,"CMPX",true);
         break;
    
    case 0xAD:
         mc6809dis_indexed(dis,cpu,"JSR",false);
         break;
    
    case 0xAE:
         mc6809dis_indexed(dis,cpu,"LDX",true);
         break;
    
    case 0xAF:
         mc6809dis_indexed(dis,cpu,"STX",true);
         break;

    case 0xB0:
         mc6809dis_extended(dis,cpu,"SUBA",false);
         break;
    
    case 0xB1:
         mc6809dis_extended(dis,cpu,"CMPA",false);
         break;
    
    case 0xB2:
         mc6809dis_extended(dis,cpu,"SBCA",false);
         break;
    
    case 0xB3:
         mc6809dis_extended(dis,cpu,"SUBD",true);
         break;
    
    case 0xB4:
         mc6809dis_extended(dis,cpu,"ANDA",false);
         break;
    
    case 0xB5:
         mc6809dis_extended(dis,cpu,"BITA",false);
         break;
    
    case 0xB6:
         mc6809dis_extended(dis,cpu,"LDA",false);
         break;
    
    case 0xB7:
         mc6809dis_extended(dis,cpu,"STA",false);
         break;
    
    case 0xB8:
         mc6809dis_extended(dis,cpu,"EORA",false);
         break;
    
    case 0xB9:
         mc6809dis_extended(dis,cpu,"ADCA",false);
         break;
    
    case 0xBA:
         mc6809dis_extended(dis,cpu,"ORA",false);
         break;
    
    case 0xBB:
         mc6809dis_extended(dis,cpu,"ADDA",false);
         break;
    
    case 0xBC:
         mc6809dis_extended(dis,cpu,"CMPX",true);
         break;
    
    case 0xBD:
         mc6809dis_extended(dis,cpu,"JSR",false);
         break;
    
    case 0xBE:
         mc6809dis_extended(dis,cpu,"LDX",true);
         break;
    
    case 0xBF:
         mc6809dis_extended(dis,cpu,"STX",true);
         break;

    case 0xC0:
         mc6809dis_immediate(dis,"SUBB",false);
         break;
    
    case 0xC1:
         mc6809dis_immediate(dis,"CMPB",false);
         break;
    
    case 0xC2:
         mc6809dis_immediate(dis,"SBCB",false);
         break;
    
    case 0xC3:
         mc6809dis_immediate(dis,"ADDD",true);
         break;
    
    case 0xC4:
         mc6809dis_immediate(dis,"ANDB",false);
         break;
    
    case 0xC5:
         mc6809dis_immediate(dis,"BITB",false);
         break;
    
    case 0xC6:
         mc6809dis_immediate(dis,"LDB",false);
         break;
    
    case 0xC7:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0xC8:
         mc6809dis_immediate(dis,"EORB",false);
         break;
    
    case 0xC9:
         mc6809dis_immediate(dis,"ADCB",false);
         break;
    
    case 0xCA:
         mc6809dis_immediate(dis,"ORB",false);
         break;
    
    case 0xCB:
         mc6809dis_immediate(dis,"ADDB",false);
         break;
    
    case 0xCC:
         mc6809dis_immediate(dis,"LDD",true);
         break;
    
    case 0xCD:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0xCE:
         mc6809dis_immediate(dis,"LDU",true);
         break;
    
    case 0xCF:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;

    case 0xD0:
         mc6809dis_direct(dis,cpu,"SUBB",false);
         break;
    
    case 0xD1:
         mc6809dis_direct(dis,cpu,"CMPB",false);
         break;
    
    case 0xD2:
         mc6809dis_direct(dis,cpu,"SBCB",false);
         break;
    
    case 0xD3:
         mc6809dis_direct(dis,cpu,"ADDD",true);
         break;
    
    case 0xD4:
         mc6809dis_direct(dis,cpu,"ANDB",false);
         break;
    
    case 0xD5:
         mc6809dis_direct(dis,cpu,"BITB",false);
         break;
    
    case 0xD6:
         mc6809dis_direct(dis,cpu,"LDB",false);
         break;
    
    case 0xD7:
         mc6809dis_direct(dis,cpu,"STB",false);
         break;
    
    case 0xD8:
         mc6809dis_direct(dis,cpu,"EORB",false);
         break;
    
    case 0xD9:
         mc6809dis_direct(dis,cpu,"ADCB",false);
         break;
    
    case 0xDA:
         mc6809dis_direct(dis,cpu,"ORB",false);
         break;
    
    case 0xDB:
         mc6809dis_direct(dis,cpu,"ADDB",false);
         break;
    
    case 0xDC:
         mc6809dis_direct(dis,cpu,"LDD",true);
         break;
    
    case 0xDD:
    	 mc6809dis_direct(dis,cpu,"STD",true);
         break;
    
    case 0xDE:
         mc6809dis_direct(dis,cpu,"LDU",true);
         break;
    
    case 0xDF:
         mc6809dis_direct(dis,cpu,"STU",true);
         break;

    case 0xE0:
         mc6809dis_indexed(dis,cpu,"SUBB",false);
         break;
    
    case 0xE1:
         mc6809dis_indexed(dis,cpu,"CMPB",false);
         break;
    
    case 0xE2:
         mc6809dis_indexed(dis,cpu,"SBCB",false);
         break;
    
    case 0xE3:
         mc6809dis_indexed(dis,cpu,"ADDD",true);
         break;
    
    case 0xE4:
         mc6809dis_indexed(dis,cpu,"ANDB",false);
         break;
    
    case 0xE5:
         mc6809dis_indexed(dis,cpu,"BITB",false);
         break;
    
    case 0xE6:
         mc6809dis_indexed(dis,cpu,"LDB",false);
         break;
    
    case 0xE7:
         mc6809dis_indexed(dis,cpu,"STB",false);
         break;
    
    case 0xE8:
         mc6809dis_indexed(dis,cpu,"EORB",false);
         break;
    
    case 0xE9:
         mc6809dis_indexed(dis,cpu,"ADCB",false);
         break;
    
    case 0xEA:
         mc6809dis_indexed(dis,cpu,"ORB",false);
         break;
    
    case 0xEB:
         mc6809dis_indexed(dis,cpu,"ADDB",false);
         break;
    
    case 0xEC:
         mc6809dis_indexed(dis,cpu,"LDD",true);
         break;
    
    case 0xED:
         mc6809dis_indexed(dis,cpu,"STD",true);
         break;
    
    case 0xEE:
         mc6809dis_indexed(dis,cpu,"LDU",true);
         break;
    
    case 0xEF:
         mc6809dis_indexed(dis,cpu,"STU",true);
         break;

    case 0xF0:
         mc6809dis_extended(dis,cpu,"SUBB",false);
         break;
    
    case 0xF1:
         mc6809dis_extended(dis,cpu,"CMPB",false);
         break;
    
    case 0xF2:
         mc6809dis_extended(dis,cpu,"SBCB",false);
         break;
    
    case 0xF3:
         mc6809dis_extended(dis,cpu,"ADDD",true);
         break;
    
    case 0xF4:
         mc6809dis_extended(dis,cpu,"ANDB",false);
         break;
    
    case 0xF5:
         mc6809dis_extended(dis,cpu,"BITB",false);
         break;
    
    case 0xF6:
         mc6809dis_extended(dis,cpu,"LDB",false);
         break;
    
    case 0xF7:
         mc6809dis_extended(dis,cpu,"STB",false);
         break;
    
    case 0xF8:
         mc6809dis_extended(dis,cpu,"EORB",false);
         break;
    
    case 0xF9:
         mc6809dis_extended(dis,cpu,"ADCB",false);
         break;
    
    case 0xFA:
         mc6809dis_extended(dis,cpu,"ORB",false);
         break;
    
    case 0xFB:
         mc6809dis_extended(dis,cpu,"ADDB",false);
         break;
    
    case 0xFC:
         mc6809dis_extended(dis,cpu,"LDD",true);
         break;
    
    case 0xFD:
         mc6809dis_extended(dis,cpu,"STD",true);
         break;
    
    case 0xFE:
         mc6809dis_extended(dis,cpu,"LDU",true);
         break;
    
    case 0xFF:
         mc6809dis_extended(dis,cpu,"STU",true);
         break;

    default:
         assert(0);
         (*cpu->fault)(cpu,MC6809_FAULT_INTERNAL_ERROR);
         break;
  }
  
  return 0;
}

/************************************************************************/

static int page2(mc6809dis__t *const dis,mc6809__t *const cpu)
{
  mc6809byte__t byte;
  
  assert(dis != NULL);
  
  byte = (*dis->read)(dis,dis->next++);
  snprintf(&dis->opcode[2],sizeof(dis->opcode)-2,"%02X",byte);
  
  switch(byte)
  {
    case 0x21:
         mc6809dis_lrelative(dis,"LBRN","");
         break;
    
    case 0x22:
         mc6809dis_lrelative(dis,"LBHI","unsigned");
         break;
    
    case 0x23:
         mc6809dis_lrelative(dis,"LBLS","unsigned");
         break;
         
    case 0x24:
         mc6809dis_lrelative(dis,"LBHS","unsigned");
         break;
         
    case 0x25:
         mc6809dis_lrelative(dis,"LBLO","unsigned");
         break;
           
    case 0x26:
         mc6809dis_lrelative(dis,"LBNE","");
         break;
         
    case 0x27:
         mc6809dis_lrelative(dis,"LBEQ","");
         break;
         
    case 0x28:
         mc6809dis_lrelative(dis,"LBVC","");
         break;
         
    case 0x29:
         mc6809dis_lrelative(dis,"LBVS","");
         break;
         
    case 0x2A:
         mc6809dis_lrelative(dis,"LBPL","");
         break;
         
    case 0x2B:
         mc6809dis_lrelative(dis,"LBMI","");
         break;
         
    case 0x2C:
         mc6809dis_lrelative(dis,"LBGE","signed");
         break;
         
    case 0x2D:
         mc6809dis_lrelative(dis,"LBLT","signed");
         break;
         
    case 0x2E:
         mc6809dis_lrelative(dis,"LBGT","signed");
         break;
         
    case 0x2F:
         mc6809dis_lrelative(dis,"LBLE","signed");
         break;
 
    case 0x3F:
         snprintf(dis->topcode,sizeof(dis->topcode),"SWI2");
         break;
         
    case 0x83:
         mc6809dis_immediate(dis,"CMPD",true);
         break;
         
    case 0x8C:
         mc6809dis_immediate(dis,"CMPY",true);
         break;
         
    case 0x8E:
         mc6809dis_immediate(dis,"LDY",true);
         break;
    
    case 0x93:
         mc6809dis_direct(dis,cpu,"CMPD",true);
         break;
         
    case 0x9C:
         mc6809dis_direct(dis,cpu,"CMPY",true);
         break;
         
    case 0x9E:
         mc6809dis_direct(dis,cpu,"LDY",true);
         break;
         
    case 0x9F:
         mc6809dis_direct(dis,cpu,"STY",true);
         break;
    
    case 0xA3:
         mc6809dis_indexed(dis,cpu,"CMPD",true);
         break;
    
    case 0xAC:
         mc6809dis_indexed(dis,cpu,"CMPY",true);
         break;
    
    case 0xAE:
         mc6809dis_indexed(dis,cpu,"LDY",true);
         break;
    
    case 0xAF:
         mc6809dis_indexed(dis,cpu,"STY",true);
         break;
    
    case 0xB3:
         mc6809dis_extended(dis,cpu,"CMPD",true);
         break;
    
    case 0xBC:
         mc6809dis_extended(dis,cpu,"CMPY",true);
         break;
    
    case 0xBE:
         mc6809dis_extended(dis,cpu,"LDY",true);
         break;
         
    case 0xBF:
         mc6809dis_extended(dis,cpu,"STY",true);
         break;
         
    case 0xCE:
         mc6809dis_immediate(dis,"LDS",true);
         break;
    
    case 0xDE:
         mc6809dis_direct(dis,cpu,"LDS",true);
         break;
         
    case 0xDF:
         mc6809dis_direct(dis,cpu,"STS",true);
         break;
    
    case 0xEE:
         mc6809dis_indexed(dis,cpu,"LDS",true);
         break;
    
    case 0xEF:
         mc6809dis_indexed(dis,cpu,"STS",true);
         break;
    
    case 0xFE:
         mc6809dis_extended(dis,cpu,"LDS",true);
         break;
    
    case 0xFF:
         mc6809dis_extended(dis,cpu,"STS",true);
         break;
    
    default:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
  }
  
  return 0;
}

/************************************************************************/

static int page3(mc6809dis__t *const dis,mc6809__t *const cpu)
{
  mc6809byte__t byte;
  
  assert(dis != NULL);
  
  byte = (*dis->read)(dis,dis->next++);
  snprintf(&dis->opcode[2],sizeof(dis->opcode)-2,"%02X",byte);
  
  switch(byte)
  {
    case 0x3F:
         snprintf(dis->topcode,sizeof(dis->topcode),"SWI3");
         break;
    
    case 0x83:
         mc6809dis_immediate(dis,"CMPU",true);
         break;
    
    case 0x8C:
         mc6809dis_immediate(dis,"CMPS",true);
         break;
    
    case 0x93:
         mc6809dis_direct(dis,cpu,"CMPU",true);
         break;
    
    case 0x9C:
         mc6809dis_direct(dis,cpu,"CMPS",true);
         break;
    
    case 0xA3:
         mc6809dis_indexed(dis,cpu,"CMPU",true);
         break;
    
    case 0xAC:
         mc6809dis_indexed(dis,cpu,"CMPS",true);
         break;
    
    case 0xB3:
         mc6809dis_extended(dis,cpu,"CMPU",true);
         break;
    
    case 0xBC:
         mc6809dis_extended(dis,cpu,"CMPS",true);
         break;
         
    default:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
  }
  
  return 0;
}

/***********************************************************************/

void mc6809dis_indexed(
	mc6809dis__t  *const dis,
	mc6809__t     *const cpu,
	const char    *const op,
	const bool           b16
)
{
  static const char regs[4] = "XYUS";
  mc6809word__t addr;
  mc6809word__t iaddr;
  mc6809word__t d16;
  mc6809byte__t byte;
  mc6809byte__t mode;
  int           reg;
  int           off;
  int           ioff;
  bool          ind;
  
  assert(dis  != NULL);
  assert(op   != NULL);
  
  mode = (*dis->read)(dis,dis->next++);
  reg  = (mode & 0x60) >> 5;
  off  = (mode & 0x1F);
  ind  = false;
  
  snprintf(dis->operand,sizeof(dis->operand),"%02X",mode);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  
  if (mode < 0x80)
  {
    if (off > 15) off -= 32;

    snprintf(dis->toperand,sizeof(dis->toperand),"%d,%c",off,regs[reg]);    
    if (cpu != NULL)
    {
      addr.w = cpu->index[reg].w + (mc6809addr__t)off;
      if (b16)
      {
        d16.b[MSB] = (*dis->read)(dis,addr.w);
        d16.b[LSB] = (*dis->read)(dis,addr.w + 1);
        snprintf(dis->data,sizeof(dis->data),"%04X = %04X",addr.w,d16.w);
      }
      else
      {
        byte = (*dis->read)(dis,addr.w);
        snprintf(dis->data,sizeof(dis->data),"%04X = %02X",addr.w,byte);
      }
    }
    return;
  }

  switch(off)
  {
    case 0x00:
         snprintf(dis->toperand,sizeof(dis->toperand),",%c+",regs[reg]);         
         if (cpu != NULL)
           addr.w = cpu->index[reg].w;
         break;
         
    case 0x01:
         snprintf(dis->toperand,sizeof(dis->toperand),",%c++",regs[reg]);         
         if (cpu != NULL)
           addr.w = cpu->index[reg].w;
         break;
         
    case 0x02:
         snprintf(dis->toperand,sizeof(dis->toperand),",-%c",regs[reg]);         
         if (cpu != NULL)
           addr.w = cpu->index[reg].w - 1;
         break;
         
    case 0x03:
         snprintf(dis->toperand,sizeof(dis->toperand),",--%c",regs[reg]);
         if (cpu != NULL)
           addr.w = cpu->index[reg].w - 2;
         break;
         
    case 0x04:
         snprintf(dis->toperand,sizeof(dis->toperand),",%c",regs[reg]);
         if (cpu != NULL)
           addr.w = cpu->index[reg].w;
         break;
         
    case 0x05:
         snprintf(dis->toperand,sizeof(dis->toperand),"B,%c",regs[reg]);
         if (cpu != NULL)
         {
           addr.b[LSB]  = cpu->B;
           addr.b[MSB]  = (cpu->B < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
         }
         break;
         
    case 0x06:
         snprintf(dis->toperand,sizeof(dis->toperand),"A,%c",regs[reg]);
         if (cpu != NULL)
         {
           addr.b[LSB]  = cpu->A;
           addr.b[MSB]  = (cpu->A < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
         }
         break;
         
    case 0x07:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x08:
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[LSB] < 0x80) ? addr.b[LSB] : addr.b[LSB] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[LSB]);
         snprintf(dis->toperand,sizeof(dis->toperand),"%d,%c",ioff,regs[reg]);
         if (cpu != NULL)
         {
           addr.b[MSB]  = (addr.b[LSB] < 0x80) ? 0x00: 0xFF;
           addr.w    += cpu->index[reg].w;
         }
         break;
         
    case 0x09:
         addr.b[MSB] = (*dis->read)(dis,dis->next++);
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = ((int16_t)addr.w);
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%04X",addr.w);
         snprintf(dis->toperand,sizeof(dis->toperand),"%d,%c",ioff,regs[reg]);
         if (cpu != NULL)
           addr.w += cpu->index[reg].w;
         break;
                  
    case 0x0A:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x0B:
         snprintf(dis->toperand,sizeof(dis->toperand),"D,%c",regs[reg]);
         if (cpu != NULL)
           addr.w = cpu->index[reg].w + cpu->d.w;
         break;
         
    case 0x0C:
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[LSB] < 0x80) ? addr.b[LSB] : addr.b[LSB] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[LSB]);
         snprintf(dis->toperand,sizeof(dis->toperand),"%d,PC",ioff);
         if (cpu != NULL)
         {
           addr.b[MSB]  = (addr.b[LSB] < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->pc.w;
         }
         break;
         
    case 0x0D:
         addr.b[MSB] = (*dis->read)(dis,dis->next++);
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = ((int16_t)addr.w);
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%04X",addr.w);
         snprintf(dis->toperand,sizeof(dis->toperand),"%d,PC",ioff);
         if (cpu != NULL)
           addr.w += cpu->pc.w;
         break;
         
    case 0x0E:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x0F:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x10:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x11:
         snprintf(dis->toperand,sizeof(dis->toperand),"[,%c++]",regs[reg]);
         if (cpu != NULL)
         {
           addr.w = cpu->index[reg].w;
           ind    = true;
         }
         break;
         
    case 0x12:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x13:
         snprintf(dis->toperand,sizeof(dis->toperand),"[,--%c]",regs[reg]);
         if (cpu != NULL)
         {
           addr.w = cpu->index[reg].w - 2;
           ind    = true;
         }
         break;
         
    case 0x14:
         snprintf(dis->toperand,sizeof(dis->toperand),"[,%c]",regs[reg]);
         if (cpu != NULL)
         {
           addr.w = cpu->index[reg].w;
           ind    = true;
         }
         break;
         
    case 0x15:
         snprintf(dis->toperand,sizeof(dis->toperand),"[B,%c]",regs[reg]);
         if (cpu != NULL)
         {
           addr.b[LSB]  = cpu->B;
           addr.b[MSB]  = (cpu->B < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
           ind        = true;
         }
         break;
         
    case 0x16:
         snprintf(dis->toperand,sizeof(dis->toperand),"[A,%c]",regs[reg]);
         if (cpu != NULL)
         {
           addr.b[LSB]  = cpu->A;
           addr.b[MSB]  = (cpu->A < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
           ind        = true;
         }
         break;
         
    case 0x17:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x18:
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[LSB] < 0x80) ? addr.b[LSB] : addr.b[LSB] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[LSB]);
         snprintf(dis->toperand,sizeof(dis->toperand),"[%d,%c]",ioff,regs[reg]);
         if (cpu != NULL)
         {
           addr.b[MSB]  = (addr.b[LSB] < 0x80) ? 0x00: 0xFF;
           addr.w    += cpu->index[reg].w;
           ind        = true;
         }
         break;

    case 0x19:
         addr.b[MSB] = (*dis->read)(dis,dis->next++);
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = ((int16_t)addr.w);
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%04X",addr.w);
         snprintf(dis->toperand,sizeof(dis->toperand),"[%d,%c]",ioff,regs[reg]);
         if (cpu != NULL)
         {
           addr.w += cpu->index[reg].w;
           ind     = true;
         }
         break;
         
    case 0x1A:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x1B:
         snprintf(dis->toperand,sizeof(dis->toperand),"[D,%c]",regs[reg]);
         if (cpu != NULL)
         {
           addr.w = cpu->index[reg].w + cpu->d.w;
           ind    = true;
         }
         break;
         
    case 0x1C:
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[LSB] < 0x80) ? addr.b[LSB] : addr.b[LSB] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[LSB]);
         snprintf(dis->toperand,sizeof(dis->toperand),"[%d,PC]",ioff);
         if (cpu != NULL)
         {
           addr.b[MSB]  = (addr.b[LSB] < 0x80) ? 0x00 : 0xFF;
           addr.w    += dis->next;
           ind        = true;
         }
         break;
         
    case 0x1D:
         addr.b[MSB] = (*dis->read)(dis,dis->next++);
         addr.b[LSB] = (*dis->read)(dis,dis->next++);
         ioff      = ((int16_t)addr.w);
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%04X",addr.w);
         snprintf(dis->toperand,sizeof(dis->toperand),"[%d,PC]",ioff);
         if (cpu != NULL)
         {
           addr.w += dis->next;
           ind     = true;
         }
         break;
         
    case 0x1E:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x1F:
         if (reg == 0)
         {
           addr.b[MSB] = (*dis->read)(dis,dis->next++);
           addr.b[LSB] = (*dis->read)(dis,dis->next++);
           snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%04X",addr.w);
           snprintf(dis->toperand,sizeof(dis->toperand),"[%04X]",addr.w);
           ind = true;
         }
         else
           (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    default:
         assert(0);
         (*dis->fault)(dis,MC6809_FAULT_INTERNAL_ERROR);
         break;
  }

  if (cpu != NULL)
  {
    if (ind)
    {
      iaddr.b[MSB] = (*dis->read)(dis,addr.w);
      iaddr.b[LSB] = (*dis->read)(dis,addr.w+1);
      
      if (b16)
      {
        d16.b[MSB] = (*dis->read)(dis,iaddr.w);
        d16.b[LSB] = (*dis->read)(dis,iaddr.w + 1);
        snprintf(dis->data,sizeof(dis->data),"[%04X] = %04X = %04X",addr.w,iaddr.w,d16.w);
      }
      else
      {
        byte = (*dis->read)(dis,iaddr.w);      
        snprintf(dis->data,sizeof(dis->data),"[%04X] = %04X = %02X",addr.w,iaddr.w,byte);
      }
    }
    else
    {
      if (b16)
      {
        d16.b[MSB] = (*dis->read)(dis,addr.w);
        d16.b[LSB] = (*dis->read)(dis,addr.w + 1);
        snprintf(dis->data,sizeof(dis->data),"%04X = %04X",addr.w,d16.w);
      }
      else
      {
        byte = (*dis->read)(dis,addr.w);
        snprintf(dis->data,sizeof(dis->data),"%04X = %02X",addr.w,byte);
      }
    }
  }  
}

/*************************************************************************/

void mc6809dis_immediate(
	mc6809dis__t *const dis,
	const char   *const op,
	const bool          b16
)
{
  mc6809word__t d16;
  mc6809byte__t byte;
  
  assert(dis != NULL);
  assert(op  != NULL);
  
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  
  if (b16)
  {
    d16.b[MSB] = (*dis->read)(dis,dis->next++);
    d16.b[LSB] = (*dis->read)(dis,dis->next++);
    snprintf(dis->operand,sizeof(dis->operand),"%04X",d16.w);
    snprintf(dis->toperand,sizeof(dis->toperand),"#%04X",d16.w);
  }
  else
  {
    byte = (*dis->read)(dis,dis->next++);
    snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
    snprintf(dis->toperand,sizeof(dis->toperand),"#%02X",byte);
  }
}

/*************************************************************************/

void mc6809dis_direct(
	mc6809dis__t *const dis,
	mc6809__t    *const cpu,
	const char   *const op,
	const bool          b16
)
{
  mc6809word__t addr;
  mc6809word__t word;
  mc6809byte__t byte;
  
  assert(dis != NULL);
  assert(op  != NULL);
  
  addr.b[MSB] = 0;
  addr.b[LSB] = (*dis->read)(dis,dis->next++);
  
  snprintf(dis->operand,sizeof(dis->operand),"%02X",addr.b[LSB]);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  snprintf(dis->toperand,sizeof(dis->toperand),"%02X",addr.b[LSB]);
  
  if (cpu != NULL)
  {
    if (b16)
    {
      word.b[MSB] = (*dis->read)(dis,addr.w++);
      word.b[LSB] = (*dis->read)(dis,addr.w);
      snprintf(dis->data,sizeof(dis->data),"%04X",word.w);
    }
    else
    {
      byte = (*dis->read)(dis,addr.w);
      snprintf(dis->data,sizeof(dis->data),"%02X",byte);
    }
  }
}
  
/*************************************************************************/  

void mc6809dis_extended(
	mc6809dis__t *const dis,
	mc6809__t    *const cpu,
	const char   *const op,
	const bool          b16
)
{
  mc6809word__t addr;
  mc6809word__t word;
  mc6809byte__t byte;
  
  assert(dis != NULL);
  assert(op  != NULL);
  
  addr.b[MSB] = (*dis->read)(dis,dis->next++);
  addr.b[LSB] = (*dis->read)(dis,dis->next++);
  
  snprintf(dis->operand,sizeof(dis->operand),"%04X",addr.w);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  snprintf(dis->toperand,sizeof(dis->toperand),"%04X",addr.w);
  
  if (cpu != NULL)
  {
    if (b16)
    {
      word.b[MSB] = (*dis->read)(dis,addr.w++);
      word.b[LSB] = (*dis->read)(dis,addr.w);
      snprintf(dis->data,sizeof(dis->data),"%04X",word.w);
    }
    else
    {
      byte = (*dis->read)(dis,addr.w);
      snprintf(dis->data,sizeof(dis->data),"%02X",byte);
    }
  }
}

/*************************************************************************/

void mc6809dis_relative(
	mc6809dis__t *const dis,
	const char   *const op,
	const char   *const data
)
{
  mc6809word__t addr;
  
  addr.b[LSB] = (*dis->read)(dis,dis->next++);
  addr.b[MSB] = (addr.b[LSB] < 0x80) ? 0x00 : 0xFF;
  snprintf(dis->operand,sizeof(dis->operand),"%02X",addr.b[LSB]);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  
  addr.w += dis->next;
  snprintf(dis->toperand,sizeof(dis->toperand),"%04X",addr.w);
  if (addr.w < dis->next)
    snprintf(dis->data,sizeof(dis->data),"backwards %s",data);
  else
    snprintf(dis->data,sizeof(dis->data),"forwards %s",data);
}

/*************************************************************************/

void mc6809dis_lrelative(
	mc6809dis__t *const dis,
	const char   *const op,
	const char   *const data
)
{
  mc6809word__t addr;
  
  addr.b[MSB] = (*dis->read)(dis,dis->next++);
  addr.b[LSB] = (*dis->read)(dis,dis->next++);
  snprintf(dis->operand,sizeof(dis->operand),"%04X",addr.w);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  
  addr.w += dis->next;
  snprintf(dis->toperand,sizeof(dis->toperand),"%04X",addr.w);
  if (addr.w < dis->next)
    snprintf(dis->data,sizeof(dis->data),"backwards %s",data);
  else
    snprintf(dis->data,sizeof(dis->data),"forwards %s",data);
}

/*************************************************************************/

static const char *const m_pshsreg[] =
{
  "CC" , "A" , "B" , "DP" , "X" , "Y" , "U" , "PC"
};

static const char *const m_pshureg[] =
{
  "CC" , "A" , "B" , "DP" , "X" , "Y" , "S" , "PC"
};

/*************************************************************************/

void mc6809dis_pshregs(char *dest,size_t size,mc6809byte__t post,bool s)
{
  const char *const *regs = s ? m_pshsreg : m_pshureg;
  const char        *sep  = "";
  
  for (int i = 7 ; i > -1 ; i--)
  {
    int mask = 1 << i;
    
    if ((post & mask) != 0)
    {   
      size_t bytes  = snprintf(dest,size,"%s%s",sep,regs[i]);
      size         -= bytes;
      dest         += bytes;
      sep           = ",";
    }
  }
}

/************************************************************************/

void mc6809dis_pulregs(char *dest,size_t size,mc6809byte__t post,bool s)
{
  const char *const *regs = s ? m_pshsreg : m_pshureg;
  const char        *sep  = "";

  for (int i = 0 ; i < 8 ; i++)
  {
    int mask = 1 << i;
    
    if ((post & mask) != 0)
    {
      size_t bytes = snprintf(dest,size,"%s%s",sep,regs[i]);
      size         -= bytes;
      dest         += bytes;
      sep           = ",";
    }
  }
}

/************************************************************************/

static void psh(mc6809dis__t *const dis,const char *const op,const bool s)
{
  mc6809byte__t      post;

  post = (*dis->read)(dis,dis->next++);
  snprintf(dis->operand,sizeof(dis->operand),"%02X",post);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);

  mc6809dis_pshregs(dis->toperand,sizeof(dis->toperand),post,s);
}

/*************************************************************************/

static void pul(mc6809dis__t *const dis,const char *const op,const bool s)
{
  mc6809byte__t      post;
  
  post = (*dis->read)(dis,dis->next++);
  snprintf(dis->operand,sizeof(dis->operand),"%02X",post);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  
  mc6809dis_pulregs(dis->toperand,sizeof(dis->toperand),post,s);
}

/*************************************************************************/

static void exgtfr(mc6809dis__t *const dis,const char *const op)
{
  static const char *const etregs[] =
  {
    "D" , "X" , "Y"  , "U"  , "S"  , "PC" , NULL , NULL ,
    "A" , "B" , "CC" , "DP" , NULL , NULL , NULL , NULL
  };

  mc6809byte__t  byte;
  const char    *s;
  const char    *d;
  
  assert(dis != NULL);
  assert(op  != NULL);
  
  byte = (*dis->read)(dis,dis->next++);
  snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
         
  if (((byte & 0x88) == 0x00) || ((byte & 0x88) == 0x88))
  {
    s = etregs[byte >> 4];
    d = etregs[byte &  0x0F];
           
    if ((s != NULL) && (d != NULL))
      snprintf(dis->toperand,sizeof(dis->toperand),"%s,%s",s,d);
    else
      (*dis->fault)(dis,MC6809_FAULT_EXG);
  }
  else
    (*dis->fault)(dis,MC6809_FAULT_EXG);  
}

/*************************************************************************/

static void ccimmediate(mc6809dis__t *const dis,const char *const op)
{
  mc6809byte__t byte;
  
  assert(dis != NULL);
  assert(op  != NULL);

  byte = (*dis->read)(dis,dis->next++);
  snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  snprintf(dis->toperand,sizeof(dis->toperand),"#%02X",byte);
  mc6809dis_cc(dis->data,sizeof(dis->data),byte);
}

/*************************************************************************/

void mc6809dis_cc(char *dest,size_t size __attribute__((unused)),mc6809byte__t c)
{
  assert(dest != NULL);
  assert(size >= 9);
  
  dest[0] = (c & 0x80) ? 'e' : '-';
  dest[1] = (c & 0x40) ? 'f' : '-';
  dest[2] = (c & 0x20) ? 'h' : '-';
  dest[3] = (c & 0x10) ? 'i' : '-';
  dest[4] = (c & 0x08) ? 'n' : '-';
  dest[5] = (c & 0x04) ? 'z' : '-';
  dest[6] = (c & 0x02) ? 'v' : '-';
  dest[7] = (c & 0x01) ? 'c' : '-';
  dest[8] = '\0';
}

/*************************************************************************/

static mc6809byte__t cctobyte(mc6809__t *const cpu)
{
  mc6809byte__t res = 0;
  
  assert(cpu != NULL);
  
  res |= cpu->cc.e ? 0x80 : 0x00;
  res |= cpu->cc.f ? 0x40 : 0x00;
  res |= cpu->cc.h ? 0x20 : 0x00;
  res |= cpu->cc.i ? 0x10 : 0x00;
  res |= cpu->cc.n ? 0x08 : 0x00;
  res |= cpu->cc.z ? 0x04 : 0x00;
  res |= cpu->cc.v ? 0x02 : 0x00;
  res |= cpu->cc.c ? 0x01 : 0x00;
  return res;
}

/**************************************************************************/
