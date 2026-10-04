/********************************
 *                              *
 *      HD6309 cross assembler  *
 *                              *
 ********************************/
#ifndef AS63_H_INCLUDED
#define AS63_H_INCLUDED

#include <stdio.h>
#include <string.h>

#if __STDC_VERSION__ >= 199901L || _MSC_VER >= 1600 || __WATCOMC__ >= 1200
#include <stdint.h>
#else
typedef signed char     int8_t;     /* 1-byte signed integer type */
typedef unsigned char   uint8_t;    /* 1-byte unsigned integer type */
typedef short           int16_t;    /* 2-byte signed integer type */
typedef unsigned short  uint16_t;   /* 2-byte unsigned integer type */
#ifdef _WIN32
typedef __int64          intmax_t;
typedef unsigned __int64 uintmax_t;
#else
typedef long            intmax_t;
typedef unsigned long   uintmax_t;
#endif
#endif

#ifdef _MSC_VER
#define strcasecmp      _stricmp
#define strncasecmp     _strnicmp
#elif defined _WIN32 || defined __DOS__
#define strcasecmp      stricmp
#define strncasecmp     strnicmp
#endif

#define STDERR          stderr
#define toXDigit(c)     (isdigit((uint8_t)(c)) ? (c - '0') : (toupper((uint8_t)(c)) - 'A' + 10))
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
#define MNEMOSIZE       10    /* maximum number of mnemonic characters       */
#define MODNAMSZ        29    /* maximum number of characters in module name */

typedef intmax_t        val_t;  /* use int/long for constant arithmetic      */
typedef uintmax_t       uval_t; /* unsigned val_t                            */

#define OPT_OA_FILE           /* enable -a option support                    */
#define OPT_FBAS              /* enable -k (FBASIC machine-code file output) */

#ifdef OPT_OA_FILE
# define OA_MAX         500
#endif
#define INCLUDIR        "."

#ifdef SMALL_HOST
#define MAXCHAR         1024  /* maximum number of characters per input line */
#define MAXLABEL        128   /* memory allocation block size for name table (nodes) */
#define FNAMESZ         127   /* maximum file name (path list) length        */
#define MAXLIB          16    /* maximum include nesting depth               */
#define LBLSIZE         21    /* maximum label name length                   */
#define GCO_MAX         40    /* maximum if nesting depth                    */
#define GP1_MAX         100   /* maximum number of ifp1 uses                 */
#define EXTRA_INCDIRS   8
#define MACRO_BUF_SIZE  (0x8000)
#else
#define MAXCHAR         16384 /* maximum number of characters per input line */
#define MAXLABEL        1024  /* memory allocation block size for name table (nodes) */
#define FNAMESZ         16384 /* maximum file name (path list) length        */
#define MAXLIB          256   /* maximum include nesting depth               */
#define LBLSIZE         128   /* maximum label name length                   */
#define GCO_MAX         256   /* maximum if nesting depth                    */
#define GP1_MAX         1024  /* maximum number of ifp1 uses                 */
#define EXTRA_INCDIRS   64
#define MACRO_BUF_SIZE  (16UL * 1024 * 1024)
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
#define CO_IFD          11
#define CO_IFND         12
#define CO_IFC          13
#define CO_IFNC         14
#define CO_IFPRAGMA     15
#define CO_IFB          16
#define CO_IFNB         17
#define CO_IFMACROD     18
#define CO_IFMACROND    19

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

/* Pseudo-op prefix is the element size or mode; opcode holds behavior flags. */
#define BLOCK_FILL      0x01  /* accept an optional fill value */
#define BLOCK_CHECK     0x02  /* validate counts, values and address space */
#define BLOCK_REVERSED  0x04  /* fill value precedes count */
#define EQU_RESOLVED    0x01  /* SET requires a resolved expression */
#define LIST_ABSOLUTE   0x01  /* LIST/NOLIST set state; OPT changes its depth */
#define RS_RESET        0x01
#define RS_SET          0x02

/* opcode table */
#define OPR_UNDOC6809  0x02
#define OPR_M6800      0x04

typedef struct optbl_t {
        char const* mnemonic;
        uint8_t prefix;
        uint8_t opcode;
        uint8_t option;
        void  (*process)(void);
} OPTBL_T;

#define COMPAT_AS63     0
#define COMPAT_LWASM    1
#define COMPAT_VASM     2

/* Shared state for macro input expansion. */
extern FILE *          gSrcFp;
extern uint8_t         gCompatMode;
extern uint8_t         gUpLo_f;
extern uint8_t *       gLinPtr;
extern uint8_t         gAllMacroParams;
extern char            gLineBuf[MAXCHAR + 2];
extern int             gCo_sp;
extern int             gCoStk[GCO_MAX + 1];
extern int             gFile_sp;
extern int             gSrcLine;

extern OPTBL_T const gOpTab[];

void
    none(void), load(void), load2(void),    store(void),
    ccr(void),  bitTransfer(void), lea(void),  memory(void),   transfer(void),
    pshs(void), puls(void), pshu(void),     pulu(void),
    tfm(void),  load4(void),immemory(void),
    mod(void),  emod(void), branch(void),   lbranch(void),
    equ(void),              rmb(void),      os9svc(void),
    rzb(void),  fcb(void),  fdb(void),      fqb(void),
    fcc(void),  fcs(void), fcn(void),  org(void),      setdp(void),
    vsct(void), psct(void), csct(void),     endsct(void),
    opt(void), pragmaDirective(void),
    alignData(void), rsOffset(void),        argumentOffsets(void),
    ignoreOperand(void), commentBlock(void), failDirective(void), symbolDirective(void),
    relativeData(void), relativeOrg(void), offsetSection(void),
    printText(void), printValue(void),
    library(void), incbin(void), incdir(void),
    undoc_imm8(void), undoc_imm16(void), undoc_flag(void),
    opeq(void), none_wq(void),
    oped(void), none_d(void),
    mnm6800(void),       mnm68hc11(void),  hc11BitOp(void),
    hc11BitBranch(void), hc11MinMax(void), hc11Emuls(void),
    endop(void);

void*   mallocE(size_t);
void    error(char const *);
void    errPrg(char const *);
void    skipSpace(void);
void    initLine(void);
void    putLine(void);
void    clearAddress(void);
void    defLabel(char const *, uint8_t, uint8_t);
uint8_t getLabel(char *);
uint8_t *getLine(void);
int     isSymbl(int);
int     isCommentChar(int);
int     popFile(void);
val_t   invExpr(void);
uint8_t *macroReadLine(uint8_t *, size_t);
void    macroReset(void);
void    macroFinish(void);
void    macroLocalLabel(char *);
int     macroControl(char const *, uint8_t);
int     macroInvoke(char const *);
int     macroDefined(char const *);
int     macroValue(char const *, val_t *);
int     macroSetValue(char const *, val_t);
int     macroNumericLocal(uint8_t const *);
int     macroConditionalFloor(void);
void    macroScopeBoundary(void);
int     pragmaEnabled(char const *);
char const *macroListingSource(void);

#endif
