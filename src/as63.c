/************************************
 *                                  *
 *  HD6309 cross assembler          *
 *                                  *
 *      hack hack, more hack!       *
 *                                  *
 ************************************/

#include    <stdio.h>
#include    <ctype.h>
#include    <string.h>
#include    <stdlib.h>
#include    "gencode.h"

#define AS63_TITLE      "HD6309 cross assembler version v1.52T\n"

#ifdef _MSC_VER
 #define ITOA10(i,a)    _itoa( (i), (a), 10 )
#elif defined _WIN32
 #define ITOA10(i,a)    snprintf( (a), sizeof (a), "%d", (i) )
#else
 #define ITOA10(i,a)    snprintf( (a), sizeof (a), "%d", (i) )
#endif


/*--------------------------------------------------------------------------*/

typedef struct lbltbl_t {
    int     line;
    val_t   value;
    struct lbltbl_t *left;
    struct lbltbl_t *right;
    char    flg;           /* 0:used  1:EQU  2:SET */
    uint8_t grp;
    uint8_t hidden;
    uint8_t nocase;
    val_t   structureSize;
    int     region;
    char    name[LBLSIZE + 1];
} LBLTBL_T;

typedef struct FILSTK2_tag {
    int     srcline;
    char    srcname[FNAMESZ + 1];
} FILSTK2_T;

#ifdef OPT_OA_FILE
typedef struct {
    int     ll;
    uint8_t nn;
} OATBL_T;
#endif

typedef struct structure_field {
    struct structure_field* next;
    char                    name[LBLSIZE + 1];
    val_t                   offset;
    val_t                   size;
} STRUCT_FIELD;

typedef struct structure_definition {
    struct structure_definition* next;
    char                         name[LBLSIZE + 1];
    STRUCT_FIELD *               fields;
    val_t size;
} STRUCT_DEF;

static STRUCT_DEF *     structures;
static STRUCT_DEF *     activeStructure;
static int              skippedStructure;

FILE *                  gSrcFp;
uint8_t                 gCompatMode;
uint8_t                 gUpLo_f;
uint8_t *               gLinPtr;
char                    gLineBuf[MAXCHAR + 2];
uint8_t                 gAllMacroParams;
int                     gCo_sp;
int                     gCoStk[GCO_MAX + 1];
int                     gFile_sp;
int                     gSrcLine;
static char             gSrcFName[FNAMESZ + 1];

#ifdef DEBUG
static uint8_t          gDebug_f;
#endif

/* Assembly. */
OPTBL_T const *         gOprPtr;
static int              gErrors;
int                     gPass;
static int              remBlock;
static int              failSeen;
uint16_t                gDp;
uint16_t                gLc;
uint16_t                gLinLc;
uint8_t                 gValid_f;
static uint8_t          gEOF_f;
uint8_t                 gOs9_f;
uint8_t                 gByte_f;
uint8_t                 gWord_f;
uint8_t                 gIdxOfs_f;
char                    gModName[MODNAMSZ + 1];

uint8_t                 gM6809_f;
uint8_t                 gM6800_f;
uint8_t                 gUndoc_f;

/* Output size and symbol offset counters. */
static uint16_t         gObjSiz;
static val_t            rsCounter;
static val_t            soCounter;
static val_t            foCounter;
static uint8_t          rsDefined;
static uint8_t          soDefined;
static uint8_t          foDefined;

/* Labels and character encoding. */
static LBLTBL_T *       gLblPtr;
static int              gLabels;
static int              gLineNo;
static uint8_t          gGrp;
static uint8_t          gSjis_f;

/* Listing and diagnostics. */
static char const *     gCmdName;
static int              gList;
static FILE *           gLstFp;
static uint8_t          gVerbos_f;
static FILE *           gErrFp;
static char const *     gErrFName;

/* Conditional assembly. */
static int              gP1_sp;
static int              gP1Stk[GP1_MAX + 1];

#ifdef OPT_OA_FILE  /* -a option */
static OATBL_T *        gOAStk;
static uint8_t          gOAchk_f;
static int              gOA_sp;
#endif

/* Assembly settings. */
#define PRAGMA_KINDS    22
#define PRAGMA_DEPTH    64
uint8_t                 pragmaEscapes;
uint8_t                 pragmaPcAsPcr;
uint8_t                 pragmaIndex0;
uint8_t                 pragmaForwardMax;
uint8_t                 pragmaAutoBranch;
static uint8_t          pragmaNoList;
static uint8_t          pragmaNoListCode;
static uint8_t          pragmaShadow;
static uint8_t          pragmaDollarLocal;
static uint8_t          pragmaAsm09;
static uint8_t          pragmaM80Ext;
static uint8_t          pragmaCondUndefZero;
static uint8_t          pragmaSymbolNoCase;
static uint8_t          pragmaExport;
uint8_t                 pragmaOperandSizeWarning;
uint8_t                 pragmaQrts;
static uint8_t          pragmaEmuExt;
static uint8_t          pragma6809Conv;
static uint8_t          pragma6309Conv;
static int              conditionalExpression;
int                     wordCharacter;

static uint8_t          pragmaDefaults[PRAGMA_KINDS];
static uint8_t          pragmaStacks[MAXLIB + 1][PRAGMA_KINDS][PRAGMA_DEPTH];
static uint8_t          pragmaDepth[ MAXLIB + 1][PRAGMA_KINDS];


/*--------------------------------------------------------------------------*/

#define IS_KANJI(c)  ( (unsigned) ( (c) ^ 0x20 ) - 0xa1U < 0x3cU )
//#define isKanji2(c) (c >= 0x40 && c <= 0xfc && (c) != 0x7f)

int     isKanji(int c)
{
    return gSjis_f && IS_KANJI(c);
}


static FILE    *fopenE(char const * fname, char const * atr)
{
    FILE * fp = fopen(fname, atr);

    if (fp == NULL) {
        fprintf(STDERR, "%s: File open error. %s.\n", gCmdName, fname);
        exit(1);
    }
    return fp;
}

void   *mallocE(size_t siz)
{
    void *p = malloc(siz);

    if (p == NULL) {
        fprintf(STDERR, "%s: Not enough memory.\n", gCmdName);
        exit(1);
    }
    return p;
}

char   *strdupAddE(char const* s, size_t add)
{
    size_t l = strlen(s) + 1;
    char*  m = mallocE(l + add);
    memcpy(m, s, l);
    if (add)
        memset(m+l, 0, add);
    return m;
}

void    errPrg(char const * s)
{
    fprintf(STDERR, "%s:BUG(%s)\n", gCmdName, s);
    exit(1);
}

static void errorWarning(char const * s, int errorMode)
{
    if (errorMode)
        gErrors++;
    if (gPass != 2)
        return;
    if (errorMode && gErrFName && gErrors - 1 == 0) {
        gErrFp = fopen(gErrFName, "w");
        if (gErrFp == NULL) {
            fprintf(STDERR, "%s: File open error. %s\n", gCmdName, gErrFName);
            gErrFp = STDERR;
        }
    }
    if (gList > 0) {
        fprintf(gLstFp, "*** %s\n", s);
    }
    fprintf(gErrFp, "%s %5d : %s\n", gSrcFName, gSrcLine, s);
}

void    error(char const * s)
{
    errorWarning(s, 1);
}

void    warning(char const * s)
{
    errorWarning(s, 0);
}

static void errLbl(char const * msg, char const * lbl)
{
    char buf[260];

    if (gPass != 2)
        return;
    sprintf(buf, "%s(%s)", msg, lbl);
    error(buf);
}

static char const *FIL_BaseName(char const *adr)
{
    char const *p = adr;

    while (*p != '\0') {
        if (*p == '/'
         #if defined(MSDOS) || defined(_WIN32)
           || *p == ':' || *p == '\\'
         #endif
       ) {
            adr = p + 1;
        }
     #if 1
        if (isKanji( (*(uint8_t *) p) ) && *(p + 1) )
            p++;
     #endif
        p++;
    }
    return adr;
}

static char    *FIL_AddChgExt(char * filename, char const * ext, uint8_t chg)
{
    char *p = strrchr((char*)FIL_BaseName(filename), '.');

    if (p == NULL) {
        strcat( filename, ext);
    } else if (chg) {
        strcpy(p, ext);
    }
    return filename;
}


/*---------------------------------------------------------------------------*/

static LINE_STATE*  lineStates;
static size_t       lineCapacity;
static int          relaxRequested;
static int          relaxChanges;
int                 branchChanges;
static int          relaxFailed;
int                 addressRegion;
int                 expressionForward;
int                 expressionFixedForward;
int                 expressionLiteralZero;
static int          expressionDepth;
int                 lineCode;
static int          restoringPragmas;
static uint8_t      pragmaConfigured[PRAGMA_KINDS];
static uint8_t      pragmaConfigStacks[MAXLIB + 1][PRAGMA_KINDS][PRAGMA_DEPTH];

LINE_STATE *lineState(void)
{
    size_t  index = (size_t)gLineNo;
    if (index >= lineCapacity) {
        size_t      capacity = lineCapacity ? lineCapacity * 2 : 1024;
        LINE_STATE *data;
        while (index >= capacity)
            capacity *= 2;
        data = mallocE(capacity * sizeof(*data));
        memset(data, 0, capacity * sizeof(*data));
        if (lineCapacity)
            memcpy(data, lineStates, lineCapacity * sizeof(*data));
        free(lineStates);
        lineStates = data;
        lineCapacity = capacity;
    }
    return &lineStates[index];
}

static void finishLine(void)
{
    if (relaxRequested) {
        LINE_STATE *state = lineState();
        int size = gLc - gLinLc;
        if (gPass == -1 && (state->address != gLinLc || state->size != size))
            ++relaxChanges;
        state->address = gLinLc;
        state->size    = size;
    }
}


/*---------------------------------------------------------------------------*/

static uint8_t hexDigit(uint8_t x)
{
    return ( (x &= 0x0f) < 10 ) ? x + '0' : x - 10 + 'A';
}

void    printByte(int b, int c)
{
    if (gPass != 2)
        return;
    b                  = (uint8_t) b;
    gLineBuf[c]        = hexDigit(b >> 4);
    gLineBuf[c + 1]    = hexDigit(b);
}

void    printWord(int w, int c)
{
    if (gPass != 2)
        return;
    w = (uint16_t) w;
    printByte( (w >> 8), c );
    printByte(w, c + 2);
}

void    clearAddress(void)
{
    int i;

    if (gPass != 2)
        return;
    for (i = 5; i < 9; i++)
        gLineBuf[i] = ' ';
}

void    printAddress(int a)
{
    if (gPass != 2)
        return;
    printWord(a, 5);
}

void    initLine(void)
{
    char *p;

    gLblPtr    = NULL;
    lineCode   = 0;
    initCodeLine();
    ++gLineNo;
    ITOA10(gLineNo, gLineBuf);
    ++gSrcLine;
    p          = gLineBuf;
    while (*p++) { ; }
    for (--p; p < (char const*)gLinPtr; p++)
        *p = ' ';
    printAddress(gLinLc = gLc);
}

void    putLine(void)
{
    char const *source = NULL;

    if (gPass == 2 && gList > 0 && !pragmaNoListCode && (!pragmaNoList || lineCode)) {
        source = macroListingSource();
        if (source) {
            fwrite(gLineBuf, 1, LINEHEAD, gLstFp);
            fputs(*source ? source : "\n", gLstFp);
        } else {
            fputs(gLineBuf, gLstFp);
        }
    }
}

/*---------------------------------------------------------------------------*/

static LBLTBL_T *   oLabel;
static int          oLrf;

