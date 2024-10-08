#ifndef _NSEEL_GLUE_AARCH64_H_
#define _NSEEL_GLUE_AARCH64_H_

#define GLUE_MOD_IS_64

// x0=return value, first parm, x1-x2 parms (x3-x7 more params)
// x8 return struct?
// x9-x15 temporary
// x16-x17 = PLT, linker
// x18 reserved (TLS)
// x19-x28 callee-saved
// x19 = worktable
// x20 = ramtable
// x21 = consttab
// x22 = worktable ptr
// x23-x28 spare 
// x29 frame pointer
// x30 link register
// x31 SP/zero

// x0=p1
// x1=p2
// x2=p3

// d0 is return value for fp?
// d/v/f0-7 = arguments/results
// 8-15 callee saved
// 16-31 temporary

// v8-v15 spill registers
#define GLUE_MAX_SPILL_REGS 8
#define GLUE_SAVE_TO_SPILL_SIZE(x) (4)
#define GLUE_RESTORE_SPILL_TO_FPREG2_SIZE(x) (4)

static void GLUE_RESTORE_SPILL_TO_FPREG2(void *b, int ws)
{
  *(unsigned int *)b = 0x1e604101 + (ws<<5); // fmov d1, d8+ws
}
static void GLUE_SAVE_TO_SPILL(void *b, int ws)
{
  *(unsigned int *)b = 0x1e604008 + ws; // fmov d8+ws, d0
}


#define GLUE_HAS_FPREG2 1

static const unsigned int GLUE_COPY_FPSTACK_TO_FPREG2[] = { 0x1e604001 }; // fmov d1, d0
static unsigned int GLUE_POP_STACK_TO_FPREG2[] = {
  0xfc4107e1 // ldr d1, [sp], #16
};

#define GLUE_MAX_FPSTACK_SIZE 0 // no stack support
#define GLUE_MAX_JMPSIZE ((1<<20) - 1024) // maximum relative jump size

// endOfInstruction is end of jump with relative offset, offset passed in is offset from end of dest instruction.
// 0 = current instruction
static void GLUE_JMP_SET_OFFSET(void *endOfInstruction, int offset)
{
  unsigned int *a = (unsigned int*) endOfInstruction - 1;
  offset += 4;
  offset >>= 2; // as dwords
  if ((a[0] & 0xFC000000) == 0x14000000)
  {
    // NC b = 0x14 + 26 bit offset
    a[0] = 0x14000000 | (offset & 0x3FFFFFF);
  }
  else if ((a[0] & 0xFF000000) == 0x54000000)
  {
    // condb = 0x54 + 20 bit offset + 5 bit condition: 0=eq, 1=ne, b=lt, c=gt, d=le, a=ge
    a[0] = 0x54000000 | (a[0] & 0xF) | ((offset & 0x7FFFF) << 5);
  }
}

static const unsigned int GLUE_JMP_NC[] = { 0x14000000 };

static const unsigned int GLUE_JMP_IF_P1_Z[]=
{
  0x7100001f, // cmp w0, #0
  0x54000000, // b.eq
};
static const unsigned int GLUE_JMP_IF_P1_NZ[]=
{
  0x7100001f, // cmp w0, #0
  0x54000001, // b.ne
};

#define GLUE_MOV_PX_DIRECTVALUE_TOFPREG2_SIZE 16 // wr=-2, sets d1
#define GLUE_MOV_PX_DIRECTVALUE_SIZE 12
static void GLUE_MOV_PX_DIRECTVALUE_GEN(void *b, INT_PTR v, int wv) 
{   
  static const unsigned int tab[3] = {
    0xd2800000, // mov x0, #0000  (val<<5) | reg
    0xf2a00000, // movk x0, #0000, lsl 16 (val<<5) | reg
    0xf2c00000, // movk x0, #0000, lsl 32 (val<<5) | reg
  };
  // 0xABAAA, B is register, A are bits of word
  unsigned int *p=(unsigned int *)b;
  int wvo = wv;
  if (wv<0) wv=0;
  p[0] = tab[0] | wv | ((v&0xFFFF)<<5);
  p[1] = tab[1] | wv | (((v>>16)&0xFFFF)<<5);
  p[2] = tab[2] | wv | (((v>>32)&0xFFFF)<<5);
  if (wvo == -2) p[3] = 0xfd400001; // ldr d1, [x0]
}

