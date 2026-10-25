/* Relocatable expressions, sections and LWOBJ16 object output. */
#ifndef OBJOUT_H_INCLUDED
#define OBJOUT_H_INCLUDED

#include "as63.h"

/* Expression operators use LWOBJ16 numbering; SHL, SHR and NOT are rewritten. */
#define REL_PLUS        1
#define REL_MINUS       2
#define REL_TIMES       3
#define REL_DIVIDE      4
#define REL_MOD         5
#define REL_BWAND       7
#define REL_BWOR        8
#define REL_BWXOR       9
#define REL_AND         10
#define REL_OR          11
#define REL_NEG         12
#define REL_COM         13
#define REL_EQ          15
#define REL_NE          16
#define REL_LT          17
#define REL_LE          18
#define REL_GT          19
#define REL_GE          20
#define REL_SHL         0x40
#define REL_SHR         0x41
#define REL_NOT         0x42

#define REL_TERMS       4     /* section bases and externals in a linear value */
#define REL_CODE        160   /* postfix bytes of a non-linear value         */

/* A value is constant + sum(scale * symbol); constants live in val_t. */
typedef struct rel_value {
    uint8_t     terms;
    uint8_t     complex;      /* code holds the complete expression */
    uint8_t     length;
    int16_t     symbol[REL_TERMS];
    long        scale[REL_TERMS];
    uint8_t     code[REL_CODE];
} REL_VALUE;

#define SECTION_BSS         0x01
#define SECTION_CONSTANT    0x02

extern uint8_t  gRelActive;

/* Expression evaluator hooks. */
void    relBegin(void);
void    relFinish(void);
int     relDepth(void);
void    relSettle(int depth);
void    relPushValue(REL_VALUE const *value);
void    relPushHere(void);
void    relPushExternal(char const *name);
void    relUnary(int op, val_t value);
void    relBinary(int op, val_t left, val_t right);

/* Pending relocatable result of the last expression. */
int     relPending(void);
void    relDiscard(void);
void    relPcr(void);
void    relocField(int size, val_t value);
void    relEndLine(void);
REL_VALUE const *relStore(REL_VALUE const *old);
REL_VALUE const *relReplace(REL_VALUE const *old, REL_VALUE const *value);
REL_VALUE const *relLabelBase(void);
unsigned long relSignature(REL_VALUE const *v);

/* Sections; option is 1:bss 2:!bss 3:constant 4:!constant. */
void    objInitPass(void);
int     objSection(void);
int     objSectionFlags(void);
uint16_t objEnterSection(char const *name, int option, uint16_t lc);
uint16_t objLeaveSection(uint16_t lc);
void    objPutByte(uint16_t offset, uint8_t b);
void    objTrack(uint16_t lc);

/* Symbols and object file. */
void    objLocalSymbol(char const *name, int group, REL_VALUE const *rel, int section, val_t value);
void    objExportSymbol(char const *name, REL_VALUE const *rel, int section, val_t value);
void    objWrite(FILE *fp);

#endif