static void printNode(LBLTBL_T const * lp)
{
    if (lp == NULL)
        return;
    printNode(lp->right);
    if (lp->line && !lp->hidden) {
        fprintf(gLstFp, "%15s %4d %04lx", lp->name, lp->line, (long)lp->value);
        fprintf(gLstFp, oLrf++ % 3 ? " " : "\n");
    }
    printNode(lp->left);
}

static void    dumpSymbol(void)
{
    if (gList > 0)
        fprintf(gLstFp, "\n");
    oLrf = 1;
    printNode(oLabel->left);
    if (oLrf % 3)
        fprintf(gLstFp, "\n");
}

static LBLTBL_T *getNode(void)
{
    static int          gi = 0;
    static LBLTBL_T *   gp = NULL;

    if (gp == NULL || gi >= MAXLABEL) {
        gp = (LBLTBL_T *) mallocE(sizeof (LBLTBL_T) * MAXLABEL);
        gi = 0;
        DEBMSGF( (STDERR, "alloc %d nodes\n", MAXLABEL) );
    }
    return gp + gi++;
}

static void    initNode(void)
{
    oLabel             = getNode();
    memset(oLabel, 0, sizeof(*oLabel));
}

typedef struct symbol_declaration {
    struct symbol_declaration*  next;
    int                         group;
    int                         kind;
    char                        name[LBLSIZE + 1];
} SYMBOL_DECLARATION;

static SYMBOL_DECLARATION *     symbolDeclarations;
static char                     declarationLabel[LBLSIZE + 1];

static LBLTBL_T *findCaseLabel(LBLTBL_T *label, char const *name, int group)
{
    LBLTBL_T *found = NULL;
    if (!label)
        return NULL;
    if (label->flg && label->nocase && label->grp == group
        && !strcasecmp(label->name, name)) return label;
    found = findCaseLabel(label->left, name, group);
    return found ? found : findCaseLabel(label->right, name, group);
}

static LBLTBL_T *findLabel(char const *name, int group)
{
    LBLTBL_T *label = oLabel;
    while (label) {
        int order = strcmp(name, label->name);
        if (!order)
            order = group - label->grp;
        if (!order) {
            LBLTBL_T *insensitive = NULL;
            if (label->flg)
                return label;
            insensitive = gUpLo_f ? NULL : findCaseLabel(oLabel, name, group);
            return insensitive ? insensitive : label;
        }
        label = order < 0 ? label->right : label->left;
    }
    return gUpLo_f ? NULL : findCaseLabel(oLabel, name, group);
}

static int symbolDeclared(char const *name, int group, int kind)
{
    SYMBOL_DECLARATION const *declaration = symbolDeclarations;
    for (; declaration; declaration = declaration->next)
        if (declaration->group == group && declaration->kind == kind
            && (!strcmp(name, declaration->name)
                || (findLabel(declaration->name, group)
                    && findLabel(declaration->name, group)->nocase
                    && !strcasecmp(name, declaration->name))))
            return 1;
    return 0;
}

static LBLTBL_T *publishedLabel(char const *name)
{
    SYMBOL_DECLARATION const *declaration = symbolDeclarations;
    LBLTBL_T *label = findLabel(name, 0);
    if (label && label->flg)
        return label;
    for (; declaration; declaration = declaration->next) {
        LBLTBL_T *published = findLabel(declaration->name, declaration->group);
        if (declaration->kind == 1 && published && published->flg
            && (!strcmp(name, declaration->name)
                || (published->nocase && !strcasecmp(name, declaration->name)))) {
            return published;
        }
    }
    return label;
}

static void declareSymbol(char const *name, int kind);

void    defLabel(char const *temp, uint8_t f, uint8_t gf)
{
    LBLTBL_T *label = oLabel;
    int group = gf ? 0 : gGrp;
    int order = 0;
    LBLTBL_T *same = NULL;
    if (f && pragmaExport)
        declareSymbol(temp, 1);
    same = findLabel(temp, group);
    if (same && same->nocase)
        temp = same->name;
    if (gf && gGrp) {
        LBLTBL_T *local = findLabel(temp, gGrp);
        if (local && local->flg && local->line != gLineNo)
            errLbl("Duplicate label definition", temp);
    }
    if (group && findLabel(temp, 0))
        group = 0;
    for (;;) {
        order = strcmp(temp, label->name);
        if (!order)
            order = group - label->grp;
        if (!order) {
            if (label->line != gLineNo && (f == 1 || label->flg == 1))
                errLbl("Duplicate label definition", temp);
            gLblPtr = label;
            if (f) {
                label->value  = gLinLc;
                label->hidden = pragmaNoList || pragmaNoListCode;
                label->region = addressRegion;
                label->nocase = gUpLo_f || pragmaSymbolNoCase;
                label->structureSize = -1;
            }
            if (!label->flg && f) {
                label->flg = f;
                label->line = gLineNo;
            }
            return;
        }
        if (order < 0) {
            if (label->right) {
                label = label->right;
            } else {
                label->right = getNode();
                label = label->right;
                break;
            }
        } else {
            if (label->left) {
                label = label->left;
            } else {
                label->left = getNode();
                label = label->left;
                break;
            }
        }
    }
    ++gLabels;
    gLblPtr         = label;
    label->value    = gLinLc;
    label->grp      = (uint8_t)group;
    label->hidden   = f && (pragmaNoList || pragmaNoListCode);
    label->nocase   = f && (gUpLo_f || pragmaSymbolNoCase);
    label->structureSize = -1;
    label->region   = addressRegion;
    label->flg      = f;
    label->line     = f ? gLineNo : 0x7fff;
    strcpy(label->name, temp);
    label->right    = label->left = NULL;
}

static LBLTBL_T *refLbl0(char const *name)
{
    LBLTBL_T *local     = findLabel(name, gGrp);
    LBLTBL_T *published = publishedLabel(name);
    if (symbolDeclared(name, gGrp, 2))
        return published ? published : (local && !local->flg ? local : NULL);
    if (local && local->flg)
        return local;
    if (published && published->flg)
        return published;
    return local ? local : published;
}

static void declareSymbol(char const *name, int kind)
{
    SYMBOL_DECLARATION *declaration;
    if (symbolDeclared(name, gGrp, kind))
        return;
    declaration        = mallocE(sizeof(*declaration));
    declaration->group = gGrp;
    declaration->kind  = kind;
    strcpy(declaration->name, name);
    declaration->next  = symbolDeclarations;
    symbolDeclarations = declaration;
    if (!findLabel(name, gGrp) && !findLabel(name, 0)) {
        LBLTBL_T *saved = gLblPtr;
        defLabel(name, 0, 0);
        gLblPtr        = saved;
    }
}

static void checkSymbolDeclarations(void)
{
    SYMBOL_DECLARATION const *declaration = symbolDeclarations;
    if (gPass != 2)
        return;
    for (; declaration; declaration = declaration->next) {
        LBLTBL_T *label = findLabel(declaration->name, declaration->group);
        if (declaration->kind != 1)
            continue;
        if (!label || !label->flg) {
            label = findLabel(declaration->name, 0);
            if ((!label || !label->flg) && gObjct != OB_ASM)
                errLbl("Exported label is undefined", declaration->name);
        } else if (publishedLabel(declaration->name) != label) {
            errLbl("Duplicate exported label", declaration->name);
        }
    }
}

static LBLTBL_T *refLabel(char const * lbl)
{
    LBLTBL_T *lp = refLbl0(lbl);

    if (lp && lp->flg == 0)
        return NULL;
    return lp;
}


/*---------------------------------------------------------------------------*/

int     isSymbl(int c)
{
    c = (uint8_t) c;
    return ( isalnum(c) || (c == '_') || (c == '.') || (c == '@') );
}

static int     isSymbl2(int c)
{
    c = (uint8_t) c;
    return ( isalpha(c) || (c == '_') || (c == '.') );
}

int     isSymbl3(int c)
{
    c = (uint8_t) c;
    return (isalnum(c) || c == '_' || c == '.' || c == '@' || c == '$'
        || (c == '?' && gCompatMode == COMPAT_LWASM));
}

int isCommentChar(int c)
{
    return c == '*' || c == '#' || c == ';';
}

uint8_t    getLabel(char * buf)
{
    uint8_t *   p;
    uint8_t     gf = 0;

    if (!isSymbl2(*gLinPtr) && !macroNumericLocal(gLinPtr))
        error("Invalid label name.");
    for (p = (uint8_t*)buf; p < (uint8_t *) buf + LBLSIZE; p++, gLinPtr++) {
        *p = *gLinPtr;
        if (!isSymbl3(*p) )
            break;
        if (gUpLo_f)
            *p = toupper(*(uint8_t const*)p);
    }
    while (isSymbl3(*gLinPtr))
        gLinPtr++;
    if (*gLinPtr == ':') {
        gLinPtr++;
        gf = gCompatMode == COMPAT_AS63;
    }
    *p = '\0';
    macroLocalLabel(buf);
    return gf;
}

void    skipSpace(void)
{
    while (isspace(*gLinPtr) && *gLinPtr != '\n')
        gLinPtr++;
}

int     checkChar(uint8_t c)
{
    if (toupper(*gLinPtr) == c) {
        gLinPtr++;
        return 1;
    }

    return 0;
}

int     checkCh_e(uint8_t c)
{
    static char buf[] = "Expected ' '.";

    if (toupper(*gLinPtr) == c) {
        gLinPtr++;
        return 1;
    }
    buf[10] = c;
    error(buf);
    return 0;
}


/*---------------------------------------------------------------------------*/
static int compatibleNumber(val_t *result)
{
    uint8_t*    start   = gLinPtr;
    uint8_t*    end     = start;
    uval_t      value   = 0;
    int         base    = 10;
    int         digit   = 0;
    int         prefix  = 0;
    if (*start == '@' || *start == '&') {
        if (gCompatMode != COMPAT_LWASM)
            return 0;
        base = *start == '@' ? 8 : 10;
        prefix = 1;
        ++start;
        end = start;
        while (isdigit(*end))
            ++end;
    } else if (isdigit(*start)) {
        if (*start == '0' && (toupper(*(start + 1)) == 'X'
            || (toupper(*(start + 1)) == 'B' && isdigit(*(start + 2)))))
            return 0;
        while (isalnum(*end))
            ++end;
        switch (toupper(*(end - 1))) {
        case 'H': base = 16; break;
        case 'B': base = 2; break;
        case 'Q':
        case 'O': base = 8; break;
        default: return 0;
        }
        --end;
    } else {
        return 0;
    }
    if (start == end)
        error("Missing digits in numeric constant.");
    while (start < end) {
        digit = isdigit(*start) ? *start - '0' : toupper(*start) - 'A' + 10;
        if (digit < 0 || digit >= base) {
            error("Invalid digit in numeric constant.");
            digit = 0;
        }
        value = value * base + digit;
        ++start;
    }
    gLinPtr = end + (prefix ? 0 : 1);
    *result = (val_t)value;
    return 1;
}

