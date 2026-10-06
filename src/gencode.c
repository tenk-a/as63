/* HD6309 instruction encoding, data generation and object output. */
#include "gencode.h"
#include <ctype.h>

/* Object buffers, section state and instruction-local state. */
FILE *                  gObjFp;
uint16_t                gObjCnt;
static uint16_t         gObjLc;
uint16_t                gEntryAddr;
uint8_t                 gObjct;
int                     gObjBufSz;
static uint8_t          gCrcBuf[3];
uint8_t                 gRmb_f;
int                     gRmb_sp;
uint8_t                 gFlex_f;
#ifdef OPT_FBAS
uint8_t                 gFBasic_f;
#endif
static int              gImVal;
static int              gIndirect;
static int              lastRtsAddress = -1;
static int              gObjPos;
static uint8_t          gObjBuf[OBJSIZE];
static int              offsetActive;
static uint8_t          gOrg_f;
static uint8_t          gPSect_f;
static uint16_t         savedCodeLc;
static uint16_t         lastOffset;
static uint16_t         rorgBase;
static uint16_t         previousOrg;
static uint8_t          reorgValid;
uint16_t                gStartAddr;
uint8_t                 gOrgSFmt_f;
uint16_t                gCSectBase;
uint8_t                 gCSectSw;
static int oPostf;
static int oPos;

void initCodePass(void)
{
    gImVal = 512;
    lastRtsAddress = -1;
    gObjPos = 0;
    gObjLc = 0;
    gObjCnt = 0;
    gRmb_sp = 0;
    offsetActive = 0;
    lastOffset = 0;
    rorgBase = 0;
    previousOrg = 0;
    reorgValid = 0;
    gCSectSw = 0;
    gPSect_f = 0;
    gOrg_f = 0;
}

void initCodeLine(void)
{
    oPostf = 15;
    oPos = 10;
}

/* Object records, checksums and emitted bytes. */
static uint16_t oChkSum;    /* Checksum for each line of S-format */