const static unsigned int GLUE_FUNC_ENTER[2] = { 0xa9bf7bfd, 0x910003fd }; // stp x29, x30, [sp, #-16]! ; mov x29, sp
#define GLUE_FUNC_ENTER_SIZE 4
const static unsigned int GLUE_FUNC_LEAVE[1] = { 0 }; // let GLUE_RET pop
#define GLUE_FUNC_LEAVE_SIZE 0
const static unsigned int GLUE_RET[]={  0xa8c17bfd, 0xd65f03c0 }; // ldp x29,x30, [sp], #16 ; ret

static int GLUE_RESET_WTP(unsigned char *out, void *ptr)
{
  const static unsigned int GLUE_SET_WTP_FROM_R19 = 0xaa1303f6; // mov r22, r19
  if (out) memcpy(out,&GLUE_SET_WTP_FROM_R19,sizeof(GLUE_SET_WTP_FROM_R19));
  return 4;
}


const static unsigned int GLUE_PUSH_P1[1]={ 0xf81f0fe0  }; // str x0, [sp, #-16]!

#define GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE(offs) ((offs)>=32768 ? 8 : 4)
static void GLUE_STORE_P1_TO_STACK_AT_OFFS(void *b, int offs)
{
  if (offs >= 32768)
  {
    // add x1, sp, (offs/4096) lsl 12
    *(unsigned int *)b = 0x914003e1 + ((offs>>12)<<10);

    // str x0, [x1, #offs & 4095]
    offs &= 4095;
    offs <<= 10-3;
    offs &= 0x7FFC00;
    ((unsigned int *)b)[1] = 0xf9000020 + offs;
  }
  else
  {
    // str x0, [sp, #offs]
    offs <<= 10-3;
    offs &= 0x7FFC00;
    *(unsigned int *)b = 0xf90003e0 + offs;
  }
}

#define GLUE_MOVE_PX_STACKPTR_SIZE 4
static void GLUE_MOVE_PX_STACKPTR_GEN(void *b, int wv)
{
  // mov xX, sp
  *(unsigned int *)b = 0x910003e0 + wv;
}

#define GLUE_MOVE_STACK_SIZE 4
static void GLUE_MOVE_STACK(void *b, int amt)
{
  if (amt>=0) 
  {
    if (amt >= 4096)
      *(unsigned int*)b = 0x914003ff | (((amt+4095)>>12)<<10);
    else
      *(unsigned int*)b = 0x910003ff | (amt << 10);
  }
  else 
  {
    amt = -amt;
    if (amt >= 4096)
      *(unsigned int*)b = 0xd14003ff | (((amt+4095)>>12)<<10);
    else
      *(unsigned int*)b = 0xd10003ff | (amt << 10);
  }
}

#define GLUE_POP_PX_SIZE 4
static void GLUE_POP_PX(void *b, int wv)
{
  ((unsigned int *)b)[0] = 0xf84107e0 | wv; // ldr x, [sp], 16
}

#define GLUE_SET_PX_FROM_P1_SIZE 4
static void GLUE_SET_PX_FROM_P1(void *b, int wv)
{
  *(unsigned int *)b  = 0xaa0003e0 | wv;
}


static const unsigned int GLUE_PUSH_P1PTR_AS_VALUE[] = 
{ 
  0xfd400007, // ldr d7, [x0]
  0xfc1f0fe7, // str d7, [sp, #-16]!
};

static int GLUE_POP_VALUE_TO_ADDR(unsigned char *buf, void *destptr)
{    
  if (buf)
  {
    unsigned int *bufptr = (unsigned int *)buf;
    *bufptr++ = 0xfc4107e7; // ldr d7, [sp], #16
    GLUE_MOV_PX_DIRECTVALUE_GEN(bufptr, (INT_PTR)destptr,0);
    bufptr += GLUE_MOV_PX_DIRECTVALUE_SIZE/4;
    *bufptr++ = 0xfd000007; // str d7, [x0]
  }
  return 2*4 + GLUE_MOV_PX_DIRECTVALUE_SIZE;
}

static int GLUE_COPY_VALUE_AT_P1_TO_PTR(unsigned char *buf, void *destptr)
{    
  if (buf)
  {
    unsigned int *bufptr = (unsigned int *)buf;
    *bufptr++ = 0xfd400007; // ldr d7, [x0]
    GLUE_MOV_PX_DIRECTVALUE_GEN(bufptr, (INT_PTR)destptr,0);
    bufptr += GLUE_MOV_PX_DIRECTVALUE_SIZE/4;
    *bufptr++ = 0xfd000007; // str d7, [x0]
  }
  return 2*4 + GLUE_MOV_PX_DIRECTVALUE_SIZE;
}