static val_t   term(void)
{
    char        temp[LBLSIZE + 1];
    LBLTBL_T *  lp;
    val_t       tv;
    uint16_t    c;

    if (compatibleNumber(&tv))
        return tv;
    switch ( (c = *gLinPtr++) ) {
    case '+':
        return term();
    case '-':
        return -term();
    case '^':
        /* if (!gOs9_f) break; */
    case '~':
        return ~term();
    case '!':
     #ifdef DRC
        if (gDrc_f)
            return ~term();
     #endif
        return (term() == 0);
    case '*':
        return gLinLc;
    case '.':
        if (gOs9_f && !isSymbl3(*gLinPtr) )
            return gCSectBase;
        break;
    case '\'':
        if (isKanji(*gLinPtr) || (wordCharacter && pragmaM80Ext))
            goto DC;
        return *gLinPtr++;
    case '"':
      DC:
        c  = *gLinPtr++;
        c  = c * 0x100 + *gLinPtr++;
        return c;
    case '$':
      XDIG:
        for (tv = 0; c = *gLinPtr, isxdigit(c); gLinPtr++)
            tv = tv * 16 + ( isdigit(c) ? (c - '0') : (toupper(c) - 'A' + 10) );
        return tv;
    case '%':
      BDIG:
        for (tv = 0; (c = *gLinPtr) == '0' || c == '1'; gLinPtr++)
            tv = tv * 2 + c - '0';
        return tv;
    case '(':
        tv = expression();
        checkCh_e(')');
        return tv;
    }
    --gLinPtr;
    if (isSymbl2(c) || macroNumericLocal(gLinPtr)) {
        getLabel(temp);
        if (!strcasecmp(temp, "sizeof") && *gLinPtr == '{') {
            ++gLinPtr;
            getLabel(temp);
            checkCh_e('}');
            lp = refLabel(temp);
            if (lp && lp->structureSize >= 0) {
                if (gLineNo < lp->line)
                    ++expressionForward;
                return lp->structureSize;
            }
            if (gPass == 1 && !refLbl0(temp))
                defLabel(temp, 0, 0);
            if (gPass == 2)
                errLbl("Unknown structure size", temp);
            gValid_f = 0;
            return 0;
        }
        if (macroValue(temp, &tv))
            return tv;
        if (conditionalExpression && pragmaCondUndefZero) {
            lp = refLabel(temp);
            if (!lp || lp->line > gLineNo)
                return 0;
        }
        if (rsDefined && !strcasecmp(temp, "__RS") )
            return rsCounter;
        if (soDefined && !strcasecmp(temp, "__SO"))
            return soCounter;
        if (foDefined && !strcasecmp(temp, "__FO"))
            return foCounter;
        if (strcasecmp(temp, "defined") == 0 || strcasecmp(temp, "used") == 0) {
            int definitionOnly = strcasecmp(temp, "defined") == 0;
            checkCh_e('(');
            getLabel(temp);
            if (macroValue(temp, &tv)) {
                checkCh_e(')');
                return 1;
            }
            if ((rsDefined && !strcasecmp(temp, "__RS"))
             || (soDefined && !strcasecmp(temp, "__SO"))
             || (foDefined && !strcasecmp(temp, "__FO"))
            ) {
                checkCh_e(')');
                return 1;
            }
            if (definitionOnly)
                lp = refLabel(temp);
            else
                lp = refLbl0(temp);
            checkCh_e(')');
            return (lp != NULL);
        } else if (gPass == 1 && refLbl0(temp) == NULL) {
            defLabel(temp, 0, 0);
        } else if (( lp = refLabel(temp) ) != NULL) {
            if (gLineNo < lp->line) {
                ++expressionForward;
                if (lp->region != addressRegion)
                    expressionFixedForward = 1;
                if ((!pragmaConfigured[6] && gCompatMode != COMPAT_LWASM)
                    || pragmaForwardMax || gPass == 1)
                    gValid_f = 0;
            }
            return (lp->value);
        }
     #ifdef OPT_OA_FILE
        if (gObjct == OB_ASM && refLbl0(temp) )
            gOAchk_f = 1;
        else
     #endif
        errLbl("Undefined label", temp);
        /* DEBMSGF((STDERR,"LABEL:%s\n",temp)); */
        return (gValid_f = 0);
    } else if (isdigit(c)) {
        if (c == '0') {
            if (toupper(*(gLinPtr + 1)) == 'X') {
                gLinPtr += 2;
                goto XDIG;
            } else if (toupper(*(gLinPtr + 1)) == 'B') {
                gLinPtr += 2;
                goto BDIG;
            }
        }
        for (tv = 0; isdigit(*gLinPtr); ++gLinPtr)
            tv = (tv * 10) + *gLinPtr - '0';
        /* DEBMSGF((STDERR,"*gLinPtr : %c(%02x) *%lx\t[digit]\n", gLinPtr,*gLinPtr,gLinPtr)); */
        return tv;
    } else {
        error("Invalid character in expression.");
        DEBMSGF( (STDERR, "*gLinPtr : %c(%02x)\t[term()]\n", *gLinPtr, *gLinPtr) );
        return (gValid_f = 0);
    }
}

static val_t   expMUL(void)
{
    val_t   val;
    char    c;

    val = term();
    for (;;) {
        c = *gLinPtr;
        if (c == '*') {
            gLinPtr++;
            val *= term();
        } else if (c == '/' || c == '%') {
            val_t v;
            gLinPtr++;
            v = term();
            if (v == 0) {
                error("Division by zero.");
                v = 1;
            }
            if (c == '/')
                val /= v;
            else
                val %= v;
        } else {
            break;
        }
    }
    return val;
}

static val_t   expADD(void)
{
    val_t val;

    val = expMUL();
    for (;;) {
        if (*gLinPtr == '+') {
            gLinPtr++;
            val += expMUL();
        } else if (*gLinPtr == '-') {
            gLinPtr++;
            val -= expMUL();
        } else {
            break;
        }
    }
    return val;
}

static val_t   expSHIFT(void)
{
    val_t val;

    val = expADD();
    for (;;) {
        if (*gLinPtr == '<' && *(gLinPtr + 1) == '<') {
            gLinPtr   += 2;
            val      <<= expADD();
        } else if (*gLinPtr == '>' && *(gLinPtr + 1) == '>') {
            gLinPtr   += 2;
            val      >>= expADD();
        } else {
            break;
        }
    }
    return val;
}

static val_t   expCO(void)
{
    val_t   val;
    uint8_t c;

    val = expSHIFT();
    for (;;) {
        c = *(gLinPtr + 1);
        switch (*gLinPtr) {
        case '<':
            if (c == '=') {
                gLinPtr   += 2;
                val        = ( val <= expSHIFT() );
            } else {
                gLinPtr++;
                val = ( val < expSHIFT() );
            }
            break;
        case '>':
            if (c == '=') {
                gLinPtr   += 2;
                val        = ( val >= expSHIFT() );
            } else {
                gLinPtr++;
                val = ( val > expSHIFT() );
            }
            break;
        default:
            goto J1;
        }
    }
  J1:
    return val;
}


static val_t   expEQEQ(void)
{
    val_t   val;
    uint8_t c;

    val = expCO();
    for (;;) {
        c = *(gLinPtr + 1);
        switch (*gLinPtr) {
        case '!':
            if (c != '=')
                goto J1;
            gLinPtr   += 2;
            val        = ( val != expCO() );
            break;
        case '=':
            if (c != '=')
                goto J1;
            gLinPtr   += 2;
            val        = ( val == expCO() );
            break;
        default:
            goto J1;
        }
    }
  J1:
    return val;
}


static val_t   expAND(void)
{
    val_t val;

    val = expEQEQ();
    while (*gLinPtr == '&' && *(gLinPtr + 1) != '&') {
        gLinPtr++;
        val &= expEQEQ();
    }
    return val;
}

static val_t   expEOR(void)
{
    val_t val;

    val = expAND();
    while ( *gLinPtr == '^' || (*gLinPtr == '?' /* && gOs9_f */ )) {
        gLinPtr++;
        val ^= expAND();
    }
    return val;
}

static val_t   expOR(void)
{
    val_t val;

    val = expEOR();
    while ( (*gLinPtr == '|' && *(gLinPtr + 1) != '|')
         || (*gLinPtr == '!' && *(gLinPtr + 1) != '=' /* && gOs9_f */ ) )
    {
        gLinPtr++;
        val |= expEOR();
    }
    return val;
}

static val_t   expLAND(void)
{
    val_t val;

    val = expOR();
    while (*gLinPtr == '&' && *(gLinPtr + 1) == '&') {
        gLinPtr   += 2;
        val        = (expOR() && val);
    }
    return val;
}

static val_t   expLOR(void)
{
    val_t val;

    val = expLAND();
    while (*gLinPtr == '|' && *(gLinPtr + 1) == '|') {
        gLinPtr   += 2;
        val        = (expLAND() || val);
    }
    return val;
}

val_t   expression(void)
{
    val_t val;
    uint8_t *start = gLinPtr;
    int byteForced = 0;
    int wordForced = 0;

    if (!expressionDepth) {
        expressionForward = expressionFixedForward = 0;
        gValid_f = 1;
        gByte_f = gWord_f = 0;
    }
    ++expressionDepth;
    if (checkChar('<') )
        byteForced = 1;
    else if (checkChar('>') )
        wordForced = 1;
    start = gLinPtr;
    val = expLOR();
    gByte_f |= byteForced;
    gWord_f |= wordForced;
    if (!--expressionDepth)
        expressionLiteralZero = *start == '0' && gLinPtr == start + 1;
    switch (*gLinPtr) {
    case ' ':
    case '\t':
    case ',':
    case ')':
    case ']':
    case ';':
    case '\n':
        break;
    default:
        error("Unexpected character.");
        DEBMSGF( (STDERR, "*gLinPtr : %c(%02x)\t[expression()]\n", *gLinPtr, *gLinPtr) );
    }
    return val;
}

uint8_t    bytExpr(void)
{
    val_t val;

    val = expression();
    if (val < -128 || 255 < val)
        error("Value does not fit in one uint8_t.");
    return (uint8_t) (val & 0xff);
}

val_t   invExpr(void)
{
    val_t r;

    r = expression();
    if (!gValid_f)
        error("Constant expression is unresolved.");
    return r;
}

static int pragmaName(char const *name, int *enabled)
{
    static char const * const names[] = {
        "6809", "6309", "6800compat", "cescapes", "pcaspcr",
        "index0tonone", "forwardrefmax", "autobranchlength", "nolist", "nolistcode", "shadow",
        "dollarlocal", "asm09", "m80ext", "condundefzero", "symbolnocase",
        "export", "operandsizewarning", "qrts", "emuext", "6809conv", "6309conv"
    };
    int     i;
    *enabled = 1;
    if (!strcmp(name, "nosymbolcase"))
        return 15;
    if (!strcmp(name, "symbolcase") || !strcmp(name, "nonosymbolcase")) {
        *enabled = 0;
        return 15;
    }
    if (!strcmp(name, "dollarnotlocal")) {
        *enabled = 0;
        return 11;
    }
    if (!strcmp(name, "nodollarnotlocal"))
        return 11;
    for (i = 0; i < PRAGMA_KINDS; ++i) {
        if (!strcmp(name, names[i]))
            return i;
    }
    if (!strcmp(name, "list") || !strcmp(name, "listcode")) {
        *enabled = 0;
        return !strcmp(name, "list") ? 8 : 9;
    }
    if (!strncmp(name, "no", 2)) {
        name    += 2;
        *enabled = 0;
    }
    for (i = 0; i < PRAGMA_KINDS; ++i)
        if (!strcmp(name, names[i]))
            return i;
    return -1;
}

