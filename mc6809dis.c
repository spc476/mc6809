
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include "mc6809dis.h"

/**************************************************************************/

static int		page2		(mc6809dis__t *const,mc6809__t *const) __attribute__((nonnull(1)));
static int		page3		(mc6809dis__t *const,mc6809__t *const) __attribute__((nonnull(1)));

static void		indexed		(mc6809dis__t *const,mc6809__t *const,const char *const,const bool) __attribute__((nonnull(1,3)));
static void		immediate	(mc6809dis__t *const,const char *const,const bool)                  __attribute__((nonnull(1)));
static void		direct		(mc6809dis__t *const,mc6809__t *const,const char *const,const bool) __attribute__((nonnull(1,3)));
static void		extended	(mc6809dis__t *const,mc6809__t *const,const char *const,const bool) __attribute__((nonnull(1,3)));
static void		relative	(mc6809dis__t *const,const char *const,const char *const)           __attribute__((nonnull));
static void		lrelative	(mc6809dis__t *const,const char *const,const char *const)           __attribute__((nonnull));
static void		psh		(mc6809dis__t *const,const char *const,const bool)                  __attribute__((nonnull));
static void		pul		(mc6809dis__t *const,const char *const,const bool)                  __attribute__((nonnull));

static void		cc		(char *dest,size_t size,mc6809byte__t) __attribute__((nonnull));
static mc6809byte__t	cctobyte	(mc6809__t *const)                     __attribute__((nonnull));

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

  cc(flags,sizeof(flags),cctobyte(cpu));    
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
  mc6809byte__t byte;
  int           rc;
  
  assert(dis != NULL);

  rc = setjmp(dis->err);
  if (rc != 0) return rc;
  
  dis->next   = dis->pc;
  dis->inst   = (*dis->read)(dis,dis->next++);
  
  dis->operand[0]  = '\0';
  dis->toperand[0] = '\0';
  dis->data[0]     = '\0';
  
  snprintf(dis->addr,  sizeof(dis->addr),  "%04X",dis->pc);
  snprintf(dis->opcode,sizeof(dis->opcode),"%02X",dis->inst);
  
  switch(dis->inst)
  {
    case 0x00:
         direct(dis,cpu,"NEG",false);
         break;
    
    case 0x01:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x02:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x03:
         direct(dis,cpu,"COM",false);
         break;
         
    case 0x04:
         direct(dis,cpu,"LSR",false);
         break;
         
    case 0x05:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x06:
         direct(dis,cpu,"ROR",false);
         break;
         
    case 0x07:
         direct(dis,cpu,"ASR",false);
         break;
         
    case 0x08:
         direct(dis,cpu,"LSL",false);
         break;
    
    case 0x09:
         direct(dis,cpu,"ROL",false);
         break;
         
    case 0x0A:
         direct(dis,cpu,"DEC",false);
         break;
         
    case 0x0B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x0C:
         direct(dis,cpu,"INC",false);
         break;
         
    case 0x0D:
         direct(dis,cpu,"TST",false);
         break;
         
    case 0x0E:
         direct(dis,cpu,"JMP",false);
         break;
         
    case 0x0F:
         direct(dis,cpu,"CLR",false);
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
         lrelative(dis,"LBRA","");
         break;
         
    case 0x17:
         lrelative(dis,"LBSR","");
         break;
    
    case 0x18:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x19:
         snprintf(dis->topcode,sizeof(dis->topcode),"DAA");
         break;
         
    case 0x1A:
         byte = (*dis->read)(dis,dis->next++);
         snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
         snprintf(dis->topcode,sizeof(dis->topcode),"ORCC");
         snprintf(dis->toperand,sizeof(dis->toperand),"#%02X",byte);
         cc(dis->data,sizeof(dis->data),byte);
         break;
         
    case 0x1B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x1C:
         byte = (*dis->read)(dis,dis->next++);
         snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
         snprintf(dis->topcode,sizeof(dis->topcode),"ANDCC");
         snprintf(dis->toperand,sizeof(dis->toperand),"#%02X",byte);
         cc(dis->data,sizeof(dis->data),byte);
         break;
    
    case 0x1D:
         snprintf(dis->topcode,sizeof(dis->topcode),"SEX");
         break;
    
    case 0x1E:
         byte = (*dis->read)(dis,dis->next++);
         snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
         snprintf(dis->topcode,sizeof(dis->topcode),"EXG");
         
         if ((byte & 0x88) == 0x00)
         {
           const char *s;
           const char *d;
           
           switch(byte & 0xF0)
           {
             case 0x00: s = "D"; break;
             case 0x10: s = "X"; break;
             case 0x20: s = "Y"; break;
             case 0x30: s = "U"; break;
             case 0x40: s = "S"; break;
             case 0x50: s = "PC"; break;
             default:
                  (*dis->fault)(dis,MC6809_FAULT_EXG);
                  return 0;
           }
           
           switch(byte & 0x0F)
           {
             case 0x00: d = "D"; break;
             case 0x01: d = "X"; break;
             case 0x02: d = "Y"; break;
             case 0x03: d = "U"; break;
             case 0x04: d = "S"; break;
             case 0x05: d = "PC"; break;
             default:
                  (*dis->fault)(dis,MC6809_FAULT_EXG);
                  return 0;
           }
           snprintf(dis->toperand,sizeof(dis->toperand),"%s,%s",s,d);
         }
         else if ((byte & 0x88) == 0x88)
         {
           const char *s;
           const char *d;
           
           switch(byte & 0xF0)
           {
             case 0x80:
             case 0x90:
             case 0xA0:
             case 0xB0:
             default:
                  (*dis->fault)(dis,MC6809_FAULT_EXG);
                  return 0;
           }
           
           switch(byte & 0x0F)
           {
             case 0x08:
             case 0x09:
             case 0x0A:
             case 0x0B:
             default:
                  (*dis->fault)(dis,MC6809_FAULT_EXG);
                  return 0;
           }
           
           snprintf(dis->toperand,sizeof(dis->toperand),"%s,%s",s,d);
         }
         else
           (*dis->fault)(dis,MC6809_FAULT_EXG);
         break;
         
    case 0x1F:
         byte = (*dis->read)(dis,dis->next++);
         snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
         snprintf(dis->topcode,sizeof(dis->topcode),"TFR");
         
         if ((byte & 0x88) == 0x00)
         {
           const char *s;
           const char *d;
           
           switch(byte & 0xF0)
           {
             case 0x00: s = "D"; break;
             case 0x10: s = "X"; break;
             case 0x20: s = "Y"; break;
             case 0x30: s = "U"; break;
             case 0x40: s = "S"; break;
             case 0x50: s = "PC"; break;
             default:
                  (*dis->fault)(dis,MC6809_FAULT_TFR);
                  return 0;
           }
           
           switch(byte & 0x0F)
           {
             case 0x00: d = "D"; break;
             case 0x01: d = "X"; break;
             case 0x02: d = "Y"; break;
             case 0x03: d = "U"; break;
             case 0x04: d = "S"; break;
             case 0x05: d = "PC"; break;
             default:
                  (*dis->fault)(dis,MC6809_FAULT_TFR);
                  return 0;
           }
           snprintf(dis->toperand,sizeof(dis->toperand),"%s,%s",s,d);
         }
         else if ((byte & 0x88) == 0x88)
         {
           const char *s;
           const char *d;
           
           switch(byte & 0xF0)
           {
             case 0x80: s = "A";  break;
             case 0x90: s = "B";  break;
             case 0xA0: s = "CC"; break;
             case 0xB0: s = "DP"; break;
             default:
                  (*dis->fault)(dis,MC6809_FAULT_TFR);
                  return 0;
           }
           
           switch(byte & 0x0F)
           {
             case 0x08: d = "A";  break;
             case 0x09: d = "B";  break;
             case 0x0A: d = "CC"; break;
             case 0x0B: d = "DP"; break;
             default:
                  (*dis->fault)(dis,MC6809_FAULT_TFR);
                  return 0;
           }
           
           snprintf(dis->toperand,sizeof(dis->toperand),"%s,%s",s,d);
         }
         else
           (*dis->fault)(dis,MC6809_FAULT_TFR);
         break;
             
    case 0x20:
         relative(dis,"BRA","");
         break;
    
    case 0x21:
         relative(dis,"BRN","");
         break;
    
    case 0x22:
         relative(dis,"BHI","unsigned");
         break;
    
    case 0x23:
         relative(dis,"BLS","unsigned");
         break;
         
    case 0x24:
         relative(dis,"BHS","unsigned");
         break;
         
    case 0x25:
         relative(dis,"BLO","unsigned");
         break;
           
    case 0x26:
         relative(dis,"BNE","");
         break;
         
    case 0x27:
         relative(dis,"BEQ","");
         break;
         
    case 0x28:
         relative(dis,"BVC","");
         break;
         
    case 0x29:
         relative(dis,"BVS","");
         break;
         
    case 0x2A:
         relative(dis,"BPL","");
         break;
         
    case 0x2B:
         relative(dis,"BMI","");
         break;
         
    case 0x2C:
         relative(dis,"BGE","signed");
         
    case 0x2D:
         relative(dis,"BLT","signed");
         break;
         
    case 0x2E:
         relative(dis,"BGT","signed");
         break;
         
    case 0x2F:
         relative(dis,"BLE","signed");
         break;
    
    case 0x30:
         indexed(dis,cpu,"LEAX",false);
         break;
    
    case 0x31:
         indexed(dis,cpu,"LEAY",false);
         break;
         
    case 0x32:
         indexed(dis,cpu,"LEAS",false);
         break;
    
    case 0x33:
         indexed(dis,cpu,"LEAU",false);
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
         byte = (*dis->read)(dis,dis->next++);
         snprintf(dis->operand,sizeof(dis->operand),"%02X",byte);
         snprintf(dis->topcode,sizeof(dis->topcode),"CWAI");
         snprintf(dis->toperand,sizeof(dis->toperand),"#%02X",byte);
         cc(dis->data,sizeof(dis->data),byte);
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
         indexed(dis,cpu,"NEG",false);
         break;
    
    case 0x61:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x62:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x63:
         indexed(dis,cpu,"COM",false);
         break;
         
    case 0x64:
         indexed(dis,cpu,"LSR",false);
         break;
         
    case 0x65:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x66:
         indexed(dis,cpu,"ROR",false);
         break;
         
    case 0x67:
         indexed(dis,cpu,"ASR",false);
         break;
         
    case 0x68:
         indexed(dis,cpu,"LSL",false);
         break;
    
    case 0x69:
         indexed(dis,cpu,"ROL",false);
         break;
         
    case 0x6A:
         indexed(dis,cpu,"DEC",false);
         break;
         
    case 0x6B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x6C:
         indexed(dis,cpu,"INC",false);
         break;
         
    case 0x6D:
         indexed(dis,cpu,"TST",false);
         break;
         
    case 0x6E:
         indexed(dis,cpu,"JMP",false);
         break;
         
    case 0x6F:
         indexed(dis,cpu,"CLR",false);
         break;

    case 0x70:
         extended(dis,cpu,"NEG",false);
         break;
    
    case 0x71:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x72:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x73:
         extended(dis,cpu,"COM",false);
         break;
         
    case 0x74:
         extended(dis,cpu,"LSR",false);
         break;
         
    case 0x75:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x76:
         extended(dis,cpu,"ROR",false);
         break;
         
    case 0x77:
         extended(dis,cpu,"ASR",false);
         break;
         
    case 0x78:
         extended(dis,cpu,"LSL",false);
         break;
    
    case 0x79:
         extended(dis,cpu,"ROL",false);
         break;
         
    case 0x7A:
         extended(dis,cpu,"DEC",false);
         break;
         
    case 0x7B:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x7C:
         extended(dis,cpu,"INC",false);
         break;
         
    case 0x7D:
         extended(dis,cpu,"TST",false);
         break;
         
    case 0x7E:
         extended(dis,cpu,"JMP",false);
         break;
         
    case 0x7F:
         extended(dis,cpu,"CLR",false);
         break;

    case 0x80:
         immediate(dis,"SUBA",false);
         break;
    
    case 0x81:
         immediate(dis,"CMPA",false);
         break;
    
    case 0x82:
         immediate(dis,"SBCA",false);
         break;
    
    case 0x83:
         immediate(dis,"SUBD",true);
         break;
    
    case 0x84:
         immediate(dis,"ANDA",false);
         break;
    
    case 0x85:
         immediate(dis,"BITA",false);
         break;
    
    case 0x86:
         immediate(dis,"LDA",false);
         break;
    
    case 0x87:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0x88:
         immediate(dis,"EORA",false);
         break;
    
    case 0x89:
         immediate(dis,"ADCA",false);
         break;
    
    case 0x8A:
         immediate(dis,"ORA",false);
         break;
    
    case 0x8B:
         immediate(dis,"ADDA",false);
         break;
    
    case 0x8C:
         immediate(dis,"CMPX",true);
         break;
    
    case 0x8D:
         relative(dis,"BSR","");
         break;
    
    case 0x8E:
         immediate(dis,"LDX",true);
         break;
    
    case 0x8F:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x90:
         direct(dis,cpu,"SUBA",false);
         break;
    
    case 0x91:
         direct(dis,cpu,"CMPA",false);
         break;
    
    case 0x92:
         direct(dis,cpu,"SBCA",false);
         break;
    
    case 0x93:
         direct(dis,cpu,"SUBD",true);
         break;
    
    case 0x94:
         direct(dis,cpu,"ANDA",false);
         break;
    
    case 0x95:
         direct(dis,cpu,"BITA",false);
         break;
    
    case 0x96:
         direct(dis,cpu,"LDA",false);
         break;
    
    case 0x97:
         direct(dis,cpu,"STA",false);
         break;
    
    case 0x98:
         direct(dis,cpu,"EORA",false);
         break;
    
    case 0x99:
         direct(dis,cpu,"ADCA",false);
         break;
    
    case 0x9A:
         direct(dis,cpu,"ORA",false);
         break;
    
    case 0x9B:
         direct(dis,cpu,"ADDA",false);
         break;
    
    case 0x9C:
         direct(dis,cpu,"CMPX",true);
         break;
    
    case 0x9D:
         direct(dis,cpu,"JMP",false);
         break;
    
    case 0x9E:
         direct(dis,cpu,"LDX",true);
         break;
    
    case 0x9F:
         direct(dis,cpu,"STX",true);
         break;

    case 0xA0:
         indexed(dis,cpu,"SUBA",false);
         break;
    
    case 0xA1:
         indexed(dis,cpu,"CMPA",false);
         break;
    
    case 0xA2:
         indexed(dis,cpu,"SBCA",false);
         break;
    
    case 0xA3:
         indexed(dis,cpu,"SUBD",true);
         break;
    
    case 0xA4:
         indexed(dis,cpu,"ANDA",false);
         break;
    
    case 0xA5:
         indexed(dis,cpu,"BITA",false);
         break;
    
    case 0xA6:
         indexed(dis,cpu,"LDA",false);
         break;
    
    case 0xA7:
         indexed(dis,cpu,"STA",false);
         break;
    
    case 0xA8:
         indexed(dis,cpu,"EORA",false);
         break;
    
    case 0xA9:
         indexed(dis,cpu,"ADCA",false);
         break;
    
    case 0xAA:
         indexed(dis,cpu,"ORA",false);
         break;
    
    case 0xAB:
         indexed(dis,cpu,"ADDA",false);
         break;
    
    case 0xAC:
         indexed(dis,cpu,"CMPX",true);
         break;
    
    case 0xAD:
         indexed(dis,cpu,"JMP",false);
         break;
    
    case 0xAE:
         indexed(dis,cpu,"LDX",true);
         break;
    
    case 0xAF:
         indexed(dis,cpu,"STX",true);
         break;

    case 0xB0:
         extended(dis,cpu,"SUBA",false);
         break;
    
    case 0xB1:
         extended(dis,cpu,"CMPA",false);
         break;
    
    case 0xB2:
         extended(dis,cpu,"SBCA",false);
         break;
    
    case 0xB3:
         extended(dis,cpu,"SUBD",true);
         break;
    
    case 0xB4:
         extended(dis,cpu,"ANDA",false);
         break;
    
    case 0xB5:
         extended(dis,cpu,"BITA",false);
         break;
    
    case 0xB6:
         extended(dis,cpu,"LDA",false);
         break;
    
    case 0xB7:
         extended(dis,cpu,"STA",false);
         break;
    
    case 0xB8:
         extended(dis,cpu,"EORA",false);
         break;
    
    case 0xB9:
         extended(dis,cpu,"ADCA",false);
         break;
    
    case 0xBA:
         extended(dis,cpu,"ORA",false);
         break;
    
    case 0xBB:
         extended(dis,cpu,"ADDA",false);
         break;
    
    case 0xBC:
         extended(dis,cpu,"CMPX",true);
         break;
    
    case 0xBD:
         extended(dis,cpu,"JMP",false);
         break;
    
    case 0xBE:
         extended(dis,cpu,"LDX",true);
         break;
    
    case 0xBF:
         extended(dis,cpu,"STX",true);
         break;

    case 0xC0:
         immediate(dis,"SUBB",false);
         break;
    
    case 0xC1:
         immediate(dis,"CMPB",false);
         break;
    
    case 0xC2:
         immediate(dis,"SBCB",false);
         break;
    
    case 0xC3:
         immediate(dis,"ADDD",true);
         break;
    
    case 0xC4:
         immediate(dis,"ANDB",false);
         break;
    
    case 0xC5:
         immediate(dis,"BITB",false);
         break;
    
    case 0xC6:
         immediate(dis,"LDB",false);
         break;
    
    case 0xC7:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0xC8:
         immediate(dis,"EORB",false);
         break;
    
    case 0xC9:
         immediate(dis,"ADCB",false);
         break;
    
    case 0xCA:
         immediate(dis,"ORB",false);
         break;
    
    case 0xCB:
         immediate(dis,"ADDB",false);
         break;
    
    case 0xCC:
         immediate(dis,"CMPX",true);
         break;
    
    case 0xCD:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
    
    case 0xCE:
         immediate(dis,"LDU",true);
         break;
    
    case 0xCF:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;

    case 0xD0:
         direct(dis,cpu,"SUBB",false);
         break;
    
    case 0xD1:
         direct(dis,cpu,"CMPB",false);
         break;
    
    case 0xD2:
         direct(dis,cpu,"SBCB",false);
         break;
    
    case 0xD3:
         direct(dis,cpu,"ADDD",true);
         break;
    
    case 0xD4:
         direct(dis,cpu,"ANDB",false);
         break;
    
    case 0xD5:
         direct(dis,cpu,"BITB",false);
         break;
    
    case 0xD6:
         direct(dis,cpu,"LDB",false);
         break;
    
    case 0xD7:
         direct(dis,cpu,"STB",false);
         break;
    
    case 0xD8:
         direct(dis,cpu,"EORB",false);
         break;
    
    case 0xD9:
         direct(dis,cpu,"ADCB",false);
         break;
    
    case 0xDA:
         direct(dis,cpu,"ORB",false);
         break;
    
    case 0xDB:
         direct(dis,cpu,"ADDB",false);
         break;
    
    case 0xDC:
         direct(dis,cpu,"CMPX",true);
         break;
    
    case 0xDD:
    	 direct(dis,cpu,"STD",true);
         break;
    
    case 0xDE:
         direct(dis,cpu,"LDU",true);
         break;
    
    case 0xDF:
         direct(dis,cpu,"STU",true);
         break;

    case 0xE0:
         indexed(dis,cpu,"SUBB",false);
         break;
    
    case 0xE1:
         indexed(dis,cpu,"CMPB",false);
         break;
    
    case 0xE2:
         indexed(dis,cpu,"SBCB",false);
         break;
    
    case 0xE3:
         indexed(dis,cpu,"ADDD",true);
         break;
    
    case 0xE4:
         indexed(dis,cpu,"ANDB",false);
         break;
    
    case 0xE5:
         indexed(dis,cpu,"BITB",false);
         break;
    
    case 0xE6:
         indexed(dis,cpu,"LDB",false);
         break;
    
    case 0xE7:
         indexed(dis,cpu,"STB",false);
         break;
    
    case 0xE8:
         indexed(dis,cpu,"EORB",false);
         break;
    
    case 0xE9:
         indexed(dis,cpu,"ADCB",false);
         break;
    
    case 0xEA:
         indexed(dis,cpu,"ORB",false);
         break;
    
    case 0xEB:
         indexed(dis,cpu,"ADDB",false);
         break;
    
    case 0xEC:
         indexed(dis,cpu,"CMPX",true);
         break;
    
    case 0xED:
         indexed(dis,cpu,"STD",true);
         break;
    
    case 0xEE:
         indexed(dis,cpu,"LDU",true);
         break;
    
    case 0xEF:
         indexed(dis,cpu,"STU",true);
         break;

    case 0xF0:
         extended(dis,cpu,"SUBB",false);
         break;
    
    case 0xF1:
         extended(dis,cpu,"CMPB",false);
         break;
    
    case 0xF2:
         extended(dis,cpu,"SBCB",false);
         break;
    
    case 0xF3:
         extended(dis,cpu,"ADDD",true);
         break;
    
    case 0xF4:
         extended(dis,cpu,"ANDB",false);
         break;
    
    case 0xF5:
         extended(dis,cpu,"BITB",false);
         break;
    
    case 0xF6:
         extended(dis,cpu,"LDB",false);
         break;
    
    case 0xF7:
         extended(dis,cpu,"STB",false);
         break;
    
    case 0xF8:
         extended(dis,cpu,"EORB",false);
         break;
    
    case 0xF9:
         extended(dis,cpu,"ADCB",false);
         break;
    
    case 0xFA:
         extended(dis,cpu,"ORB",false);
         break;
    
    case 0xFB:
         extended(dis,cpu,"ADDB",false);
         break;
    
    case 0xFC:
         extended(dis,cpu,"CMPX",true);
         break;
    
    case 0xFD:
         extended(dis,cpu,"STD",true);
         break;
    
    case 0xFE:
         extended(dis,cpu,"LDU",true);
         break;
    
    case 0xFF:
         extended(dis,cpu,"STU",true);
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
         lrelative(dis,"LBRN","");
         break;
    
    case 0x22:
         lrelative(dis,"LBHI","unsigned");
         break;
    
    case 0x23:
         lrelative(dis,"LBLS","unsigned");
         break;
         
    case 0x24:
         lrelative(dis,"LBHS","unsigned");
         break;
         
    case 0x25:
         lrelative(dis,"LBLO","unsigned");
         break;
           
    case 0x26:
         lrelative(dis,"LBNE","");
         break;
         
    case 0x27:
         lrelative(dis,"LBEQ","");
         break;
         
    case 0x28:
         lrelative(dis,"LBVC","");
         break;
         
    case 0x29:
         lrelative(dis,"LBVS","");
         break;
         
    case 0x2A:
         lrelative(dis,"LBPL","");
         break;
         
    case 0x2B:
         lrelative(dis,"LBMI","");
         break;
         
    case 0x2C:
         lrelative(dis,"LBGE","signed");
         
    case 0x2D:
         lrelative(dis,"LBLT","signed");
         break;
         
    case 0x2E:
         lrelative(dis,"LBGT","signed");
         break;
         
    case 0x2F:
         lrelative(dis,"LBLE","signed");
         break;
 
    case 0x3F:
         snprintf(dis->topcode,sizeof(dis->topcode),"SWI2");
         break;
         
    case 0x83:
         immediate(dis,"CMPD",true);
         break;
         
    case 0x8C:
         immediate(dis,"CMPY",true);
         break;
         
    case 0x8E:
         immediate(dis,"LDY",true);
         break;
    
    case 0x93:
         direct(dis,cpu,"CMPD",true);
         break;
         
    case 0x9C:
         direct(dis,cpu,"CMPY",true);
         break;
         
    case 0x9E:
         direct(dis,cpu,"LDY",true);
         break;
         
    case 0x9F:
         direct(dis,cpu,"STY",true);
         break;
    
    case 0xA3:
         indexed(dis,cpu,"CMPD",true);
         break;
    
    case 0xAC:
         indexed(dis,cpu,"CMPY",true);
         break;
    
    case 0xAE:
         indexed(dis,cpu,"LDY",true);
         break;
    
    case 0xAF:
         indexed(dis,cpu,"STY",true);
         break;
    
    case 0xB3:
         extended(dis,cpu,"CMPD",true);
         break;
    
    case 0xBC:
         extended(dis,cpu,"CMPY",true);
         break;
    
    case 0xBE:
         extended(dis,cpu,"LDY",true);
         break;
         
    case 0xBF:
         extended(dis,cpu,"STY",true);
         break;
         
    case 0xCE:
         immediate(dis,"LDS",true);
         break;
    
    case 0xDE:
         direct(dis,cpu,"LDS",true);
         break;
         
    case 0xDF:
         direct(dis,cpu,"STS",true);
         break;
    
    case 0xEE:
         indexed(dis,cpu,"LDS",true);
         break;
    
    case 0xEF:
         indexed(dis,cpu,"STS",true);
         break;
    
    case 0xFE:
         extended(dis,cpu,"LDS",true);
         break;
    
    case 0xFF:
         extended(dis,cpu,"STS",true);
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
  
  assert(cpu != NULL);
  
  byte = (*dis->read)(dis,dis->next++);
  snprintf(&dis->opcode[2],sizeof(dis->opcode)-2,"%02X",byte);
  
  switch(byte)
  {
    case 0x3F:
         snprintf(dis->topcode,sizeof(dis->topcode),"SWI3");
         break;
    
    case 0x83:
         immediate(dis,"CMPU",true);
         break;
    
    case 0x8C:
         immediate(dis,"CMPS",true);
         break;
    
    case 0x93:
         direct(dis,cpu,"CMPU",true);
         break;
    
    case 0x9C:
         direct(dis,cpu,"CMPS",true);
         break;
    
    case 0xA3:
         indexed(dis,cpu,"CMPU",true);
         break;
    
    case 0xAC:
         indexed(dis,cpu,"CMPS",true);
         break;
    
    case 0xB3:
         extended(dis,cpu,"CMPU",true);
         break;
    
    case 0xBC:
         extended(dis,cpu,"CMPS",true);
         break;
         
    default:
         (*dis->fault)(dis,MC6809_FAULT_INSTRUCTION);
         break;
  }
  
  return 0;
}