#define GLUE_CALL_CODE(bp, cp, rt) do { \
  GLUE_SCR_TYPE f; \
  static const double consttab[] = {  \
    NSEEL_CLOSEFACTOR, \
    0.0, \
    1.0, \
    -1.0, \
    -0.5, /* for invsqrt */ \
    1.5, \
  }; \
  if (!(h->compile_flags&NSEEL_CODE_COMPILE_FLAG_NOFPSTATE) && \
      !((f=glue_getscr())&(1<<24))) {  \
    glue_setscr(f|(1<<24)); \
    eel_callcode64(bp, cp, rt, (void *)consttab); \
    glue_setscr(f); \
  } else eel_callcode64(bp, cp, rt, (void *)consttab);\
  } while(0)

#ifndef _MSC_VER
static void eel_callcode64(INT_PTR bp, INT_PTR cp, INT_PTR rt, void *consttab)
{
  __asm__(
          "mov x1, %2\n"
          "mov x2, %3\n"
          "mov x3, %1\n"
          "mov x0, %0\n"
          "stp x29, x30, [sp, #-64]!\n"
          "stp x18, x20, [sp, 16]\n"
          "stp x21, x19, [sp, 32]\n"
          "stp x22, x23, [sp, 48]\n"
          "mov x29, sp\n"
          "mov x19, x3\n"
          "mov x20, x1\n"
          "mov x21, x2\n"
          "blr x0\n"
          "ldp x29, x30, [sp], 16\n"
          "ldp x18, x20, [sp], 16\n"
          "ldp x21, x19, [sp], 16\n"
          "ldp x22, x23, [sp], 16\n"
            ::"r" (cp), "r" (bp), "r" (rt), "r" (consttab) :"x0","x1","x2","x3","x4","x5","x6","x7",
                                                            "x8","x9","x10","x11","x12","x13","x14","x15",
							    "v8","v9","v10","v11","v12","v13","v14","v15");
             
};
#else
void eel_callcode64(INT_PTR bp, INT_PTR cp, INT_PTR rt, void *consttab);
#endif

static unsigned char *EEL_GLUE_set_immediate(void *_p, INT_PTR newv)
{
  unsigned int *p=(unsigned int *)_p;
  WDL_ASSERT(!(newv>>48));
//    0xd2800000, // mov x0, #0000  (val<<5) | reg
 //   0xf2a00000, // movk x0, #0000, lsl 16 (val<<5) | reg
  //  0xf2c00000, // movk x0, #0000, lsl 32 (val<<5) | reg
  while (((p[0]>>5)&0xffff)!=0xdead ||
         ((p[1]>>5)&0xffff)!=0xbeef ||
         ((p[2]>>5)&0xffff)!=0xbeef) p++;

  p[0] = (p[0] & 0xFFE0001F) | ((newv&0xffff)<<5);
  p[1] = (p[1] & 0xFFE0001F) | (((newv>>16)&0xffff)<<5);
  p[2] = (p[2] & 0xFFE0001F) | (((newv>>32)&0xffff)<<5);

  return (unsigned char *)(p+2);
}

#define GLUE_SET_PX_FROM_WTP_SIZE sizeof(int)
static void GLUE_SET_PX_FROM_WTP(void *b, int wv)
{
  *(unsigned int *)b = 0xaa1603e0 + wv; // mov x, x22
}

static int GLUE_POP_FPSTACK_TO_PTR(unsigned char *buf, void *destptr)
{
  if (buf)
  {
    unsigned int *bufptr = (unsigned int *)buf;
    GLUE_MOV_PX_DIRECTVALUE_GEN(bufptr, (INT_PTR)destptr,0);
    bufptr += GLUE_MOV_PX_DIRECTVALUE_SIZE/4;

    *bufptr++ = 0xfd000000; // str d0, [x0] 
  }
  return GLUE_MOV_PX_DIRECTVALUE_SIZE + sizeof(int);
}

#define GLUE_POP_FPSTACK_SIZE 0
static const unsigned int GLUE_POP_FPSTACK[1] = { 0 }; // no need to pop, not a stack

static const unsigned int GLUE_POP_FPSTACK_TOSTACK[] = {
  0xfc1f0fe0, // str d0, [sp, #-16]!

};

static const unsigned int GLUE_POP_FPSTACK_TO_WTP[] = { 
  0xfc0086c0, // str d0, [x22], #8
};

#define GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE 4
static void GLUE_PUSH_VAL_AT_PX_TO_FPSTACK(void *b, int wv)
{
  *(unsigned int *)b = 0xfd400000 + (wv<<5); // ldr d0, [xX]
}