static int pragmaValue(int kind)
{
    switch (kind) {
    case 0: return gM6809_f                 != 0;
    case 1: return gM6809_f                 == 0;
    case 2: return gM6800_f                 != 0;
    case 3: return pragmaEscapes            != 0;
    case 4: return pragmaPcAsPcr            != 0;
    case 5: return pragmaIndex0             != 0;
    case 6: return pragmaForwardMax         != 0;
    case 7: return pragmaAutoBranch         != 0;
    case 8: return pragmaNoList             != 0;
    case 9: return pragmaNoListCode         != 0;
    case 10:return pragmaShadow             != 0;
    case 11:return pragmaDollarLocal        != 0;
    case 12:return pragmaAsm09              != 0;
    case 13:return pragmaM80Ext             != 0;
    case 14:return pragmaCondUndefZero      != 0;
    case 15:return pragmaSymbolNoCase       != 0;
    case 16:return pragmaExport             != 0;
    case 17:return pragmaOperandSizeWarning != 0;
    case 18:return pragmaQrts               != 0;
    case 19:return pragmaEmuExt             != 0;
    case 20:return pragma6809Conv           != 0;
    case 21:return pragma6309Conv           != 0;
    }
    return 0;
}

static void pragmaSet(int kind, int enabled)
{
    if (!restoringPragmas)
        pragmaConfigured[kind]        = 1;
    switch (kind) {
    case 0: gM6809_f                  = enabled; break;
    case 1: gM6809_f                  = !enabled;break;
    case 2: gM6800_f                  = enabled; break;
    case 3: pragmaEscapes             = enabled; break;
    case 4: pragmaPcAsPcr             = enabled; break;
    case 5: pragmaIndex0              = enabled; break;
    case 6:
        pragmaForwardMax              = enabled;
        if (!enabled && !restoringPragmas)
            relaxRequested            = 1;
        break;
    case 7:
        pragmaAutoBranch              = enabled;
        if (enabled)
            relaxRequested            = 1;
        break;
    case 8: pragmaNoList              = enabled; break;
    case 9: pragmaNoListCode          = enabled; break;
    case 10:pragmaShadow              = enabled; break;
    case 11:pragmaDollarLocal         = enabled; break;
    case 12:pragmaAsm09               = enabled; break;
    case 13:pragmaM80Ext              = enabled; break;
    case 14:pragmaCondUndefZero       = enabled; break;
    case 15:pragmaSymbolNoCase        = enabled; break;
    case 16:pragmaExport              = enabled; break;
    case 17:pragmaOperandSizeWarning  = enabled; break;
    case 18:pragmaQrts                = enabled; break;
    case 19:pragmaEmuExt              = enabled; break;
    case 20:pragma6809Conv            = enabled; break;
    case 21:pragma6309Conv            = enabled; break;
    }
}

int pragmaEnabled(char const *name)
{
    int enabled = 0;
    int kind = pragmaName(name, &enabled);
    return kind >= 0 && pragmaValue(kind) == enabled;
}

int pragmaIndexConfigured(void)
{
    return pragmaConfigured[5] != 0;
}

static int readPragma(int *enabled)
{
    char name[LBLSIZE + 1];
    int n = 0;
    skipSpace();
    while (isSymbl(*gLinPtr)) {
        if (n < LBLSIZE)
            name[n++] = tolower(*gLinPtr);
        ++gLinPtr;
    }
    name[n] = 0;
    return pragmaName(name, enabled);
}

static void handlePragma(int mode)
{
    int       enabled = 0;

    clearAddress();
    do {
        int kind = readPragma(&enabled);
        if (kind < 0) {
            if (!mode)
                error("Unknown or unsupported pragma.");
        } else if (mode < 2) {
            pragmaSet(kind, enabled);
        } else {
            uint8_t* depth = &pragmaDepth[gFile_sp][kind];
            if (mode == 2) {
                if (*depth < PRAGMA_DEPTH) {
                    pragmaConfigStacks[gFile_sp][kind][*depth] = pragmaConfigured[kind];
                    pragmaStacks[gFile_sp][kind][(*depth)++]   = pragmaValue(kind);
                }
            } else if (*depth) {
                pragmaSet(kind, pragmaStacks[gFile_sp][kind][--*depth]);
                pragmaConfigured[kind] = pragmaConfigStacks[gFile_sp][kind][*depth];
            }
        }
        skipSpace();
    } while (checkChar(','));

    while (*gLinPtr && *gLinPtr != '\n')
        ++gLinPtr;
}

void pragmaDirective(void)
{
    handlePragma(0);
}

void labelValue(val_t value)
{
    if (gLblPtr) {
        gLblPtr->value = value;
        macroSetValue(gLblPtr->name, value);
    }
    clearAddress();
    printWord(value, 5);
}

/*----------------------------------*/

void    equ(void)
{
    val_t value;
    skipSpace();
    value = (gOprPtr->opcode & EQU_RESOLVED) ? invExpr() : expression();
    labelValue(value);
}

/*----------------------------------*/

static int comparisonOperand(char *dst, int size)
{
    int quote  = 0;
    int length = 0;
    skipSpace();
    if (*gLinPtr == '\"' || *gLinPtr == '\'')
        quote = *gLinPtr++;
    while (*gLinPtr && *gLinPtr != '\n') {
        if (quote ? *gLinPtr == quote : (*gLinPtr == ',' || isspace(*gLinPtr) || *gLinPtr == ';'))
            break;
        if (length >= size - 1) {
            error("Comparison operand is too long.");
            return 0;
        }
        dst[length++] = *gLinPtr++;
    }
    dst[length] = 0;
    if (quote && !checkChar(quote)) {
        error("Missing closing quote.");
        return 0;
    }
    return 1;
}

static int textOperand(char * dst, int size)
{
    uint8_t     c;
    int         quote = 0;
    int         n     = 0;
    skipSpace();
    if (*gLinPtr == '"' || *gLinPtr == '\'') {
        quote = *gLinPtr++;
    } else if (*gLinPtr == '<') {
        ++gLinPtr;
        quote = '>';
    }
    while ((c = *gLinPtr) != 0 && c != '\n') {
        if (quote ? (c == quote) : (isspace(c) || c == ',' || c == ';')) {
            break;
        }
        if (n >= size - 1) {
            error("Operand is too long.");
            dst[0] = 0;
            return 0;
        }
        dst[n++] = c;
        ++gLinPtr;
    }
    dst[n] = 0;
    if (quote && !checkChar(quote)) {
        error("Missing closing quote.");
        return 0;
    }
    return 1;
}

int nextComma(void)
{
    uint8_t *saved = gLinPtr;
    skipSpace();
    if (checkChar(',') )
        return 1;
    gLinPtr = saved;
    return 0;
}

void symbolDirective(void)
{
    char name[LBLSIZE + 1];
    int kind = gOprPtr->prefix;
    clearAddress();
    if (*declarationLabel) {
        declareSymbol(declarationLabel, kind);
        while (*gLinPtr && *gLinPtr != '\n')
            ++gLinPtr;
    } else {
        do {
            skipSpace();
            if (!isSymbl2(*gLinPtr)) {
                error("Expected a symbol name.");
                return;
            }
            getLabel(name);
            declareSymbol(name, kind);
        } while (nextComma());
    }
 #ifdef OPT_OA_FILE
    if (gObjct == OB_ASM && gPass == 2)
        oa_putStr(gLineBuf + LINEHEAD, 0);
 #endif
}

void rsOffset(void)
{
    int   mode = gOprPtr->opcode & 15;
    int   kind = gOprPtr->opcode >> 4;
    val_t * counter = (kind == 1 && gCompatMode != COMPAT_VASM) ? &soCounter
                    :  kind == 2 ? &foCounter
                    :              &rsCounter;
    val_t count;
    val_t next;
    int size = gOprPtr->prefix;
    if (!strcmp(gOprPtr->mnemonic, "RS") && gCompatMode != COMPAT_VASM)
        size = 1;
    clearAddress();
    if (mode == RS_RESET) {
        *counter = 0;
    } else if (mode == RS_EVEN) {
        if (*counter == 65535) {
            error("RS offset is outside 0..65535.");
            return;
        }
        *counter += *counter & 1;
    } else if (mode == RS_SET) {
        skipSpace();
        count = invExpr();
        if (count < (kind == 2 ? -65535 : 0) || count > 65535) {
            error(kind == 0 ? "RS offset is outside 0..65535." : "Offset is outside the supported range.");
            return;
        }
        *counter = count;
    } else {
        skipSpace();
        count = (!*gLinPtr || *gLinPtr == '\n' || *gLinPtr == ';') ? 0 : invExpr();
        if (count < 0 || count > 65535 / size) {
            error("RS count is outside the supported range.");
            return;
        }
        next = *counter + (kind == 2 ? -1 : 1) * count * size;
        if (next < (kind == 2 ? -65535 : 0) || next > 65535) {
            error("RS offset is outside 0..65535.");
            return;
        }
        labelValue(kind == 2 && gCompatMode == COMPAT_VASM ? next : *counter);
        *counter = next;
    }
    if (gCompatMode == COMPAT_VASM && kind != 2) {
        soCounter = rsCounter;
        rsDefined = soDefined = 1;
    } else if (kind == 1) {
        soDefined = 1;
    } else if (kind == 2) {
        foDefined = 1;
    } else {
        rsDefined = 1;
    }
}

void ignoreOperand(void)
{
    clearAddress();
    while (*gLinPtr && *gLinPtr != '\n')
        ++gLinPtr;
}

void commentBlock(void)
{
    remBlock = gOprPtr->prefix;
    ignoreOperand();
}

void failDirective(void)
{
    char message[MAXCHAR + 1];
    skipSpace();
    if (gOprPtr->prefix) {
        int n = 0;
        while (*gLinPtr && *gLinPtr != '\n' && n < MAXCHAR)
            message[n++] = *gLinPtr++;
        message[n] = 0;
    } else if (!textOperand(message, sizeof(message))) {
        return;
    }
    if (gOprPtr->prefix == 2) {
        warning(message);
    } else {
        error(message);
        failSeen = 1;
    }
}

void assertDirective(void)
{
    char message[MAXCHAR + 1] = "Assertion failed.";
    val_t result = 0;
    int errors = gErrors;
    clearAddress();
    skipSpace();
    result = expression();
    if (nextComma() && !textOperand(message, sizeof(message)))
        return;
    if (gPass == 2 && gErrors == errors && !result) {
        error(message);
        failSeen = 1;
    }
}

void argumentOffsets(void)
{
    val_t offset = 4;
    char  name[LBLSIZE + 1];
    int   size;
    skipSpace();
    if (checkChar('#')) {
        offset = invExpr();
        skipSpace();
        if (!checkCh_e(','))
            return;
    }
    do {
        skipSpace();
        getLabel(name);
        size = 2;
        if (strlen(name) > 2 && name[strlen(name)-2] == '.') {
            int suffix = toupper(*(uint8_t const*)(name + strlen(name) - 1));
            if (suffix != 'B' && suffix != 'W' && suffix != 'L') {
                error("Invalid CARGS size.");
                return;
            }
            size = (suffix == 'L') ? 4 : 2;
            name[strlen(name)-2] = 0;
        }
        if (offset < -65535 || offset > 65535) {
            error("CARGS offset is outside the supported range.");
            return;
        }
        defLabel(name, 1, 1);
        labelValue(offset);
        offset += size;
    } while (nextComma());
}