/***********************************************************************/

static void indexed(
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
        d16.b[M] = (*dis->read)(dis,addr.w);
        d16.b[L] = (*dis->read)(dis,addr.w + 1);
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
           addr.b[L]  = cpu->B;
           addr.b[M]  = (cpu->B < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
         }
         break;
         
    case 0x06:
         snprintf(dis->toperand,sizeof(dis->toperand),"A,%c",regs[reg]);
         if (cpu != NULL)
         {
           addr.b[L]  = cpu->A;
           addr.b[M]  = (cpu->A < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
         }
         break;
         
    case 0x07:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x08:
         addr.b[L] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[L] < 0x80) ? addr.b[L] : addr.b[L] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[L]);
         snprintf(dis->toperand,sizeof(dis->toperand),"%d,%c",ioff,regs[reg]);
         if (cpu != NULL)
         {
           addr.b[M]  = (addr.b[L] < 0x80) ? 0x00: 0xFF;
           addr.w    += cpu->index[reg].w;
         }
         break;
         
    case 0x09:
         addr.b[M] = (*dis->read)(dis,dis->next++);
         addr.b[L] = (*dis->read)(dis,dis->next++);
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
         addr.b[L] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[L] < 0x80) ? addr.b[L] : addr.b[L] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[L]);
         snprintf(dis->toperand,sizeof(dis->toperand),"%d,PC",ioff);
         if (cpu != NULL)
         {
           addr.b[M]  = (addr.b[L] < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->pc.w;
         }
         break;
         
    case 0x0D:
         addr.b[M] = (*dis->read)(dis,dis->next++);
         addr.b[L] = (*dis->read)(dis,dis->next++);
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
           addr.b[L]  = cpu->B;
           addr.b[M]  = (cpu->B < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
           ind        = true;
         }
         break;
         
    case 0x16:
         snprintf(dis->toperand,sizeof(dis->toperand),"[A,%c]",regs[reg]);
         if (cpu != NULL)
         {
           addr.b[L]  = cpu->A;
           addr.b[M]  = (cpu->A < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->index[reg].w;
           ind        = true;
         }
         break;
         
    case 0x17:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x18:
         addr.b[L] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[L] < 0x80) ? addr.b[L] : addr.b[L] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[L]);
         snprintf(dis->toperand,sizeof(dis->toperand),"[%d,%c]",ioff,regs[reg]);
         if (cpu != NULL)
         {
           addr.b[M]  = (addr.b[L] < 0x80) ? 0x00: 0xFF;
           addr.w    += cpu->index[reg].w;
           ind        = true;
         }
         break;

    case 0x19:
         addr.b[M] = (*dis->read)(dis,dis->next++);
         addr.b[L] = (*dis->read)(dis,dis->next++);
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
         addr.b[L] = (*dis->read)(dis,dis->next++);
         ioff      = (addr.b[L] < 0x80) ? addr.b[L] : addr.b[L] - 256;
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%02X",addr.b[L]);
         snprintf(dis->toperand,sizeof(dis->toperand),"[%d,PC]",ioff);
         if (cpu != NULL)
         {
           addr.b[M]  = (addr.b[L] < 0x80) ? 0x00 : 0xFF;
           addr.w    += cpu->pc.w;
           ind        = true;
         }
         break;
         
    case 0x1D:
         addr.b[M] = (*dis->read)(dis,dis->next++);
         addr.b[L] = (*dis->read)(dis,dis->next++);
         ioff      = ((int16_t)addr.w);
         snprintf(&dis->operand[2],sizeof(dis->operand)-2,"%04X",addr.w);
         snprintf(dis->toperand,sizeof(dis->toperand),"[%d,PC]",ioff);
         if (cpu != NULL)
         {
           addr.w += cpu->pc.w;
           ind     = true;
         }
         break;
         
    case 0x1E:
         (*dis->fault)(dis,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x1F:
         if (reg == 0)
         {
           addr.b[M] = (*dis->read)(dis,dis->next++);
           addr.b[L] = (*dis->read)(dis,dis->next++);
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
      iaddr.b[M] = (*dis->read)(dis,addr.w);
      iaddr.b[L] = (*dis->read)(dis,addr.w+1);
      
      if (b16)
      {
        d16.b[M] = (*dis->read)(dis,iaddr.w);
        d16.b[L] = (*dis->read)(dis,iaddr.w + 1);
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
        d16.b[M] = (*dis->read)(dis,addr.w);
        d16.b[L] = (*dis->read)(dis,addr.w + 1);
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

static void immediate(
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
    d16.b[M] = (*dis->read)(dis,dis->next++);
    d16.b[L] = (*dis->read)(dis,dis->next++);
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

static void direct(
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
  
  addr.b[M] = 0;
  addr.b[L] = (*dis->read)(dis,dis->next++);
  
  snprintf(dis->operand,sizeof(dis->operand),"%02X",addr.b[L]);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  snprintf(dis->toperand,sizeof(dis->toperand),"%02X",addr.b[L]);
  
  if (cpu != NULL)
  {
    if (b16)
    {
      word.b[M] = (*dis->read)(dis,addr.w++);
      word.b[L] = (*dis->read)(dis,addr.w);
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

static void extended(
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
  
  addr.b[M] = (*dis->read)(dis,dis->next++);
  addr.b[L] = (*dis->read)(dis,dis->next++);
  
  snprintf(dis->operand,sizeof(dis->operand),"%04X",addr.w);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  snprintf(dis->toperand,sizeof(dis->toperand),"%04X",addr.w);
  
  if (cpu != NULL)
  {
    if (b16)
    {
      word.b[M] = (*dis->read)(dis,addr.w++);
      word.b[L] = (*dis->read)(dis,addr.w);
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

static void relative(
	mc6809dis__t *const dis,
	const char   *const op,
	const char   *const data
)
{
  mc6809word__t addr;
  
  addr.b[L] = (*dis->read)(dis,dis->next++);
  addr.b[M] = (addr.b[L] < 0x80) ? 0x00 : 0xFF;
  snprintf(dis->operand,sizeof(dis->operand),"%02X",addr.b[L]);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  
  addr.w += dis->next;
  snprintf(dis->toperand,sizeof(dis->toperand),"%04X",addr.w);
  if (addr.w < dis->next)
    snprintf(dis->data,sizeof(dis->data),"backwards %s",data);
  else
    snprintf(dis->data,sizeof(dis->data),"forwards %s",data);
}

/*************************************************************************/

static void lrelative(
	mc6809dis__t *const dis,
	const char   *const op,
	const char   *const data
)
{
  mc6809word__t addr;
  
  addr.b[M] = (*dis->read)(dis,dis->next++);
  addr.b[L] = (*dis->read)(dis,dis->next++);
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

static void psh(mc6809dis__t *const dis,const char *const op,const bool s)
{
  mc6809byte__t  post;
  size_t         len;
  size_t         bytes;
  char          *p;
  char          *sep;
  
  len  = sizeof(dis->toperand);
  p    = dis->toperand;
  sep  = "";
  post = (*dis->read)(dis,dis->next++);
  snprintf(dis->operand,sizeof(dis->operand),"%02X",post);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);

  if ((post & 0x80) != 0)
  {
    bytes = snprintf(p,len,"PC");
    len  -= bytes;
    p    += bytes;
    sep  = ",";
  }
  
  if ((post & 0x40) != 0)
  {
    bytes = snprintf(p,len,"%s%s",sep,s ? "U" : "S");
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x20) != 0)
  {
    bytes = snprintf(p,len,"%sY",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x10) != 0)
  {
    bytes = snprintf(p,len,"%sX",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x08) != 0)
  {
    bytes = snprintf(p,len,"%sDP",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x04) != 0)
  {
    bytes = snprintf(p,len,"%sB",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x02) != 0)
  {
    bytes = snprintf(p,len,"%sA",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x01) != 0)
    snprintf(p,len,"%sCC",sep);
}

/*************************************************************************/

static void pul(mc6809dis__t *const dis,const char *const op,const bool s)
{
  mc6809byte__t post;
  size_t        len;
  size_t        bytes;
  char         *p;
  char         *sep;
  
  len  = sizeof(dis->toperand);
  p    = dis->toperand;
  sep  = "";
  post = (*dis->read)(dis,dis->next++);
  snprintf(dis->operand,sizeof(dis->operand),"%02X",post);
  snprintf(dis->topcode,sizeof(dis->topcode),"%s",op);
  
  if ((post & 0x01) != 0)
  {
    bytes = snprintf(p,len,"CC");
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x02) != 0)
  {
    bytes = snprintf(p,len,"%sA",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x04) != 0)
  {
    bytes = snprintf(p,len,"%sB",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x08) != 0)
  {
    bytes = snprintf(p,len,"%sDP",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x10) != 0)
  {
    bytes = snprintf(p,len,"%sX",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x20) != 0)
  {
    bytes = snprintf(p,len,"%sY",sep);
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x40) != 0)
  {
    bytes = snprintf(p,len,"%s%s",sep,s ? "U" : "S");
    len  -= bytes;
    p    += bytes;
    sep   = ",";
  }
  
  if ((post & 0x80) != 0)
    snprintf(p,len,"%sPC",sep);
}

/*************************************************************************/

static void cc(char *dest,size_t size __attribute__((unused)),mc6809byte__t c)
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
