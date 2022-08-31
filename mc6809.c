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
#include <assert.h>

#include "mc6809.h"

/**************************************************************************/

static int              page2           (mc6809__t *) __attribute__((nonnull));
static int              page3           (mc6809__t *) __attribute__((nonnull));

static mc6809byte__t    op_neg          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_com          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_lsr          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_ror          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_asr          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_lsl          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_rol          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_dec          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_inc          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static void             op_tst          (mc6809__t *,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_clr          (mc6809__t *)                     __attribute__((nonnull));

static mc6809byte__t    op_sub          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static void             op_cmp          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_sbc          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_and          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static void             op_bit          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static void             op_ldst         (mc6809__t *,mc6809byte__t const)                     __attribute__((nonnull));
static mc6809byte__t    op_eor          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_adc          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_or           (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));
static mc6809byte__t    op_add          (mc6809__t *,mc6809byte__t const,mc6809byte__t const) __attribute__((nonnull));

static mc6809addr__t    op_sub16        (mc6809__t *,mc6809addr__t const,mc6809addr__t const) __attribute__((nonnull));
static void             op_cmp16        (mc6809__t *,mc6809addr__t const,mc6809addr__t const) __attribute__((nonnull));
static void             op_ldst16       (mc6809__t *,mc6809addr__t const)                     __attribute__((nonnull));
static mc6809addr__t    op_add16        (mc6809__t *,mc6809addr__t const,mc6809addr__t const) __attribute__((nonnull));

/***************************************************************************/

static inline bool bhs(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return !cpu->cc.c;
}

static inline bool blo(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.c;
}

static inline bool bhi(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return !cpu->cc.c && !cpu->cc.z;
}

static inline bool bls(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.c || cpu->cc.z;
}

static inline bool bne(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return !cpu->cc.z;
}

static inline bool beq(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.z;
}

static inline bool bge(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.n == cpu->cc.v;
}

static inline bool blt(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.n != cpu->cc.v;
}

static inline bool bgt(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return (cpu->cc.n == cpu->cc.v) && !cpu->cc.z;
}

static inline bool ble(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.z || (cpu->cc.n != cpu->cc.v);
}

static inline bool bpl(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return !cpu->cc.n;
}

static inline bool bmi(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.n;
}

static inline bool bvc(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return !cpu->cc.v;
}

static inline bool bvs(mc6809__t *cpu)
{
  assert(cpu != NULL);
  return cpu->cc.v;
}

/**************************************************************************/

void mc6809_reset(mc6809__t *cpu)
{
  assert(cpu != NULL);
  
  cpu->cycles      = 0;
  cpu->nmi_armed   = false;
  cpu->nmi         = false;
  cpu->firq        = false;
  cpu->irq         = false;
  cpu->cwai        = false;
  cpu->sync        = false;
  cpu->page2       = false;
  cpu->page3       = false;
  cpu->pc.b[MSB]   = (*cpu->read)(cpu,MC6809_VECTOR_RESET,false);
  cpu->pc.b[LSB]   = (*cpu->read)(cpu,MC6809_VECTOR_RESET + 1,false);
  cpu->dp          = 0;
  cpu->cc.e        = false;
  cpu->cc.f        = true;
  cpu->cc.h        = false;
  cpu->cc.i        = true;
  cpu->cc.n        = false;
  cpu->cc.z        = false;
  cpu->cc.v        = false;
  cpu->cc.c        = false;
}

/***************************************************************************/

int mc6809_run(mc6809__t *cpu)
{
  int rc;
  
  assert(cpu != NULL);
  
  do
    rc = mc6809_step(cpu);
  while (rc == 0);
  
  return rc;
}

/**************************************************************************/