void printText(void)
{
    char    text[MAXCHAR + 1];
    int     more = 0;
    int     mode = gOprPtr->prefix;
    val_t   value = 0;
    clearAddress();
    do {
        skipSpace();
        if (mode == 1 && *gLinPtr != '"' && *gLinPtr != '\'') {
            value = expression();
            if (gPass == 2)
                printf("%ld", (long)value);
        } else {
            if (*gLinPtr == '"' || *gLinPtr == '\'') {
                if (!textOperand(text, sizeof(text)) )
                    return;
            } else {
                int n = 0;
                while (*gLinPtr && *gLinPtr != '\n' && *gLinPtr != ';' && *gLinPtr != ',')
                    text[n++] = *gLinPtr++;
                while ( n && isspace( *(uint8_t const*)(text + n - 1) ) )
                    --n;
                text[n] = 0;
                if (*gLinPtr == ';') {
                    while (*gLinPtr && *gLinPtr != '\n')
                        ++gLinPtr;
                }
            }
            if (gPass == 2)
                printf("%s%s", text, mode ? "" : "\n");
        }
        more = mode == 2 ? 0 : nextComma();
    } while (more);
    if (gPass == 2) {
        if (mode == 1)
            putchar('\n');
        else if (mode == 2)
            printf(" %lX\n", (unsigned long)gLc);
    }
}

void printValue(void)
{
    val_t val;
    clearAddress();
    skipSpace();

    do {
        skipSpace();
        val = expression();
        if (gPass == 2) {
            unsigned int    u = (unsigned int) val;
            char            ascii[5];
            char            binary[33];
            int             i;
            for (i = 0; i < 4; ++i) {
                int c = ( u >> ( (3 - i) * 8 ) ) & 255;
                ascii[i] = c >= 32 && c <= 126 ? c : '.';
            }
            ascii[4]   = 0;
            for (i = 0; i < 32; ++i)
                binary[i] = '0' + ( ( u >> (31 - i) ) & 1 );

            binary[32] = 0;
            printf("$%lX %ld \"%s\" %%%s\n", (long)u, (long)val, ascii, binary);
        }
    } while ( nextComma() );
}

void    endop(void)
{
    uint16_t w;

    clearAddress();
    fclose(gSrcFp);
    if (popFile() != 0)
        return;
    gEOF_f = 1;
    skipSpace();
    w      = (*gLinPtr != '\n') ? expression() : 0;
    if (gEntryAddr == 0xFFFF)
        gEntryAddr = w == 0 ? gStartAddr : w;
    printWord(gEntryAddr, 5);
    if (gRmb_f && gFBasic_f)
        gObjCnt -= gRmb_sp;
}


/*----------------------------------*/

void    opt(void)
{
    if (gOprPtr->prefix == LIST_ABSOLUTE) {
        clearAddress();
        if (gPass == 2 && gLstFp)
            gList = gOprPtr->opcode;
        return;
    }
    skipSpace();
    if (*gLinPtr == 'l') {
        ++gList;
        ++gLinPtr;
    } else if (*gLinPtr == '-' && *(gLinPtr + 1) == 'l') {
        --gList;
        gLinPtr += 2;
    }
}

/*---------------------------------------------------------------------------*/

/* Include files. */
static FILE *           gFileStk[MAXLIB];
static FILSTK2_T        gFilStk2[MAXLIB];
static char const *     gIncDirs[EXTRA_INCDIRS * 2 + 1];
static int              gIncDirCount;
static int              gIncDirExtraTop;


int fileOperand(char *name, int size, int *searchOnly)
{
    skipSpace();
    *searchOnly = *gLinPtr == '<';
    if (!textOperand(name, size))
        return 0;
    if (!strncmp(name, "$INC/", 5)) {
        memmove(name, name + 5, strlen(name + 5) + 1);
        *searchOnly = 1;
    }
    return 1;
}

FILE *openSearch(char const *name, char const *mode, int searchOnly)
{
    FILE *fp = NULL;
    char path[FNAMESZ + 1];
    int i = 0;

    if (!searchOnly) {
        fp = fopen(name, mode);
        if (fp)
            return fp;
    }
    if (name[0] == '/' || name[0] == '\\' || (name[0] && name[1] == ':'))
        return NULL;
    for (i = 0; i < gIncDirCount; ++i) {
        char const *dir = gIncDirs[i];
        if (strlen(dir) + strlen(name) + 2 > sizeof(path))
            continue;
        strcpy(path, dir);
        if (*path && path[strlen(path) - 1] != '/' && path[strlen(path) - 1] != '\\')
            strcat(path, "/");
        strcat(path, name);
        fp = fopen(path, mode);
        if (fp)
            return fp;
    }
    return NULL;
}

void incdir(void)
{
    char path[FNAMESZ + 1];
    clearAddress();
    if (!textOperand(path, sizeof(path)) )
        return;
    if (!*path) {
        error("Empty include directory.");
        return;
    }
    if (gIncDirCount - gIncDirExtraTop == EXTRA_INCDIRS) {
        error("Too many include directories.");
        return;
    }
    gIncDirs[gIncDirCount++] = strdupAddE(path, 0);
}

void    library(void)
{
    FILE *fp = NULL;
    char fname[FNAMESZ + 1];
    int searchOnly = 0;
    clearAddress();
    if (!fileOperand(fname, sizeof(fname), &searchOnly))
        return;
    if (gVerbos_f)
        fprintf(STDERR, "[%s]\n", fname);
    DEBMSGF( (STDERR, "include %s  (#%d)\n", fname, gFile_sp + 1) );
    fp = openSearch(fname, "r", searchOnly);
    if (!fp) {
        error("Cannot open include file.");
        return;
    }

    if (gFile_sp >= MAXLIB) {
        error("Include nesting is too deep.");
        exit(1);
    }
    strcpy(gFilStk2[gFile_sp].srcname, gSrcFName);
    gFilStk2[gFile_sp].srcline = gSrcLine;
    strcpy(gSrcFName, fname);
    gSrcLine                   = 0;
    gFileStk[gFile_sp++]       = gSrcFp;
    gSrcFp                     = fp;
}

int     popFile(void)
{
    memset(pragmaDepth[gFile_sp], 0, sizeof(pragmaDepth[gFile_sp]));
    if (gFile_sp <= 0)
        return 0;
    gSrcFp     = gFileStk[--gFile_sp];
    strcpy(gSrcFName, gFilStk2[gFile_sp].srcname);
    gSrcLine   = gFilStk2[gFile_sp].srcline;
    return 1;
}


/*---------------------------------------------------------------------------*/

/* Search links are mutable; opcode definitions remain read-only. */
static OPTBL_T const*  oOpHash[256];
static OPTBL_T const** oOpNext;

static int hash(char const* s)
{
    unsigned int h = 0;
    while (*s)
        h = (h << 2) + h + (uint8_t)*s++;
    return h & 0xff;
}

OPTBL_T const* srchOpTbl(char const * s)
{
    OPTBL_T const* q;
    for (q = oOpHash[hash(s)]; q != NULL; q = oOpNext[q - gOpTab]) {
        if (strcmp(s, q->mnemonic) == 0)
            return q;
    }
    return NULL;
}

static void initOpTbl(void)
{
    size_t  count = 0;
    int     h;
    OPTBL_T const* p;
    while (*gOpTab[count].mnemonic)
        ++count;
    free(oOpNext);
    oOpNext = mallocE(count * sizeof(*oOpNext));
    memset((void*)oOpHash, 0, sizeof(oOpHash));
    /* Reverse insertion preserves the original table's lookup order. */
    while (count) {
        p = gOpTab + --count;
        h = hash(p->mnemonic);
        oOpNext[count] = oOpHash[h];
        oOpHash[h]     = p;
    }
}


/*---------------------------------------------------------------------------*/

typedef struct conditional_result {
    int kind;
    int depth;
    int state;
    int consumed;
    int value;
    int errors;
} CONDITIONAL_RESULT;

static CONDITIONAL_RESULT*  conditionalResults;
static size_t               conditionalCount;
static size_t               conditionalCapacity;
static size_t               conditionalCursor;

static CONDITIONAL_RESULT *nextConditional(int kind)
{
    CONDITIONAL_RESULT *result;
    if (gPass == 1) {
        if (conditionalCount == conditionalCapacity) {
            size_t capacity = conditionalCapacity ? conditionalCapacity * 2 : 256;
            CONDITIONAL_RESULT *data = mallocE(capacity * sizeof(*data));
            if (conditionalCount)
                memcpy(data, conditionalResults, conditionalCount * sizeof(*data));
            free(conditionalResults);
            conditionalResults  = data;
            conditionalCapacity = capacity;
        }
        result = &conditionalResults[conditionalCount++];
        memset(result, 0, sizeof(*result));
        result->kind = kind;
        return result;
    }
    if (conditionalCursor >= conditionalCount
        || conditionalResults[conditionalCursor].kind != kind) {
        error("Conditional sequence differs from pass 1.");
        return NULL;
    }
    result = &conditionalResults[conditionalCursor++];
    if (result->errors)
        error("Invalid or unresolved conditional expression in pass 1.");
    return result;
}

static void co_ifBody(uint8_t f)
{
    int val = 0;

    if ((f == CO_ENDC || f == CO_ELSE || f == CO_ELIF)
        && gCo_sp <= macroConditionalFloor()) {
        error("Conditional directive without a matching IF in this block.");
        return;
    }

    if ((f >= CO_IF || f == CO_IFP1) && f != CO_ELIF && gCoStk[gCo_sp] < 0) {
        /* Do not evaluate operands inside an inactive parent branch. */
        if (gCo_sp >= GCO_MAX) {
            error("Conditional nesting is too deep.");
            return;
        }
        gCoStk[++gCo_sp] = -2;
        return;
    }

    if (f == CO_IFPRAGMA) {
        int enabled;
        int kind = readPragma(&enabled);
        val = kind >= 0 && pragmaValue(kind) == enabled;
    } else if (f == CO_IFB || f == CO_IFNB) {
        skipSpace();
        val = !*gLinPtr || *gLinPtr == '\n' || isCommentChar(*gLinPtr);
        if (f == CO_IFNB)
            val = !val;
    } else if (f == CO_IFMACROD || f == CO_IFMACROND) {
        char name[LBLSIZE + 1];
        skipSpace();
        getLabel(name);
        val = macroDefined(name);
        if (f == CO_IFMACROND)
            val = !val;
    } else if (f == CO_IFD || f == CO_IFND) {
        char        name[LBLSIZE + 1];
        LBLTBL_T *  lp;
        val_t internal = 0;
        skipSpace();
        getLabel(name);
        lp     = refLabel(name);
        val    = macroValue(name, &internal)
               || (rsDefined && !strcasecmp(name, "__RS"))
               || (soDefined && !strcasecmp(name, "__SO"))
               || (foDefined && !strcasecmp(name, "__FO"))
               || (lp && lp->line < gLineNo);
        if (f == CO_IFND)
            val = !val;
    } else if (f == CO_IFC || f == CO_IFNC) {
        char a[MAXCHAR + 1];
        char b[MAXCHAR + 1];
        if (!comparisonOperand(a, sizeof(a)))
            return;
        skipSpace();
        if (!checkCh_e(','))
            return;
        if (!comparisonOperand(b, sizeof(b)))
            return;
        val = !strcmp(a, b);
        if (f == CO_IFNC)
            val = !val;
    } else if (f >= CO_IF && (f != CO_ELIF || gCoStk[gCo_sp] == -1)) {
        skipSpace();
        val = invExpr();
    }
    if (f != CO_ELIF && (f >= CO_IF || f == CO_IFP1) && gCo_sp >= GCO_MAX) {
        error("Conditional nesting is too deep.");
        return;
    }
    if (f == CO_ENDC && gCo_sp == 0) {
        error("ENDIF without a matching IF.");
        return;
    }

    switch (f) {
    case CO_IFP1:
        gCoStk[++gCo_sp]   = (gPass == 1) ? 2 : -3;
        break;

    case CO_IFN:
        val                = !val;
        goto J1;

    case CO_IFGE:
        val                = (val >= 0);
        goto J1;

    case CO_IFGT:
        val                = (val > 0);
        goto J1;

    case CO_IFLE:
        val                = (val <= 0);
        goto J1;

    case CO_IFLT:
        val                = (val < 0);

    case CO_IFPRAGMA:
    case CO_IFD:
    case CO_IFND:
    case CO_IFC:
    case CO_IFNC:
    case CO_IFB:
    case CO_IFNB:
    case CO_IFMACROD:
    case CO_IFMACROND:
    case CO_IF:
      J1:
        gCoStk[++gCo_sp]   = (val) ? 1 : -1;
        break;

    case CO_ELIF:
        if (gCoStk[gCo_sp] == -1) {
            gCoStk[gCo_sp] = (val) ? 1 : -1;
            break;
        }
        /*[[through]];*/

    case CO_ELSE:
        switch (gCoStk[gCo_sp]) {
        case  0: error("ELSE or ELSIF without a matching IF."); break;
        case -1: gCoStk[gCo_sp]    = 1 ; break;
        case  1: gCoStk[gCo_sp]    = -2; break;
        case  2: error("ELSE or ELSIF cannot be paired with IFP1.");
        }
        break;

    case CO_ENDC:
        if (gCoStk[gCo_sp] == 2) {
            if (gP1_sp > GP1_MAX)
                error("Too many IFP1 directives.");
            gP1Stk[gP1_sp++] = gLineNo;
        } else if (gCoStk[gCo_sp] == -3) {
            gLineNo = gP1Stk[gP1_sp++];
        }
        gCoStk[gCo_sp] = 0;
        if (--gCo_sp < 0)
            error("ENDIF without a matching IF.");
 #ifdef DEBUG
        break;

    default:
        error("Invalid argument to co_if().");
 #endif
    }
}

