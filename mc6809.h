
#ifndef MC6809_H
#define MC6809_H

#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>

#ifndef __GNUC__
#  define __attribute__(x)
#endif

#if defined(__i386)
#  define M 1
#  define L 0
#else
#  warning You need to define the byte order
#endif

/************************************************************************/

#define MC6809_VECTOR_RSVP	0xFFF0
#define MC6809_VECTOR_SWI3	0xFFF2
#define MC6809_VECTOR_SWI2	0xFFF4
#define MC6809_VECTOR_FIRQ	0xFFF6
#define MC6809_VECTOR_IRQ	0xFFF8
#define MC6809_VECTOR_SWI	0xFFFA
#define MC6809_VECTOR_NMI	0xFFFC
#define MC6809_VECTOR_RESET	0xFFFE

/************************************************************************/

typedef enum
{
  MC6809_FAULT_INTERNAL_ERROR = 1,
  MC6809_FAULT_INSTRUCTION,
  MC6809_FAULT_ADDRESS_MODE,
  MC6809_FAULT_EXG,
  MC6809_FAULT_TFR,
  MC6809_FAULT_user
} mc6809fault__t;

typedef uint8_t  mc6809byte__t;
typedef uint16_t mc6809addr__t;
typedef union
{
  mc6809byte__t b[2];
  mc6809addr__t w;
} mc6809word__t;

typedef struct mc6809
{
  mc6809word__t pc;
  mc6809word__t index[4];
  mc6809word__t d;
  mc6809byte__t dp;
  struct
  {
    bool e;
    bool f;
    bool h;
    bool i;
    bool n;
    bool z;
    bool v;
    bool c;
  } cc;
  
  unsigned long cycles;
  mc6809addr__t instpc;
  mc6809byte__t inst;
  bool          nmi_armed;
  bool          nmi;
  bool          firq;
  bool          irq;
  bool          cwai;
  bool          sync;
  bool          page2;
  bool          page3;
  jmp_buf       err;
  
  void           *user;
  mc6809byte__t (*read) (struct mc6809 *,mc6809addr__t,bool);
  void          (*write)(struct mc6809 *,mc6809addr__t,mc6809byte__t);
  void          (*fault)(struct mc6809 *,mc6809fault__t);

  /*----------------------------------------------------------------------
  ; WARNING: Below be dragons!
  ;
  ; Below here, nothing can be said.  These fields are used internally by
  ; the MC6809 emulation engine, with results changing between even minor
  ; versions.  Don't rely upon any values you may see here when using this
  ; thing.  You have been warned.
  ;-----------------------------------------------------------------------*/
  
  mc6809word__t addr;
  mc6809byte__t data;
  mc6809word__t d16;
} mc6809__t;

#define X	index[0]
#define Y	index[1]
#define U	index[2]
#define S	index[3]
#define A	d.b[M]
#define B	d.b[L]

/**********************************************************************/

void	mc6809_reset	(mc6809__t *const) __attribute__((nonnull));
int	mc6809_run	(mc6809__t *const) __attribute__((nonnull));
int	mc6809_step	(mc6809__t *const) __attribute__((nonnull));

/**********************************************************************/

#endif