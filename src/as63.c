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
#define EXT
#include    "as63.h"

#define AS63_TITLE      "HD6309 cross assembler version 01.46T\n"

#ifdef _MSC_VER
 #define strcasecmp     _stricmp
 #define ITOA10(i,a)    _itoa( (i), (a), 10 )
#elif defined _WIN32
 #define strcasecmp     stricmp
 #define ITOA10(i,a)    snprintf( (a), sizeof (a), "%d", (i) )
#else
 #define ITOA10(i,a)    snprintf( (a), sizeof (a), "%d", (i) )
#endif


/*--------------------------------------------------------------------------*/

#define IS_KANJI(c)  ( (unsigned) ( (c) ^ 0x20 ) - 0xa1U < 0x3cU )
//#define isKanji2(c) (c >= 0x40 && c <= 0xfc && (c) != 0x7f)

int     isKanji(int c)
{
    return gSjis_f && IS_KANJI(c);
}


FILE    *fopenE(char const * fname, char const * atr)
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

char const *FIL_BaseName(char const *adr)
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

char    *FIL_ChgExt(char * filename, char const * ext)
{
    char *p = strrchr((char*)FIL_BaseName(filename), '.');

    if (p == NULL) {
        strcat(filename, ".");
        strcat( filename, ext);
    } else {
        strcpy(p + 1, ext);
    }
    return filename;
}


/*---------------------------------------------------------------------------*/

typedef struct line_state {
    int     address;
    int     size;
    int     branchSize;
    int     branchLong;
} LINE_STATE;

static LINE_STATE*  lineStates;
static size_t       lineCapacity;
static int          relaxRequested;
static int          relaxChanges;
static int          branchChanges;
static int          relaxFailed;
static int          addressRegion;
static int          expressionForward;
static int          expressionFixedForward;
static int          expressionLiteralZero;
static int          expressionDepth;
static int          lineCode;
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

void finishLine(void)
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

static uint16_t oChkSum;    /* Checksum for each line of S-format */

static uint8_t hexDigit(uint8_t x)
{
    return ( (x &= 0x0f) < 10 ) ? x + '0' : x - 10 + 'A';
}

static void put2hex(int b)
{
    if (!gObjct)
        return;
    b          = (uint8_t) b;
    putc(hexDigit(b >> 4), gObjFp);
    putc(hexDigit(b), gObjFp);
    oChkSum   += b;
}

static void put4hex(int w)
{
    w = (uint16_t) w;
    put2hex(w >> 8);
    put2hex(w);
}

void    flushObj(void)
{
    int i;

    if (gPass == 2 && gObjct && gObjPos > 0) {
        if (gObjct == OB_SFMT) {
            oChkSum = 0;
            fputs("S1", gObjFp);
            put2hex(gObjPos + 3);
            put4hex(gObjLc);
            for (i = 0; i < gObjPos; i++)
                put2hex(gObjBuf[i]);
            put2hex(~oChkSum);
            putc('\n', gObjFp);
        }
        else if (gFlex_f) {
            putc(0x02, gObjFp);
            putc(gObjLc >> 8, gObjFp);
            putc(gObjLc , gObjFp);
            putc(gObjPos, gObjFp);
            fwrite(gObjBuf, gObjPos, 1, gObjFp);
        }
     #ifdef OPT_OA_FILE
        else if (gObjct == OB_ASM && gObjPos) {
            fprintf(gObjFp, "\tfcb $%02x", gObjBuf[0]);
            for (i = 1; i < gObjPos; i++)
                fprintf(gObjFp, ",$%02x", gObjBuf[i]);
            putc('\n', gObjFp);
        }
     #endif
        else {
            fwrite(gObjBuf, gObjPos, 1, gObjFp);
        }
    }
    gObjPos    = 0;
    gObjLc     = gLc;
}

void    termObj(void)
{
    flushObj();
    if (gObjct == OB_SFMT) {
        fputs("S903", gObjFp);
        oChkSum = 3;
        put4hex(gEntryAddr);
        put2hex(~oChkSum);
        putc('\n', gObjFp);
    } else if (gFlex_f) {
        putc(0x16, gObjFp);
        putc(gEntryAddr >> 8, gObjFp);
        putc(gEntryAddr, gObjFp);
    }
}

static void crc(int b)
{
    int w;

    b              = (uint8_t) b;
    w              = b ^ gCrcBuf[0];
    gCrcBuf[0]     =  gCrcBuf[1];
    gCrcBuf[1]     =  gCrcBuf[2];
    gCrcBuf[1]    ^= w >> 7;
    gCrcBuf[2]     =  (uint8_t) (w << 1);
    gCrcBuf[1]    ^= w >> 2;
    gCrcBuf[2]    ^= w << 6;
    w             ^= w << 1;
    w             ^= w << 2;
    w             ^= w << 4;
    if (w & 0x80) {
        gCrcBuf[0]    ^= 0x80;
        gCrcBuf[2]    ^= 0x21;
    }
}

void    putObj(int b)
{
    if (gPass != 2)
        return;
    if (gObjPos >= gObjBufSz)
        flushObj();
    gObjBuf[gObjPos++] = (uint8_t) b;
}

void    put2obj(int w)
{
    w = (uint16_t) w;
    putObj(w >> 8);
    putObj(w);
}

uint8_t    putB(int b)
{
    lineCode = 1;
    if (offsetActive) {
        ++gLc;
        return (uint8_t)b;
    }
    if (gRmb_f && gFBasic_f && gObjct == OB_BIN && gRmb_sp) {
        while (gRmb_sp-- > 0)
            putObj(0);
        gRmb_sp++;
    }
    b = (uint8_t) b;
    putObj(b);
    ++gObjCnt;
    ++gLc;
    if (gOs9_f && gPass == 2)
        crc(b);
    return (uint8_t) b;
}

int     put2B(uint16_t w)
{
    putB( (uint8_t) (w >> 8) );
    putB( (uint8_t) w );
    return w;
}

#ifdef OPT_OA_FILE
void    oa_putStr(char const * s, int n)
{
    if (gPass != 2)
        return;
    flushObj();
    fputs(s, gObjFp);
    gObjCnt   += n;
    gLc       += n;
}
#endif


/*---------------------------------------------------------------------------*/

static int oPostf, oPos;

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

void    putByte(int b)
{
    b = (uint8_t) b;
    printByte(putB(b), oPostf);
}

void    postByte(int b)
{
    printByte(putB(b), oPostf);
    oPostf += 3;
}

void    putWord(int w)
{
    printWord(put2B(w), oPostf);
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

void    printChar(int c, int p)
{
    gLineBuf[p] = (uint8_t) c;
}

void    initLine(void)
{
    char *p;

    gLblPtr    = NULL;
    lineCode   = 0;
    oPostf     = 15;
    oPos       = 10;
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
    if (gPass == 2 && gList > 0 && !pragmaNoListCode && (!pragmaNoList || lineCode)) {
        fputs(gLineBuf, gLstFp);
    }
}

static void flushLine(void)
{
    char *p;

    putLine();
    for (p = gLineBuf; p < gLineBuf + LINEHEAD; p++)
        *p = ' ';
    *p++   = '\n';
    *p     = '\0';
    printAddress(gLc);
    oPos   = 10;
}

void    put1Byte(int b)
{
    if ((23 - 3) < oPos )
        flushLine();
    printByte(putB(b), oPos);
    oPos += 3;
}

void    put1Byt2(int b)
{
    if (23 < oPos)
        flushLine();
    printByte(putB(b), oPos);
    oPos += 2;
}

void    put1Word(int w)
{
    if (23 - 5 < oPos)
        flushLine();
    printWord(put2B(w), oPos);
    oPos += 5;
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
        fprintf(gLstFp, "%15s %4d %04x", lp->name, lp->line, lp->value);
        fprintf(gLstFp, oLrf++ % 3 ? " " : "\n");
    }
    printNode(lp->left);
}

void    dumpSymbol(void)
{
    if (gList > 0)
        fprintf(gLstFp, "\n");
    oLrf = 1;
    printNode(oLabel->left);
    if (oLrf % 3)
        fprintf(gLstFp, "\n");
}

LBLTBL_T *getNode(void)
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

void    initNode(void)
{
    oLabel             = getNode();
    oLabel->name[0]    = '\0';
    oLabel->right      = oLabel->left = NULL;
}

void    defLabel(char const * temp, uint8_t f, uint8_t gf)
{
    LBLTBL_T *  lp = oLabel;
    int         i;

    for (;;) {
        if (( i = strcmp(temp, lp->name) ) == 0) {
            if (lp->grp == 0 || lp->grp == gGrp) {
                if (lp->line != gLineNo && (f == 1 || lp->flg == 1) )
                    errLbl("Duplicate label definition", temp);
                (gLblPtr = lp)->value = gLinLc;
                if (f) {
                    lp->hidden = pragmaNoList || pragmaNoListCode;
                    lp->region = addressRegion;
                }
                if (lp->flg == 0) {
                    lp->flg    = f;
                    lp->line   = gLineNo;
                    if (gf)
                        lp->grp = 0;
                    else
                        lp->grp = gGrp;
                }
                return;
            } else {
                i = (gGrp > lp->grp) ? 1 : -1;
            }
        }
        if (i < 0) {
            if (lp->right != NULL) {
                lp = lp->right;
            } else {
                lp->right  = getNode();
                lp         = lp->right;
                break;
            }
        } else {
            if (lp->left != NULL) {
                lp = lp->left;
            } else {
                lp->left   = getNode();
                lp         = lp->left;
                break;
            }
        }
    }
    if (lp == NULL)
        errPrg("defLabel()");
    gLabels++;
    gLblPtr    = lp;
    lp->value  = gLinLc;
    if (gf)
        lp->grp = 0;
    else
        lp->grp = gGrp;
    lp->hidden = f && (pragmaNoList || pragmaNoListCode);
    lp->region = addressRegion;
    lp->flg    = f;
    if (f)
        lp->line = gLineNo;
    else
        lp->line = 0x7fff;
    strcpy(lp->name, temp);
    lp->right  = lp->left = NULL;
    return;
}