static void co_ifEvaluate(uint8_t f)
{
    ++conditionalExpression;
    co_ifBody(f);
    --conditionalExpression;
}

static val_t conditionalValue(void)
{
    val_t value = 0;
    ++conditionalExpression;
    value = invExpr();
    --conditionalExpression;
    return value;
}

static void co_if(uint8_t f)
{
    CONDITIONAL_RESULT *result;
    uint8_t *start = gLinPtr;
    int errors = gErrors;
    if (gCompatMode == COMPAT_AS63) {
        co_ifEvaluate(f);
        return;
    }
    result = nextConditional(f);
    if (!result)
        return;
    if (gPass == 1) {
        co_ifEvaluate(f);
        result->depth = gCo_sp;
        result->state = gCoStk[gCo_sp];
        result->consumed = (int)(gLinPtr - start);
        result->errors = gErrors - errors;
    } else {
        if (gCo_sp > result->depth)
            gCoStk[gCo_sp] = 0;
        gCo_sp = result->depth;
        gCoStk[gCo_sp] = result->state;
        gLinPtr += result->consumed;
    }
}

static val_t inlineCondition(void)
{
    CONDITIONAL_RESULT *result;
    uint8_t *start = gLinPtr;
    int errors = gErrors;
    if (gCompatMode == COMPAT_AS63)
        return conditionalValue();
    result = nextConditional(0);
    if (!result)
        return 0;
    if (gPass == 1) {
        result->value = conditionalValue() != 0;
        result->consumed = (int)(gLinPtr - start);
        result->errors = gErrors - errors;
    } else {
        gLinPtr += result->consumed;
    }
    return result->value;
}

static int  getMnemonic(void)
{
    static uint8_t  temp[LBLSIZE + 1];
    uint8_t*        p = temp;
    uint8_t* pp = temp + LBLSIZE;
    OPTBL_T const*  q;
    uint8_t const *begin = NULL;
    char macroName[LBLSIZE + 1];
    size_t length = 0;

  NEXT_MNEMONIC:
    p = temp;
    skipSpace();
    if (*gLinPtr == '\n' || *gLinPtr == '\0' || isCommentChar(*gLinPtr))
        return 0;
    if (*gLinPtr == '=') {
        ++gLinPtr;
        if (gCoStk[gCo_sp] < 0)
            return 0;
        gOprPtr = srchOpTbl("=");
        return 1;
    }
    if (!isSymbl2(*gLinPtr)) {
        error("Invalid mnemonic.");
        DEBMSGF( (STDERR, "*gLinPtr : %c\n", *gLinPtr) );
        return 0;
    }

    begin = gLinPtr;
    while ( isSymbl( *p = toupper(*gLinPtr) )) {
        if (p++ >= pp)
            goto ERR;
        gLinPtr++;
    }
    *p = '\0';
    length = (size_t)(p - temp);
    memcpy(macroName, begin, length);
    macroName[length] = 0;
    if (gCoStk[gCo_sp] >= 0
        && (pragmaShadow || !srchOpTbl((char const *)temp))
        && macroInvoke(macroName))
        return 0;

    if (p > temp + 4 && *(p - 2) == '_' && *(p - 1) == 'I') {
        *(gLinPtr - 2) = *(gLinPtr - 1) = ' ';
        *(p - 2)       = '\0';
        skipSpace();
        if (*gLinPtr == '\n')
            goto ERR;
        *--gLinPtr     = '#';
    }

    if (( q = srchOpTbl((char const*)temp) ) != NULL) {
        if ((q->option & OPR_M6800) && !gM6800_f )
            warning("[WARNING] M6800 mnemonic used without -6.");
        if (q->option & OPR_UNDOC6809) {
            if (!gM6809_f)
                error("6809 undocumented instruction requires -8.");
            else if (!gUndoc_f)
                warning("[WARNING] 6809 undocumented instruction used without -z.");
        } else if (gM6809_f && (q->option & 0x01)) {
            error("6309 instruction used in 6809 mode.");
        }
        if (q->process == NULL) {
            co_if(q->prefix);
            return 0;
        } else if (gCoStk[gCo_sp] < 0) {
            return 0;
        }
        if (strcmp(q->mnemonic, "IIF") == 0) {
            val_t condition;
            skipSpace();
            condition = inlineCondition();
            skipSpace();
            if (checkChar(','))
                skipSpace();
            if (!condition) {
                while (*gLinPtr && *gLinPtr != '\n')
                    ++gLinPtr;
                return 0;
            }
            if (!*gLinPtr || *gLinPtr == '\n' || *gLinPtr == ';') {
                error("Missing instruction after IIF.");
                return 0;
            }
            if (macroControl("", 0))
                return 0;
            goto NEXT_MNEMONIC;
        }
        gOprPtr = q;
        return 1;
    }
    if (gCoStk[gCo_sp] < 0)
        return 0;
  ERR:
    error("Unknown mnemonic.");
    return 0;
}

static void resetStructures(void)
{
    while (structures) {
        STRUCT_DEF *definition = structures;
        STRUCT_FIELD *field = definition->fields;
        structures = definition->next;
        while (field) {
            STRUCT_FIELD *next = field->next;
            free(field);
            field = next;
        }
        free(definition);
    }
    activeStructure = NULL;
    skippedStructure = 0;
}

static STRUCT_DEF *findStructure(char const *name)
{
    STRUCT_DEF *definition = structures;
    for (; definition; definition = definition->next) {
        if (!strcmp(definition->name, name)
            || ((gUpLo_f || (refLabel(definition->name) && refLabel(definition->name)->nocase))
                && !strcasecmp(definition->name, name)))
            return definition;
    }
    return NULL;
}

static int qualifiedName(char *name, char const *prefix, char const *field)
{
    size_t prefixSize = strlen(prefix);
    size_t fieldSize = strlen(field);
    if (prefixSize + fieldSize + 1 > LBLSIZE) {
        error("Structure field name is too long.");
        return 0;
    }
    strcpy(name, prefix);
    name[prefixSize] = '.';
    strcpy(name + prefixSize + 1, field);
    return 1;
}

static void structureSymbol(char const *name, val_t value, val_t size, uint8_t global)
{
    defLabel(name, 1, global);
    gLblPtr->value = value;
    gLblPtr->structureSize = size;
 #ifdef OPT_OA_FILE
    if (gObjct == OB_ASM && gPass == 2 && !activeStructure
        && (global || symbolDeclared(name, gGrp, 1))) {
        oa_putStr(name, 0);
        oa_putStr(":\n", 0);
    }
 #endif
}

static void structureField(char const *name, val_t offset, val_t size)
{
    STRUCT_FIELD *field = activeStructure->fields;
    STRUCT_FIELD *added = NULL;
    char qualified[LBLSIZE + 1];
    if (!*name)
        return;
    for (; field; field = field->next) {
        if (!strcmp(field->name, name)) {
            error("Duplicate structure field.");
            return;
        }
    }
    if (!qualifiedName(qualified, activeStructure->name, name))
        return;
    added = mallocE(sizeof(*added));
    strcpy(added->name, name);
    added->offset = offset;
    added->size = size;
    added->next = activeStructure->fields;
    activeStructure->fields = added;
    structureSymbol(qualified, offset, size, 0);
}

static int structureControl(char const *label, uint8_t global)
{
    uint8_t *saved = gLinPtr;
    char word[LBLSIZE + 1];
    char nested[LBLSIZE + 1];
    size_t n = 0;
    int active = gCoStk[gCo_sp] >= 0;
    int closing = 0;
    val_t size = 0;
    val_t base = 0;
    STRUCT_DEF *definition = NULL;
    STRUCT_FIELD *field = NULL;
    OPTBL_T const *op = NULL;
    skipSpace();
    while (isSymbl(*gLinPtr) && n < LBLSIZE) {
        word[n++] = gUpLo_f ? (char)toupper(*gLinPtr++) : (char)*gLinPtr++;
    }
    word[n] = 0;
    closing = !strcasecmp(word, "ENDSTRUCT") || !strcasecmp(word, "ENDS");
    if (skippedStructure) {
        if (!strcasecmp(word, "STRUCT"))
            ++skippedStructure;
        if (closing)
            --skippedStructure;
        clearAddress();
        return 1;
    }
    if (!strcasecmp(word, "STRUCT")) {
        if (!active) {
            skippedStructure = 1;
            return 1;
        }
        if (activeStructure || !*label || findStructure(label)) {
            error("Missing, duplicate or nested structure name.");
            return 1;
        }
        definition = mallocE(sizeof(*definition));
        memset(definition, 0, sizeof(*definition));
        strcpy(definition->name, label);
        definition->next = structures;
        structures = activeStructure = definition;
        structureSymbol(label, 0, 0, global);
        clearAddress();
        return 1;
    }
    if (closing) {
        if (active && !activeStructure)
            error("ENDSTRUCT without STRUCT.");
        if (active && activeStructure) {
            LBLTBL_T *symbol = refLabel(activeStructure->name);
            if (symbol)
                symbol->structureSize = activeStructure->size;
            activeStructure = NULL;
        }
        clearAddress();
        return 1;
    }
    definition = findStructure(word);
    if (!activeStructure && !definition) {
        gLinPtr = saved;
        return 0;
    }
    if (activeStructure) {
        if (!*word) {
            clearAddress();
            return 1;
        }
        strcpy(nested, word);
        for (n = 0; nested[n]; ++n)
            nested[n] = (char)toupper(*(uint8_t const *)(nested + n));
        op = srchOpTbl(nested);
        if (op && !op->process) {
            gLinPtr = saved;
            return 0;
        }
        if (!active)
            return 1;
        if (definition == activeStructure) {
            error("Recursive structure field.");
            return 1;
        }
        if (definition) {
            size = definition->size;
        } else if (op && op->process == rmb) {
            skipSpace();
            size = invExpr();
            n = op->prefix ? op->prefix : 1;
            if (size < 0 || size > 65535 / (val_t)n) {
                error("Invalid structure field size.");
                return 1;
            }
            size *= (val_t)n;
        } else {
            error("Only reservations and structure fields are allowed in STRUCT.");
            return 1;
        }
        base = activeStructure->size;
        if (size > 65535 - base) {
            error("Structure exceeds address space.");
            return 1;
        }
        structureField(label, base, size);
        if (definition && *label) {
            for (field = definition->fields; field; field = field->next) {
                if (qualifiedName(nested, label, field->name))
                    structureField(nested, base + field->offset, field->size);
            }
        }
        activeStructure->size += size;
        clearAddress();
        return 1;
    }
    base = gCSectSw ? gCSectBase : gLc;
    if (!active)
        return 1;
    if (*label) {
        structureSymbol(label, base, definition->size, global);
        for (field = definition->fields; field; field = field->next) {
            if (qualifiedName(nested, label, field->name))
                structureSymbol(nested, base + field->offset, field->size, global);
        }
    }
    reserveBytes(definition->size, 1);
    return 1;
}

