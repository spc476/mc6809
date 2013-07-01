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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mc6809.h"
#include "mc6809dis.h"

/*************************************************************************/

struct userdata
{
  mc6809byte__t *memory;
  bool          *readonly;
  mc6809__t      cpu;
  mc6809dis__t   dis;
};

/************************************************************************/

static bool		loadimage	(struct userdata *,const char *,const char *);
static bool		dumpcore	(struct userdata *);
static mc6809byte__t	cpu_read	(mc6809__t *,mc6809addr__t,bool);
static void		cpu_write	(mc6809__t *,mc6809addr__t,mc6809byte__t);
static void		cpu_fault	(mc6809__t *,mc6809fault__t);
static mc6809byte__t	dis_read	(mc6809dis__t *,mc6809addr__t);
static void		dis_fault	(mc6809dis__t *,mc6809fault__t);

/************************************************************************/

int main(int argc,char *argv[])
{
  struct userdata ud;
  int             rc;
  
  if (argc == 1)
  {
    fprintf(stderr,"usage: %s imagefile loadaddr ... \n",argv[0]);
    fprintf(stderr,"\tloadaddr = HHHH[r] where H = hexdigit,\n");
    fprintf(stderr,"\t           and r = r (for read only)\n");
    return EXIT_FAILURE;
  }
  
  ud.memory = malloc(65536uL * sizeof(mc6809byte__t));
  if (ud.memory == NULL)
  {
    perror("malloc()");
    return EXIT_FAILURE;
  }
  
  ud.readonly = calloc(65536uL,sizeof(bool));
  if (ud.readonly == NULL)
  {
    perror("malloc()");
    free(ud.memory);
    return EXIT_FAILURE;
  }
  
  ud.cpu.user  = &ud;
  ud.cpu.read  = cpu_read;
  ud.cpu.write = cpu_write;
  ud.cpu.fault = cpu_fault;
  
  ud.dis.user  = &ud;
  ud.dis.read  = dis_read;
  ud.dis.fault = dis_fault;

  for ( int i = 1 ; i < argc ; i += 2 )
  {
    if (!loadimage(&ud,argv[i],argv[i + 1]))
    {
      perror(argv[i]);
      free(ud.readonly);
      free(ud.memory);
      return EXIT_FAILURE;
    }
  }
  
  mc6809_reset(&ud.cpu);
  rc = mc6809_run(&ud.cpu);
  if (rc != 0)
  {
    switch(rc)
    {
      default:
      case MC6809_FAULT_INTERNAL_ERROR:
           fprintf(stderr,"An internal error occured that should not happen.\n");
           break;
           
      case MC6809_FAULT_INSTRUCTION:
           fprintf(stderr,"Illegal instruction @ %04X\n",ud.cpu.pc.w - 1);
           break;
           
      case MC6809_FAULT_ADDRESS_MODE:
           fprintf(stderr,"Illegal addressing mode in instruction @ %04X\n",ud.cpu.instpc);
           break;
           
      case MC6809_FAULT_EXG:
           fprintf(stderr,"Undefined EXG behavior in instruction @ %04X\n",ud.cpu.instpc);
           break;
           
      case MC6809_FAULT_TFR:
           fprintf(stderr,"Undefined TFR behavior in instruction @ %04X\n",ud.cpu.instpc);
           break;
           
      case MC6809_FAULT_user:
           fprintf(stderr,
           		"This can be used to signal other errors, but you should\n"
           		"only see this if you check the source code.\n"
           	);
           break;
    }
    
    dumpcore(&ud);
  }
  
  free(ud.readonly);
  free(ud.memory);
  
  return rc;
}

/*********************************************************************/

static bool loadimage(
	struct userdata *ud,
	const char      *filename,
	const char      *taddr
)
{
  mc6809addr__t  addr;
  FILE          *fp;
  char          *ro;
  int            c;
  
  assert(ud       != NULL);
  assert(filename != NULL);
  assert(taddr    != NULL);
  
  addr = strtoul(taddr,&ro,16);
  if (*ro != 'r')
    ro = NULL;

  fp = fopen(filename,"rb");
  if (fp == NULL)
    return false;
  
  while((c = fgetc(fp)) != EOF)
  {
    ud->memory[addr] = c;
    if (ro)
      ud->readonly[addr] = true;
    addr++;
  }
  
  fclose(fp);
  return true;
}

/***********************************************************************/