static uint8_t hexDigit(uint8_t x)
{
    return ((x &= 0x0f) < 10) ? x + '0' : x - 10 + 'A';
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

static void    flushObj(void)
{
    int i = 0;

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
    int w = 0;

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

static void    putObj(int b)
{
    if (gPass != 2)
        return;
    if (gObjPos >= gObjBufSz)
        flushObj();
    gObjBuf[gObjPos++] = (uint8_t) b;
}

static void    put2obj(int w)
{
    w = (uint16_t) w;
    putObj(w >> 8);
    putObj(w);
}

static uint8_t    putB(int b)
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

static int     put2B(uint16_t w)
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

/* Emission with listing continuation. */
static void flushLine(void)
{
    char *p = gLineBuf;

    putLine();
    for (p = gLineBuf; p < gLineBuf + LINEHEAD; p++)
        *p = ' ';
    *p++ = '\n';
    *p = '\0';
    printAddress(gLc);
    oPos = 10;
}

static void    put1Byte(int b)
{
    if ((23 - 3) < oPos )
        flushLine();
    printByte(putB(b), oPos);
    oPos += 3;
}

static void    put1Byt2(int b)
{
    if (23 < oPos)
        flushLine();
    printByte(putB(b), oPos);
    oPos += 2;
}

static void    put1Word(int w)
{
    if (23 - 5 < oPos)
        flushLine();
    printWord(put2B(w), oPos);
    oPos += 5;
}

/* Operand bytes and addressing modes. */
static void    putByte(int b)
{
    b = (uint8_t) b;
    printByte(putB(b), oPostf);
}

static void    postByte(int b)
{
    printByte(putB(b), oPostf);
    oPostf += 3;
}

static void    putWord(int w)
{
    printWord(put2B(w), oPostf);
}

static void    imm4Expr(int16_t * v1, int16_t * v2)
{
    val_t val = 0;

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

static void    putCode(int grp, int mode)
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

static int     getReg(int r)
{
    int         reg = 0;
    uint8_t     c = 0;
    uint8_t     d = 0;
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
        } else {
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

static int     regNo(int r)
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

static int     checkByte(int b)
{
    if (pragmaOperandSizeWarning && gPass == 2 && (gValid_f || expressionForward) && gWord_f
        && b >= -128 && b <= 127)
        warning("An 8-bit indexed offset can be used.");
    return ( gByte_f || (-128 <= b && b <= 127 && gValid_f && !gWord_f) );
}

static int     index0(int frame, int reg)
{
    int xr = 0;

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
    return (!gIdxOfs_f && gCompatMode != COMPAT_LWASM && !pragmaIndexConfigured())
        || (!gByte_f && !gWord_f);
}

static int indexZeroAllowed(val_t value, int reg)
{
    if (!gValid_f || value != 0)
        return 0;
    if (gCompatMode == COMPAT_LWASM || pragmaIndexConfigured()) {
        return !gByte_f && !gWord_f
            && (reg == W || (!gIdxOfs_f && pragmaIndex0 && !expressionLiteralZero));
    }
    return (!gIdxOfs_f || reg == W) && indexSmallAllowed();
}

static void operand(int grp, int mode)
{
    val_t   val = 0;
    int     reg = 0;

    skipSpace();
    if (( mode & (IMMEDIATE | IMMEDIATE2) ) && checkChar('#')) {
        putCode(grp, IMMEDIATE_MODE);
        if (mode & IMMEDIATE) {
            putByte(bytExpr());
        } else {
            wordCharacter = 1;
            putWord(expression());
            wordCharacter = 0;
        }
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
                /* error("Invalid register."); */
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
    int   reg = 0;
    val_t source = 0;
    val_t destination = 0;
    val_t address = 0;
    val_t offset = 0;

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
    val_t   val = 0;

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
    int16_t val = 0;
    int16_t val2 = 0;

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
    if (!gOprPtr->prefix && gOprPtr->opcode == 0x39)
        lastRtsAddress = (uint16_t)(gLc - 1);
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
    size_t j = 0;

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
    int         found = 0;

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
    val_t       val = 0;
    uint8_t *   p = NULL;
    int         base = 0;
    size_t      i = 0;

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
    int i = 0;

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
    HC11MEM_T   m = {0};
    uint8_t     mask = 0;
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
    HC11MEM_T   m = {0};
    uint8_t     mask = 0;
    val_t       target = 0;
    val_t       disp = 0;
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
    HC11MEM_T   m = {0};
    int         isMin = gOprPtr->opcode & 1;
    int         isMem = gOprPtr->opcode & 2;
    int         skip = 0;

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
    uint8_t const*  p = NULL;
    int             j = 0;

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

void    transfer(void)
{
    int r1 = 0;
    int r2 = 0;

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
    int     r1 = 0;
    int     r2 = 0;
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

static int qrtsBranch(int originallyLong)
{
    uint8_t *target = gLinPtr;
    int forced = 0;
    int opcode = gOprPtr->opcode;
    int distance = 0;
    int longBranch = originallyLong && !pragmaAutoBranch;
    if (!pragmaQrts)
        return 0;
    while (isspace(*target) && *target != '\n')
        ++target;
    if (*target == '<' || *target == '>') {
        forced = *target++;
        longBranch = forced == '>';
    }
    if (strncasecmp((char const *)target, "?RTS", 4) || isSymbl3(target[4]))
        return 0;
    gLinPtr = target + 4;
    if (opcode == 0x16)
        opcode = 0x20;
    if (opcode < 0x20 || opcode > 0x2f) {
        error("?RTS requires a conditional branch or BRA/BRN.");
        return 1;
    }
    distance = lastRtsAddress - gLinLc - 2;
    if (lastRtsAddress >= 0 && distance >= -128 && distance <= 127) {
        if (longBranch) {
            if (opcode == 0x20) {
                put1Byte(0x16);
                put1Word(lastRtsAddress - gLinLc - 3);
            } else {
                put1Byte(0x10);
                put1Byte(opcode);
                put1Word(lastRtsAddress - gLinLc - 4);
            }
        } else {
            put1Byte(opcode);
            put1Byte(distance);
        }
    } else {
        if (longBranch) {
            if (opcode == 0x20 || opcode == 0x21) {
                put1Byte(opcode == 0x20 ? 0x21 : 0x20);
                put1Byte(1);
            } else {
                put1Byte(0x10);
                put1Byte(opcode ^ 1);
                put1Word(1);
            }
        } else {
            put1Byte(opcode ^ 1);
            put1Byte(1);
        }
        put1Byte(0x39);
        lastRtsAddress = (uint16_t)(gLc - 1);
    }
    return 1;
}

void    branch(void)
{
    val_t val = 0;

    if (qrtsBranch(0))
        return;
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
    if (qrtsBranch(1))
        return;
    if (pragmaAutoBranch) {
        automaticBranch(1);
        return;
    }
    skipSpace();
    target = expression();
    if (pragmaOperandSizeWarning && gPass == 2 && (gValid_f || expressionForward)
        && target - gLinLc - 2 >= -128 && target - gLinLc - 2 <= 127)
        warning("A short branch can be used.");
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

/* Synthetic instructions. */
void registerConvenience(void)
{
    int reg = gOprPtr->mnemonic[strlen(gOprPtr->mnemonic) - 1] == 'E' ? 14 : 15;
    put1Word(0x1000 + gOprPtr->opcode);
    put1Byte((gOprPtr->opcode == 0x30 ? reg : 12) * 16 + reg);
}

void    none_d(void)
{
    uint8_t const* p = NULL;
    uint8_t        i = 0;
    uint8_t        l = 0;
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
    int val = 0;
    int reg = 0;
    int i = 0;
    int val2 = 0;

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
    int16_t     val = 0;
    int16_t     val2 = 0;
    int16_t     reg = 0;
    int16_t     i = 0;
    uint16_t    op[2] = {0, 0};

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
            if (checkChar('-')) {
                if (checkChar('-')) {
                    if (checkChar('-')) {
                        putOpeqR(op, 0x83, 0x83, getReg(INDEXREG));
                    } else {
                        putOpeqR(op, 0x82, 0x83, getReg(INDEXREG));
                    }
                } else {
                    putOpeqR(op, 0x84, 0x83, getReg(INDEXREG));
                }
            } else {
                putOpeqR(op, 0x01, 0x82, getReg(INDEXREG));
            }
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

/* Data, module and section directives. */
void    setdp(void)
{
    skipSpace();
    clearAddress();
    printByte(gDp = invExpr(), 15);
}

void    mod(void)
{
    uint16_t      os9hdr[4] = {0, 0, 0, 0};
    uint8_t       sum = 0;
    uint8_t       l = 0;
    uint8_t const *p = NULL;

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
        uint16_t high = (uint16_t)((uval_t)value >> 16);
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
    val_t val = 0;

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
                uint8_t b = 0;
                uint8_t c = 0;
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

static int stringEscape(void)
{
    int     value = 0;
    int     digits = 0;
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
    uint8_t b = 0;

    skipSpace();
    if (!*gLinPtr || *gLinPtr == '\n') {
        error("Missing string operand.");
        return;
    }
    do {
        uint8_t c = *gLinPtr++;
        if (c == '$' && toupper(*gLinPtr) == 'M') {
            getLabel(temp);
            if (strcasecmp(temp, "modnam") == 0 /*&& gModName*/) {
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
    val_t count = 0;
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

void reserveBytes(val_t count, int checked)
{
    val_t position = gCSectSw ? gCSectBase : gLc;
    if (checked && (count < 0 || count > 65536L - position)) {
        error("Reservation exceeds address space or has a negative count.");
        return;
    }
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

void rmb(void)
{
    int size = gOprPtr->prefix ? gOprPtr->prefix : 1;
    int checked = gOprPtr->opcode & BLOCK_CHECK;
    val_t count = 0;
    skipSpace();
    count = invExpr();
    if (checked && (count < 0 || count > 65535 / size)) {
        error("Reservation exceeds address space or has a negative count.");
        return;
    }
    reserveBytes(count * size, checked);
}

void    org(void)
{
    uint16_t origin = 0;

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

void incbin(void)
{
    FILE*   fp = NULL;
    char    name[FNAMESZ + 1];
    val_t   offset    = 0;
    val_t   length    = -1;
    long    fileSize  = 0;
    long    available = 0;
    int     searchOnly = 0;

    if (!fileOperand(name, sizeof(name), &searchOnly))
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
    fp = openSearch(name, "rb", searchOnly);
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
    long    count = 0;
    val_t   position = (gCSectSw == 2) ? gCSectBase : gLc;
    if (gOprPtr->prefix == 1) {
        skipSpace();
        boundary = invExpr();
        if (gCompatMode == COMPAT_VASM) {
            if (boundary < 0 || boundary > 16) {
                error("ALIGN exponent must be 0..16.");
                return;
            }
            boundary = (val_t)1L << boundary;
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

void relativeData(void)
{
    val_t value = 0;
    skipSpace();
    do {
        skipSpace();
        value = expression() - gLc;
        dataValue(value, gOprPtr->prefix, 1);
    } while (nextComma());
}

void relativeOrg(void)
{
    val_t target = 0;
    val_t fill = 0;
    skipSpace();
    target = invExpr();
    if (nextComma()) {
        skipSpace();
        fill = expression();
    }
    if (target < 0 || target > 65535L - rorgBase || target + rorgBase < gLc) {
        error("Invalid RORG operand.");
        return;
    }
    fillBlock(target + rorgBase - gLc, fill, 1, BLOCK_CHECK);
}

void offsetSection(void)
{
    val_t start = 0;
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

/* Output framing invoked by the assembly driver. */
void beginCodeOutput(uint16_t size)
{
 #ifdef OPT_FBAS
    if (gPass == 2 && gFBasic_f) {
        putObj(0);
        put2obj(size);
        if (gStartAddr == 0xFFFF)
            gStartAddr = 0;
        put2obj(gStartAddr);
    }
 #endif
#ifndef OPT_FBAS
    (void)size;
#endif
}

void endCodeOutput(void)
{
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
}

void endCodeSection(void)
{
    if (offsetActive) {
        lastOffset = gLc;
        gLc = savedCodeLc;
        offsetActive = 0;
    }
}
