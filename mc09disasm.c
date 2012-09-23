
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mc6809dis.h"

/*************************************************************************/

static mc6809byte__t dis_read(
	mc6809dis__t  *dis,
	mc6809addr__t  addr __attribute__((unused))
)
{
  int c;
  
  assert(dis       != NULL);
  assert(dis->user != NULL);
  
  c = fgetc((FILE *)dis->user);
  if (c == EOF)
    longjmp(dis->err,MC6809_DIS_DONE);
  return c;
}

/*************************************************************************/

static void dis_fault(
	mc6809dis__t   *dis,
	mc6809fault__t  fault
)
{
  assert(dis       != NULL);
  assert(dis->user != NULL);
  
  fprintf(stderr,"fault: %d\n",fault);
  fclose((FILE *)dis->user);
  exit(EXIT_FAILURE);
}

/*************************************************************************/

int main(int argc,char *argv[])
{
  mc6809dis__t   dis;
  mc6809addr__t  start;
  const char    *fname;
  FILE          *fp;
  int            rc;
  
  start = 0;
  
  switch(argc)
  {
    case 4: start  = strtoul(argv[3],NULL,16);
    case 3: dis.pc = strtoul(argv[2],NULL,16);
    case 2: fname  = argv[1]; break;
    default:
         fprintf(stderr,"usage: %s bin [load = 0 [start = 0]]\n",argv[0]);
         return EXIT_FAILURE;
  }
    
  fp = fopen(fname,"rb");
  if (fp == NULL)
  {
    perror(fname);
    return EXIT_FAILURE;
  }

  if (start != 0)
  {
    if (start < dis.pc)
    {
      fprintf(stderr,"starting address must be equal to or larger than load address\n");
      fclose(fp);
      return EXIT_FAILURE;
    }
    
    while(dis.pc != start)
    {
      fgetc(fp);
      dis.pc++;
    }
  }
  
  dis.user  = fp;
  dis.read  = dis_read;
  dis.fault = dis_fault;
  
  rc = mc6809dis_run(&dis,NULL);
  
  fclose(fp);
  
  if (rc != MC6809_DIS_DONE)
  {
    fprintf(stderr,"%s: %d\n",fname,rc);
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}

/*************************************************************************/