static bool dumpcore(struct userdata *ud)
{
  FILE *fp;
  
  fp = fopen("mc6809-core","wb");
  if (fp != NULL)
  {
    fwrite(ud->memory,1,65536uL,fp);
    fwrite(&ud->cpu.pc.b[MSB],1,1,fp);
    fwrite(&ud->cpu.pc.b[LSB],1,1,fp);
    fwrite(&ud->cpu.X.b[MSB],1,1,fp);
    fwrite(&ud->cpu.X.b[LSB],1,1,fp);
    fwrite(&ud->cpu.Y.b[MSB],1,1,fp);
    fwrite(&ud->cpu.Y.b[LSB],1,1,fp);
    fwrite(&ud->cpu.U.b[MSB],1,1,fp);
    fwrite(&ud->cpu.U.b[LSB],1,1,fp);
    fwrite(&ud->cpu.S.b[MSB],1,1,fp);
    fwrite(&ud->cpu.S.b[LSB],1,1,fp);
    fwrite(&ud->cpu.dp,1,1,fp);
    fwrite(&ud->cpu.A,1,1,fp);
    fwrite(&ud->cpu.B,1,1,fp);
    fputc(mc6809_cctobyte(&ud->cpu),fp);
    fclose(fp);
    return true;
  }
  else
    return false;
}

/***********************************************************************/
static mc6809byte__t cpu_read(
	mc6809__t     *cpu,
	mc6809addr__t  addr,
	bool           ifetch __attribute__((unused))
)
{
  struct userdata *ud;
  char             inst[128];
  char             regs[128];
  
  assert(cpu       != NULL);
  assert(cpu->user != NULL);
  
  ud = cpu->user;
  assert(ud->memory != NULL);
  
  if (cpu->instpc == addr)	/* start of an instruction */
  {
    ud->dis.pc = addr;
    mc6809dis_step(&ud->dis,cpu);
    mc6809dis_format(&ud->dis,inst,sizeof(inst));
    mc6809dis_registers(cpu,regs,sizeof(regs));
    printf("%s | %s\n",regs,inst);
  }
  
  return ud->memory[addr];
}

/************************************************************************/

static void cpu_write(
	mc6809__t     *cpu,
	mc6809addr__t  addr,
	mc6809byte__t  b
)
{
  struct userdata *ud;
  
  assert(cpu       != NULL);
  assert(cpu->user != NULL);
  
  ud = cpu->user;
  
  assert(ud->memory   != NULL);
  assert(ud->readonly != NULL);
  
  if (!ud->readonly[addr])
    ud->memory[addr] = b;
}

/************************************************************************/

static void cpu_fault(
	mc6809__t      *cpu __attribute__((unused)),
	mc6809fault__t  fault
)
{
  assert(cpu != NULL);  
  longjmp(cpu->err,fault);
}

/***********************************************************************/

static mc6809byte__t dis_read(
	mc6809dis__t  *dis,
	mc6809addr__t  addr
)
{
  struct userdata *ud;
  
  assert(dis       != NULL);
  assert(dis->user != NULL);
  
  ud = dis->user;
  assert(ud->memory != NULL);  
  return ud->memory[addr];
}

/***********************************************************************/

static void dis_fault(
	mc6809dis__t *dis,
	mc6809fault__t fault
)
{
  assert(dis != NULL);
  
  switch(fault)
  {
    default:
    case MC6809_FAULT_INTERNAL_ERROR:
         fprintf(stderr,"An internal error occured during disassembly that should not happen.\n");
         break;
           
    case MC6809_FAULT_INSTRUCTION:
         fprintf(stderr,"Illegal instruction decoded @ %04X\n",dis->next - 1);
         break;
         
    case MC6809_FAULT_ADDRESS_MODE:
         fprintf(stderr,"Illegal addressing mode decoded @ %04X\n",dis->pc);
         break;
           
    case MC6809_FAULT_EXG:
         fprintf(stderr,"Undefined EXG behavior decoded @ %04X\n",dis->pc);
         break;
           
    case MC6809_FAULT_TFR:
         fprintf(stderr,"Undefined TFR behavior decoded @ %04X\n",dis->pc);
         break;
           
    case MC6809_FAULT_user:
         fprintf(stderr,
           		"This can be used to signal other errors, but you should\n"
         		"only see this if you check the source code.\n"
           	);
         break;
  }
  
  longjmp(dis->err,fault);
}

/***********************************************************************/
