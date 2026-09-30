/********************************
 *                              *
 *      HD6309 cross assembler  *
 *                              *
 ********************************/
#ifndef AS63_H_INCLUDED
#define AS63_H_INCLUDED

#include <stdio.h>
#include <string.h>

#ifdef EXT
 #define EXTERN
#else
 #define EXTERN extern
#endif

#if __STDC_VERSION__ >= 199901L || _MSC_VER >= 1600
#include <stdint.h>
#else
typedef unsigned char   uint8_t;    /* 1-byte unsigned integer type */
typedef unsigned short  uint16_t;   /* 2-byte unsigned integer type */
typedef short           int16_t;    /* 2-byte signed integer type */
#endif

#define __(x)           x

#define STDERR          stderr
#define toXDigit(c)     (isdigit(c) ? (c - '0') : (toupper(c) - 'A' + 10))
#define e_puts(s)       fprintf(STDERR,"%s", s)
#ifndef OS9
#define stpcpy(d,s)     (strcpy((d),(s)),(d)+strlen(d))
#endif

#ifdef  DEBUG
 #define DEBMSGF(x)     if (gDebug_f) (fprintf x)
#else
 #define DEBMSGF(x)
#endif

/*---------------------------------------------------------------------------*/

#define LINEHEAD        24    /* source line display position in listing     */
#define OBJSIZE         32    /* obj output buffer size (bytes): one S-record line */
#define MNEMOSIZE       8     /* maximum number of mnemonic characters       */
#define MODNAMSZ        29    /* maximum number of characters in module name */

typedef int  val_t;           /* int<->long: use int/long for constant arithmetic */

#define OPT_OA_FILE           /* enable -a option support                    */
#define OPT_OPTIMIZE          /* enable optimization (-y)                    */
#define OPT_FBAS              /* enable -k (FBASIC machine-code file output) */
#define OPT_FLEX              /* enable -x (FLEX executable file output)     */
#define OPT_EXT_INST          /* enable extended instructions                */
#define OPT_UNDOC             /* enable 6809 undocument instructions         */
#define OPT_M6800             /* enable 6800 family mnemonic                 */

#ifdef OPT_OA_FILE
# define OA_MAX         500
#endif
#define INCLUDIR        "."

#ifdef SMALL_HOST
#define MAXCHAR         1024  /* maximum number of characters per input line */
#define MAXLABEL        256   /* memory allocation block size for name table (nodes) */
#define FNAMESZ         127   /* maximum file name (path list) length        */
#define MAXLIB          16    /* maximum include nesting depth               */
#define LBLSIZE         21    /* maximum label name length                   */
#define MAXOPTIM        3000  /* maximum number of long branches convertible to short */
#define GCO_MAX         40    /* maximum if nesting depth                    */
#define GP1_MAX         100   /* maximum number of ifp1 uses                 */
#else
#define MAXCHAR         16384 /* maximum number of characters per input line */
#define MAXLABEL        1024  /* memory allocation block size for name table (nodes) */
#define FNAMESZ         16384 /* maximum file name (path list) length        */
#define MAXLIB          256   /* maximum include nesting depth               */
#define LBLSIZE         128   /* maximum label name length                   */
#define MAXOPTIM        8192  /* maximum number of long branches convertible to short */
#define GCO_MAX         256   /* maximum if nesting depth                    */
#define GP1_MAX         1024  /* maximum number of ifp1 uses                 */
#endif

/* register notation */
#define NONE            0
#define CC              0x01
#define A               0x02
#define B               0x04
#define D               0x06
#define DP              0x08
#define X               0x10
#define Y               0x20
#define U               0x40
#define S               0x80
#define PC              0x100
#define PCR             0x200
#define E               0x400
#define F               0x800
#define W               0xC00
#define V               0x1000
#define N               0x2000
#define INDEXREG        (X|Y|U|S)
#define OFFSETRG        (A|B|E|F)
#define ALLREG          (CC|A|B|D|DP|X|Y|U|S|PC)
#define X63REG          (E|F|W|V|N)

/* addressing mode */
#define IMMEDIATE       0x01
#define IMMEDIATE2      0x02
#define DIRECT          0x04
#define INDEX           0x08
#define EXTEND          0x10
#define LOAD            (IMMEDIATE|DIRECT|INDEX|EXTEND)
#define LOAD2           (IMMEDIATE2|DIRECT|INDEX|EXTEND)
#define STORE           (DIRECT|INDEX|EXTEND)
#define MEMORY          (DIRECT|INDEX|EXTEND)

/* addressing mode variation group */
#define GROUP0          0
#define GROUP1          1
#define GROUP2          2
#define GROUP3          3       /* tfm */

/* mode offset */
#define NO_MODE         0
#define IMMEDIATE_MODE  0
#define DIRECT_MODE     1
#define INDEX_MODE      2
#define EXTEND_MODE     3


/* conditional assembler */
#define CO_ELSE         1
#define CO_ENDC         2
#define CO_IFP1         3
#define CO_IF           4
#define CO_IFN          5
#define CO_ELIF         6
#define CO_IFGE         7
#define CO_IFGT         8
#define CO_IFLE         9
#define CO_IFLT         10

/* none_wq */
#define WQ_TSTQ         0
#define WQ_CLRQ         (1*4)
#define WQ_COMQ         (2*4)
#define WQ_LSRQ         (3*4)
#define WQ_ASRQ         (4*4)
#define WQ_RORQ         (5*4)
#define WQ_ROLQ         (6*4)
#define WQ_LSLW         (7*4)
#define WQ_NEGW         (8*4)
#define WQ_ASRW         (9*4)
#define WQ_LSLQ         (10*4+2)
#define WQ_INCQ         (12*4)
#define WQ_DECQ         (13*4+2)
#define WQ_NEGQ         (15*4+2)

