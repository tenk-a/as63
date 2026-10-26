/* Internal interface between assembly control and code generation. */
#ifndef GENCODE_H_INCLUDED
#define GENCODE_H_INCLUDED

#include "as63.h"

/* Assembly state and operand settings owned by as63.c. */
extern uint8_t          pragmaEscapes;
extern char             gModName[MODNAMSZ + 1];
extern OPTBL_T const *  gOprPtr;
extern int              gPass;
extern uint16_t         gDp;
extern uint16_t         gLc;
extern uint16_t         gLinLc;
extern uint8_t          gValid_f;
extern uint8_t          gOs9_f;
extern uint8_t          gByte_f;
extern uint8_t          gWord_f;
extern uint8_t          gIdxOfs_f;
extern uint8_t          gM6809_f;
extern uint8_t          gM6800_f;
extern uint8_t          gUndoc_f;
extern uint8_t          pragmaPcAsPcr;
extern uint8_t          pragmaIndex0;
extern uint8_t          pragmaForwardMax;
extern uint8_t          pragmaAutoBranch;
extern uint8_t          gOptLevel;
extern uint8_t          pragmaOperandSizeWarning;
extern uint8_t          pragmaQrts;
extern int              branchChanges;
extern int              addressRegion;
extern int              expressionForward;
extern int              expressionFixedForward;
extern int              expressionLiteralZero;
extern int              lineCode;
extern int              wordCharacter;

/* Output state owned by gencode.c. */
extern uint8_t          gCSectSw;
extern uint16_t         gCSectBase;
extern uint8_t          gOrgSFmt_f;
extern uint8_t          gOrgPad;
extern uint16_t         gStartAddr;
extern FILE *           gObjFp;
extern uint16_t         gObjCnt;
extern uint16_t         gEntryAddr;
extern uint8_t          gAddressOverride;
extern uint16_t         gOptionStartAddr;
extern uint16_t         gOptionEntryAddr;
extern out_fmt_t        gOutFmt;
extern int              gObjBufSz;
extern uint8_t          gRmb_f;
extern int              gRmb_sp;
extern bin_fmt_t        gBinFmt;

/* Per-line relaxation state owned by as63.c. */
typedef struct LINE_STATE {
    int     address;
    int     size;
    int     branchSize;
    uint8_t branchLong;
    uint8_t pcrSize;
    uint8_t pcrLong;
    uint8_t expandedPcrSize[2];
    uint8_t expandedPcrLong[2];
    uint8_t skipFailed;
    uint8_t branchSet;
} LINE_STATE;

/* Parsing, diagnostics, relaxation and listing services from as63.c. */
void warning(char const *s);
int  isSymbl3(int c);
int  checkChar(uint8_t c);
int  checkCh_e(uint8_t c);
int  pragmaIndexConfigured(void);
val_t   expression(void);
uint8_t bytExpr(void);
LINE_STATE *lineState(void);
void requestRelaxation(void);
FILE *openSearch(char const *name, char const *mode, int searchOnly);
int  fileOperand(char *name, int size, int *searchOnly);
void labelValue(val_t value);
int  nextComma(void);
void printAddress(int a);
int  isKanji(int c);
void printByte(int b, int c);
void printWord(int w, int c);

/* Pass/line initialization and output services from gencode.c. */
void initCodePass(void);
void initCodeLine(void);
void beginCodeOutput(uint16_t size);
void endCodeOutput(void);
void endCodeSection(void);
int  offsetBlockActive(void);
void reserveBytes(val_t count, int checked);
void termObj(void);
void oa_putStr(char const *s, int n);

#endif