#define GLUE_POP_FPSTACK_TO_WTP_TO_PX_SIZE (sizeof(GLUE_POP_FPSTACK_TO_WTP) + GLUE_SET_PX_FROM_WTP_SIZE)
static void GLUE_POP_FPSTACK_TO_WTP_TO_PX(unsigned char *buf, int wv)
{
  GLUE_SET_PX_FROM_WTP(buf,wv); 
  memcpy(buf + GLUE_SET_PX_FROM_WTP_SIZE,GLUE_POP_FPSTACK_TO_WTP,sizeof(GLUE_POP_FPSTACK_TO_WTP));
};

static const unsigned int GLUE_SET_P1_Z[] =  { 0x52800000 }; // mov w0, #0
static const unsigned int GLUE_SET_P1_NZ[] = { 0x52800020 }; // mov w0, #1


static void *GLUE_realAddress(void *fn, int *size)
{
  while ((*(int*)fn & 0xFC000000) == 0x14000000)
  {
    int offset = (*(int*)fn & 0x3FFFFFF);
    if (offset & 0x2000000)
      offset |= 0xFC000000;

    fn = (int*)fn + offset;
  }
  static const unsigned int sig[] = {
#ifndef _MSC_VER
    0xaa0003e0,
#endif
    0xaa0103e1,
#ifndef _MSC_VER
    0xaa0203e2
#endif
  };
  unsigned char *p = (unsigned char *)fn;

  while (memcmp(p,sig,sizeof(sig))) p+=4;
  p+=sizeof(sig);
  fn = p;

  while (memcmp(p,sig,sizeof(sig))) p+=4;
  *size = (int)(p - (unsigned char *)fn);
  return fn;
}



#ifndef _MSC_VER
#define GLUE_SCR_TYPE unsigned long
static unsigned long __attribute__((unused)) glue_getscr()
{
  unsigned long rv;
  asm volatile ( "mrs %0, fpcr" : "=r" (rv));
  return rv;
}
static void  __attribute__((unused)) glue_setscr(unsigned long v)
{
  asm volatile ( "msr fpcr, %0" :: "r"(v));
}
#else
#define GLUE_SCR_TYPE unsigned long long
GLUE_SCR_TYPE glue_getscr();
void glue_setscr(unsigned long long);
#endif

void eel_enterfp(int _s[2]) 
{
  GLUE_SCR_TYPE *s = (GLUE_SCR_TYPE*)_s;
  s[0] = glue_getscr();
  glue_setscr(s[0] | (1<<24));
}
void eel_leavefp(int _s[2]) 
{
  const GLUE_SCR_TYPE *s = (GLUE_SCR_TYPE*)_s;
  glue_setscr(s[0]);
}

#define GLUE_HAS_FUSE 1
static int GLUE_FUSE(compileContext *ctx, unsigned char *code, int left_size, int right_size, int fuse_flags, int spill_reg)
{
  if (left_size>=4 && right_size == 4)
  {
    unsigned int instr = ((unsigned int *)code)[-1];
    if (spill_reg >= 0 && (instr & 0xfffffc1f) == 0x1e604001) // fmov d1, dX
    {
      const int src_reg = (instr>>5)&0x1f;
      if (src_reg == spill_reg + 8)
      {
        instr = ((unsigned int *)code)[0];
        if ((instr & 0xffffcfff) == 0x1e600820)
        {
          ((unsigned int *)code)[-1] = instr + ((src_reg-1) << 5);
          return -4;
        }
      }
    }
  }
  return 0;
}

#ifdef _M_ARM64EC
#define DEF_F1(n) static double eel_##n(double a) { return n(a); }
#define DEF_F2(n) static double eel_##n(double a, double b) { return n(a,b); }
DEF_F1(cos)
#define cos eel_cos
DEF_F1(sin)
#define sin eel_sin
DEF_F1(tan)
#define tan eel_tan
DEF_F1(log)
#define log eel_log
DEF_F1(log10)
#define log10 eel_log10
DEF_F1(acos)
#define acos eel_acos
DEF_F1(asin)
#define asin eel_asin
DEF_F1(atan)
#define atan eel_atan
DEF_F1(exp)
#define exp eel_exp
DEF_F2(pow)
#define pow eel_pow
DEF_F2(atan2)
#define atan2 eel_atan2
// ceil and floor will be wrapped by defs in nseel-compiler.c

#pragma comment(lib,"onecore.lib")
#endif


#endif