/* output file type */
#define OB_BIN          1
#define OB_SFMT         2
/*#define OBJ_ROF */
#define OB_ASM          4

/* opcode table */
#ifdef OPT_UNDOC
#define OPR_UNDOC6809  0x02
#endif
#ifdef OPT_M6800
#define OPR_M6800      0x04
#endif

typedef struct optbl_t {
        char   *mnemonic;
        uint8_t prefix;
        uint8_t opcode;
        uint8_t option;
        void  (*process)(void);
        struct optbl_t *nl;
} OPTBL_T;

/* label table */
typedef struct lbltbl_t {
        int     line;
        val_t   value;
        struct lbltbl_t *left, *right;
        char    flg;           /* 0:used  1:EQU  2:SET */
        uint8_t grp;
        char    name[LBLSIZE + 1];
} LBLTBL_T;


/*-------------------------------- var -------------------------------------*/
#ifdef DEBUG
 EXTERN uint8_t gDebug_f;
#endif

/* assembly */
EXTERN FILE    *gSrcFp;
EXTERN OPTBL_T *gOprPtr;
EXTERN int      gErrors , gPass;
EXTERN int      gImVal  , gIndirect;
EXTERN uint16_t gDp;
EXTERN uint16_t gLc     , gLinLc , gObjLc;
EXTERN uint8_t  gValid_f, gEOF_f;
EXTERN uint8_t  gOs9_f  , gOrg_f , gOrgSFmt_f;
EXTERN uint8_t  gByte_f , gWord_f, gIdxOfs_f;
EXTERN char     gModName[MODNAMSZ+1];
EXTERN uint8_t  gM6809_f;
#ifdef OPT_M6800
 EXTERN uint8_t gM6800_f;
#endif
#ifdef OPT_UNDOC
 EXTERN uint8_t gUndoc_f;
#endif

/* object output */
EXTERN FILE    *gObjFp;
EXTERN uint16_t gObjSiz,    gObjCnt;
EXTERN uint16_t gStartAddr, gEntryAddr;
EXTERN uint8_t  gObjct;
EXTERN int      gObjPos;
EXTERN int      gObjBufSz;
EXTERN uint8_t  gObjBuf[OBJSIZE];
EXTERN uint8_t  gCrcBuf[3];
EXTERN int      gRmb_sp,    gRmb_f;
#ifdef OPT_FBAS
 EXTERN uint8_t gFBasic_f;
#endif
#ifdef OPT_FLEX
 EXTERN uint8_t gFlex_f;
#endif

/* labels */
EXTERN LBLTBL_T *gLblPtr;
EXTERN int      gLabels,  gLineNo;
EXTERN uint16_t gCSectBase;
EXTERN uint8_t  gGrp,     gCSectSw;
EXTERN uint8_t  gUpLo_f,  gPSect_f;
EXTERN uint8_t  gSjis_f;


/* listing and message display */
EXTERN char   *gCmdName;
EXTERN int     gList;
EXTERN FILE   *gLstFp;
EXTERN char   *gLinPtr;
EXTERN uint8_t gVerbos_f;
EXTERN char    gLineBuf[MAXCHAR+2];
EXTERN FILE   *gErrFp;
EXTERN char   *gErrFName;

#ifdef OPT_OPTIMIZE
 /* for optimization (long->short branch) */
 EXTERN int    *gOptStk, gOpt_sp, gOptChg, gOptCount;
 EXTERN uint8_t gOpt_f;
#endif

/* manage 'if' (conditional assembly) */
EXTERN int   gCo_sp;
EXTERN int   gCoStk[GCO_MAX+1];
EXTERN int   gP1_sp;
EXTERN int   gP1Stk[GP1_MAX+1];

/* use for library inclusion */
EXTERN FILE *gFileStk[MAXLIB];
EXTERN int   gFile_sp;
EXTERN char  gSrcFName[FNAMESZ+1];
EXTERN int   gSrcLine;
EXTERN struct FILSTK2_tag {
            int  srcline;
            char srcname[FNAMESZ+1];
        } gFilStk2[MAXLIB];
#ifdef INCLUDIR
 EXTERN char *gIncDirName;
#endif

#ifdef OPT_OA_FILE  /* -a option */
 typedef struct {
     int     ll;
     uint8_t nn;
 } OATBL_T;
 EXTERN OATBL_T *gOAStk;
 EXTERN uint8_t  gOAchk_f;
 EXTERN int      gOA_sp;
 void oa_putStr(char *, int);
#endif

extern OPTBL_T gOpTab[];


/*-- Function --*/
void
    none(void), load(void), load2(void),    store(void),
    ccr(void),  lea(void),  memory(void),   transfer(void),
    pshs(void), puls(void), pshu(void),     pulu(void),
    mod(void),  emod(void), branch(void),   lbranch(void),
    equ(void),  set(void),  rmb(void),      os9svc(void),
    rzb(void),  fcb(void),  fdb(void),      flb(void),
    library(void),
    fcc(void),  fcs(void),  org(void),      setdp(void),
    vsct(void), psct(void), csct(void),     endsct(void),
    opt(void),  nam(void),  page(void),     spc(void),
    tfm(void),  load4(void),immemory(void),
 #ifdef OPT_UNDOC
    undoc_imm8(void), undoc_imm16(void), undoc_flag(void),
 #endif
 #ifdef OPT_EXT_INST
    opeq(void), none_wq(void),
    oped(void), none_d(void),
 #endif
 #ifdef OPT_M6800
  mnm6800(void), mnm68hc11(void), hc11BitOp(void), hc11BitBranch(void), hc11MinMax(void), hc11Emuls(void),
 #endif
    endop(void);

#endif