int mc6809_step(mc6809__t *cpu)
{
  mc6809addr__t ea;
  mc6809word__t d16;
  mc6809byte__t inst;
  mc6809byte__t data;
  volatile int  rc;
  
  assert(cpu != NULL);
  
  rc = setjmp(cpu->err);
  if (rc != 0) return rc;
  
  if (cpu->nmi && cpu->nmi_armed)
  {
    cpu->nmi = false;
    if (!cpu->cwai)
    {
      cpu->cc.e    = true;
      cpu->cycles += 19;
      (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[LSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[MSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[LSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[MSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[LSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[MSB]);
      (*cpu->write)(cpu,--cpu->S.w,cpu->dp);
      (*cpu->write)(cpu,--cpu->S.w,cpu->B);
      (*cpu->write)(cpu,--cpu->S.w,cpu->A);
      (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
    }
    cpu->cc.f      = true;
    cpu->cc.i      = true;
    cpu->pc.b[MSB] = (*cpu->read)(cpu,MC6809_VECTOR_NMI,false);
    cpu->pc.b[LSB] = (*cpu->read)(cpu,MC6809_VECTOR_NMI + 1,false);
    cpu->cwai      = false;
    cpu->sync      = false;
    return 0;
  }
  else if (cpu->firq)
  {
    unsigned long cycles;
    
    if (cpu->sync)
    {
      cpu->cycles += 3;
      cpu->sync    = false;
      cpu->irq     = false;
      cycles       = 7;
    }
    else
      cycles = 10;
      
    if (!cpu->cc.f)
    {
      cpu->firq = false;
      if (!cpu->cwai)
      {
        cpu->cc.e    = false;
        cpu->cycles += cycles;
        (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
        (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
      }
      cpu->cc.f      = true;
      cpu->cc.i      = true;
      cpu->pc.b[MSB] = (*cpu->read)(cpu,MC6809_VECTOR_FIRQ,false);
      cpu->pc.b[LSB] = (*cpu->read)(cpu,MC6809_VECTOR_FIRQ + 1,false);
      cpu->cwai      = false;
    }
    return 0;
  }
  else if (cpu->irq)
  {
    unsigned long cycles;
    
    if (cpu->sync)
    {
      cpu->cycles += 3;
      cpu->sync    = false;
      cpu->irq     = false;
      cycles       = 16;
    }
    else
      cycles = 19;
      
    if (!cpu->cc.i)
    {
      cpu->irq  = false;
      if (!cpu->cwai)
      {
        cpu->cc.e    = true;
        cpu->cycles += cycles;
        (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[LSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[MSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[LSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[MSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[LSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[MSB]);
        (*cpu->write)(cpu,--cpu->S.w,cpu->dp);
        (*cpu->write)(cpu,--cpu->S.w,cpu->B);
        (*cpu->write)(cpu,--cpu->S.w,cpu->A);
        (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
      }
      cpu->cc.i      = true;
      cpu->pc.b[MSB] = (*cpu->read)(cpu,MC6809_VECTOR_IRQ,false);
      cpu->pc.b[LSB] = (*cpu->read)(cpu,MC6809_VECTOR_IRQ + 1,false);
      cpu->cwai      = false;
    }
    return 0;
  }
  
  cpu->cycles++;
  if (cpu->cwai || cpu->sync)
    return 0;
    
  cpu->instpc = cpu->pc.w;
  inst        = (*cpu->read)(cpu,cpu->pc.w++,true);
  
  /*------------------------------------------------------------------
  ; While the cycle counts may appear to be one less than stated in the
  ; manual, the additional cycle has already been calculated above.
  ;--------------------------------------------------------------------*/
  
  switch(inst)
  {
    case 0x00:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_neg(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x01:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x02:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x03:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_com(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x04:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_lsr(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x05:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x06:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_ror(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x07:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_asr(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x08:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_lsl(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x09:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_rol(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x0A:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_dec(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x0B:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x0C:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,op_inc(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x0D:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         op_tst(cpu,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x0E:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->pc.w = ea;
         break;
         
    case 0x0F:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->read) (cpu,ea,false);
         (*cpu->write)(cpu,ea,op_clr(cpu));
         break;
         
    case 0x10:
         return page2(cpu);
         
    case 0x11:
         return page3(cpu);
         
    case 0x12:
         cpu->cycles++;
         break;
         
    case 0x13:
         cpu->cycles++;
         cpu->sync = true;
         break;
         
    case 0x14:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x15:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x16:
         cpu->cycles += 4;
         cpu->pc.w = mc6809_lrelative(cpu);
         break;
         
    case 0x17:
         cpu->cycles += 8;
         ea = mc6809_lrelative(cpu);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         cpu->pc.w = ea;
         break;
         
    case 0x18:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x19:  /* DAA */
         cpu->cycles++;
         {
           mc6809byte__t msn = cpu->A >> 4;
           mc6809byte__t lsn = cpu->A & 0x0F;
           mc6809byte__t cf  = 0;
           bool          h   = cpu->cc.h;
           bool          c   = cpu->cc.c;
           
           if (cpu->cc.c || (msn > 9) || ((msn > 8) && (lsn > 9)))
             cf |= 0x60;
           if (cpu->cc.h || (lsn > 9))
             cf |= 0x06;
             
           cpu->A    = op_add(cpu,cpu->A,cf);
           cpu->cc.h = h;
           cpu->cc.c = cpu->cc.c | c;
           cpu->cc.v = false;
         }
         break;
         
    case 0x1A:
         cpu->cycles += 2;
         data    = (*cpu->read)(cpu,cpu->pc.w++,true);
         data   |= mc6809_cctobyte(cpu);
         mc6809_bytetocc(cpu,data);
         break;
         
    case 0x1B:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x1C:
         cpu->cycles += 2;
         data    = (*cpu->read)(cpu,cpu->pc.w++,true);
         data   &= mc6809_cctobyte(cpu);
         mc6809_bytetocc(cpu,data);
         break;
         
    case 0x1D:
         cpu->cycles++;
         cpu->A = (cpu->B > 0x7F) ? 0xFF: 0x00;
         cpu->cc.n = (cpu->d.w >  0x7FFF);
         cpu->cc.z = (cpu->d.w == 0x0000);
         cpu->cc.v = false;
         break;
         
    case 0x1E:
         cpu->cycles += 7;
         d16.b[LSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
         
         if ((d16.b[LSB] & 0x88) == 0x00)
         {
           mc6809addr__t *src;
           mc6809addr__t *dest;
           mc6809addr__t  tmp;
           
           switch(d16.b[LSB] & 0xF0)
           {
             case 0x00: src = &cpu->d.w;  break;
             case 0x10: src = &cpu->X.w;  break;
             case 0x20: src = &cpu->Y.w;  break;
             case 0x30: src = &cpu->U.w;  break;
             case 0x40: src = &cpu->S.w;  break;
             case 0x50: src = &cpu->pc.w; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_EXG);
                  return 0;
           }
           
           switch(d16.b[LSB] & 0x0F)
           {
             case 0x00: dest = &cpu->d.w;  break;
             case 0x01: dest = &cpu->X.w;  break;
             case 0x02: dest = &cpu->Y.w;  break;
             case 0x03: dest = &cpu->U.w;  break;
             case 0x04: dest = &cpu->S.w;  cpu->nmi_armed = true; break;
             case 0x05: dest = &cpu->pc.w; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_EXG);
                  return 0;
           }
           
           tmp   = *src;
           *src  = *dest;
           *dest = tmp;
         }
         else if ((d16.b[LSB] & 0x88) == 0x88)
         {
           mc6809byte__t *src;
           mc6809byte__t *dest;
           mc6809byte__t  tmp;
           mc6809byte__t  ccs;
           mc6809byte__t  ccd;
           
           switch(d16.b[LSB] & 0xF0)
           {
             case 0x80: src = &cpu->A; break;
             case 0x90: src = &cpu->B; break;
             case 0xA0: src = &ccs; ccs = mc6809_cctobyte(cpu); break;
             case 0xB0: src = &cpu->dp; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_EXG);
                  return 0;
           }
           
           switch(d16.b[LSB] & 0x0F)
           {
             case 0x08: dest = &cpu->A; break;
             case 0x09: dest = &cpu->B; break;
             case 0x0A: dest = &ccd; ccd = mc6809_cctobyte(cpu); break;
             case 0x0B: dest = &cpu->dp; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_EXG);
                  return 0;
           }
           
           tmp   = *src;
           *src  = *dest;
           *dest = tmp;
           
           if (src  == &ccs) mc6809_bytetocc(cpu,ccs);
           if (dest == &ccd) mc6809_bytetocc(cpu,ccd);
         }
         else
           (*cpu->fault)(cpu,MC6809_FAULT_EXG);
         break;
         
    case 0x1F:
         cpu->cycles += 6;
         d16.b[LSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
         
         if ((d16.b[LSB] & 0x88) == 0x00)
         {
           switch(d16.b[LSB] & 0xF0)
           {
             case 0x00: ea = cpu->d.w;  break;
             case 0x10: ea = cpu->X.w;  break;
             case 0x20: ea = cpu->Y.w;  break;
             case 0x30: ea = cpu->U.w;  break;
             case 0x40: ea = cpu->S.w;  cpu->nmi_armed = true; break;
             case 0x50: ea = cpu->pc.w; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_TFR);
                  return 0;
           }
           
           switch(d16.b[LSB] & 0x0F)
           {
             case 0x00: cpu->d.w = ea;  break;
             case 0x01: cpu->X.w = ea;  break;
             case 0x02: cpu->Y.w = ea;  break;
             case 0x03: cpu->U.w = ea;  break;
             case 0x04: cpu->S.w = ea;  cpu->nmi_armed = true; break;
             case 0x05: cpu->pc.w = ea; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_TFR);
                  return 0;
           }
         }
         else if ((d16.b[LSB] & 0x88) == 0x88)
         {
           switch(d16.b[LSB] & 0xF0)
           {
             case 0x80: data = cpu->A; break;
             case 0x90: data = cpu->B; break;
             case 0xA0: data = mc6809_cctobyte(cpu); break;
             case 0xB0: data = cpu->dp; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_TFR);
                  return 0;
           }
           
           switch(d16.b[LSB] & 0x0F)
           {
             case 0x08: cpu->A = data; break;
             case 0x09: cpu->B = data; break;
             case 0x0A: mc6809_bytetocc(cpu,data); break;
             case 0x0B: cpu->dp = data; break;
             default:
                  (*cpu->fault)(cpu,MC6809_FAULT_TFR);
                  return 0;
           }
         }
         else
           (*cpu->fault)(cpu,MC6809_FAULT_TFR);
         break;
         
    case 0x20:
         cpu->cycles += 2;
         cpu->pc.w = mc6809_relative(cpu);
         break;
         
    case 0x21:
         cpu->cycles += 2;
         mc6809_relative(cpu);
         break;
         
    case 0x22:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bhi(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x23:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bls(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x24:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bhs(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x25:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (blo(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x26:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bne(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x27:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (beq(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x28:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bvc(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x29:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bvs(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x2A:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bpl(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x2B:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bmi(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x2C:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bge(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x2D:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (blt(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x2E:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (bgt(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x2F:
         cpu->cycles += 2;
         ea = mc6809_relative(cpu);
         if (ble(cpu))
           cpu->pc.w = ea;
         break;
         
    case 0x30:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->X.w = ea;
         cpu->cc.z = (cpu->X.w == 0x0000);
         break;
         
    case 0x31:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->Y.w = ea;
         cpu->cc.z = (cpu->Y.w == 0x0000);
         break;
         
    case 0x32:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->S.w = ea;
         cpu->nmi_armed = true;
         break;
         
    case 0x33:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->U.w = ea;
         break;
         
    case 0x34:
         cpu->cycles += 4;
         data = (*cpu->read)(cpu,cpu->pc.w++,true);
         if (data & 0x80)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
           (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         }
         if (data & 0x40)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[LSB]);
           (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[MSB]);
         }
         if (data & 0x20)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[LSB]);
           (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[MSB]);
         }
         if (data & 0x10)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[LSB]);
           (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[MSB]);
         }
         if (data & 0x08)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->S.w,cpu->dp);
         }
         if (data & 0x04)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->S.w,cpu->B);
         }
         if (data & 0x02)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->S.w,cpu->A);
         }
         if (data & 0x01)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
         }
         break;
         
    case 0x35:
         cpu->cycles += 4;
         data = (*cpu->read)(cpu,cpu->pc.w++,true);
         if (data & 0x01)
         {
           cpu->cycles++;
           mc6809_bytetocc(cpu,(*cpu->read)(cpu,cpu->S.w++,false));
         }
         if (data & 0x02)
         {
           cpu->cycles++;
           cpu->A = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         if (data & 0x04)
         {
           cpu->cycles++;
           cpu->B = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         if (data & 0x08)
         {
           cpu->cycles++;
           cpu->dp = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         if (data & 0x10)
         {
           cpu->cycles += 2;
           cpu->X.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->X.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         if (data & 0x20)
         {
           cpu->cycles += 2;
           cpu->Y.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->Y.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         if (data & 0x40)
         {
           cpu->cycles += 2;
           cpu->U.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->U.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         if (data & 0x80)
         {
           cpu->cycles += 2;
           cpu->pc.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->pc.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         break;
         
    case 0x36:
         cpu->cycles += 4;
         data = (*cpu->read)(cpu,cpu->pc.w++,true);
         if (data & 0x80)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->U.w,cpu->pc.b[LSB]);
           (*cpu->write)(cpu,--cpu->U.w,cpu->pc.b[MSB]);
         }
         if (data & 0x40)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->U.w,cpu->S.b[LSB]);
           (*cpu->write)(cpu,--cpu->U.w,cpu->S.b[MSB]);
         }
         if (data & 0x20)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->U.w,cpu->Y.b[LSB]);
           (*cpu->write)(cpu,--cpu->U.w,cpu->Y.b[MSB]);
         }
         if (data & 0x10)
         {
           cpu->cycles += 2;
           (*cpu->write)(cpu,--cpu->U.w,cpu->X.b[LSB]);
           (*cpu->write)(cpu,--cpu->U.w,cpu->X.b[MSB]);
         }
         if (data & 0x08)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->U.w,cpu->dp);
         }
         if (data & 0x04)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->U.w,cpu->B);
         }
         if (data & 0x02)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->U.w,cpu->A);
         }
         if (data & 0x01)
         {
           cpu->cycles++;
           (*cpu->write)(cpu,--cpu->U.w,mc6809_cctobyte(cpu));
         }
         break;
         
    case 0x37:
         cpu->cycles += 4;
         data = (*cpu->read)(cpu,cpu->pc.w++,true);
         if (data & 0x01)
         {
           cpu->cycles++;
           mc6809_bytetocc(cpu,(*cpu->read)(cpu,cpu->U.w++,false));
         }
         if (data & 0x02)
         {
           cpu->cycles++;
           cpu->A = (*cpu->read)(cpu,cpu->U.w++,false);
         }
         if (data & 0x04)
         {
           cpu->cycles++;
           cpu->B = (*cpu->read)(cpu,cpu->U.w++,false);
         }
         if (data & 0x08)
         {
           cpu->cycles++;
           cpu->dp = (*cpu->read)(cpu,cpu->U.w++,false);
         }
         if (data & 0x10)
         {
           cpu->cycles += 2;
           cpu->X.b[MSB] = (*cpu->read)(cpu,cpu->U.w++,false);
           cpu->X.b[LSB] = (*cpu->read)(cpu,cpu->U.w++,false);
         }
         if (data & 0x20)
         {
           cpu->cycles += 2;
           cpu->Y.b[MSB] = (*cpu->read)(cpu,cpu->U.w++,false);
           cpu->Y.b[LSB] = (*cpu->read)(cpu,cpu->U.w++,false);
         }
         if (data & 0x40)
         {
           cpu->cycles += 2;
           cpu->S.b[MSB] = (*cpu->read)(cpu,cpu->U.w++,false);
           cpu->S.b[LSB] = (*cpu->read)(cpu,cpu->U.w++,false);
           cpu->nmi_armed = true;
         }
         if (data & 0x80)
         {
           cpu->cycles += 2;
           cpu->pc.b[MSB] = (*cpu->read)(cpu,cpu->U.w++,false);
           cpu->pc.b[LSB] = (*cpu->read)(cpu,cpu->U.w++,false);
         }
         break;
         
    case 0x38:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x39:
         cpu->cycles += 4;
         cpu->pc.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         cpu->pc.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         break;
         
    case 0x3A:
         cpu->cycles += 2;
         d16.b[LSB] = cpu->B;
         d16.b[MSB] = 0;
         cpu->X.w += d16.w;
         break;
         
    case 0x3B:
         cpu->cycles += 5;
         mc6809_bytetocc(cpu,(*cpu->read)(cpu,cpu->S.w++,false));
         if (cpu->cc.e)
         {
           cpu->cycles  += 9;
           cpu->A        = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->B        = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->dp       = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->X.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->X.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->Y.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->Y.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->U.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
           cpu->U.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         }
         cpu->pc.b[MSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         cpu->pc.b[LSB] = (*cpu->read)(cpu,cpu->S.w++,false);
         break;
         
    case 0x3C:
         cpu->cycles += 20;
         data = (*cpu->read)(cpu,cpu->pc.w++,true);
         mc6809_bytetocc(cpu,mc6809_cctobyte(cpu) & data);
         cpu->cc.e = true;
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->dp);
         (*cpu->write)(cpu,--cpu->S.w,cpu->B);
         (*cpu->write)(cpu,--cpu->S.w,cpu->A);
         (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
         cpu->cwai = true;
         break;
         
    case 0x3D:
         cpu->cycles += 10;
         cpu->d.w  = (mc6809addr__t)cpu->A * (mc6809addr__t)cpu->B;
         cpu->cc.c = (cpu->B & 0x80) == 0x80;
         cpu->cc.z = (cpu->d.w == 0x0000);
         break;
         
    case 0x3E:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x3F:
         cpu->cycles += 18;
         cpu->cc.e    = true;
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->dp);
         (*cpu->write)(cpu,--cpu->S.w,cpu->B);
         (*cpu->write)(cpu,--cpu->S.w,cpu->A);
         (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
         cpu->cc.f      = true;
         cpu->cc.i      = true;
         cpu->pc.b[MSB] = (*cpu->read)(cpu,MC6809_VECTOR_SWI,false);
         cpu->pc.b[LSB] = (*cpu->read)(cpu,MC6809_VECTOR_SWI + 1,false);
         break;
         
    case 0x40:
         cpu->cycles++;
         cpu->A = op_neg(cpu,cpu->A);
         break;
         
    case 0x41:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x42:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x43:
         cpu->cycles++;
         cpu->A = op_com(cpu,cpu->A);
         break;
         
    case 0x44:
         cpu->cycles++;
         cpu->A = op_lsr(cpu,cpu->A);
         break;
         
    case 0x45:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x46:
         cpu->cycles++;
         cpu->A = op_ror(cpu,cpu->A);
         break;
         
    case 0x47:
         cpu->cycles++;
         cpu->A = op_asr(cpu,cpu->A);
         break;
         
    case 0x48:
         cpu->cycles++;
         cpu->A = op_lsl(cpu,cpu->A);
         break;
         
    case 0x49:
         cpu->cycles++;
         cpu->A = op_rol(cpu,cpu->A);
         break;
         
    case 0x4A:
         cpu->cycles++;
         cpu->A = op_dec(cpu,cpu->A);
         break;
         
    case 0x4B:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x4C:
         cpu->cycles++;
         cpu->A = op_inc(cpu,cpu->A);
         break;
         
    case 0x4D:
         cpu->cycles++;
         op_tst(cpu,cpu->A);
         break;
         
    case 0x4E:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x4F:
         cpu->cycles++;
         cpu->A = op_clr(cpu);
         break;
         
    case 0x50:
         cpu->cycles++;
         cpu->B = op_neg(cpu,cpu->B);
         break;
         
    case 0x51:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x52:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x53:
         cpu->cycles++;
         cpu->B = op_com(cpu,cpu->B);
         break;
         
    case 0x54:
         cpu->cycles++;
         cpu->B = op_lsr(cpu,cpu->B);
         break;
         
    case 0x55:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x56:
         cpu->cycles++;
         cpu->B = op_ror(cpu,cpu->B);
         break;
         
    case 0x57:
         cpu->cycles++;
         cpu->B = op_asr(cpu,cpu->B);
         break;
         
    case 0x58:
         cpu->cycles++;
         cpu->B = op_lsl(cpu,cpu->B);
         break;
         
    case 0x59:
         cpu->cycles++;
         cpu->B = op_rol(cpu,cpu->B);
         break;
         
    case 0x5A:
         cpu->cycles++;
         cpu->B = op_dec(cpu,cpu->B);
         break;
         
    case 0x5B:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x5C:
         cpu->cycles++;
         cpu->B = op_inc(cpu,cpu->B);
         break;
         
    case 0x5D:
         cpu->cycles++;
         op_tst(cpu,cpu->B);
         break;
         
    case 0x5E:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x5F:
         cpu->cycles++;
         cpu->B = op_clr(cpu);
         break;
         
    case 0x60:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_neg(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x61:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x62:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x63:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_com(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x64:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_lsr(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x65:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x66:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_ror(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x67:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_asr(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x68:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_lsl(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x69:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_rol(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x6A:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_dec(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x6B:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x6C:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,op_inc(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x6D:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         op_tst(cpu,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x6E:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->pc.w = ea;
         break;
         
    case 0x6F:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->read) (cpu,ea,false);
         (*cpu->write)(cpu,ea,op_clr(cpu));
         break;
         
    case 0x70:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_neg(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x71:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x72:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x73:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_com(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x74:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_lsr(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x75:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x76:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_ror(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x77:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_asr(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x78:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_lsl(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x79:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_rol(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x7A:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_dec(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x7B:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x7C:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,op_inc(cpu,(*cpu->read)(cpu,ea,false)));
         break;
         
    case 0x7D:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         op_tst(cpu,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x7E:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->pc.w = ea;
         break;
         
    case 0x7F:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->read) (cpu,ea,false);
         (*cpu->write)(cpu,ea,op_clr(cpu));
         break;
         
    case 0x80:
         cpu->cycles++;
         cpu->A = op_sub(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x81:
         cpu->cycles++;
         op_cmp(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x82:
         cpu->cycles++;
         cpu->A = op_sbc(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x83:
         cpu->cycles += 3;
         d16.b[MSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         d16.b[LSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         cpu->d.w     = op_sub16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0x84:
         cpu->cycles++;
         cpu->A = op_and(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x85:
         cpu->cycles++;
         op_bit(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x86:
         cpu->cycles++;
         cpu->A = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_ldst(cpu,cpu->A);
         break;
         
    case 0x87:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x88:
         cpu->cycles++;
         cpu->A = op_eor(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x89:
         cpu->cycles++;
         cpu->A = op_adc(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x8A:
         cpu->cycles++;
         cpu->A = op_or(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x8B:
         cpu->cycles++;
         cpu->A = op_add(cpu,cpu->A,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0x8C:
         cpu->cycles += 3;
         d16.b[MSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         d16.b[LSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_cmp16(cpu,cpu->X.w,d16.w);
         break;
         
    case 0x8D:
         cpu->cycles += 6;
         ea = mc6809_relative(cpu);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         cpu->pc.w = ea;
         break;
         
    case 0x8E:
         cpu->cycles   += 2;
         cpu->X.b[MSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         cpu->X.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_ldst16(cpu,cpu->X.w);
         break;
         
    case 0x8F:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0x90:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = op_sub(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x91:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         op_cmp(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x92:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = op_sbc(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x93:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         cpu->d.w   = op_sub16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0x94:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = op_and(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x95:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         op_bit(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x96:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = (*cpu->read)(cpu,ea,false);
         op_ldst(cpu,cpu->A);
         break;
         
    case 0x97:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,cpu->A);
         op_ldst(cpu,cpu->A);
         break;
         
    case 0x98:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = op_eor(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x99:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = op_adc(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x9A:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = op_or(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x9B:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->A = op_add(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0x9C:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->X.w,d16.w);
         break;
         
    case 0x9D:
         cpu->cycles += 6;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         cpu->pc.w = ea;
         break;
         
    case 0x9E:
         cpu->cycles += 4;
         ea = mc6809_direct(cpu);
         cpu->X.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->X.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->X.w);
         break;
         
    case 0x9F:
         cpu->cycles += 4;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea++,cpu->X.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->X.b[LSB]);
         op_ldst16(cpu,cpu->X.w);
         break;
         
    case 0xA0:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = op_sub(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xA1:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         op_cmp(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xA2:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = op_sbc(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xA3:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         cpu->d.w   = op_sub16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xA4:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = op_and(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xA5:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         op_bit(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xA6:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = (*cpu->read)(cpu,ea,false);
         op_ldst(cpu,cpu->A);
         break;
         
    case 0xA7:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,cpu->A);
         op_ldst(cpu,cpu->A);
         break;
         
    case 0xA8:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = op_eor(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xA9:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = op_adc(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xAA:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = op_or(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xAB:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->A = op_add(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xAC:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->X.w,d16.w);
         break;
         
    case 0xAD:
         cpu->cycles += 6;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         cpu->pc.w = ea;
         break;
         
    case 0xAE:
         cpu->cycles += 4;
         ea = mc6809_indexed(cpu);
         cpu->X.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->X.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->X.w);
         break;
         
    case 0xAF:
         cpu->cycles += 4;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea++,cpu->X.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->X.b[LSB]);
         op_ldst16(cpu,cpu->X.w);
         break;
         
    case 0xB0:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = op_sub(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xB1:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         op_cmp(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xB2:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = op_sbc(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xB3:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         cpu->d.w   = op_sub16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xB4:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = op_and(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xB5:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         op_bit(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xB6:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = (*cpu->read)(cpu,ea,false);
         op_ldst(cpu,cpu->A);
         break;
         
    case 0xB7:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,cpu->A);
         op_ldst(cpu,cpu->A);
         break;
         
    case 0xB8:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = op_eor(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xB9:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = op_adc(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xBA:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = op_or(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xBB:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->A = op_add(cpu,cpu->A,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xBC:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->X.w,d16.w);
         break;
         
    case 0xBD:
         cpu->cycles += 7;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         cpu->pc.w = ea;
         break;
         
    case 0xBE:
         cpu->cycles += 5;
         ea = mc6809_extended(cpu);
         cpu->X.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->X.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->X.w);
         break;
         
    case 0xBF:
         cpu->cycles += 5;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea++,cpu->X.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->X.b[LSB]);
         op_ldst16(cpu,cpu->X.w);
         break;
         
    case 0xC0:
         cpu->cycles++;
         cpu->B = op_sub(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xC1:
         cpu->cycles++;
         op_cmp(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xC2:
         cpu->cycles++;
         cpu->B = op_sbc(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xC3:
         cpu->cycles += 3;
         d16.b[MSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         d16.b[LSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         cpu->d.w     = op_add16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xC4:
         cpu->cycles++;
         cpu->B = op_and(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xC5:
         cpu->cycles++;
         op_bit(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xC6:
         cpu->cycles++;
         cpu->B = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_ldst(cpu,cpu->B);
         break;
         
    case 0xC7:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0xC8:
         cpu->cycles++;
         cpu->B = op_eor(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xC9:
         cpu->cycles++;
         cpu->B = op_adc(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xCA:
         cpu->cycles++;
         cpu->B = op_or(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xCB:
         cpu->cycles++;
         cpu->B = op_add(cpu,cpu->B,(*cpu->read)(cpu,cpu->pc.w++,true));
         break;
         
    case 0xCC:
         cpu->cycles += 2;
         cpu->A       = (*cpu->read)(cpu,cpu->pc.w++,true);
         cpu->B       = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_ldst16(cpu,cpu->d.w);
         break;
         
    case 0xCD:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0xCE:
         cpu->cycles   += 2;
         cpu->U.b[MSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         cpu->U.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_ldst16(cpu,cpu->U.w);
         break;
         
    case 0xCF:
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
         
    case 0xD0:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = op_sub(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xD1:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         op_cmp(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xD2:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = op_sbc(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xD3:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         cpu->d.w   = op_add16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xD4:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = op_and(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xD5:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         op_bit(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xD6:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = (*cpu->read)(cpu,ea,false);
         op_ldst(cpu,cpu->B);
         break;
         
    case 0xD7:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea,cpu->B);
         op_ldst(cpu,cpu->B);
         break;
         
    case 0xD8:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = op_eor(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xD9:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = op_adc(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xDA:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = op_or(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xDB:
         cpu->cycles += 3;
         ea = mc6809_direct(cpu);
         cpu->B = op_add(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xDC:
         cpu->cycles += 4;
         ea = mc6809_direct(cpu);
         cpu->A    = (*cpu->read)(cpu,ea++,false);
         cpu->B    = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->d.w);
         break;
         
    case 0xDD:
         cpu->cycles += 6;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea++,cpu->A);
         (*cpu->write)(cpu,ea,cpu->B);
         op_ldst16(cpu,cpu->d.w);
         break;
         
    case 0xDE:
         cpu->cycles += 4;
         ea = mc6809_direct(cpu);
         cpu->U.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->U.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->U.w);
         break;
         
    case 0xDF:
         cpu->cycles += 4;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea++,cpu->U.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->U.b[LSB]);
         op_ldst16(cpu,cpu->U.w);
         break;
         
    case 0xE0:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = op_sub(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xE1:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         op_cmp(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xE2:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = op_sbc(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xE3:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         cpu->d.w   = op_add16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xE4:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = op_and(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xE5:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         op_bit(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xE6:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = (*cpu->read)(cpu,ea,false);
         op_ldst(cpu,cpu->B);
         break;
         
    case 0xE7:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea,cpu->B);
         op_ldst(cpu,cpu->B);
         break;
         
    case 0xE8:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = op_eor(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xE9:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = op_adc(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xEA:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = op_or(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xEB:
         cpu->cycles += 3;
         ea = mc6809_indexed(cpu);
         cpu->B = op_add(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xEC:
         cpu->cycles += 4;
         ea = mc6809_indexed(cpu);
         cpu->A = (*cpu->read)(cpu,ea++,false);
         cpu->B = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->d.w);
         break;
         
    case 0xED:
         cpu->cycles += 6;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea++,cpu->A);
         (*cpu->write)(cpu,ea,cpu->B);
         op_ldst16(cpu,cpu->d.w);
         break;
         
    case 0xEE:
         cpu->cycles += 4;
         ea = mc6809_indexed(cpu);
         cpu->U.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->U.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->U.w);
         break;
         
    case 0xEF:
         cpu->cycles += 4;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea++,cpu->U.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->U.b[LSB]);
         op_ldst16(cpu,cpu->U.w);
         break;
         
    case 0xF0:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = op_sub(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xF1:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         op_cmp(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xF2:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = op_sbc(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xF3:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         cpu->d.w   = op_add16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xF4:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = op_and(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xF5:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         op_bit(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xF6:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = (*cpu->read)(cpu,ea,false);
         op_ldst(cpu,cpu->B);
         break;
         
    case 0xF7:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea,cpu->B);
         op_ldst(cpu,cpu->B);
         break;
         
    case 0xF8:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = op_eor(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xF9:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = op_adc(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xFA:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = op_or(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xFB:
         cpu->cycles += 4;
         ea = mc6809_extended(cpu);
         cpu->B = op_add(cpu,cpu->B,(*cpu->read)(cpu,ea,false));
         break;
         
    case 0xFC:
         cpu->cycles += 5;
         ea = mc6809_extended(cpu);
         cpu->A = (*cpu->read)(cpu,ea++,false);
         cpu->B = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->d.w);
         break;
         
    case 0xFD:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea++,cpu->A);
         (*cpu->write)(cpu,ea,cpu->B);
         op_ldst16(cpu,cpu->d.w);
         break;
         
    case 0xFE:
         cpu->cycles += 5;
         ea = mc6809_extended(cpu);
         cpu->U.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->U.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->U.w);
         break;
         
    case 0xFF:
         cpu->cycles += 5;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea++,cpu->U.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->U.b[LSB]);
         op_ldst16(cpu,cpu->U.w);
         break;
         
    default:
         assert(0);
         (*cpu->fault)(cpu,MC6809_FAULT_INTERNAL_ERROR);
         break;
  }
  
  return 0;
}

/************************************************************************/

static int page2(mc6809__t *cpu)
{
  mc6809addr__t ea;
  mc6809word__t d16;
  mc6809byte__t inst;
  
  assert(cpu != NULL);
  
  cpu->page2 = true;
  inst       = (*cpu->read)(cpu,cpu->pc.w++,true);
  
  /*-----------------------------------------------------------------------
  ; While the cycle counts may appear to be one less than stated in the
  ; manual, the addtional cycle has already been calculated via the
  ; mc6809_step() function (the same applies for the page3() routine).
  ;-----------------------------------------------------------------------*/
  
  switch(inst)
  {
    case 0x21:
         cpu->cycles += 4;
         mc6809_lrelative(cpu);
         break;
         
    case 0x22:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bhi(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x23:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bls(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x24:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bhs(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x25:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (blo(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x26:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bne(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x27:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (beq(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x28:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bvc(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x29:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bvs(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x2A:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bpl(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x2B:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bmi(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x2C:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bge(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x2D:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (blt(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x2E:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (bgt(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x2F:
         cpu->cycles += 4;
         ea = mc6809_lrelative(cpu);
         if (ble(cpu))
         {
           cpu->cycles++;
           cpu->pc.w = ea;
         }
         break;
         
    case 0x3F:
         cpu->cycles += 19;
         cpu->cc.e    = true;
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->dp);
         (*cpu->write)(cpu,--cpu->S.w,cpu->B);
         (*cpu->write)(cpu,--cpu->S.w,cpu->A);
         (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
         cpu->pc.b[MSB] = (*cpu->read)(cpu,MC6809_VECTOR_SWI2,false);
         cpu->pc.b[LSB] = (*cpu->read)(cpu,MC6809_VECTOR_SWI2 + 1,false);
         break;
         
    case 0x83:
         cpu->cycles += 4;
         d16.b[MSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         d16.b[LSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_cmp16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0x8C:
         cpu->cycles += 4;
         d16.b[MSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         d16.b[LSB]   = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_cmp16(cpu,cpu->Y.w,d16.w);
         break;
         
    case 0x8E:
         cpu->cycles   += 3;
         cpu->Y.b[MSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         cpu->Y.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_ldst16(cpu,cpu->Y.w);
         break;
         
    case 0x93:
         cpu->cycles += 6;
         ea = mc6809_direct(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0x9C:
         cpu->cycles += 6;
         ea = mc6809_direct(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->Y.w,d16.w);
         break;
         
    case 0x9E:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         cpu->Y.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->Y.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->Y.w);
         break;
         
    case 0x9F:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea++,cpu->Y.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->Y.b[LSB]);
         op_ldst16(cpu,cpu->Y.w);
         break;
         
    case 0xA3:
         cpu->cycles += 6;
         ea = mc6809_indexed(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xAC:
         cpu->cycles += 6;
         ea = mc6809_indexed(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->Y.w,d16.w);
         break;
         
    case 0xAE:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         cpu->Y.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->Y.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->Y.w);
         break;
         
    case 0xAF:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea++,cpu->Y.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->Y.b[LSB]);
         op_ldst16(cpu,cpu->Y.w);
         break;
         
    case 0xB3:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->d.w,d16.w);
         break;
         
    case 0xBC:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->Y.w,d16.w);
         break;
         
    case 0xBE:
         cpu->cycles += 5;
         ea = mc6809_extended(cpu);
         cpu->Y.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->Y.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->Y.w);
         break;
         
    case 0xBF:
         cpu->cycles += 5;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea++,cpu->Y.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->Y.b[LSB]);
         op_ldst16(cpu,cpu->Y.w);
         break;
         
    case 0xCE:
         cpu->cycles   += 3;
         cpu->S.b[MSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         cpu->S.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_ldst16(cpu,cpu->S.w);
         cpu->nmi_armed = true;
         break;
         
    case 0xDE:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         cpu->S.b[MSB]    = (*cpu->read)(cpu,ea++,false);
         cpu->S.b[LSB]    = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->S.w);
         cpu->nmi_armed = true;
         break;
         
    case 0xDF:
         cpu->cycles += 5;
         ea = mc6809_direct(cpu);
         (*cpu->write)(cpu,ea++,cpu->S.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->S.b[LSB]);
         op_ldst16(cpu,cpu->S.w);
         break;
         
    case 0xEE:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         cpu->S.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->S.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->S.w);
         cpu->nmi_armed = true;
         break;
         
    case 0xEF:
         cpu->cycles += 5;
         ea = mc6809_indexed(cpu);
         (*cpu->write)(cpu,ea++,cpu->S.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->S.b[LSB]);
         op_ldst16(cpu,cpu->S.w);
         break;
         
    case 0xFE:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         cpu->S.b[MSB] = (*cpu->read)(cpu,ea++,false);
         cpu->S.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_ldst16(cpu,cpu->S.w);
         cpu->nmi_armed = true;
         break;
         
    case 0xFF:
         cpu->cycles += 6;
         ea = mc6809_extended(cpu);
         (*cpu->write)(cpu,ea++,cpu->S.b[MSB]);
         (*cpu->write)(cpu,ea,cpu->S.b[LSB]);
         op_ldst16(cpu,cpu->S.w);
         break;
         
    default:
         cpu->cycles++;
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
  }
  
  cpu->page2 = false;
  return 0;
}

/************************************************************************/

static int page3(mc6809__t *cpu)
{
  mc6809addr__t ea;
  mc6809word__t d16;
  mc6809byte__t inst;
  
  assert(cpu != NULL);
  
  cpu->page3 = true;
  inst       = (*cpu->read)(cpu,cpu->pc.w++,true);
  
  /*-----------------------------------------------------------------
  ; see the comment in mc6809_step() for an explanation on cycle counts
  ;------------------------------------------------------------------*/
  
  switch(inst)
  {
    case 0x3F:
         cpu->cycles += 19;
         cpu->cc.e    = true;
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->pc.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->U.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->Y.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[LSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->X.b[MSB]);
         (*cpu->write)(cpu,--cpu->S.w,cpu->dp);
         (*cpu->write)(cpu,--cpu->S.w,cpu->B);
         (*cpu->write)(cpu,--cpu->S.w,cpu->A);
         (*cpu->write)(cpu,--cpu->S.w,mc6809_cctobyte(cpu));
         cpu->pc.b[MSB] = (*cpu->read)(cpu,MC6809_VECTOR_SWI3,false);
         cpu->pc.b[LSB] = (*cpu->read)(cpu,MC6809_VECTOR_SWI3 + 1,false);
         break;
         
    case 0x83:
         cpu->cycles += 4;
         d16.b[MSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
         d16.b[LSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_cmp16(cpu,cpu->U.w,d16.w);
         break;
         
    case 0x8C:
         cpu->cycles += 4;
         d16.b[MSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
         d16.b[LSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
         op_cmp16(cpu,cpu->S.w,d16.w);
         break;
         
    case 0x93:
         cpu->cycles += 6;
         ea = mc6809_direct(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->U.w,d16.w);
         break;
         
    case 0x9C:
         cpu->cycles += 6;
         ea = mc6809_direct(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->S.w,d16.w);
         break;
         
    case 0xA3:
         cpu->cycles += 6;
         ea = mc6809_indexed(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->U.w,d16.w);
         break;
         
    case 0xAC:
         cpu->cycles += 6;
         ea = mc6809_indexed(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->S.w,d16.w);
         break;
         
    case 0xB3:
         cpu->cycles += 7;
         ea = mc6809_extended(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->U.w,d16.w);
         break;
         
    case 0xBC:
         cpu->cycles += 7;
         ea = mc6809_extended(cpu);
         d16.b[MSB] = (*cpu->read)(cpu,ea++,false);
         d16.b[LSB] = (*cpu->read)(cpu,ea,false);
         op_cmp16(cpu,cpu->S.w,d16.w);
         break;
         
    default:
         cpu->cycles++;
         (*cpu->fault)(cpu,MC6809_FAULT_INSTRUCTION);
         break;
  }
  
  cpu->page3 = false;
  return 0;
}

/***********************************************************************/

mc6809addr__t mc6809_direct(mc6809__t *cpu)
{
  mc6809word__t ea;
  
  assert(cpu != NULL);
  
  ea.b[MSB] = cpu->dp;
  ea.b[LSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
  return ea.w;
}

/***********************************************************************/

mc6809addr__t mc6809_relative(mc6809__t *cpu)
{
  mc6809word__t ea;
  
  assert(cpu != NULL);
  
  ea.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
  ea.b[MSB]  = (ea.b[LSB] > 0x7F) ? 0xFF : 0x00;
  ea.w      += cpu->pc.w;
  return ea.w;
}

/***********************************************************************/

mc6809addr__t mc6809_lrelative(mc6809__t *cpu)
{
  mc6809word__t ea;
  
  assert(cpu != NULL);
  
  ea.b[MSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
  ea.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
  ea.w      += cpu->pc.w;
  return ea.w;
}

/*********************************************************************/

mc6809addr__t mc6809_extended(mc6809__t *cpu)
{
  mc6809word__t ea;
  
  assert(cpu != NULL);
  
  ea.b[MSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
  ea.b[LSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
  return ea.w;
}

/***********************************************************************/

mc6809addr__t mc6809_indexed(mc6809__t *cpu)
{
  mc6809word__t ea;
  mc6809word__t d16;
  mc6809byte__t mode;
  int           reg;
  int           off;
  
  assert(cpu != NULL);
  
  mode = (*cpu->read)(cpu,cpu->pc.w++,true);
  reg  = (mode & 0x60) >> 5;
  off  = (mode & 0x1F);
  
  if (mode < 0x80)
  {
    cpu->cycles++;
    ea.w = (mc6809addr__t)off;
    if (ea.w > 15) ea.w |= 0xFFE0;
    ea.w += cpu->index[reg].w;
    return ea.w;
  }
  
  switch(off)
  {
    case 0x00:
         cpu->cycles += 2;
         ea.w         = cpu->index[reg].w++;
         break;
         
    case 0x01:
         cpu->cycles       += 3;
         ea.w               = cpu->index[reg].w;
         cpu->index[reg].w += 2;
         break;
         
    case 0x02:
         cpu->cycles += 2;
         ea.w         = --cpu->index[reg].w;
         break;
         
    case 0x03:
         cpu->cycles       += 3;
         cpu->index[reg].w -= 2;
         ea.w               = cpu->index[reg].w;
         break;
         
    case 0x04:
         ea.w = cpu->index[reg].w;
         break;
         
    case 0x05:
         cpu->cycles++;
         ea.b[LSB]  = cpu->B;
         ea.b[MSB]  = (cpu->B < 0x80) ? 0x00 : 0xFF;
         ea.w      += cpu->index[reg].w;
         break;
         
    case 0x06:
         cpu->cycles++;
         ea.b[LSB]  = cpu->A;
         ea.b[MSB]  = (cpu->A < 0x80) ? 0x00: 0xFF;
         ea.w      += cpu->index[reg].w;
         break;
         
    case 0x07:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x08:
         cpu->cycles++;
         ea.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[MSB]  = (ea.b[LSB] < 0x80) ? 0x00 : 0xFF;
         ea.w      += cpu->index[reg].w;
         break;
         
    case 0x09:
         cpu->cycles    += 4;
         ea.b[MSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[LSB]  = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.w      += cpu->index[reg].w;
         break;
         
    case 0x0A:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x0B:
         cpu->cycles += 4;
         ea.w         = cpu->index[reg].w + cpu->d.w;
         break;
         
    case 0x0C:
         cpu->cycles++;
         ea.b[LSB] = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[MSB] = (ea.b[LSB] < 0x80) ? 0x00 : 0xFF;
         ea.w     += cpu->pc.w;
         break;
         
    case 0x0D:
         cpu->cycles += 5;
         ea.b[MSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[LSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.w        += cpu->pc.w;
         break;
         
    case 0x0E:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x0F:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x10:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x11:
         cpu->cycles       += 6;
         ea.w               = cpu->index[reg].w;
         cpu->index[reg].w += 2;
         d16.b[MSB]         = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]         = (*cpu->read)(cpu,ea.w,false);
         ea.w               = d16.w;
         break;
         
    case 0x12:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x13:
         cpu->cycles       += 6;
         cpu->index[reg].w -= 2;
         ea.b[MSB]          = (*cpu->read)(cpu,cpu->index[reg].w,false);
         ea.b[LSB]          = (*cpu->read)(cpu,cpu->index[reg].w + 1,false);
         break;
         
    case 0x14:
         cpu->cycles += 3;
         ea.b[MSB]    = (*cpu->read)(cpu,cpu->index[reg].w,false);
         ea.b[LSB]    = (*cpu->read)(cpu,cpu->index[reg].w + 1,false);
         break;
         
    case 0x15:
         cpu->cycles += 4;
         ea.b[LSB]    = cpu->B;
         ea.b[MSB]    = (cpu->B < 0x80) ? 0x00 : 0xFF;
         ea.w        += cpu->index[reg].w;
         d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
         ea.w         = d16.w;
         break;
         
    case 0x16:
         cpu->cycles += 4;
         ea.b[LSB]    = cpu->A;
         ea.b[MSB]    = (cpu->A < 0x80) ? 0x00 : 0xFF;
         ea.w        += cpu->index[reg].w;
         d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
         ea.w         = d16.w;
         break;
         
    case 0x17:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x18:
         cpu->cycles += 4;
         ea.b[LSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[MSB]    = (ea.b[LSB] < 0x80) ? 0x00 : 0xFF;
         ea.w        += cpu->index[reg].w;
         d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
         ea.w         = d16.w;
         break;
         
    case 0x19:
         cpu->cycles += 7;
         ea.b[MSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[LSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.w        += cpu->index[reg].w;
         d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
         ea.w         = d16.w;
         break;
         
    case 0x1A:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x1B:
         cpu->cycles += 7;
         ea.w         = cpu->d.w;
         ea.w        += cpu->index[reg].w;
         d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
         ea.w         = d16.w;
         break;
         
    case 0x1C:
         cpu->cycles += 4;
         ea.b[LSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[MSB]    = (ea.b[LSB] < 0x80) ? 0x00 : 0xFF;
         ea.w        += cpu->pc.w;
         d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
         ea.w         = d16.w;
         break;
         
    case 0x1D:
         cpu->cycles += 8;
         ea.b[MSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.b[LSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
         ea.w        += cpu->pc.w;
         d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
         d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
         ea.w         = d16.w;
         break;
         
    case 0x1E:
         (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    case 0x1F:
         if (reg == 0)
         {
           cpu->cycles += 5;
           ea.b[MSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
           ea.b[LSB]    = (*cpu->read)(cpu,cpu->pc.w++,true);
           d16.b[MSB]   = (*cpu->read)(cpu,ea.w++,false);
           d16.b[LSB]   = (*cpu->read)(cpu,ea.w,false);
           ea.w         = d16.w;
         }
         else
           (*cpu->fault)(cpu,MC6809_FAULT_ADDRESS_MODE);
         break;
         
    default:
         assert(0);
         (*cpu->fault)(cpu,MC6809_FAULT_INTERNAL_ERROR);
         break;
  }
  
  return ea.w;
}

/*************************************************************************/

mc6809byte__t mc6809_cctobyte(mc6809__t *cpu)
{
  mc6809byte__t r = 0;
  
  assert(cpu != NULL);
  
  if (cpu->cc.c) r |= 0x01;
  if (cpu->cc.v) r |= 0x02;
  if (cpu->cc.z) r |= 0x04;
  if (cpu->cc.n) r |= 0x08;
  if (cpu->cc.i) r |= 0x10;
  if (cpu->cc.h) r |= 0x20;
  if (cpu->cc.f) r |= 0x40;
  if (cpu->cc.e) r |= 0x80;
  
  return r;
}

/************************************************************************/

void mc6809_bytetocc(mc6809__t *cpu,mc6809byte__t const r)
{
  assert(cpu != NULL);
  
  cpu->cc.c = ((r & 0x01) != 0);
  cpu->cc.v = ((r & 0x02) != 0);
  cpu->cc.z = ((r & 0x04) != 0);
  cpu->cc.n = ((r & 0x08) != 0);
  cpu->cc.i = ((r & 0x10) != 0);
  cpu->cc.h = ((r & 0x20) != 0);
  cpu->cc.f = ((r & 0x40) != 0);
  cpu->cc.e = ((r & 0x80) != 0);
}

/*************************************************************************/

static mc6809byte__t op_neg(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = -src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = (src == 0x80);
  cpu->cc.c = (src != 0x00);
  return res;
}

/*************************************************************************/

static mc6809byte__t op_com(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = ~src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = false;
  cpu->cc.c = true;
  return res;
}

/*************************************************************************/

static mc6809byte__t op_lsr(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = src >> 1;
  cpu->cc.n = false;
  cpu->cc.z = (res == 0x00);
  cpu->cc.c = (src &  0x01) == 0x01;
  return res;
}

/*************************************************************************/

static mc6809byte__t op_ror(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res        = src >> 1;
  res       |= cpu->cc.c ? 0x80 : 0x00;
  cpu->cc.n  = (res > 0x7F);
  cpu->cc.z  = (res == 0x00);
  cpu->cc.c  = (src &  0x01) == 0x01;
  return res;
}

/*************************************************************************/

static mc6809byte__t op_asr(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res        = src >> 1;
  res       |= (src >  0x7F) ? 0x80 : 0x00;
  cpu->cc.n  = (res >  0x7F);
  cpu->cc.z  = (res == 0x00);
  cpu->cc.c  = (src &  0x01) == 0x01;
  return res;
}

/*************************************************************************/

static mc6809byte__t op_lsl(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = src << 1;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = ((src ^ res) & 0x80) == 0x80;
  cpu->cc.c = (src >  0x7F);
  return res;
}

/************************************************************************/

static mc6809byte__t op_rol(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res = src << 1;
  res |= cpu->cc.c ? 0x01 : 0x00;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = ((src ^ res) & 0x80) == 0x80;
  cpu->cc.c = (src >  0x7F);
  return res;
}

/************************************************************************/

static mc6809byte__t op_dec(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = src - 1;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = (src == 0x80);
  return res;
}

/************************************************************************/

static mc6809byte__t op_inc(mc6809__t *cpu,mc6809byte__t const src)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = src + 1;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = (src == 0x7F);
  return res;
}

/************************************************************************/

static void op_tst(mc6809__t *cpu,mc6809byte__t const src)
{
  assert(cpu != NULL);
  
  cpu->cc.n = (src >  0x7F);
  cpu->cc.z = (src == 0x00);
  cpu->cc.v = false;
}

/************************************************************************/

static mc6809byte__t op_clr(mc6809__t *cpu)
{
  assert(cpu != NULL);
  
  cpu->cc.n = false;
  cpu->cc.z = true;
  cpu->cc.v = false;
  cpu->cc.c = false;
  return 0;
}

/************************************************************************/

static mc6809byte__t op_sub(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  mc6809byte__t ci;
  
  assert(cpu != NULL);
  
  res       = dest - src;
  ci        = res ^ dest ^ src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.c = src > dest;
  cpu->cc.v = ((ci & 0x80) != 0) ^ cpu->cc.c;
  return res;
}

/************************************************************************/

static void op_cmp(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  mc6809byte__t ci;
  
  assert(cpu != NULL);
  
  res       = dest - src;
  ci        = res ^ dest ^ src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.c = src > dest;
  cpu->cc.v = ((ci & 0x80) != 0) ^ cpu->cc.c;
}

/************************************************************************/

static mc6809byte__t op_sbc(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  mc6809byte__t ci;
  
  assert(cpu       != NULL);
  
  res       = dest - src - cpu->cc.c;
  ci        = res ^ dest ^ src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.c = src + cpu->cc.c > dest;
  cpu->cc.v = ((ci & 0x80) != 0) ^ cpu->cc.c;
  return res;
}

/************************************************************************/

static mc6809byte__t op_and(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = dest & src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = false;
  return res;
}

/************************************************************************/

static void op_bit(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = dest & src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = false;
}

/************************************************************************/

static void op_ldst(
        mc6809__t           *cpu,
        mc6809byte__t const  dest
)
{
  assert(cpu != NULL);
  
  cpu->cc.n = (dest >  0x7F);
  cpu->cc.z = (dest == 0x00);
  cpu->cc.v = false;
}

/************************************************************************/

static mc6809byte__t op_eor(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = dest ^ src;
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = false;
  return res;
}

/************************************************************************/

static mc6809byte__t op_adc(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  mc6809byte__t ci;
  
  assert(cpu       != NULL);
  
  res       = dest + src + cpu->cc.c;
  ci        = res ^ dest ^ src;
  cpu->cc.h = (ci  &  0x10);
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.c = (res < dest + cpu->cc.c);
  cpu->cc.v = ((ci & 0x80) != 0) ^ cpu->cc.c;
  return res;
}

/************************************************************************/

static mc6809byte__t op_or(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  
  assert(cpu != NULL);
  
  res       = dest | src;
  cpu->cc.n = (res >  0x7f);
  cpu->cc.z = (res == 0x00);
  cpu->cc.v = false;
  return res;
}

/************************************************************************/

static mc6809byte__t op_add(
        mc6809__t           *cpu,
        mc6809byte__t const  dest,
        mc6809byte__t const  src
)
{
  mc6809byte__t res;
  mc6809byte__t ci;
  
  assert(cpu != NULL);
  
  res       = dest + src;
  ci        = res ^ dest ^ src;
  cpu->cc.h = (ci  &  0x10);
  cpu->cc.n = (res >  0x7F);
  cpu->cc.z = (res == 0x00);
  cpu->cc.c = (res <  dest);
  cpu->cc.v = ((ci & 0x80) != 0) ^ cpu->cc.c;
  return res;
}

/************************************************************************/

static mc6809addr__t op_sub16(
        mc6809__t           *cpu,
        mc6809addr__t const  dest,
        mc6809addr__t const  src
)
{
  mc6809addr__t res;
  mc6809addr__t ci;
  
  assert(cpu != NULL);
  
  res       = dest - src;
  ci        = res ^ dest ^ src;
  cpu->cc.n = (res >  0x7FFF);
  cpu->cc.z = (res == 0x0000);
  cpu->cc.c = src > dest;
  cpu->cc.v = ((ci & 0x8000) != 0) ^ cpu->cc.c;
  return res;
}

/************************************************************************/

static void op_cmp16(
        mc6809__t           *cpu,
        mc6809addr__t const  dest,
        mc6809addr__t const  src
)
{
  mc6809addr__t res;
  mc6809addr__t ci;
  
  assert(cpu != NULL);
  
  res       = dest - src;
  ci        = res ^ dest ^ src;
  cpu->cc.n = (res >  0x7FFF);
  cpu->cc.z = (res == 0x0000);
  cpu->cc.c = src > dest;
  cpu->cc.v = ((ci & 0x8000) != 0) ^ cpu->cc.c;
}

/************************************************************************/

static void op_ldst16(
        mc6809__t           *cpu,
        mc6809addr__t const  data
)
{
  assert(cpu != NULL);
  cpu->cc.n = (data >  0x7FFF);
  cpu->cc.z = (data == 0x0000);
  cpu->cc.v = false;
}

/************************************************************************/

static mc6809addr__t op_add16(
        mc6809__t           *cpu,
        mc6809addr__t const  dest,
        mc6809addr__t const  src
)
{
  mc6809addr__t res;
  mc6809addr__t ci;
  
  assert(cpu != NULL);
  
  res       = dest + src;
  ci        = res ^ dest ^ src;
  cpu->cc.n = (res >  0x7FFF);
  cpu->cc.z = (res == 0x0000);
  cpu->cc.c = (res <  dest);
  cpu->cc.v = ((ci & 0x8000) != 0) ^ cpu->cc.c;
  return res;
}

/************************************************************************/