static LBLTBL_T *refLbl0(char const * lbl)
{
    LBLTBL_T *  lp = oLabel;
    int         i;

    while (lp != NULL) {
        if (( i = strcmp(lbl, lp->name) ) == 0) {
            if (lp->grp == 0 || lp->grp == gGrp)
                break;
            i = (gGrp > lp->grp) ? 1 : -1;
        }
        lp = (i < 0) ? (lp->right) : (lp->left);
    }
    return lp;
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

int     isSymbl2(int c)
{
    c = (uint8_t) c;
    return ( isalpha(c) || (c == '_') || (c == '.') );
}

int     isSymbl3(int c)
{
    c = (uint8_t) c;
    return (isalnum(c) || c == '_' || c == '.' || c == '@' || c == '$');
}

uint8_t    getLabel(char * buf)
{
    uint8_t *   p;
    uint8_t     gf = 0;

    if (!isSymbl2(*gLinPtr) )
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
        gf = 1;
    }
    *p = '\0';
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
val_t       expression(void);

static int compatibleNumber(val_t *result)
{
    uint8_t*    start = gLinPtr;
    uint8_t*    end   = start;
    unsigned long value = 0;
    int base   = 10;
    int digit  = 0;
    int prefix = 0;
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
        if (isKanji(*gLinPtr) )
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
    if (isSymbl2(c)) {
        getLabel(temp);
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
            defLabel(temp, 0, 1);
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

void    imm4Expr(int16_t * v1, int16_t * v2)
{
    val_t val;

    val = expression();
    if (checkChar(',')) {
        *v1    = (int16_t) val;
        *v2    = (int16_t) expression();
    } else {
        *v1    = (int16_t) (val >> 16);
        *v2    = (int16_t) val;
    }
}


/*---------------------------------------------------------------------------*/

void    putCode(int grp, int mode)
{
    /* addressing mode offset table 'gOffset[group][mode]' */
    static int8_t const gOffset[4][4] =  {
        { 0x00, 0x00, 0x00, 0             },
        { 0x00, 0x00, 0x60, 0x70          },
        { 0x00, 0x10, 0x20, 0x30          },
        { 0x00, 0x01, 2,    3             }
    };

    if (gOprPtr->prefix) {
        printByte(putB(gOprPtr->prefix), 10);
        printByte(putB(gOprPtr->opcode + gOffset[grp][mode]), 12);
    } else {
        printByte(putB(gOprPtr->opcode + gOffset[grp][mode]), 10);
        if (gImVal != 512)
            postByte(gImVal);
    }
}

int     getReg(int r)
{
    int         reg = 0;
    uint8_t     c;
    uint8_t     d;
    uint8_t*    l_p = gLinPtr;
    uint8_t     b   = toupper(*gLinPtr);
    gLinPtr++;
    c      = toupper(*gLinPtr);
    if (!isSymbl3(c)) {
        switch (b) {
        case 'A':   reg = A;    break;
        case 'B':   reg = B;    break;
        case 'D':   reg = D;    break;
        case 'X':   reg = X;    break;
        case 'Y':   reg = Y;    break;
        case 'U':   reg = U;    break;
        case 'S':   reg = S;    break;
        case 'W':   reg = W;    break;
        case 'E':   reg = E;    break;
        case 'F':   reg = F;    break;
        case 'V':   reg = V;    break;
        case 'N':   reg = N;    break;
        }
    } else {
        ++gLinPtr;
        d = toupper(*gLinPtr);
        if (!isSymbl3(d)) {
            if (c == 'C') {
                if (b == 'C')
                    reg = CC;
                else if (b == 'P')
                    reg = PC;
            } else if (b == 'D' && c == 'P') {
                reg = DP;
            }
        } else if (b == 'P' && c == 'C' && d == 'R' && !isSymbl3( *(gLinPtr + 1) )) {
            ++gLinPtr;
            reg = PCR;
        } else {
            while ( isSymbl3(*gLinPtr) )
                ++gLinPtr;
        }
    }
    if (gM6809_f && (reg & X63REG)) {
        if (reg == V) {
            if (!gUndoc_f)
                warning("[WARNING] Register V used in 6809 mode.");
        } else
        {
            error("Register E, F, W, or V used in 6809 mode.");
        }
    }
    if (reg == PC && (r & PCR) && (pragmaPcAsPcr || gCompatMode == COMPAT_VASM))
        reg = PCR;
    if (r & reg)
        return reg;
    if (r == OFFSETRG)
        gLinPtr = l_p;
    else
        error("Invalid register.");
    return 0;
}

int     regNo(int r)
{
    switch ( getReg(r)) {
    case D : return 0;
    case X : return 1;
    case Y : return 2;
    case U : return 3;
    case S : return 4;
    case PC: return 5;
    case W : return 6;
    case V : return 7;
    case A : return 8;
    case B : return 9;
    case CC: return 10;
    case DP: return 11;
    case N : return 12;
    case E : return 14;
    case F : return 15;
    default:
        error("Unknown register.");
        return -1;
    }
}


/*---------------------------------------------------------------------------*/

int     checkByte(int b)
{
    return ( gByte_f || (-128 <= b && b <= 127 && gValid_f && !gWord_f) );
}

int     index0(int frame, int reg)
{
    int xr;

    switch (reg) {
    case X:
        xr = 0x00;
        break;
    case Y:
        xr = 0x20;
        break;
    case U:
        xr = 0x40;
        break;
    case S:
        xr = 0x60;
        break;
    case W:
        switch (frame) {
        case 0x83:              /* ,--W */
            frame  = 0xef;
            break;
        case 0x81:              /* ,W++ */
            frame  = 0xcf;
            break;
        case 0x84:              /* ,W */
            frame  = 0x8f;
            break;
        case 0x89:              /* nnnn,W */
            frame  = 0xaf;
            break;
        default:
            error("Invalid indexed addressing mode.");
        }
        return ( frame ^ (gIndirect ? 0x1f : 0) );
    default:
        xr = 0;
    }
    return ( frame | xr | (gIndirect ? 0x10 : 0) );
}


static void indexM(int frame, int reg)
{
    postByte( index0(frame, reg) );
}

static int indexSmallAllowed(void)
{
    return (!gIdxOfs_f && gCompatMode != COMPAT_LWASM && !pragmaConfigured[5])
        || (!gByte_f && !gWord_f);
}

static int indexZeroAllowed(val_t value, int reg)
{
    if (!gValid_f || value != 0)
        return 0;
    if (gCompatMode == COMPAT_LWASM || pragmaConfigured[5]) {
        return !gByte_f && !gWord_f
            && (reg == W || (!gIdxOfs_f && pragmaIndex0 && !expressionLiteralZero));
    }
    return (!gIdxOfs_f || reg == W) && indexSmallAllowed();
}

static void operand(int grp, int mode)
{
    val_t   val;
    int     reg;

    skipSpace();
    if (( mode & (IMMEDIATE | IMMEDIATE2) ) && checkChar('#')) {
        putCode(grp, IMMEDIATE_MODE);
        if (mode & IMMEDIATE)
            putByte( bytExpr() );
        else
            putWord( expression() );
        return;
    }
    gIndirect = checkChar('[');
    if ((mode & INDEX) && checkChar(',')) {
        putCode(grp, INDEX_MODE);
        if (checkChar('-')) {
            if (checkChar('-') )
                indexM( 0x83, getReg(INDEXREG | W) );
            else if (gIndirect)
                error("[,-R] is not allowed.");
            else
                indexM( 0x82, getReg(INDEXREG) );
        } else {
            reg = getReg(INDEXREG | W);
            if (checkChar('+')) {
                if (checkChar('+') )
                    indexM(0x81, reg);
                else if (gIndirect)
                    error("[,R+] is not allowed.");
                else if (reg == W)
                    error(",W+ is not allowed.");
                else
                    indexM(0x80, reg);
            } else {
                indexM(0x84, reg);
            }
        }
    } else if ((mode & INDEX) && ( reg = getReg(OFFSETRG) ) > 0) {
        if (checkCh_e(',')) {
            putCode(grp, INDEX_MODE);
            switch (reg) {
            case A:
                val    = 0x86;
                break;
            case B:
                val    = 0x85;
                break;
            case D:
                val    = 0x8b;
                break;
            case E:
                val    = 0x87;
                break;
            case F:
                val    = 0x8a;
                break;
            case W:
                val    = 0x8e;
                break;
            default:
                //error("Invalid register.");
                val    = 0x86;
                break;
            }
            indexM( val, getReg(INDEXREG) );
        }
    } else {
        val = expression();
        if ((mode & INDEX) && checkChar(',')) {
            putCode(grp, INDEX_MODE);
            switch ( (reg = getReg(INDEXREG | W | PC | PCR)) ) {
            case X:
            case Y:
            case U:
            case S:
            case W:
                if (indexZeroAllowed(val, reg) ) {
                    indexM(0x84, reg);
                } else if (gValid_f && -16 <= val && val <= 15
                          && indexSmallAllowed()
                          && !gIndirect && reg != W)
                {
                    indexM(val & 0x1f, reg);
                } else if (checkByte(val)) {
                    indexM(0x88, reg);
                    putByte(val);
                } else {
                    indexM(0x89, reg);
                    putWord(val);
                }
                break;
            case PC:
                if (checkByte(val)) {
                    indexM(0x8c, 0);
                    putByte(val);
                } else {
                    indexM(0x8d, 0);
                    putWord(val);
                }
                break;
            case PCR:
                if (checkByte( val -= gLinLc + 3 + (gOprPtr->prefix ? 1 : 0) )) {
                    indexM(0x8c, 0);
                    putByte(val);
                } else {
                    indexM(0x8d, 0);
                    putWord(val - 1);
                }
            }
        } else if ((mode & INDEX) && gIndirect) {
            putCode(grp, INDEX_MODE);
            postByte(0x9f);
            putWord(val);
        } else if ((mode & DIRECT)
            && ( gByte_f || ( (uint16_t)( val - (gDp << 8) ) <= 255 && gValid_f && !gWord_f ) ) )
        {
            putCode(grp, DIRECT_MODE);
            putByte( val - (gDp << 8) );
        } else if (mode & EXTEND) {
            putCode(grp, EXTEND_MODE);
            putWord(val);
        } else {
            error("Invalid addressing mode.");
        }
    }
    if (gIndirect)
        checkCh_e(']');
}


/*---------------------------------------------------------------------------*/

void    load(void)
{
    operand(GROUP2, LOAD);
}

void    load2(void)
{
    operand(GROUP2, LOAD2);
}

void    store(void)
{
    operand(GROUP2, STORE);
}

void    memory(void)
{
    operand(GROUP1, MEMORY);
}

void    lea(void)
{
    operand(GROUP0, INDEX);
}

void    ccr(void)
{
    operand(GROUP0, IMMEDIATE);
}

void bitTransfer(void)
{
    /* reg[7:6], src_bit[5:3], dst_bit[2:0]. */
    int   reg;
    val_t source;
    val_t destination;
    val_t address;
    val_t offset;

    skipSpace();
    reg = getReg(CC | A | B);
    if (reg && reg != CC && reg != A && reg != B)
        error("Invalid register for bit operation; use CC, A, or B.");
    skipSpace();
    if (!checkCh_e(','))
        return;
    skipSpace();
    source = expression();
    if ((gValid_f || gPass == 2) && (source < 0 || source > 7))
        error("Source bit must be 0..7.");
    skipSpace();
    if (!checkCh_e(','))
        return;
    skipSpace();
    destination = expression();
    if ((gValid_f || gPass == 2) && (destination < 0 || destination > 7))
        error("Destination bit must be 0..7.");
    skipSpace();
    if (!checkCh_e(','))
        return;
    skipSpace();
    address = expression();
    offset  = address - (gDp << 8);
    if (gWord_f)
        error("Bit operations require direct addressing.");
    else if (!gByte_f && (gValid_f || gPass == 2) && (offset < 0 || offset > 255))
        error("Bit operation address is outside the direct page.");
    putCode(GROUP0, NO_MODE);
    postByte( (reg == A ? 0x40 : reg == B ? 0x80 : 0)
             |((source & 7) << 3) | (destination & 7) );
    putByte(offset);
}

void undoc_imm8(void)
{
    skipSpace();
    if (!checkCh_e('#') )
        return;
    putCode(GROUP0, NO_MODE);
    putByte( bytExpr() );
}

void undoc_imm16(void)
{
    skipSpace();
    if (!checkCh_e('#') )
        return;
    putCode(GROUP0, NO_MODE);
    putWord( expression() );
}

void undoc_flag(void)
{
    OPTBL_T const * saved = gOprPtr;
    OPTBL_T selected = *saved;
    val_t   val;

    skipSpace();
    if (!checkCh_e('#') )
        return;
    val                = expression();

    gOprPtr = &selected;
    if (gWord_f || ( !gByte_f && (val < -128 || 255 < val) )) {
        selected.opcode = 0x8f;
        putCode(GROUP0, NO_MODE);
        putWord(val);
    } else {
        selected.opcode = 0x87;
        putCode(GROUP0, NO_MODE);
        putByte(val);
    }
    gOprPtr = saved;
}

void    load4(void)
{
    int16_t val;
    int16_t val2;

    skipSpace();
    if (checkChar('#')) {
        imm4Expr(&val, &val2);
        printByte(putB(0xcd), 10);
        putWord(val);
        printWord(put2B(val2), 19);
    } else {
        operand(GROUP2, LOAD2);
    }
}

void    immemory(void)
{
    skipSpace();
    if (checkCh_e('#')) {
        gImVal = bytExpr();
        checkCh_e(',');
        operand(GROUP1, MEMORY);
        gImVal = 512;
    }
}


/*----------------------------------*/

void    none(void)
{
    putCode(GROUP0, NO_MODE);
}

void    mnm68hc11(void)
{
    static const struct {
        uint8_t len;
        uint8_t code[7];
    }   tbl[] = {
        { 7, { 0x34, 0x03, 0x4f, 0x31, 0xab, 0x35, 0x03 } },                                    /* aby -> pshs a,cc; clra; leay d,y; puls cc,a */
        { 6, { 0x34, 0x01, 0x31, 0x62, 0x35, 0x01 }       },                                    /* tsy -> pshs cc; leay 2,s; puls cc */
    };
    int j;

    if (gOprPtr->opcode == 0 && !gM6809_f) {
        static uint8_t const aby6309[] = { 0x34, 0x03, 0x4f, 0x10, 0x30, 0x02, 0x35, 0x03 };    /* pshs a,cc; clra; addr d,y; puls cc,a */
        for (j = 0; j < sizeof (aby6309); ++j)
            put1Byte(aby6309[j]);
        return;
    }
    for (j = 0; j < tbl[gOprPtr->opcode].len; ++j)
        put1Byte(tbl[gOprPtr->opcode].code[j]);
}

typedef struct {
    uint8_t indexed;
    uint8_t extended;
    uint8_t len;
    uint8_t code[3];
} HC11MEM_T;

static int  hc11IndexHere(void)
{
    uint8_t *p = gLinPtr;
    int         found;

    skipSpace();
    found      = (toupper(*gLinPtr) == 'X' || toupper(*gLinPtr) == 'Y')
                 && !isSymbl3( *(gLinPtr + 1) );
    gLinPtr    = p;
    return found;
}

static int  hc11IndexReg(void)
{
    skipSpace();
    if (toupper(*gLinPtr) == 'X' && !isSymbl3( *(gLinPtr + 1) )) {
        ++gLinPtr;
        return 0;
    }
    if (toupper(*gLinPtr) == 'Y' && !isSymbl3( *(gLinPtr + 1) )) {
        ++gLinPtr;
        return 0x20;
    }
    error("Only X or Y is allowed for this mnemonic.");
    return -1;
}

/* Parse the direct or X/Y-indexed operand accepted by the HC11 bit forms. */
static int  hc11Mem(HC11MEM_T * m)
{
    val_t       val;
    uint8_t *   p;
    int         base;
    int         i;

    m->indexed = m->extended = m->len = 0;
    for (i = 0; i < sizeof (m->code); ++i)
        m->code[i] = 0;
    skipSpace();
    if (checkChar(',')) {
        val        = 0;
        base       = hc11IndexReg();
        if (base < 0)
            return 0;
        m->indexed = 1;
    } else {
        val    = expression();
        p      = gLinPtr;
        if (checkChar(',') && hc11IndexHere()) {
            base       = hc11IndexReg();
            if (base < 0)
                return 0;
            m->indexed = 1;
        } else {
            gLinPtr = p;
            if (gByte_f || (gValid_f && 0 <= val && val <= 255 && !gWord_f)) {
                if (gValid_f && (val < 0 || 255 < val) )
                    error("Direct address is out of range.");
                m->len     = 1;
                m->code[0] = (uint8_t) val;
            } else {
                if (gValid_f && (val < 0 || 65535 < val) )
                    error("Extended address is out of range.");
                m->extended    = 1;
                m->len         = 2;
                m->code[0]     = (uint8_t) (val >> 8);
                m->code[1]     = (uint8_t) val;
            }
            return 1;
        }
    }
    if (gValid_f && -16 <= val && val <= 15 && !gByte_f && !gWord_f) {
        m->len     = 1;
        m->code[0] = (uint8_t) ( (val & 0x1f) | base );
    } else if (gValid_f && -128 <= val && val <= 127 && !gWord_f) {
        m->len     = 2;
        m->code[0] = (uint8_t) (0x88 | base);
        m->code[1] = (uint8_t) val;
    } else {
        m->len     = 3;
        m->code[0] = (uint8_t) (0x89 | base);
        m->code[1] = (uint8_t) (val >> 8);
        m->code[2] = (uint8_t) val;
    }
    return 1;
}

static void hc11MemTail(HC11MEM_T const* m)
{
    int i;

    for (i = 0; i < m->len; ++i)
        put1Byte(m->code[i]);
}

static void hc11MemOp(uint8_t direct, uint8_t indexed, uint8_t extended, HC11MEM_T const * m)
{
    put1Byte( m->indexed ? indexed : (m->extended ? extended : direct) );
    hc11MemTail(m);
}

static int  hc11Mask(uint8_t * mask)
{
    if (!checkCh_e(',') )
        return 0;
    skipSpace();
    checkChar('#');
    *mask = bytExpr();
    return 1;
}

void    hc11BitOp(void)
{
    HC11MEM_T   m;
    uint8_t     mask;
    int         clear = gOprPtr->opcode;

    if (!hc11Mem(&m) || !hc11Mask(&mask) )
        return;
    if (!gM6809_f) {
        put1Byte( m.indexed  ? (clear ? 0x62 : 0x61)
              : ( m.extended ? (clear ? 0x72 : 0x71)
              :                (clear ? 0x02 : 0x01) ) );
        put1Byte(clear ? (uint8_t) ~mask : mask);
        hc11MemTail(&m);
        return;
    }
    put1Byte(0x34); put1Byte(0x02);                 /* pshs a */
    hc11MemOp(0x96, 0xa6, 0xb6, &m);                /* lda */
    put1Byte(clear ? 0x84 : 0x8a);                  /* anda/ora */
    put1Byte(clear ? (uint8_t) ~mask : mask);       /* #mask */
    hc11MemOp(0x97, 0xa7, 0xb7, &m);                /* sta */
    put1Byte(0x35); put1Byte(0x02);                 /* puls a */
}

void    hc11BitBranch(void)
{
    HC11MEM_T   m;
    uint8_t     mask;
    val_t       target;
    val_t       disp;
    int         set = gOprPtr->opcode == 0;

    if (!hc11Mem(&m) || !hc11Mask(&mask) || !checkCh_e(',') )
        return;
    target = expression();
    put1Byte(0x34); put1Byte(0x03);                 /* pshs a,cc */
    hc11MemOp(0x96, 0xa6, 0xb6, &m);                /* lda */
    put1Byte(0x84); put1Byte(mask);                 /* anda #mask */
    if (set) {
        put1Byte(0x81); put1Byte(mask);             /* cmpa #mask */
    }
    put1Byte(0x26); put1Byte(4);                    /* bne false */
    put1Byte(0x35); put1Byte(0x03);                 /* puls a,cc */
    disp   = target - gLc - 2;
    if (gValid_f && (disp < -128 || 127 < disp) )
        error("Short branch target is out of range.");
    put1Byte(0x20); put1Byte((uint8_t)disp);        /* bra target */
    put1Byte(0x35); put1Byte(0x03);                 /* false: puls a,cc */
}

void    hc11MinMax(void)
{
    HC11MEM_T   m;
    int         isMin = gOprPtr->opcode & 1;
    int         isMem = gOprPtr->opcode & 2;
    int         skip;

    if (!hc11Mem(&m) )
        return;
    if (!m.indexed) {
        error("Indexed addressing is required for this mnemonic.");
        return;
    }
    put1Byte(0x34); put1Byte(0x04);                 /* pshs b */
    hc11MemOp(0xd6, 0xe6, 0xf6, &m);                /* ldb */
    put1Byte(0x11);                                 /* cba */
    put1Byte(isMin ? 0x25 : 0x24);                  /* bcs/bcc no change */
    skip   = isMem ? 5 + m.len : 2;
    put1Byte( (uint8_t) skip );
    if (isMem) {
        put1Byte(0x34); put1Byte(0x01);             /* pshs cc */
        hc11MemOp(0x97, 0xa7, 0xb7, &m);            /* sta */
        put1Byte(0x35); put1Byte(0x01);             /* puls cc */
    } else {
        put1Byte(0x1f); put1Byte(0x98);             /* tfr b,a */
    }
    put1Byte(0x35); put1Byte(0x04);                 /* puls b */
}

void    hc11Emuls(void)
{
    if (gM6809_f) {
        error("EMULS requires 6309 mode.");
        return;
    }
    put1Byte(0x10); put1Byte(0x38);                 /* pshs w */
    put1Byte(0x34); put1Byte(0x20);                 /* pshs y */
    put1Byte(0x11); put1Byte(0xaf); put1Byte(0xe1); /* muld ,s++ */
    put1Byte(0x1f); put1Byte(0x02);                 /* tfr d,y */
    put1Byte(0x1f); put1Byte(0x60);                 /* tfr w,d */
    put1Byte(0x10); put1Byte(0x39);                 /* puls w */
}

void    mnm6800(void)
{
    static const struct {
        uint8_t len;
        uint8_t code[4];
    } tbl[] = {
        { 2, { 0x1c, 0xfe } },              /* clc  -> andcc #$fe   */
        { 2, { 0x1a, 0x01 } },              /* sec  -> orcc #$01    */
        { 2, { 0x1c, 0xef } },              /* cli  -> andcc #$ef   */
        { 2, { 0x1a, 0x10 } },              /* sei  -> orcc #$10    */
        { 2, { 0x1c, 0xfd } },              /* clv  -> andcc #$fd   */
        { 2, { 0x1a, 0x02 } },              /* sev  -> orcc #$02    */
        { 2, { 0x1c, 0xbf } },              /* clf  -> andcc #$bf   */
        { 2, { 0x1a, 0x40 } },              /* sef  -> orcc #$40    */
        { 2, { 0x1c, 0xfb } },              /* clz  -> andcc #$fb   */
        { 2, { 0x1a, 0x04 } },              /* sez  -> orcc #$04    */
        { 2, { 0x34, 0x02 } },              /* psha -> pshs a       */
        { 2, { 0x34, 0x04 } },              /* pshb -> pshs b       */
        { 2, { 0x35, 0x02 } },              /* pula -> puls a       */
        { 2, { 0x35, 0x04 } },              /* pulb -> puls b       */
        { 2, { 0x34, 0x10 } },              /* pshx -> pshs x       */
        { 2, { 0x35, 0x10 } },              /* pulx -> puls x       */
        { 2, { 0x32, 0x7f } },              /* des  -> leas -1,s    */
        { 2, { 0x30, 0x1f } },              /* dex  -> leax -1,x    */
        { 2, { 0x32, 0x61 } },              /* ins  -> leas 1,s     */
        { 2, { 0x30, 0x01 } },              /* inx  -> leax 1,x     */
        { 2, { 0x3c, 0xff } },              /* wai  -> cwai #$ff    */
        { 3, { 0x1f, 0x89, 0x4d} },         /* tab  -> tfr a,b;tsta */
        { 3, { 0x1f, 0x98, 0x4d} },         /* tba  -> tfr b,a;tsta */
        { 2, { 0x1f, 0x8a } },              /* tap  -> tfr a,cc     */
        { 2, { 0x1f, 0xa8 } },              /* tpa  -> tfr cc,a     */
        { 2, { 0x1f, 0x41 } },              /* tsx  -> tfr s,x      */
        { 2, { 0x1f, 0x14 } },              /* txs  -> tfr x,s      */
        { 4, { 0x34, 0x04, 0xab, 0xe0} },   /* aba  -> 6309: addr b,a; 6809: pshs b; adda ,s+ */
        { 4, { 0x34, 0x04, 0xa1, 0xe0} },   /* cba  -> 6309: cmpr b,a; 6809: pshs b; cmpa ,s+ */
        { 4, { 0x34, 0x04, 0xa0, 0xe0} },   /* sba  -> 6309: subr b,a; 6809: pshs b; suba ,s+ */
    };
    uint8_t const*  p;
    int             j;

    if (!gM6800_f)
        warning("[WARNING] M6800 mnemonic used without -6.");
    if (!gM6809_f && gOprPtr->opcode >= 27) {
        static uint8_t const opr[] = { 0x30, 0x37, 0x32 };
        put1Byte(0x10);
        put1Byte(opr[gOprPtr->opcode - 27]);
        put1Byte(0x98);                     /* B,A */
        return;
    }
    p = tbl[gOprPtr->opcode].code;
    for (j = 0; j < tbl[gOprPtr->opcode].len; ++j)
        put1Byte(p[j]);
}

void    setdp(void)
{
    skipSpace();
    clearAddress();
    printByte(gDp = invExpr(), 15);
}

void    transfer(void)
{
    int r1;
    int r2;

    skipSpace();
    putCode(GROUP0, NO_MODE);
    if (( r1 = regNo(ALLREG | X63REG) ) < 0 )
        goto ERR;
    checkCh_e(',');
    if (( r2 = regNo(ALLREG | X63REG) ) < 0 )
        goto ERR;
    if (
        !gUndoc_f &&
        ( (r1 ^ r2) & 0x08 ) && r1 != 0x0c && r2 != 0x0c
   ) {
        warning("[WARNING] Registers have different sizes.");
    }
    putByte( (r1 << 4) | r2 );
  ERR:
    return;
}

void    tfm(void)
{
    int     r1;
    int     r2;
    uint8_t md = ' ';
    skipSpace();
    if (( r1 = regNo(INDEXREG | D) ) < 0 )
        goto ER;
    if (*gLinPtr == '+')
        md = *gLinPtr++;
    else if (*gLinPtr == '-')
        md = *gLinPtr++;
    checkCh_e(',');
    if (( r2 = regNo(INDEXREG | D) ) < 0 )
        goto ER;
    if (*gLinPtr == '+') {
        gLinPtr++;
        if (md == '+')      /* tfm r+,r+ */
            putCode(GROUP3, 0);
        else if (md == ' ') /* tfm r+,r */
            putCode(GROUP3, 3);
        else
            goto ER;
    } else if (*gLinPtr == '-') {
        gLinPtr++;
        if (md == '-')      /* tfm r-,r- */
            putCode(GROUP3, 1);
        else
            goto ER;
    } else if (md == '+') { /* tfm r,r+ */
        putCode(GROUP3, 2);
    } else {
        goto ER;
    }
    putByte( (r1 << 4) | r2 );
    return;

  ER:
    put1Word(0);
    put1Byte(0);
    error("Invalid TFM operand.");
}

static void pshpul(uint8_t u, uint8_t h)
{
    uint16_t m1 = 0;
    uint16_t m2 = 0;

    skipSpace();
    do {
        switch (getReg(ALLREG | W)) {
        case CC:
            m2 = 0x01;
            break;
        case A:
            m2 = 0x02;
            break;
        case B:
            m2 = 0x04;
            break;
        case D:
            m2 = 0x06;
            break;
        case DP:
            m2 = 0x08;
            break;
        case X:
            m2 = 0x10;
            break;
        case Y:
            m2 = 0x20;
            break;
        case S:
            if (u == 0)
                error("Register S is not allowed in PSHS or PULS.");
            m2 = 0x40;
            break;
        case U:
            if (u)
                error("Register U is not allowed in PSHU or PULU.");
            m2 = 0x40;
            break;
        case PC:
            m2 = 0x80;
            break;
        case W:
            m2 = 0x100;
            break;
        default:
            break;
        }
        if (m1 & m2)
            error("The same register is specified more than once.");
        m1 |= m2;
    } while ( checkChar(',') );
    if (m1 & 0x100) {                       /* w */
        if (h)
            put1Word(u ? 0x103b : 0x1039);  /* puluw pulsw */
        if (m1 & 0xff) {
            put1Byte(gOprPtr->opcode);
            put1Byte(m1 & 0xff);
        }
        if (!h)                             /* psh */
            put1Word(u ? 0x103a : 0x1038);  /* pshuw pshsw */
    } else {
        putCode(GROUP0, NO_MODE);
        putByte(m1);
    }
}

void    pshs(void)
{
    pshpul(0, 0);
}

void    puls(void)
{
    pshpul(0, 1);
}

void    pshu(void)
{
    pshpul(1, 0);
}

void    pulu(void)
{
    pshpul(1, 1);
}


/*----------------------------------*/

static void automaticBranch(int originallyLong)
{
    OPTBL_T        op          = *gOprPtr;
    OPTBL_T const* original    = gOprPtr;
    LINE_STATE*    state       = lineState();
    val_t          target      = 0;
    val_t          distance    = 0;
    int shortOpcode = originallyLong
        ? (op.opcode == 0x16 ? 0x20 : op.opcode == 0x17 ? 0x8d : op.opcode)
        : op.opcode;
    int longSize = (shortOpcode == 0x20 || shortOpcode == 0x8d) ? 3 : 4;
    int size     = state->branchSize;
    int resolved = 0;
    skipSpace();
    target   = expression();
    resolved = gValid_f || (gPass != 1 && expressionForward && !pragmaForwardMax);
    distance = target - gLinLc - 2;
    if (gPass != 2 && gPass != -2) {
        if (gPass != 1 && expressionForward == 1 && !expressionFixedForward
            && state->branchSize)
            distance = target - state->address - state->branchSize;
        size = (!state->branchLong && resolved && -128 <= distance && distance <= 127)
            ? 2 : longSize;
        if (state->branchSize == 2 && size != 2 && !gByte_f)
            state->branchLong = 1;
    }
    if (gByte_f) {
        size = 2;
    } else if (gWord_f) {
        size = longSize;
    }
    state->branchSize = size;
    if (size == 2) {
        op.prefix = 0;
        op.opcode = shortOpcode;
        if (gPass == 2 && (target - gLinLc - 2 < -128 || target - gLinLc - 2 > 127))
            error("Short branch target is out of range.");
        if (originallyLong)
            ++branchChanges;
    } else {
        op.prefix = longSize == 4 ? 0x10 : 0;
        op.opcode = shortOpcode == 0x20 ? 0x16 : shortOpcode == 0x8d ? 0x17 : shortOpcode;
        if (!originallyLong)
            ++branchChanges;
    }
    gOprPtr = &op;
    putCode(GROUP0, NO_MODE);
    if (size == 2) {
        putByte(target - gLinLc - size);
    } else {
        putWord(target - gLinLc - size);
    }
    gOprPtr = original;
}

void    branch(void)
{
    val_t val;

    if (pragmaAutoBranch) {
        automaticBranch(0);
        return;
    }
    skipSpace();
    if ((val = expression() - gLinLc - 2) < -128 || 127 < val )
        error("Short branch target is out of range.");
    putCode(GROUP0, NO_MODE);
    putByte(val);
}

void    lbranch(void)
{
    val_t target = 0;
    if (pragmaAutoBranch) {
        automaticBranch(1);
        return;
    }
    skipSpace();
    target = expression();
    putCode(GROUP0, NO_MODE);
    putWord(target - gLinLc - (gOprPtr->prefix ? 4 : 3));
}


/*----------------------------------*/

void    os9svc(void)
{
    putCode(GROUP0, NO_MODE);
    skipSpace();
    printByte(putB( bytExpr() ), 15);
}

void    mod(void)
{
    uint16_t      os9hdr[4];
    uint8_t       sum;
    uint8_t       l;
    uint8_t const *p;

    gOs9_f     = 1;
    gObjLc     = gLc = 0;
    gCrcBuf[0] = gCrcBuf[1] = gCrcBuf[2] = 0xff;
    skipSpace();
    put1Word(os9hdr[0] = 0x87cd);
    put1Word(os9hdr[1] = expression());
    checkCh_e(',');
    put1Word(os9hdr[2] = expression());
    checkCh_e(',');
    l          = (expression() & 0xff);
    put1Byte(l);
    checkCh_e(',');
    put1Byte(os9hdr[3] = (expression() & 0xFF));
    os9hdr[3] |= l * 0x100;
    for (p = (uint8_t const *) os9hdr, sum = (uint8_t)~0, l = 8; l-- > 0;) {
        sum ^= *p++;
    }
    put1Byte(sum & 0xff);
    if (checkChar(',')) {
        put1Word( expression() );
        checkCh_e(',');
        put1Word( expression() );
    }
    return;
}

void    emod(void)
{
    /* if (gOs9_f == 0) error("EMOD without MOD."); */
    gOs9_f = 0;
    put1Byte(~gCrcBuf[0]);
    put1Byte(~gCrcBuf[1]);
    put1Byte(~gCrcBuf[2]);
    gOs9_f = 1;
}

static void dataValue(val_t value, int size, int listing)
{
    if (size == 4) {
        uint16_t high = (uint16_t)((unsigned int)value >> 16);
        if (listing)
            put1Word(high);
        else
            put2B(high);
    }
    if (size >= 2) {
        if (listing)
            put1Word((uint16_t)value);
        else
            put2B((uint16_t)value);
    } else {
        if (listing)
            put1Byte(value);
        else
            putB(value);
    }
}

void    fdb(void)
{
    skipSpace();
    do {
        if (checkChar('"')) {
            while (((*gLinPtr != '"') || (*++gLinPtr == '"')) && (*gLinPtr != '\n')) {
                if (isKanji(*gLinPtr) && gLinPtr[1]) {
                    put1Byte(*gLinPtr++);
                    put1Byte(*gLinPtr++);
                } else {
                    put1Word(*gLinPtr++);
                }
            }
        } else {
            dataValue(expression(), 2, 1);
        }
    } while ( checkChar(',') );
}

void    fqb(void)
{
    val_t val;

    skipSpace();
    do {
        val = expression();
        dataValue(val, 4, 1);
    } while ( checkChar(',') );
}

void    fcb(void)
{
    skipSpace();
    do {
        if (checkChar('"')) {
            while (((*gLinPtr != '"') || (*++gLinPtr == '"')) && (*gLinPtr != '\n')) {
                put1Byte(*gLinPtr++);
            }
        } else if (checkChar('#')) {
            for (;;) {
                uint8_t b;
                uint8_t c;
                b  = *gLinPtr;
                if (!isxdigit(b) )
                    break;
                c  = *++gLinPtr;
                if (!isxdigit(c)) {
                    error("FCB hexadecimal data after # must have an even number of digits.");
                    break;
                }
                put1Byte( toXDigit(b) * 16 + toXDigit(c) );
                gLinPtr++;
            }
        } else if (checkChar('>')) {
            val_t value = expression();
            if (gCompatMode == COMPAT_LWASM) {
                error("FCB > prefix is not supported in lwasm mode.");
            } else if (gCompatMode == COMPAT_VASM) {
                dataValue((value >> 8) & 255, 1, 0);
            } else {
                dataValue(value, 2, 1);
            }
        } else {
            dataValue(bytExpr(), 1, 1);
        }
    } while ( checkChar(',') );
}

static int pragmaName(char const *name, int *enabled)
{
    static char const * const names[] = {
        "6809", "6309", "6800compat", "cescapes", "pcaspcr",
        "index0tonone", "forwardrefmax", "autobranchlength", "nolist", "nolistcode"
    };
    int     i;
    *enabled = 1;
    for (i = 0; i < PRAGMA_KINDS; ++i)
        if (!strcmp(name, names[i]))
            return i;
    if (!strcmp(name, "list") || !strcmp(name, "listcode")) {
        *enabled = 0;
        return !strcmp(name, "list") ? 8 : 9;
    }
    if (!strncmp(name, "no", 2)) {
        name    += 2;
        *enabled = 0;
    }
    for (i = 0; i < PRAGMA_KINDS; ++i)
        if (!strcmp(name, names[i])) return i;
    return -1;
}

static int pragmaValue(int kind)
{
    switch (kind) {
    case 0: return gM6809_f         != 0;
    case 1: return gM6809_f         == 0;
    case 2: return gM6800_f         != 0;
    case 3: return pragmaEscapes    != 0;
    case 4: return pragmaPcAsPcr    != 0;
    case 5: return pragmaIndex0     != 0;
    case 6: return pragmaForwardMax != 0;
    case 7: return pragmaAutoBranch != 0;
    case 8: return pragmaNoList     != 0;
    case 9: return pragmaNoListCode != 0;
    }
    return 0;
}

static void pragmaSet(int kind, int enabled)
{
    if (!restoringPragmas)
        pragmaConfigured[kind] = 1;
    switch (kind) {
    case 0: gM6809_f         = enabled; break;
    case 1: gM6809_f         = !enabled;break;
    case 2: gM6800_f         = enabled; break;
    case 3: pragmaEscapes    = enabled; break;
    case 4: pragmaPcAsPcr    = enabled; break;
    case 5: pragmaIndex0     = enabled; break;
    case 6:
        pragmaForwardMax     = enabled;
        if (!enabled && !restoringPragmas)
            relaxRequested   = 1;
        break;
    case 7:
        pragmaAutoBranch     = enabled;
        if (enabled)
            relaxRequested   = 1;
        break;
    case 8: pragmaNoList     = enabled; break;
    case 9: pragmaNoListCode = enabled; break;
    }
}

static int readPragma(int *enabled)
{
    char name[LBLSIZE + 1];
    int n = 0;
    skipSpace();
    while (isSymbl(*gLinPtr)) {
        if (n < LBLSIZE) name[n++] = tolower(*gLinPtr);
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
                    pragmaStacks[gFile_sp][kind][(*depth)++] = pragmaValue(kind);
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

static int stringEscape(void)
{
    int     value;
    int     digits;
    int     c = *gLinPtr++;
    switch (c) {
    case 'a': return 7;
    case 'b': return 8;
    case 't': return 9;
    case 'n': return 10;
    case 'v': return 11;
    case 'f': return 12;
    case 'r': return 13;
    case 'x':
        value  = 0;
        digits = 0;
        while (isxdigit(*gLinPtr)) {
            value = ((value << 4) | toXDigit(*gLinPtr)) & 255;
            ++gLinPtr;
            ++digits;
        }
        if (!digits)
            error("Missing hexadecimal string escape digits.");
        return value;
    default:
        if (c >= '0' && c <= '7') {
            value = c - '0';
            for (digits = 1; digits < 3 && *gLinPtr >= '0' && *gLinPtr <= '7'; ++digits)
                value = value * 8 + *gLinPtr++ - '0';
            return value & 255;
        }
        return c;
    }
}

static void fccs(uint8_t a)
{
    char    temp[MNEMOSIZE + 1];
    uint8_t b;

    skipSpace();
    if (!*gLinPtr || *gLinPtr == '\n') {
        error("Missing string operand.");
        return;
    }
    do {
        uint8_t c = *gLinPtr++;
        if (c == '$' && toupper(*gLinPtr) == 'M') {
            getLabel(temp);
            if (strcasecmp(temp, "modnam") == 0 && gModName) {
                uint8_t const* p = (uint8_t const*)gModName;
                while ( (b = *p++) != '\0') {
                    if (a == 1 && *p == '\0')
                        b |= 0x80;
                    put1Byte(b);
                }
            } else {
                error("Invalid FCC operand.");
            }
        } else if (isSymbl3(c) || c == '%' || c == '(') {
            --gLinPtr;
            put1Byte( bytExpr() );
        } else {
            while ( (b = *gLinPtr++) != c) {
                if (b == '\n') {
                    error("Missing closing delimiter.");
                    return;
                } else {
                    if (pragmaEscapes && b == '\\') {
                        if (!*gLinPtr || *gLinPtr == '\n') {
                            error("Missing closing delimiter.");
                            return;
                        }
                        b = (uint8_t)stringEscape();
                    }
                    if (a == 1 && *gLinPtr == c)
                        b |= 0x80;
                    put1Byte(b);
                }
            }
        }
        if (a == 2)
            put1Byte(0);
    } while (*gLinPtr++ == ',');
    --gLinPtr;
}

void    fcc(void)
{
    fccs(0);
}

void    fcs(void)
{
    fccs(1);
}

void fcn(void)
{
    fccs(2);
}

static int nextComma(void);

static int blockCount(val_t count, int size)
{
    if (count < 0 || count > 65535 / size || count > (65536L - gLc) / size) {
        error("Data block exceeds address space or has a negative count.");
        return 0;
    }
    return 1;
}

static void fillBlock(val_t count, val_t fill, int size, int mode)
{
    if (mode & BLOCK_CHECK) {
        if (!blockCount(count, size))
            return;
        if (size == 1 && (fill < -128 || fill > 255)) {
            error("Fill value does not fit in one byte.");
            return;
        }
        if (size == 2 && (fill < -32768 || fill > 65535)) {
            error("Fill value does not fit in one word.");
            return;
        }
    }
    printWord(gLc, 5);
    while (count-- > 0)
        dataValue(fill, size, 0);
}

void    rzb(void)
{
    val_t count;
    val_t fill = 0;
    int   size = gOprPtr->prefix ? gOprPtr->prefix : 1;
    int   mode = gOprPtr->opcode;

    if (!strcmp(gOprPtr->mnemonic, "DS") && gCompatMode != COMPAT_VASM)
        size = 1;
    skipSpace();
    if (mode & BLOCK_REVERSED) {
        fill = expression();
        skipSpace();
        if (!checkCh_e(','))
            return;
        skipSpace();
    }
    count = invExpr();
    if ((mode & BLOCK_FILL) && nextComma()) {
        skipSpace();
        fill = expression();
    }
    fillBlock(count, fill, size, mode);
}

static void labelValue(val_t value)
{
    if (gLblPtr)
        gLblPtr->value = value;
    clearAddress();
    printWord(value, 5);
}

static val_t offsetField(val_t base, val_t count, int checked)
{
    long next = (long)base + count;
    if (checked && (next < 0 || next > 65535)) {
        error("RS offset is outside 0..65535.");
        return base;
    }
    labelValue(base);
    return (val_t)next;
}

/*----------------------------------*/

void    rmb(void)
{
    val_t count;
    int   size     = gOprPtr->prefix ? gOprPtr->prefix : 1;
    int   checked  = gOprPtr->opcode & BLOCK_CHECK;
    val_t position = gCSectSw ? gCSectBase : gLc;

    skipSpace();
    count = invExpr();
    if (checked && (count < 0 || count > 65535 / size
        || count > (65536L - position) / size))
    {
        error("Reservation exceeds address space or has a negative count.");
        return;
    }
    count *= size;
    if (offsetActive) {
        if (count < 0 || count > 65535L - gLc)
            error("OFFSET exceeds address space.");
        else
            gLc += (uint16_t)count;
    } else if (gCSectSw) {
        gCSectBase = (uint16_t)offsetField(gCSectBase, count, checked);
    } else if (gOrgSFmt_f && (gObjct == OB_SFMT || gFlex_f)) {
        flushObj();
        gLc += (uint16_t)count;
    } else if (gRmb_f && gFBasic_f) {
        printWord(gLc, 5);
        gRmb_sp += count;
        gObjCnt += count;
        gLc     += (uint16_t)count;
    } else {
        fillBlock(count, 0, 1, 0);
    }
}

void    equ(void)
{
    val_t value;
    skipSpace();
    value = (gOprPtr->opcode & EQU_RESOLVED) ? invExpr() : expression();
    labelValue(value);
}

void    org(void)
{
    uint16_t origin;

    ++addressRegion;
    if (offsetActive) {
        lastOffset   = gLc;
        gLc          = savedCodeLc;
        offsetActive = 0;
    }
    skipSpace();
    if (gOprPtr->prefix) {
        if (!reorgValid) {
            error("REORG without a previous ORG.");
            return;
        }
        origin = previousOrg;
    } else {
        origin = (uint16_t)invExpr();
    }
    if (!gOprPtr->prefix) {
        previousOrg = gOs9_f ? gCSectBase : gLc;
        reorgValid = 1;
    }
    if (gStartAddr == 0xFFFF && gOrg_f == 0)
        gStartAddr = origin;
    if (gOs9_f) {
        gCSectSw   = 1;
        gCSectBase = origin;
    } else {
        if (gCompatMode == COMPAT_LWASM || gOprPtr->prefix || gOrg_f == 0
            || (gOrgSFmt_f && (gObjct == OB_SFMT || gFlex_f))
        ) {
            flushObj();
            gObjLc = gLc = origin;
        } else if (gLc > origin) {
            error("Invalid ORG operand.");
            return;
        } else if (gLc < origin) {
            fillBlock(origin - gLc, 0, 1, 0);
        }
    }
    printAddress(origin);
    if (!gOrg_f)
        rorgBase = origin;
    gOrg_f = 1;
}

void    csct(void)
{
    gCSectSw = 2;
    skipSpace();
    if (*gLinPtr == '\n')
        gCSectBase = 0;
    else
        gCSectBase = invExpr();
    printWord(gCSectBase, 5);
}

void    endsct(void)
{
    if (offsetActive) {
        lastOffset   = gLc;
        gLc          = savedCodeLc;
        offsetActive = 0;
    } else if (gCSectSw > 1) {
        gCSectSw = 0;
    } else if (gPSect_f) {
        gPSect_f = 0;
    } else {
        error("ENDSECT without a matching section.");
    }
}

void    psct(void)
{
    gPSect_f = 1;
}

void    vsct(void)
{
    gCSectSw = 3;
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
    char const* prefix= "";
    skipSpace();
    if (*gLinPtr == '"' || *gLinPtr == '\'') {
        quote = *gLinPtr++;
    } else if (*gLinPtr == '<') {
        ++gLinPtr;
        quote = '>';
    }
 #ifdef INCLUDIR
    if (quote == '>') {
        prefix = gIncDirName;
    } else if (*gLinPtr == '$') {
        char name[LBLSIZE + 1];
        ++gLinPtr;
        getLabel(name);
        if (strcmp(name, "INC")) {
            error("Invalid include file name.");
            return 0;
        }
        prefix = gIncDirName;
    }
 #endif
    if (strlen(prefix) >= (size_t)size) {
        error("Operand is too long.");
        return 0;
    }
    strcpy(dst, prefix);
    n = (int)strlen(prefix);
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

static int nextComma(void)
{
    uint8_t *saved = gLinPtr;
    skipSpace();
    if (checkChar(',') )
        return 1;
    gLinPtr = saved;
    return 0;
}

static FILE *openSearch(char const * name, char const * mode)
{
    FILE *  fp = fopen(name, mode);
    char    path[FNAMESZ + 1];
    int     i;

    if (fp)
        return fp;
    if (name[0] == '/' || name[0] == '\\' || (name[0] && name[1] == ':') )
        return NULL;
    for (i = -1; i < extraIncCount; ++i) {
        char const * dir = i < 0 ? gIncDirName : extraIncDirs[i];
        if (strlen(dir) + strlen(name) + 2 > sizeof (path) )
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
    if (extraIncCount == EXTRA_INCDIRS) {
        error("Too many include directories.");
        return;
    }
    extraIncDirs[extraIncCount] = mallocE(strlen(path) + 1);
    strcpy(extraIncDirs[extraIncCount++], path);
}

void incbin(void)
{
    FILE*   fp;
    char    name[FNAMESZ + 1];
    val_t   offset    = 0;
    val_t   length    = -1;
    long    fileSize  = 0;
    long    available = 0;

    if (!textOperand(name, sizeof(name)) )
        return;
    if (nextComma()) {
        skipSpace();
        offset = invExpr();
        if (nextComma()) {
            skipSpace();
            length = invExpr();
            if (length < 0) {
                error("Invalid INCBIN length.");
                return;
            }
        }
    }
    fp = openSearch(name, "rb");
    if (!fp) {
        error("Cannot open INCBIN file.");
        return;
    }
    if (fseek(fp, 0, SEEK_END) || (fileSize = ftell(fp)) < 0) {
        error("Unreadable INCBIN file.");
        fclose(fp);
        return;
    }
    if (offset < 0)
        offset += fileSize;
    if (offset < 0 || offset > fileSize || fseek(fp, offset, SEEK_SET)) {
        error("Invalid INCBIN offset or unreadable file.");
        fclose(fp);
        return;
    }
    available  = fileSize - offset;
    if (length >= 0 && length < available)
        available = length;
    if (available > 65535L || available > 65536L - gLc) {
        error("INCBIN exceeds address space.");
        fclose(fp);
        return;
    }
    while (available-- > 0) {
        int c = fgetc(fp);
        if (c == EOF) {
            error("Cannot read INCBIN file.");
            break;
        }
        put1Byte(c);
    }
    fclose(fp);
}

void alignData(void)
{
    val_t   offset   = 0;
    val_t   boundary = 2;
    val_t   fill     = 0;
    long    count;
    val_t   position = (gCSectSw == 2) ? gCSectBase : gLc;
    if (gOprPtr->prefix == 1) {
        skipSpace();
        boundary = invExpr();
        if (gCompatMode == COMPAT_VASM) {
            if (boundary < 0 || boundary > 16) {
                error("ALIGN exponent must be 0..16.");
                return;
            }
            boundary = 1L << boundary;
        } else {
            if (boundary <= 0 || boundary > 65536L) {
                error("ALIGN boundary must be 1..65536.");
                return;
            }
            if (nextComma()) {
                skipSpace();
                fill = expression() & 255;
            }
        }
    } else if (gOprPtr->prefix == 2) {
        skipSpace();
        offset     = invExpr();
        skipSpace();
        if (!checkCh_e(',') )
            return;
        skipSpace();
        boundary   = invExpr();
        if (boundary <= 0 || boundary > 65536 || offset < 0 || offset > 65535){
            error("Invalid CNOP offset or alignment.");
            return;
        }
    }
    if (gOprPtr->prefix == 3) {
        count = gCompatMode == COMPAT_VASM ? (position & 1) + 1 : !(position & 1);
    } else {
        count = (boundary - position % boundary) % boundary + offset;
    }
    if (gCSectSw == 2) {
        if (count > 65535L - position)
            error("Alignment exceeds offset range.");
        else
            gCSectBase += (uint16_t)count;
        clearAddress();
    } else {
        fillBlock(count, fill, 1, BLOCK_CHECK);
    }
}

void rsOffset(void)
{
    int   mode = gOprPtr->opcode & 15;
    int   kind = gOprPtr->opcode >> 4;
    val_t * counter = (kind == 1) ? &soCounter : kind == 2 ? &foCounter : &rsCounter;
    val_t count;
    val_t next;
    int size = gOprPtr->prefix;
    if (!strcmp(gOprPtr->mnemonic, "RS") && gCompatMode != COMPAT_VASM)
        size = 1;
    clearAddress();
    if (mode == RS_RESET) {
        *counter = 0;
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
        count = invExpr();
        if (count < 0 || count > 65535 / size) {
            error("RS count is outside the supported range.");
            return;
        }
        next = *counter + (kind == 2 ? -1 : 1) * count * size;
        if (next < (kind == 2 ? -65535 : 0) || next > 65535) {
            error("RS offset is outside 0..65535.");
            return;
        }
        labelValue(*counter);
        *counter = next;
    }
    if (kind == 1)
        soDefined = 1;
    else if (kind == 2)
        foDefined = 1;
    else
        rsDefined = 1;
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

void relativeData(void)
{
    val_t value;
    skipSpace();
    do {
        skipSpace();
        value = expression() - gLc;
        dataValue(value, gOprPtr->prefix, 1);
    } while (nextComma());
}

void relativeOrg(void)
{
    val_t target;
    skipSpace();
    target = invExpr();
    if (target < 0 || target > 65535L - rorgBase || target + rorgBase < gLc) {
        error("Invalid RORG operand.");
        return;
    }
    fillBlock(target + rorgBase - gLc, 0, 1, BLOCK_CHECK);
}

void offsetSection(void)
{
    val_t start;
    skipSpace();
    start = (*gLinPtr == '\n' || *gLinPtr == ';') ?
            (offsetActive ? gLc : lastOffset) : invExpr();
    if (start < 0 || start > 65535) {
        error("Invalid OFFSET operand.");
        return;
    }
    if (!offsetActive) {
        flushObj();
        savedCodeLc = gLc;
    }
    offsetActive = 1;
    gLc          = start;
    clearAddress();
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
    int     more;
    clearAddress();
    do {
        skipSpace();
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
            printf("%s\n", text);
        more = nextComma();
    } while (more);
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
            printf("$%X %d \"%s\" %%%s\n", u, val, ascii, binary);
        }
    } while ( nextComma() );
}

void    library(void)
{
    FILE *fp;
    char fname[FNAMESZ + 1];
    clearAddress();
    if (!textOperand(fname, sizeof(fname)))
        return;
    if (gVerbos_f)
        fprintf(STDERR, "[%s]\n", fname);
    DEBMSGF( (STDERR, "include %s  (#%d)\n", fname, gFile_sp + 1) );
    fp = openSearch(fname, "r");
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
        if (gPass == 2 && gLstFp) gList = gOprPtr->opcode;
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

void    none_d(void)
{
    uint8_t const* p;
    uint8_t        i;
    uint8_t        l;
    static uint8_t const tbl[16][5] = {
        { 4, 0x40, 0x50, 0x82, 0x00          }, /* negd 0x1040 */
        { 0, 0,    0,    0,    0             },
        { 0, 0,    0,    0,    0             },
        { 2, 0x53, 0x43, 0,    0             }, /* comd 0x1043 */
        { 2, 0x44, 0x56, 0,    0             }, /* lsrd 0x1044 */
        { 0, 0,    0,    0,    0             },
        { 2, 0x46, 0x56, 0,    0             }, /* rord 0x1046 */
        { 2, 0x47, 0x56, 0,    0             }, /* asrd 0x1047 */
        { 2, 0x58, 0x49, 0,    0             }, /* lsld 0x1048 */
        { 2, 0x59, 0x49, 0,    0             }, /* rold 0x1049 */
        { 3, 0x83, 0x00, 0x01, 0             }, /* decd 0x104a */
        { 0, 0,    0,    0,    0             },
        { 3, 0xc3, 0x00, 0x01, 0             }, /* incd 0x104c */
        { 2, 0xed, 0x7e, 0,    0             }, /* tstd 0x104d */
        { 0, 0,    0,    0,    0             },
        { 2, 0x5f, 0x4f, 0,    0             } /* clrd: clrb;clra */
    };

    if (gM6809_f == 0) {
        putCode(GROUP0, NO_MODE);               /*  none(); */
        return;
    }
    if (gM6809_f && gUndoc_f && gOprPtr->opcode == 0x40) {
        printByte(putB(0x50), 10);              /* negb */
        printByte(putB(0x42), 12);              /* ngca */
        return;
    }
    p = tbl[gOprPtr->opcode - 0x40];
    for (i = 10, l = *p++; l--; i += 2)
        printByte(putB(*p++), i);
}

static void putOped(uint8_t o1, uint8_t d1, uint8_t o2, uint8_t d2)
{
    put1Byte(gOprPtr->opcode + o1);
    put1Byte(d1);
    put1Byte(gOprPtr->opcode + o2);
    put1Byte(d2);
}

static void putOpedR(uint8_t d1, uint8_t d2, int reg)
{
    putOped( 0x40 + 0x20, index0(d1, reg), 0x20, index0(d2, reg) );
}

void    oped(void)  /* andd  ord  eord  adcd  sbcd */
{
    int val;
    int reg;
    int i;
    int val2;

    if (gM6809_f == 0) {
        load2();
        return;
    }
    gIndirect = 0;
    skipSpace();

    if (checkChar('#')) {
        val = expression();
        putOped(0x40, val & 0xff, 0, val >> 8);
    } else if (checkChar(',')) {
        if (checkChar('-')) {
            if (checkChar('-') )
                putOpedR( 0x82, 0x82, getReg(INDEXREG) );
            else
                putOpedR( 0x84, 0x82, getReg(INDEXREG) );
        } else {
            reg = getReg(INDEXREG);
            if (checkChar('+')) {
                if (checkChar('+') )
                    putOpedR(1, 0x81, reg);
                else
                    putOpedR(1, 0x80, reg);
            } else {
                    putOpedR(1, 0x84, reg);
            }
        }
    } else {
        val = expression();
        if (checkChar(',')) {
            switch ( (reg = getReg(INDEXREG | PC | PCR)) ) {
            case X:
            case Y:
            case U:
            case S:
                for (++val, i = 1; i >= 0; --i, --val) {
                    put1Byte(gOprPtr->opcode + 0x20 + i * 0x40);
                    if (gValid_f && -16 <= val && val <= 15 && indexSmallAllowed()) {
                        put1Byte( index0( indexZeroAllowed(val, reg) ? 0x84 : (val & 0x1f), reg ) );
                    } else if (checkByte(val)) {
                        put1Byte( index0(0x88, reg) );
                        put1Byte(val);
                    } else {
                        put1Byte( index0(0x89, reg) );
                        put1Word(val);
                    }
                }
                break;

            case PC:
                /* Offset is relative to the end of the complete expansion. */
                val2 = val + 1 + ((checkByte(val) && -128 <= val && val <= 127) ? 3 : 4);
                for (i = 1; i >= 0; --i) {
                    int disp = i ? val2 : val;
                    put1Byte(gOprPtr->opcode + 0x20 + i * 0x40);
                    if (checkByte(disp) && -128 <= disp && disp <= 127) {
                        put1Byte(index0(0x8c, 0));
                        put1Byte(disp);
                    } else {
                        put1Byte(index0(0x8d, 0));
                        put1Word(disp);
                    }
                }
                break;

            case PCR:
                val2 = gLinLc + 3;
                for (++val, i = 1; i >= 0; --i, --val, val2 += 3) {
                    put1Byte(gOprPtr->opcode + 0x20 + i * 0x40);
                    if (checkByte(val - val2)) {
                        put1Byte( index0(0x8c, 0) );
                        put1Byte(val - val2);
                    } else {
                        val2++;
                        put1Byte( index0(0x8d, 0) );
                        put1Word(val - val2);
                    }
                }
            }
        } else if (gByte_f || ((uint16_t)(val - (gDp << 8)) <= 254 && gValid_f && !gWord_f)) {
            putOped(0x40 + 0x10, val - (gDp << 8) + 1, 0x10, val - (gDp << 8));
        } else {
            put1Byte(gOprPtr->opcode + 0x30 + 0x40);
            put1Word(val + 1);
            put1Byte(gOprPtr->opcode + 0x30);
            put1Word(val);
        }
    }
}

void    none_wq(void)
{
    static uint8_t const tbl[] = {
        0x10, 0xed, 0x7c, 0,                /*  tstq  stq -4,s; */
        0x10, 0x5f, 0x10, 0x4f,             /*  clrq  clrw;clrd */
        0x10, 0x53, 0x10, 0x43,             /*  comq  comw;comd */
        0x10, 0x44, 0x10, 0x56,             /*  lsrq  lsrd;rorw */
        0x10, 0x47, 0x10, 0x56,             /*  asrq  asrd;rorw */
        0x10, 0x46, 0x10, 0x56,             /*  rorq  rord;rorw */
        0x10, 0x59, 0x10, 0x49,             /*  rolq  rolw;rold */
        0x1c, 0xfe, 0x10, 0x59,             /*  lslw  andcc #$fe;rolw */
        0x10, 0x53, 0x10, 0x5c,             /*  negw  comw;incw */
        0x10, 0x38, 0x68, 0xe1, 0x10, 0x56, /*  asrw  pshw;asl ,s++;rorw */
        0x1c, 0xfe, 0x10, 0x59, 0x10, 0x49, /*  lslq  andcc #$fe;rolw;rold */
        0x10, 0x5c, 0x26, 0x02, 0x10, 0x4c, /*  incq  incw;bne *+4;incd */
        0x10, 0x5d, 0x26, 0x02, 0x10, 0x4a, 0x10, 0x5a,
        /* decq  tstw;bne *+4;decd;decw */
        0x10, 0x43, 0x10, 0x53, 0x10, 0x5c, 0x26, 0x02,0x10, 0x4c,
        /*  negq  comd;comw;incw;bne *+4;incd*/
        0
    };
    uint8_t const*  p = tbl + gOprPtr->prefix;
    uint8_t         l = gOprPtr->opcode;

    while (l--)
        put1Byt2(*p++);
}

static void putOpeqR(uint16_t const * op, uint8_t d1, uint8_t d2, int reg)
{
    put1Word(op[1] + 0x20);
    put1Byte( index0(d1, reg) );
    put1Word(*op + 0x20);
    put1Byte( index0(d2, reg) );
}

void        opeq(void)  /* addq  subq */
{
    int16_t     val;
    int16_t     val2;
    int16_t     reg;
    int16_t     i;
    uint16_t    op[2];

    op[0]      = gOprPtr->prefix + 0x1000;  /* d */
    op[1]      = gOprPtr->opcode + 0x1000;  /* w */
    gIndirect  = 0;
    skipSpace();

    if (checkChar('#')) {
        imm4Expr(&val, &val2);
        put1Word(op[1]);
        put1Word(val2);
        put1Word(op[0]);
        put1Word(val);
    } else if (checkChar(',')) {
        if (checkChar('-')) {
            if (checkChar('-') )
                if (checkChar('-') )
                    if (checkChar('-') )
                        putOpeqR( op, 0x83, 0x83, getReg(INDEXREG) );
                    else
                        putOpeqR( op, 0x82, 0x83, getReg(INDEXREG) );
                else
                        putOpeqR( op, 0x84, 0x83, getReg(INDEXREG) );
            else
                        putOpeqR( op, 0x01, 0x82, getReg(INDEXREG) );
        } else {
            reg = getReg(INDEXREG);
            if (checkChar('+')) {
                if (checkChar('+')) {
                    if (checkChar('+')) {
                        if (checkChar('+')) {
                            putOpeqR(op, 2, 0x84, reg);
                            put1Byte((reg == Y) ? 0x31 :
                                     (reg == U) ? 0x33 :
                                     (reg == S) ? 0x32 : 0x30 );
                            put1Byte(index0(4, reg));
                        } else {
                            putOpeqR(op, 2, 0x84, reg);
                            put1Byte((reg == Y) ? 0x31 :
                                     (reg == U) ? 0x33 :
                                     (reg == S) ? 0x32 : 0x30 );
                            put1Byte(index0(3, reg));
                        }
                    } else {
                        putOpeqR(op, 2, 0x81, reg);
                    }
                } else {
                    putOpeqR(op, 2, 0x80, reg);
                }
            } else {
                putOpeqR(op, 2, 0x84, reg);
            }
        }
    } else {
        val = expression();
        if (checkChar(',')) {
            switch ( (reg = getReg(INDEXREG | PC | PCR)) ) {
            case X:
            case Y:
            case U:
            case S:
                for (val += 2, i = 1; i >= 0; --i, val -= 2) {
                    put1Word(op[i] + 0x20);
                    if (gValid_f && -16 <= val && val <= 15 && indexSmallAllowed()) {
                        put1Byte( index0( indexZeroAllowed(val, reg) ? 0x84 : (val & 0x1f), reg ) );
                    } else if (checkByte(val)) {
                        put1Byte( index0(0x88, reg) );
                        put1Byte(val);
                    } else {
                        put1Byte( index0(0x89, reg) );
                        put1Word(val);
                    }
                }
                break;

            case PC:
                for (val += 2, i = 1; i >= 0; --i, val -= 2) {
                    put1Word(op[i] + 0x20);
                    if (checkByte(val)) {
                        put1Byte( index0(0x8c, 0) );
                        put1Byte(val);
                    } else {
                        put1Byte( index0(0x8d, 0) );
                        put1Word(val);
                    }
                }
                break;

            case PCR:
                val2 = gLinLc + 4;
                for (val += 2, i = 1; i >= 0; --i, val -= 2, val2 += 4) {
                    put1Word(op[i] + 0x20);
                    if (checkByte(val - val2)) {
                        put1Byte( index0(0x8c, 0) );
                        put1Byte(val - val2);
                    } else {
                        val2++;
                        put1Byte( index0(0x8d, 0) );
                        put1Word(val - val2);
                    }
                }
            }
        } else if (gByte_f
            || ((uint16_t)(val - (gDp << 8)) <= 253 && gValid_f && !gWord_f)) {
            put1Word(op[1] + 0x10);
            put1Byte(val - (gDp << 8) + 2);
            put1Word(op[0] + 0x10);
            put1Byte( val - (gDp << 8) );
        } else {
            put1Word(op[1] + 0x30);
            put1Word(val + 2);
            put1Word(op[0] + 0x30);
            put1Word(val);
        }
    }
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

void initOpTbl(void)
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

static CONDITIONAL_RESULT *conditionalResults;
static size_t conditionalCount;
static size_t conditionalCapacity;
static size_t conditionalCursor;

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

static void co_ifEvaluate(uint8_t f)
{
    int val = 0;

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
    } else if (f == CO_IFD || f == CO_IFND) {
        char        name[LBLSIZE + 1];
        LBLTBL_T *  lp;
        skipSpace();
        getLabel(name);
        lp     = refLabel(name);
        val    =  (rsDefined && !strcasecmp(name, "__RS"))
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
        return invExpr();
    result = nextConditional(0);
    if (!result)
        return 0;
    if (gPass == 1) {
        result->value = invExpr() != 0;
        result->consumed = (int)(gLinPtr - start);
        result->errors = gErrors - errors;
    } else {
        gLinPtr += result->consumed;
    }
    return result->value;
}

static int  getMnemonic(void)
{
    static uint8_t  temp[MNEMOSIZE + 1];
    uint8_t*        p = temp;
    uint8_t* pp = temp + MNEMOSIZE;
    OPTBL_T const*  q;

  NEXT_MNEMONIC:
    p = temp;
    skipSpace();
    if (*gLinPtr == '\n' || *gLinPtr == '\0')
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

    while ( isSymbl( *p = toupper(*gLinPtr) )) {
        if (p++ >= pp)
            goto ERR;
        gLinPtr++;
    }
    *p = '\0';

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
            goto NEXT_MNEMONIC;
        }
        gOprPtr = q;
        return 1;
    }
  ERR:
    error("Unknown mnemonic.");
    return 0;
}

static uint8_t *getLine(void)
{
    gLinPtr = (uint8_t*)gLineBuf + LINEHEAD;
    return (uint8_t*)fgets((char*)gLinPtr, MAXCHAR - LINEHEAD, gSrcFp);
}

static uint8_t oneLine(void)
{
    char    temp[LBLSIZE + 1];
    uint8_t c;
    uint8_t f;
    uint8_t gf;

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

    if (c == '*' || c == '#') {
        if (c == '*') {
            char     word[MNEMOSIZE + 1];
            int      n     = 0;
            int      mode  = -1;
            uint8_t* start = ++gLinPtr;
            while (isSymbl(*gLinPtr) && n < MNEMOSIZE)
                word[n++]  = toupper(*gLinPtr++);
            word[n] = 0;
            if      (!strcmp(word, "PRAGMA"))     mode = 1;
            else if (!strcmp(word, "PRAGMAPUSH")) mode = 2;
            else if (!strcmp(word, "PRAGMAPOP"))  mode = 3;

            if (mode >= 0 && (!*gLinPtr || isspace(*gLinPtr))) {
                if (gCoStk[gCo_sp] >= 0) handlePragma(mode);
            } else {
                gLinPtr = start;
            }
        }
        clearAddress();
    } else {
        gf = temp[0] = '\0';
        if (!isspace(c) && c != '\n')
            gf = getLabel(temp);
     #ifdef OPT_OA_FILE
        if (gObjct == OB_ASM && gf && gPass == 2)
            oa_putStr(gLineBuf + LINEHEAD, 0);
     #endif
        f  = getMnemonic();
        if (temp[0] && gCoStk[gCo_sp] >= 0)
            defLabel(temp, f && !strcmp(gOprPtr->mnemonic, "SET") ? 2 : 1, gf);
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
    while (extraIncCount)
        free(extraIncDirs[--extraIncCount]);

    rsCounter = rsDefined = 0;
    soCounter = foCounter = soDefined    = foDefined  = 0;
    remBlock  = failSeen  = offsetActive = lastOffset = rorgBase = 0;
    previousOrg = reorgValid = 0;
    gImVal    = 512;
 #ifdef OPT_OA_FILE
    gOA_sp    =
 #endif
    gSrcLine  =
    branchChanges = relaxChanges =
    gFile_sp  = gLineNo = gErrors  = gP1_sp   = gCo_sp = gObjPos = gRmb_sp =
    gObjCnt   = gLc     = gDp      = gObjLc   =
    gEOF_f    = gGrp    = gCSectSw = gPSect_f = gOrg_f = (uint8_t) 0;
    if (gVerbos_f)
        fprintf(STDERR, (gPass == -1) ? "<pass 1.5>\n" : "<pass %d>\n", gPass);
}

static void assemble(int argc, char * * argv)
{
    int     i;
    uint8_t f;

    initPass();
 #ifdef OPT_FBAS
    if (gPass == 2 && gFBasic_f) {
        putObj(0);
        put2obj(gObjSiz);
        if (gStartAddr == 0xFFFF)
            gStartAddr = 0;
        put2obj(gStartAddr);
    }
 #endif

    for (i = 1; i < argc; i++) {
        if (*argv[i] == '-')
            continue;

        strncpy(gSrcFName, argv[i], FNAMESZ);
        ++gGrp;
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
                if (*gLinPtr != '\n' && !isspace(*gLinPtr)) {
                    error("Unexpected character.");
                    DEBMSGF((STDERR, "*gLinPtr : %c(%02x)\t[asemmble()]\n"
                                   , *gLinPtr, *gLinPtr));
                }
            }
            finishLine();
            putLine();
        }
    }

 #ifdef OPT_FBAS
    if (gFBasic_f && gPass == 2) {
        putObj(0xff);
        put2obj(0x0000);
        if (gEntryAddr == 0xFFFF)
            gEntryAddr = gStartAddr;
        put2obj(gEntryAddr);
        putObj(0x1a);
    }
 #endif
    if (remBlock)
        error("REM without EREM.");
    if (offsetActive) {
        lastOffset   = gLc;
        gLc          = savedCodeLc;
        offsetActive = 0;
    }
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
static uint8_t  oList_f, oSymbol_f;

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
    e_puts(" -?  Show this help\n");
    e_puts(" -3  6309 mode (default)\n");
    e_puts(" -8  6809 mode\n");
    e_puts(" -9  OS-9 standard ASM mode\n");
    e_puts(" -6  Enable M6800-family mnemonic compatibility\n");
    e_puts(" -z  Enable undocumented 6809/6309 opcodes/operands\n");
    e_puts(" -p  Selectable 0, 5, 8, or 16-bit offsets.\n");
    e_puts(" -y  Enable automatic branch sizing\n");
    e_puts(" -q  Allow address gaps caused by ORG or RMB\n");
    e_puts(" -u  Case-sensitive labels    -n  Case-insensitive labels\n");
    e_puts(" -s  Show symbol table        -v  Show progress\n");
    e_puts(" -j  use SJIS character.\n");
    e_puts(" -m<mod_name>  Set the $modnam string variable\n");
    e_puts(" -d<LBL>[=Val] Define LBL as Val (default: 1)\n");
    e_puts(" -t=<ASM>      Assembler compatibility mode: as63 lwasm vasm\n");
    e_puts(" -o[=FILE]     Write binary object to FILE\n");
    e_puts(" -f[=FILE]     Write S-Record object to FILE\n");
    e_puts(" -x[=FILE]     Write a FLEX binary executable to FILE\n");
 #ifdef OPT_OA_FILE
    e_puts(" -a[=FILE]     Write object as FCB data to FILE\n");
 #endif
    e_puts(" -e[=ERR_FILE] Write source errors to ERR_FILE\n");
 #ifdef INCLUDIR
    e_puts(" -i[=INC_DIR]  Set the directory referenced by $INC\n");
 #endif
    e_puts(" -l[=LST_FILE] Write assembly listing to LIST_FILE\n");
 #ifdef OPT_FBAS
    e_puts(" -k[Start[,Enter]]  Write an F-BASIC machine-language file\n");
    e_puts(" -r  In F-BASIC format, omit trailing zeros after RMB\n");
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
        gLblPtr->line  = 1;
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
    uint8_t const *pp;
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
                oObjFName  = p;
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

     #ifdef INCLUDIR
        case 'I':
            gIncDirName    = ".";
            if (*p)
                gIncDirName = p;
            if (strlen(gIncDirName) >= FNAMESZ - 10) {
                e_puts("File name is too long.\n");
                exit(1);
            }
            goto LOOPOUT;
     #endif

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
 #ifdef INCLUDIR
    gIncDirName    = INCLUDIR;
 #endif
    gErrFp         = STDERR;
    gErrFName      =
        oLstFName  = oObjFName = NULL;
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
            options(p);
        }
    }

    if (*gSrcFName == '\0') {
        fprintf(STDERR, "usage: %s [-opts] src_file...(-? help)\n",
                gCmdName);
        exit(1);
    }
    if (oObjFName == NULL && gObjct) {
        char * filename = mallocE(FNAMESZ + 1);
        oObjFName = filename;
        FIL_ChgExt(strcpy(filename, gSrcFName),
      #ifdef OPT_OA_FILE
            (gObjct == OB_ASM) ? "oa" :
      #endif
            (gObjct == OB_SFMT) ? "s" :
            gFlex_f ? "cmd" :
            "o");
    }
    if (gErrFName == (char *) (~0)) {
        char * filename = mallocE(FNAMESZ + 1);
        gErrFName = filename;
        FIL_ChgExt(strcpy(filename, gSrcFName), "err");
    }
    if (*gModName == '\0') {
        getModNam(gModName, gSrcFName);
    }

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