uint8_t *getLine(void)
{
    gLinPtr = (uint8_t*)gLineBuf + LINEHEAD;
    return macroReadLine(gLinPtr, MAXCHAR - LINEHEAD);
}

static uint8_t oneLine(void)
{
    char    temp[LBLSIZE + 1];
    uint8_t c;
    uint8_t f;
    uint8_t gf;
    uint8_t *start = NULL;

    while (getLine() == NULL) {
        fclose(gSrcFp);
        if (popFile() == 0)
            return 2;
    }

    initLine();
    c = *gLinPtr;
    if (remBlock) {
        char word[MNEMOSIZE + 1];
        int  n = 0;
        skipSpace();
        while (isSymbl(*gLinPtr) && n < MNEMOSIZE)
            word[n++] = toupper(*gLinPtr++);
        word[n]       = 0;
        if (strcmp(word, "EREM") == 0)
            remBlock  = 0;
        clearAddress();
        return 0;
    }

    start = gLinPtr;
    skipSpace();
    c = *gLinPtr;
    if (!c || c == '\n')
        macroScopeBoundary();
    if (isCommentChar(c)) {
        if (c == '*') {
            char     word[MNEMOSIZE + 1];
            int      n     = 0;
            int      mode  = -1;
            uint8_t* pragmaStart = ++gLinPtr;
            while (isSymbl(*gLinPtr) && n < MNEMOSIZE)
                word[n++]  = toupper(*gLinPtr++);
            word[n] = 0;
            if      (!strcmp(word, "PRAGMA"))     mode = 1;
            else if (!strcmp(word, "PRAGMAPUSH")) mode = 2;
            else if (!strcmp(word, "PRAGMAPOP"))  mode = 3;

            if (mode >= 0 && (!*gLinPtr || isspace(*gLinPtr))) {
                if (gCoStk[gCo_sp] >= 0)
                    handlePragma(mode);
            } else {
                gLinPtr = pragmaStart;
            }
        }
        clearAddress();
    } else {
        gLinPtr = start;
        c = *gLinPtr;
        gf = temp[0] = '\0';
        if (!isspace(c) && c != '\n')
            gf = getLabel(temp);
        if (structureControl(temp, gf))
            return 0;
        if (macroControl(temp, gf))
            return 0;
        declarationLabel[0] = 0;
        f = getMnemonic();
        if (temp[0] && gCoStk[gCo_sp] >= 0) {
            if (f && gOprPtr->process == symbolDirective) {
                strcpy(declarationLabel, temp);
            } else {
                defLabel(temp, f && !strcmp(gOprPtr->mnemonic, "SET") ? 2 : 1, gf);
             #ifdef OPT_OA_FILE
                if (gObjct == OB_ASM && gPass == 2
                    && (gf || symbolDeclared(temp, gGrp, 1))
                    && gOAStk[gOA_sp].ll != gLineNo) {
                    if (f && gOprPtr->process == equ) {
                        oa_putStr(gLineBuf + LINEHEAD, 0);
                    } else {
                        oa_putStr(temp, 0);
                        oa_putStr(":\n", 0);
                    }
                }
             #endif
            }
        }
        return f;
    }
    return 0;
}

static void initPass(void)
{
    int kind = 0;
    conditionalCursor = 0;
    if (gPass == 1) {
        conditionalCount = 0;
        pragmaIndex0 = gCompatMode == COMPAT_LWASM;
        pragmaForwardMax = gCompatMode == COMPAT_LWASM;
        pragmaShadow = gCompatMode != COMPAT_LWASM;
        pragmaDollarLocal = !gOs9_f;
        for (kind = 0; kind < PRAGMA_KINDS; ++kind)
            pragmaDefaults[kind] = pragmaValue(kind);
    }
    restoringPragmas = 1;
    for (kind = 0; kind < PRAGMA_KINDS; ++kind)
        pragmaSet(kind, pragmaDefaults[kind]);
    restoringPragmas = 0;
    memset(pragmaConfigured, 0, sizeof(pragmaConfigured));
    addressRegion = 0;
    memset(pragmaDepth, 0, sizeof(pragmaDepth));
    macroReset();
    resetStructures();
    initCodePass();
    while (gIncDirCount > gIncDirExtraTop)
        free((void *)gIncDirs[--gIncDirCount]);

    rsCounter = rsDefined = 0;
    soCounter = foCounter = soDefined    = foDefined  = 0;
    remBlock = failSeen = 0;
 #ifdef OPT_OA_FILE
    gOA_sp    =
 #endif
    gSrcLine  =
    branchChanges = relaxChanges =
    gFile_sp  = gLineNo = gErrors  = gP1_sp   = gCo_sp =
    gLc       = gDp     =
    gEOF_f    = gGrp    = (uint8_t) 0;
    if (gVerbos_f)
        fprintf(STDERR, (gPass == -1) ? "<pass 1.5>\n" : "<pass %d>\n", gPass);
}

static void assemble(int argc, char * * argv)
{
    int     i;
    uint8_t f;

    initPass();
    beginCodeOutput(gObjSiz);

    for (i = 1; i < argc; i++) {
        if (*argv[i] == '-')
            continue;

        strncpy(gSrcFName, argv[i], FNAMESZ);
        ++gGrp;
        macroScopeBoundary();
        DEBMSGF( (STDERR, "open %s\n", gSrcFName) );
        gSrcFp = fopenE(gSrcFName, "r");
        if (gVerbos_f)
            fprintf(STDERR, "[%s]\n", gSrcFName);

        while (!gEOF_f) {
            if (( f = oneLine() ) == 2 ) {
                break;
            } else if (f != 0) {
                if (gCSectSw > 1
                   && gOprPtr->process != rmb
                   && strcmp(gOprPtr->mnemonic, "ENDSECT")
                   && !(gCSectSw == 2 && gOprPtr->process == alignData)
                ) {
                    if (gCSectSw == 3) {
                        if (strcmp(gOprPtr->mnemonic, "FDB")
                         && strcmp(gOprPtr->mnemonic, "FCB")
                         && strcmp(gOprPtr->mnemonic, "DC.B")
                         && strcmp(gOprPtr->mnemonic, "DC.W")
                         && strcmp(gOprPtr->mnemonic, "DC.L")
                         && strcmp(gOprPtr->mnemonic, "FCC")
                         && strcmp(gOprPtr->mnemonic, "FCS")
                         && strcmp(gOprPtr->mnemonic, "RZB") ) {
                            error("Instruction is not allowed between VSECT and ENDSECT.");
                        }
                    } else {
                        error("Only RMB is allowed between CSECT and ENDSECT.");
                    }
                }

             #ifdef OPT_OA_FILE
                if (gObjct == OB_ASM && gPass == 2 && gOAStk[gOA_sp].ll == gLineNo) {
                    DEBMSGF((STDERR,"OPT_OA_FILE#2:%d line=%d size=%d / gLineNo=%d\n", gOA_sp, gOAStk[gOA_sp].ll, gOAStk[gOA_sp].nn, gLineNo) );
                    oa_putStr(gLineBuf + LINEHEAD, gOAStk[gOA_sp++].nn);
                } else if (gPass == -2) {
                    uint16_t bb;
                    bb         = gObjCnt;
                    gOAchk_f   = 0;
                    gOprPtr->process();
                    if (gOAchk_f) {
                        gOAStk[gOA_sp].ll  = gLineNo;
                        gOAStk[gOA_sp].nn  = gObjCnt - bb;
                        DEBMSGF( (STDERR, "OPT_OA_FILE#-2:line=%d  size=%d\n",
                                  gOAStk[gOA_sp].ll, gOAStk[gOA_sp].nn) );
                        if (++gOA_sp >= OA_MAX)
                            error("Too many unassembled lines for the -a option.");
                    }
                } else
             #endif
                {
                    gOprPtr->process();
                }
                if (*gLinPtr != '\n' && !isspace(*gLinPtr) && !isCommentChar(*gLinPtr)) {
                    error("Unexpected character.");
                    DEBMSGF((STDERR, "*gLinPtr : %c(%02x)\t[asemmble()]\n"
                                   , *gLinPtr, *gLinPtr));
                }
            }
            finishLine();
            putLine();
        }
    }

    endCodeOutput();
    checkSymbolDeclarations();
    macroFinish();
    if (activeStructure || skippedStructure)
        error("STRUCT without ENDSTRUCT.");
    if (remBlock)
        error("REM without EREM.");
    endCodeSection();
    gObjSiz = gObjCnt;
}


/*---------------------------------------------------------------------------*/

static uint16_t xstrtoui(uint8_t const *p, uint8_t const **q)
{
    uint16_t w = 0;

    for (; isxdigit(*p); p++) {
        w = w * 16 +
            ( isdigit(*p) ? (*p - '0') :
             (toupper(*p) - 'A' + 10) );
    }
    if (q)
        *q = p;
    return w;
}

static void getModNam(char *modnam, char const *fnam)
{
    char const *base = FIL_BaseName(fnam);
    char const *end  = strrchr(base, '.');
    size_t length = end ? (size_t)(end - base) : strlen(base);

    if (length > MODNAMSZ)
        length = MODNAMSZ;
    memcpy(modnam, base, length);
    modnam[length] = '\0';
}


/*---------------------------------------------------------------------------*/
static char const *oLstFName, *oObjFName;
static uint8_t     oList_f  ,  oSymbol_f;

static void printLog(void)
{
    if (gList > 0) {
        fprintf(gLstFp, "\n    Total Errors %d\n", gErrors);
        if (gVerbos_f)
            fprintf(gLstFp, "    Total labels %d\n", gLabels);
    }
    fprintf(STDERR, "\n    Obj_Size = $%04x(%d)\n", gObjCnt, gObjCnt);
    fprintf(STDERR, "    Total Errors %d\n", gErrors);
    if (gVerbos_f)
        fprintf(STDERR, "    Total labels %d\n", gLabels);
    if (pragmaDefaults[7] || branchChanges)
        fprintf(STDERR, "    Total Optimized Branch %d\n", branchChanges);
}

static void usage(void)
{
    fprintf(STDERR, "usage: %s [-opts] src_file...\n", gCmdName);
    e_puts(" -?     Show this help\n");
    e_puts(" -3     6309 mode (default)\n");
    e_puts(" -8     6809 mode\n");
    e_puts(" -9     OS-9 standard ASM mode\n");
    e_puts(" -6     Enable M6800-family mnemonic compatibility\n");
    e_puts(" -z     Enable undocumented 6809/6309 opcodes/operands\n");
    e_puts(" -p     Selectable 0, 5, 8, or 16-bit offsets.\n");
    e_puts(" -y     Enable automatic branch sizing\n");
    e_puts(" -q     Allow address gaps caused by ORG or RMB\n");
    e_puts(" -u     Case-sensitive labels    -n  Case-insensitive labels\n");
    e_puts(" -s     Show symbol table        -v  Show progress\n");
    e_puts(" -j     use SJIS character.\n");
    e_puts(" --allmp       Allow 35 macro arguments (default: 9)\n");
    e_puts(" -m<mod_name>  Set the $modnam string variable\n");
    e_puts(" -d<L>[=Val]   Define L as Val (default: 1)\n");
    e_puts(" -t=<ASM>      Assembler compatibility mode: as63 lwasm vasm\n");
    e_puts(" -o[=FILE]     Write binary object to FILE\n");
    e_puts(" -f[=FILE]     Write S-Record object to FILE\n");
    e_puts(" -x[=FILE]     Write a FLEX binary executable to FILE\n");
 #ifdef OPT_OA_FILE
    e_puts(" -a[=FILE]     Write object as FCB data to FILE\n");
 #endif
    e_puts(" -e[=FILE]     Write source errors to FILE\n");
    e_puts(" -i[=INC_DIR]  Add an include directory (repeatable; searched in order)\n");
    e_puts(" -l[=LST_FILE] Write assembly listing to LIST_FILE\n");
 #ifdef OPT_FBAS
    e_puts(" -k[Start[,Enter]]  Write an F-BASIC machine-language file\n");
    e_puts(" -r            In F-BASIC format, omit trailing zeros after RMB\n");
 #endif
 #ifdef DEBUG
    e_puts(" -c  Debug mode\n");
 #endif
    DEBMSGF( (STDERR, "Extended instructions enabled (D,Q)\n") );
    exit(0);
}

static void optsDefLbl(int argc, char * * argv)
{
    char        temp[LBLSIZE + 1];
    char const *p;
    int         i;

    for (i = 1; i < argc; ++i) {
        p              = argv[i];
        if (*p++ != '-')
            continue;
        if (*p++ != 'd' && *(p - 1) != 'D')
            continue;
        if (*p == '\0') {
            e_puts("No label name specified for -d.\n");
            continue;
        }
        gLinPtr        = (uint8_t*)strncpy(gLineBuf+LINEHEAD, p, MAXCHAR - LINEHEAD - 2);
        getLabel(temp);
        defLabel(temp, 2, 1); /* set,global label */
        gLblPtr->line  = 0;
        gLblPtr->grp   = 0;
        if (*gLinPtr++ == '=') {
            if (*gLinPtr == '$') {
                gLinPtr++;
                gLblPtr->value = (int) xstrtoui(gLinPtr, NULL);
            } else {
                gLblPtr->value = atoi((char const*)gLinPtr);
            }
        } else {
            gLblPtr->value = 1;
        }
        DEBMSGF((STDERR, "-d opts : %s  (temp %s) : %s = $%x\n"
                       , p, temp, gLblPtr->name, gLblPtr->value));
    }
}

static void options(char const *p)
{
    uint8_t const* pp;
    uint8_t     c;

    while ( (c = *(uint8_t const*)p) != '\0') {
        if (*++p == '=')
            ++p;

        switch ( toupper(c)) {
     #ifdef DEBUG
        case 'C':
            gDebug_f   = 1;
            break;
     #endif
        case 'P':
            gIdxOfs_f  = 1;
            break;

        case '?':
        case 'H':
            usage();

        case 'D':
            goto LOOPOUT;

        case 'Q':
            gOrgSFmt_f = 1;
            break;

        case 'U':
            gUpLo_f    = 0;
            break;

        case 'N':
            gUpLo_f    = 1;
            break;

        case 'V':
            gVerbos_f  = 1;
            break;

        case 'Z':
            gUndoc_f   = 1;
            break;

        case '3':
            gM6809_f   = 0;
            break;

        case '8':
            gM6809_f   = 1;
            break;

        case '6':
            gM6800_f   = 1;
            break;

        case '9':
            gOs9_f     = gUpLo_f = 1;
            break;

        case 'S':
            oSymbol_f  = 1;
            break;

        case 'J':
            gSjis_f    = 1;
            break;

        case 'T':
            if (!strcasecmp(p, "lwasm")) {
                gCompatMode = COMPAT_LWASM;
            } else if (!strcasecmp(p, "vasm")) {
                gCompatMode = COMPAT_VASM;
            } else if (!strcasecmp(p, "as63")) {
                gCompatMode = COMPAT_AS63;
            } else {
                e_puts("Unknown compatibility mode. Use -t=as63, -t=lwasm or -t=vasm.\n");
                exit(1);
            }
            goto LOOPOUT;

        case 'L':
            if (*p) {
                oLstFName  = p;
                oList_f    = 1;
            } else {
                oList_f = 0;
            }
            gList      = 1;
            goto LOOPOUT;

        case 'M':
            if (*p) {
                strncpy(gModName, p, MODNAMSZ);
            }
            goto LOOPOUT;

        case 'E':
            if (*p)
                gErrFName = p;
            else
                gErrFName = (char const *) (~0);
            goto LOOPOUT;

     #ifdef OPT_OA_FILE
        case 'A':
            if ((gOAStk = (OATBL_T *)calloc(OA_MAX,sizeof(OATBL_T))) == NULL) {
                e_puts("Not enough memory for the -a option.\n");
                break;
            }
            gFlex_f        = 0;
            gObjBufSz      = 16;
            gObjct         = OB_ASM;
            goto OB;
     #endif

        case 'O':
            gFlex_f        = 0;
            gObjct         = OB_BIN;
            goto OB;

        case 'F':
            gFlex_f        = 0;
            gObjct         = OB_SFMT;
            goto OB;

        case 'X':
            gFlex_f        = 1;
         #ifdef OPT_FBAS
            gFBasic_f      = 0;
         #endif
            gObjct         = OB_BIN;
            goto OB;
       OB:
            if (*p)
                oObjFName  = strdupAddE(p, 4);
            goto LOOPOUT;

        case 'Y':
            pragmaAutoBranch = 1;
            break;

     #ifdef OPT_FBAS
        case 'K':
            gFBasic_f      = 1;
            gFlex_f        = 0;
            gRmb_sp        = 0;
            if (*p) {
                gStartAddr = xstrtoui((uint8_t const*)p, &pp);
                p          = (char const*)pp;
                if (gStartAddr == 0)
                    gStartAddr = 0xFFFF;
                if (*p == ',') {
                    gEntryAddr = xstrtoui((uint8_t const*)p + 1, &pp);
                    /*p = pp;*/
                }
                if (gEntryAddr == 0)
                    gEntryAddr = 0xFFFF;
            }
            goto LOOPOUT;
        case 'R':
            gRmb_f         = 1;
            break;
     #endif

        case 'I':
            if (!*p)
                p = ".";
            if (strlen(p) >= FNAMESZ - 10) {
                e_puts("File name is too long.\n");
                exit(1);
            }
            if (gIncDirCount == EXTRA_INCDIRS) {
                e_puts("Too many include directories for -i.\n");
                exit(1);
            }
            gIncDirs[gIncDirCount++] = p;
            goto LOOPOUT;

        default:
            fprintf(STDERR, "%s: Invalid option (-%c).\n", gCmdName, c);
            exit(1);
        }
    }
  LOOPOUT:;
}

int main(int argc, char *argv[])
{
    static char const title[] = AS63_TITLE;
    char const *p;
    int             i;
    int relaxationPasses = 0;

    gCmdName       = argv[0];
    gObjBufSz      = OBJSIZE;
    gEntryAddr     = gStartAddr = 0xFFFF;
    gObjFp         = NULL;
    gLstFp         = stdout;
    gErrFp         = STDERR;
    gErrFName      = oLstFName = oObjFName = NULL;
    oList_f        = gModName[0] = gSrcFName[0] = gSrcFName[FNAMESZ] = '\0';
    gUpLo_f        = 1;

    e_puts(title);
    for (i = 1; i < argc; i++) {
        p = argv[i];
        if (*p != '-') {
            if (gSrcFName[0] == '\0')
                strncpy(gSrcFName, p, FNAMESZ);
        } else {
            if (*++p == '\0')
                usage();
            if (!strcasecmp(p, "-allmp"))
                gAllMacroParams = 1;
            else
                options(p);
        }
    }

    if (*gSrcFName == '\0') {
        fprintf(STDERR, "usage: %s [-opts] src_file...(-? help)\n", gCmdName);
        return 1;
    }
    if (!oObjFName && gObjct) {
        char const* ext = (gObjct == OB_SFMT) ? ".s19"
                      #ifdef OPT_OA_FILE
                        : (gObjct == OB_ASM) ? ".oa"
                      #endif
                        : gFlex_f ? ".cmd"
                        : ".bin";
        oObjFName = strdupAddE(gSrcFName, 4);
        FIL_AddChgExt(oObjFName, ext, 1);
    }
    if (gErrFName == (char *) (~0)) {
        gErrFName = FIL_AddChgExt(strdupAddE(gSrcFName, 4), ".err", 1);
    }
    if (*gModName == '\0') {
        getModNam(gModName, gSrcFName);
    }

    for (i = 0; i < gIncDirCount; ++i) {
        if (!strcmp(gIncDirs[i], "."))
            break;
    }
    if (i == gIncDirCount)
        gIncDirs[gIncDirCount++] = ".";
    gIncDirExtraTop = gIncDirCount;

    DEBMSGF( (STDERR, "gModName = %s\n", gModName) );
    initNode();
    initOpTbl();
    optsDefLbl(argc, argv);

    gPass  = 1;
    DEBMSGF( (STDERR, "enter pass 1\n") );
    assemble(argc, argv);

    if (relaxRequested) {
        gPass = -1;
        DEBMSGF( (STDERR, "enter pass 1.5\n") );
        do {
            assemble(argc, argv);
            if (relaxRequested && ++relaxationPasses >= 1024 && relaxChanges) {
                relaxFailed = 1;
                break;
            }
        } while (relaxChanges);
    }

 #ifdef OPT_OA_FILE
    if (gObjct == OB_ASM) {
        gPass = -2;
        DEBMSGF( (STDERR, "enter pass 1.9\n") );
        assemble(argc, argv);
    }
 #endif

    gPass  = 2;
    DEBMSGF( (STDERR, "enter pass 2\n") );
    if (gList || oSymbol_f) {
        if (oList_f) {
            DEBMSGF( (STDERR, "open %s\n", oLstFName) );
            gLstFp = fopenE(oLstFName, "w");
        } else {
            gLstFp = stdout;
        }
    }
    if (gObjct) {
        DEBMSGF( (STDERR, "open %s\n", oObjFName) );
        gObjFp = fopenE(oObjFName, (gObjct == OB_BIN) ? "wb" : "w");
    }

    assemble(argc, argv);
    if (relaxFailed) {
        error("Instruction sizes did not converge.");
        failSeen = 1;
    }

    if (gObjct) {
        termObj();
     #if 0 /*def OS9*/
        if (gObjct == OB_BIN) {
            chmod(oObjFName, 0x07 /*S_EXEC|S_IWRITE|S_IREAD*/);
        }
     #endif
    }
    if (oSymbol_f)
        dumpSymbol();
    printLog();

    DEBMSGF( (STDERR, "close all files\n") );
    if (gObjct) {
        fclose(gObjFp);
        if (failSeen)
            remove(oObjFName);
    }
    if (oList_f)
        fclose(gLstFp);
    if (gErrFName == NULL)
        fclose(gErrFp);

    return (gErrors ? 1 : 0);
}
