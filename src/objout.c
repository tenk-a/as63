/* Relocatable expressions, sections and LWOBJ16 / VOBJ object output. */
#include "gencode.h"
#include "objout.h"
#include <stdlib.h>

#ifdef SMALL_HOST
#define REL_STACK       16
#else
#define REL_STACK       64
#endif
#define REL_EXTERNAL    0x4000    /* symbol ids from here name externals */

typedef struct OBJ_RELOC {
    struct OBJ_RELOC *  next;
    uint16_t            offset;
    uint8_t             size;
    uint8_t             length;
    uint8_t             pc;         /* VOBJ: relative to the field address */
    int                 symbol;     /* VOBJ: section index or REL_EXTERNAL + n */
    unsigned long       mask;       /* VOBJ: bits taken from symbol + addend */
    long                addend;
    uint8_t             code[REL_CODE];
} OBJ_RELOC;

typedef struct OBJ_SYMBOL {
    struct OBJ_SYMBOL * next;
    long                value;
    char *              name;
} OBJ_SYMBOL;

typedef struct OBJ_SECTION {
    char *              name;
    char *              attr;           /* VOBJ attribute letters */
    uint8_t             flags;
    int                 pass;           /* pass number of the last entry */
    uint16_t            lc;
    long                size;
    uint8_t *           code;
    long                capacity;
    OBJ_RELOC *         relocs;
    OBJ_RELOC **        relocTail;
    OBJ_SYMBOL *        locals;
    OBJ_SYMBOL *        exports;
    REL_VALUE           base;
} OBJ_SECTION;

uint8_t                 gRelActive;

static OBJ_SECTION **   sections;
static int              sectionCount;
static int              currentSection;     /* index + 1; 0 outside sections */
static int              passNumber;
static char **          externals;
static int              externalCount;
static OBJ_SYMBOL *     absExports;         /* VOBJ exported constants */

static REL_VALUE        relStack[REL_STACK];
static int              relSp;
static REL_VALUE        relResult;
static uint8_t          relResultSet;
static uint8_t          relValid;
static uint8_t          relOverflow;
static uint8_t          outsideReported;


/*---------------------------------------------------------------------------*/

static char *copyName(char const *s)
{
    char *p = mallocE(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

static void *growArray(void *data, int count, size_t size)
{
    /* Arrays grow in blocks of 16 entries. */
    void *p;
    if (count % 16)
        return data;
    p = mallocE((count + 16) * size);
    if (count)
        memcpy(p, data, count * size);
    free(data);
    return p;
}

static int isAbsolute(REL_VALUE const *v)
{
    return !v->complex && !v->terms;
}

static int plain(REL_VALUE const *v)
{
    return !v->complex && !v->mask;
}

static int isBase(REL_VALUE const *v)
{
    int i;
    for (i = 0; i < sectionCount; ++i)
        if (v == &sections[i]->base)
            return 1;
    return 0;
}

static void addTerm(REL_VALUE *v, int symbol, long scale)
{
    int i;
    if (!scale)
        return;
    for (i = 0; i < v->terms; ++i) {
        if (v->symbol[i] == symbol) {
            v->scale[i] += scale;
            if (!v->scale[i]) {
                --v->terms;
                v->symbol[i] = v->symbol[v->terms];
                v->scale[i]  = v->scale[v->terms];
            }
            return;
        }
    }
    if (v->terms >= REL_TERMS) {
        relOverflow = 1;
        return;
    }
    v->symbol[v->terms]  = (int16_t)symbol;
    v->scale[v->terms++] = scale;
}

static void addValue(REL_VALUE *v, REL_VALUE const *w, long factor)
{
    int i;
    for (i = 0; i < w->terms; ++i)
        addTerm(v, w->symbol[i], w->scale[i] * factor);
}

static void scaleValue(REL_VALUE *v, long factor)
{
    int i;
    if (!factor)
        v->terms = 0;
    for (i = 0; i < v->terms; ++i)
        v->scale[i] *= factor;
}

/* Postfix code of LWOBJ16 incomplete references. */
static void codeByte(REL_VALUE *v, int b)
{
    if (v->length >= REL_CODE) {
        relOverflow = 1;
        return;
    }
    v->code[v->length++] = (uint8_t)b;
}

static void codeInt(REL_VALUE *v, val_t n)
{
    codeByte(v, 0x01);
    codeByte(v, (int)((n >> 8) & 0xff));
    codeByte(v, (int)(n & 0xff));
}

static void codeOp(REL_VALUE *v, int op)
{
    codeByte(v, 0x04);
    codeByte(v, op);
}

static void codeName(REL_VALUE *v, char const *s)
{
    while (*s)
        codeByte(v, *s++);
    codeByte(v, 0);
}

static void codeSymbol(REL_VALUE *v, int symbol)
{
    if (symbol >= REL_EXTERNAL) {
        codeByte(v, 0x02);
        codeName(v, externals[symbol - REL_EXTERNAL]);
    } else {
        /* Section bases are the synthetic local symbol "\x02name". */
        codeByte(v, 0x03);
        codeByte(v, 0x02);
        codeName(v, sections[symbol]->name);
    }
}

static void codeValue(REL_VALUE *out, REL_VALUE const *v, val_t value)
{
    int i;
    int items = 0;
    if (v->complex) {
        for (i = 0; i < v->length; ++i)
            codeByte(out, v->code[i]);
        return;
    }
    if (value || !v->terms) {
        codeInt(out, value);
        ++items;
    }
    for (i = 0; i < v->terms; ++i) {
        codeSymbol(out, v->symbol[i]);
        if (v->scale[i] != 1) {
            codeInt(out, v->scale[i]);
            codeOp(out, REL_TIMES);
        }
        if (++items > 1)
            codeOp(out, REL_PLUS);
    }
}


/*---------------------------------------------------------------------------*/
/* Evaluation stack: each term pushes one value and each operator folds them. */

static REL_VALUE *relPush(void)
{
    REL_VALUE *v = NULL;
    if (relSp >= REL_STACK) {
        relOverflow = 1;
        v = &relStack[REL_STACK - 1];
    } else {
        v = &relStack[relSp++];
    }
    v->terms = v->complex = v->length = 0;
    v->mask  = 0;
    v->premask = 0;
    return v;
}

static REL_VALUE relPop(void)
{
    static REL_VALUE const absolute;
    if (relSp <= 0)
        return absolute;
    return relStack[--relSp];
}

void relBegin(void)
{
    if (!gRelActive)
        return;
    if (relResultSet && gPass == 2)
        error("Relocatable value cannot be used here.");
    relResultSet = 0;
    relOverflow  = 0;
    relSp        = 0;
}

void relFinish(void)
{
    if (!gRelActive)
        return;
    relResult = relPop();
    relSp     = 0;
    if (relOverflow) {
        if (gPass == 2)
            error("Relocatable expression is too complex.");
        relOverflow = 0;
        relResult.terms = relResult.complex = relResult.length = 0;
        relResult.mask  = 0;
        gValid_f = 0;
    }
    if (!isAbsolute(&relResult)) {
        relResultSet = 1;
        relValid     = gValid_f;
        gValid_f     = 0;
    }
}

int relDepth(void)
{
    return relSp;
}

void relSettle(int depth)
{
    if (!gRelActive)
        return;
    while (relSp < depth + 1)
        relPush();
    if (relSp > depth + 1)
        relSp = depth + 1;
}

void relPushValue(REL_VALUE const *value)
{
    REL_VALUE *v;
    if (!gRelActive)
        return;
    v = relPush();
    if (value)
        *v = *value;
}

void relPushHere(void)
{
    relPushValue(relLabelBase());
}

void relPushExternal(char const *name)
{
    REL_VALUE *v;
    int i;
    if (!gRelActive)
        return;
    for (i = 0; i < externalCount; ++i)
        if (!strcmp(externals[i], name))
            break;
    if (i == externalCount) {
        externals = growArray(externals, externalCount, sizeof(*externals));
        externals[externalCount++] = copyName(name);
    }
    v = relPush();
    addTerm(v, REL_EXTERNAL + i, 1);
}

void relUnary(int op, val_t value)
{
    REL_VALUE v;
    REL_VALUE *r;
    int i;
    if (!gRelActive)
        return;
    v = relPop();
    r = relPush();
    if (isAbsolute(&v))
        return;
    if (op == REL_NEG && plain(&v)) {
        *r = v;
        for (i = 0; i < r->terms; ++i)
            r->scale[i] = -r->scale[i];
        return;
    }
    r->complex = 1;
    codeValue(r, &v, value);
    if (op == REL_NOT) {
        codeInt(r, 0);
        codeOp(r, REL_EQ);
    } else {
        codeOp(r, op);
    }
}

static int lowestBit(unsigned long m)
{
    int n = 0;
    while (!(m & 1)) {
        m >>= 1;
        ++n;
    }
    return n;
}

/* VOBJ keeps "value & 2^k-1" and "value >> n" as one mask: (v & mask) >> ctz(mask). */
static int relMasked(int op, REL_VALUE const *l, REL_VALUE const *r, val_t left, val_t right, REL_VALUE *v)
{
    REL_VALUE const *x;
    val_t k, xv;
    unsigned long base, mask;
    int shift, n;
    if (isAbsolute(l)) {
        x = r; k = left; xv = right;
    } else if (isAbsolute(r)) {
        x = l; k = right; xv = left;
    } else {
        return 0;
    }
    if (x->complex)
        return 0;
    if (x == r && op != REL_BWAND)
        return 0;
    base  = x->mask ? x->mask : 0xffffUL;
    shift = x->mask ? lowestBit(x->mask) : 0;
    switch (op) {
    case REL_SHR:
        if (k < 0 || k > 15)
            return 0;
        n = (int)k;
        break;
    case REL_DIVIDE:
        for (n = 1; n < 16 && ((val_t)1 << n) != k; ++n)
            ;
        if (n >= 16)
            return 0;
        break;
    case REL_BWAND:
        if (k <= 0 || (k & (k + 1)))
            return 0;
        mask = base & ((unsigned long)k << shift);
        goto done;
    default:
        return 0;
    }
    mask = base & (~0UL << (shift + n));
  done:
    if (!mask)
        return 0;
    *v = *x;
    v->mask = mask;
    if (!x->mask)
        v->premask = (long)xv;
    return 1;
}

int relShortPcrAllowed(void)
{
    return gOutFmt == OB_VOBJ;
}

void relBinary(int op, val_t left, val_t right)
{
    REL_VALUE l;
    REL_VALUE r;
    REL_VALUE d;
    REL_VALUE *v;
    if (!gRelActive)
        return;
    r = relPop();
    l = relPop();
    v = relPush();
    if (isAbsolute(&l) && isAbsolute(&r))
        return;
    if (gOutFmt == OB_VOBJ && relMasked(op, &l, &r, left, right, v))
        return;
    switch (op) {
    case REL_PLUS:
    case REL_MINUS:
        if (plain(&l) && plain(&r)) {
            *v = l;
            addValue(v, &r, op == REL_MINUS ? -1 : 1);
            return;
        }
        break;
    case REL_TIMES:
        if (isAbsolute(&l) && plain(&r)) {
            *v = r;
            scaleValue(v, (long)left);
            return;
        }
        if (isAbsolute(&r) && plain(&l)) {
            *v = l;
            scaleValue(v, (long)right);
            return;
        }
        break;
    case REL_SHL:
    case REL_SHR:
        if (!isAbsolute(&r) || right < 0 || right > 15) {
            if (gPass == 2)
                error("Relocatable shift is not supported.");
            return;
        }
        if (op == REL_SHL && plain(&l)) {
            *v = l;
            scaleValue(v, 1L << right);
            return;
        }
        v->complex = 1;
        codeValue(v, &l, left);
        codeInt(v, 1L << right);
        codeOp(v, op == REL_SHL ? REL_TIMES : REL_DIVIDE);
        return;
    case REL_EQ:
    case REL_NE:
    case REL_LT:
    case REL_LE:
    case REL_GT:
    case REL_GE:
        /* Values in the same section compare by their offsets. */
        if (plain(&l) && plain(&r)) {
            d = l;
            addValue(&d, &r, -1);
            if (isAbsolute(&d))
                return;
        }
        break;
    }
    v->complex = 1;
    codeValue(v, &l, left);
    codeValue(v, &r, right);
    codeOp(v, op);
}


/*---------------------------------------------------------------------------*/
/* The relocatable result of the last expression until a field consumes it. */

int relPending(void)
{
    return relResultSet;
}

void relDiscard(void)
{
    relResultSet = 0;
}

REL_VALUE const *relReplace(REL_VALUE const *old, REL_VALUE const *value)
{
    if (old && !isBase(old))
        free((void *)old);
    return value;
}

REL_VALUE const *relStore(REL_VALUE const *old)
{
    REL_VALUE *v = NULL;
    if (relResultSet) {
        relResultSet = 0;
        v  = mallocE(sizeof(*v));
        *v = relResult;
    }
    return relReplace(old, v);
}

unsigned long relSignature(REL_VALUE const *v)
{
    unsigned long h = 0;
    int i;
    if (!v)
        return 0;
    h = v->complex ? 0x9e37UL : 1;
    h = h * 31 + v->mask;
    h = h * 31 + (unsigned long)v->premask;
    for (i = 0; i < v->terms; ++i)
        h = h * 31 + (unsigned long)v->symbol[i] * 17 + (unsigned long)v->scale[i];
    for (i = 0; i < v->length; ++i)
        h = h * 31 + v->code[i];
    return h ? h : 1;
}

REL_VALUE const *relLabelBase(void)
{
    OBJ_SECTION const *s;
    if (!gRelActive || !currentSection || offsetBlockActive())
        return NULL;
    s = sections[currentSection - 1];
    return (s->flags & SECTION_CONSTANT) ? NULL : &s->base;
}

void relPcr(void)
{
    REL_VALUE const *base = relLabelBase();
    if (!base)
        return;
    if (!relResultSet) {
        relResult.terms = relResult.complex = relResult.length = 0;
        relResult.mask  = 0;
        relResultSet = 1;
        relValid     = gValid_f;
        gValid_f     = 0;
    }
    if (relResult.complex) {
        if (gPass == 2)
            error("Relocatable PC-relative expression is too complex.");
        return;
    }
    addValue(&relResult, base, -1);
    if (isAbsolute(&relResult)) {
        relResultSet = 0;
        gValid_f     = relValid;
    }
}

/* VOBJ relocations are one symbol plus an addend, optionally PC-relative. */
static void vobjReloc(OBJ_SECTION *s, int size, val_t value)
{
    REL_VALUE const *v = &relResult;
    OBJ_RELOC *r;
    int plus = -1;
    int minus = -1;
    int ok = !v->complex;
    int i;
    for (i = 0; ok && i < v->terms; ++i) {
        if (v->scale[i] == 1 && plus < 0)
            plus = v->symbol[i];
        else if (v->scale[i] == -1 && minus < 0)
            minus = v->symbol[i];
        else
            ok = 0;
    }
    if (plus < 0 || (minus >= 0 && (minus != currentSection - 1 || v->mask)))
        ok = 0;
    if (!ok) {
        error("Relocation is not supported by the vobj format.");
        return;
    }
    r = mallocE(sizeof(*r));
    memset(r, 0, sizeof(*r));
    r->offset = gLc;
    r->size   = (uint8_t)size;
    r->pc     = minus >= 0;
    r->symbol = plus;
    r->mask   = v->mask;
    r->addend = (v->mask ? v->premask : (long)value) + (r->pc ? (long)gLc : 0L);
    *s->relocTail = r;
    s->relocTail  = &r->next;
}

void relocField(int size, val_t value)
{
    OBJ_SECTION *s;
    OBJ_RELOC *r;
    REL_VALUE code;
    if (!relResultSet)
        return;
    relResultSet = 0;
    if (gPass != 2 || !currentSection || offsetBlockActive())
        return;
    s = sections[currentSection - 1];
    if (s->flags & (SECTION_BSS | SECTION_CONSTANT))
        return;
    if (gOutFmt == OB_VOBJ) {
        vobjReloc(s, size, value);
        return;
    }
    code.length = 0;
    relOverflow = 0;
    codeValue(&code, &relResult, value);
    if (relOverflow) {
        error("Relocatable expression is too complex.");
        relOverflow = 0;
        return;
    }
    r = mallocE(sizeof(*r));
    memset(r, 0, sizeof(*r));
    r->offset  = gLc;
    r->size    = (uint8_t)size;
    r->length  = code.length;
    memcpy(r->code, code.code, code.length);
    *s->relocTail = r;
    s->relocTail  = &r->next;
}

void relEndLine(void)
{
    if (relResultSet && gPass == 2)
        error("Relocatable value cannot be used here.");
    relResultSet    = 0;
    outsideReported = 0;
}


/*---------------------------------------------------------------------------*/
/* Sections keep their own location counters; output goes to memory images. */

void objInitPass(void)
{
    int i;
    for (i = 0; i < sectionCount; ++i) {
        sections[i]->lc   = 0;
        sections[i]->size = 0;
    }
    ++passNumber;
    currentSection = 0;
    relResultSet   = 0;
}

int objSection(void)
{
    return currentSection;
}

int objSectionFlags(void)
{
    return currentSection ? sections[currentSection - 1]->flags : 0;
}

static char const *defaultAttr(char const *name)
{
    if (!strcasecmp(name, "bss") || !strcasecmp(name, ".bss"))
        return "aurw";
    if (!strcasecmp(name, "data") || !strcasecmp(name, ".data"))
        return "adrw";
    return "acrx";
}

uint16_t objEnterSection(char const *name, int option, char const *attr, uint16_t lc)
{
    OBJ_SECTION *s = NULL;
    int i;
    if (currentSection)
        objLeaveSection(lc);
    for (i = 0; i < sectionCount; ++i) {
        if (!strcmp(sections[i]->name, name)) {
            s = sections[i];
            break;
        }
    }
    if (s && (option || attr) && s->pass == passNumber)
        warning("[WARNING] Section flags of an existing section are ignored.");
    if (!s) {
        s = mallocE(sizeof(*s));
        memset(s, 0, sizeof(*s));
        s->name      = copyName(name);
        s->relocTail = &s->relocs;
        if (gOutFmt == OB_VOBJ) {
            s->attr = copyName(attr ? attr : defaultAttr(name));
            if (strchr(s->attr, 'u'))
                s->flags |= SECTION_BSS;
        } else {
            if (!strcasecmp(name, "bss") || !strcasecmp(name, ".bss"))
                s->flags |= SECTION_BSS;
            if (!strcasecmp(name, "_constant") || !strcasecmp(name, "_constants"))
                s->flags |= SECTION_CONSTANT;
            switch (option) {
            case 1: s->flags |=  SECTION_BSS;      break;
            case 2: s->flags &= ~SECTION_BSS;      break;
            case 3: s->flags |=  SECTION_CONSTANT; break;
            case 4: s->flags &= ~SECTION_CONSTANT; break;
            }
        }
        addTerm(&s->base, i, 1);
        sections = growArray(sections, sectionCount, sizeof(*sections));
        sections[sectionCount++] = s;
    }
    s->pass = passNumber;
    currentSection = i + 1;
    return (s->flags & SECTION_CONSTANT) ? 0 : s->lc;
}

uint16_t objLeaveSection(uint16_t lc)
{
    OBJ_SECTION *s;
    if (!currentSection)
        return lc;
    objTrack(lc);
    s = sections[currentSection - 1];
    if (!(s->flags & SECTION_CONSTANT))
        s->lc = lc;
    currentSection = 0;
    return 0;
}

void objTrack(uint16_t lc)
{
    OBJ_SECTION *s;
    if (!currentSection)
        return;
    s = sections[currentSection - 1];
    if (!(s->flags & SECTION_CONSTANT) && lc > s->size)
        s->size = lc;
}

void objPutByte(uint16_t offset, uint8_t b)
{
    OBJ_SECTION *s;
    if (!currentSection) {
        if (!outsideReported)
            error("Instruction generating output outside of a section.");
        outsideReported = 1;
        return;
    }
    s = sections[currentSection - 1];
    if (!(s->flags & SECTION_CONSTANT) && offset + 1L > s->size)
        s->size = offset + 1L;
    if (s->flags & (SECTION_BSS | SECTION_CONSTANT))
        return;
    if (offset >= s->capacity) {
        /* Grow in 1 KiB steps; a 16-bit host cannot hold the final byte. */
        long     n = ((long)offset | 0x3ff) + 1;
        uint8_t *p;
        if (n > 0xffffL)
            n = 0xffffL;
        if (offset >= n) {
            error("Section exceeds 65535 bytes.");
            return;
        }
        p = mallocE((size_t)n);
        memset(p, 0, (size_t)n);
        if (s->code)
            memcpy(p, s->code, (size_t)s->capacity);
        free(s->code);
        s->code     = p;
        s->capacity = n;
    }
    s->code[offset] = b;
}


/*---------------------------------------------------------------------------*/
/* Symbols and the LWOBJ16 file. */

static int symbolSection(REL_VALUE const *rel, int section)
{
    if (!rel)
        return (section && (sections[section - 1]->flags & SECTION_CONSTANT)) ? section : 0;
    if (rel->complex || rel->mask || rel->terms != 1 || rel->scale[0] != 1
        || rel->symbol[0] >= REL_EXTERNAL)
        return 0;
    return rel->symbol[0] + 1;
}

static void addSymbol(OBJ_SYMBOL **list, char const *name, val_t value)
{
    OBJ_SYMBOL *p = mallocE(sizeof(*p));
    p->name  = copyName(name);
    p->value = (long)value;
    p->next  = NULL;
    while (*list)
        list = &(*list)->next;
    *list = p;
}

void objLocalSymbol(char const *name, int group, REL_VALUE const *rel, int section, val_t value)
{
    char buf[LBLSIZE + 16];
    int s = symbolSection(rel, section);
    if (!s)
        return;
    /* Like lwasm, "\x01" and a context number keep file-local names apart. */
    if (group)
        sprintf(buf, "%s\x01%d", name, group);
    else
        strcpy(buf, name);
    addSymbol(&sections[s - 1]->locals, buf, value);
}

void objExportSymbol(char const *name, REL_VALUE const *rel, int section, val_t value)
{
    char buf[LBLSIZE + 64];
    OBJ_SYMBOL const *p;
    int s = symbolSection(rel, section);
    if (!s && gOutFmt == OB_VOBJ && !rel) {
        /* An absolute constant. */
        for (p = absExports; p; p = p->next)
            if (!strcmp(p->name, name))
                return;
        addSymbol(&absExports, name, value);
        return;
    }
    if (!s) {
        sprintf(buf, "[WARNING] Exported symbol is not in a section(%s)", name);
        warning(buf);
        return;
    }
    for (p = sections[s - 1]->exports; p; p = p->next)
        if (!strcmp(p->name, name))
            return;
    addSymbol(&sections[s - 1]->exports, name, value);
}

static void writeName(FILE *fp, char const *s)
{
    fputs(s, fp);
    putc(0, fp);
}

static void writeWord(FILE *fp, long w)
{
    putc((int)((w >> 8) & 0xff), fp);
    putc((int)(w & 0xff), fp);
}

static void lwobjWrite(FILE *fp)
{
    static uint8_t const zero[256];
    OBJ_SECTION const *s;
    OBJ_SYMBOL const *p;
    OBJ_RELOC const *r;
    long n;
    int i;

    fwrite("LWOBJ16", 8, 1, fp);
    /* lwasm writes the newest section first; wildcard link lines follow it. */
    for (i = sectionCount; i-- > 0; ) {
        s = sections[i];
        if (s->size > 0xffff)
            error("Section exceeds 65535 bytes.");
        writeName(fp, s->name);
        if (s->flags & SECTION_BSS)
            putc(0x01, fp);
        if (s->flags & SECTION_CONSTANT)
            putc(0x02, fp);
        putc(0, fp);

        if (!(s->flags & SECTION_CONSTANT)) {
            putc(0x02, fp);
            writeName(fp, s->name);
            writeWord(fp, 0);
        }
        for (p = s->locals; p; p = p->next) {
            writeName(fp, p->name);
            writeWord(fp, p->value);
        }
        putc(0, fp);

        for (p = s->exports; p; p = p->next) {
            writeName(fp, p->name);
            writeWord(fp, p->value);
        }
        putc(0, fp);

        for (r = s->relocs; r; r = r->next) {
            if (r->size == 1) {
                putc(0xff, fp);
                putc(0x01, fp);
            }
            fwrite(r->code, r->length, 1, fp);
            putc(0, fp);
            writeWord(fp, r->offset);
        }
        putc(0, fp);

        n = (s->flags & SECTION_CONSTANT) ? 0 : s->size;
        writeWord(fp, n);
        if (!(s->flags & (SECTION_BSS | SECTION_CONSTANT))) {
            /* Reserved bytes after the last emitted byte are zero. */
            long stored = s->capacity < n ? s->capacity : n;
            if (stored > 0)
                fwrite(s->code, (size_t)stored, 1, fp);
            for (n -= stored; n > 0; n -= (long)sizeof(zero))
                fwrite(zero, (size_t)(n < (long)sizeof(zero) ? n : (long)sizeof(zero)), 1, fp);
        }
    }
    putc(0, fp);
}


/*---------------------------------------------------------------------------*/
/* The VOBJ file, as read by vlink. */

#define V_LABSYM        1
#define V_IMPORT        2
#define V_EXPRESSION    3
#define V_EXPORT        8
#define V_TYPE_SECTION  3
#define V_REL_ABS       1
#define V_REL_PC        2

/* Numbers 0..127 take a byte; others 0x84 and four little-endian bytes. */
static void vNumber(FILE *fp, long n)
{
    unsigned long u;
    if (n >= 0 && n <= 127) {
        putc((int)n, fp);
        return;
    }
    u = (unsigned long)n;
    putc(0x84, fp);
    putc((int)(u & 0xff), fp);
    putc((int)((u >> 8) & 0xff), fp);
    putc((int)((u >> 16) & 0xff), fp);
    putc((int)((u >> 24) & 0xff), fp);
}

static void vSymbol(FILE *fp, char const *name, int type, int flags, int section, long value)
{
    writeName(fp, name);
    vNumber(fp, type);
    vNumber(fp, flags);
    vNumber(fp, section);
    vNumber(fp, value);
    vNumber(fp, 0);
}

static int hasName(OBJ_SYMBOL const *list, char const *name)
{
    for (; list; list = list->next)
        if (!strcmp(list->name, name))
            return 1;
    return 0;
}

static void vobjWrite(FILE *fp)
{
    static uint8_t const zero[256];
    OBJ_SECTION *s;
    OBJ_SYMBOL const *p;
    OBJ_RELOC *r;
    long n;
    long count;
    int i;

    count = sectionCount + externalCount;
    for (i = 0; i < sectionCount; ++i) {
        for (p = sections[i]->locals; p; p = p->next)
            count += !hasName(sections[i]->exports, p->name);
        for (p = sections[i]->exports; p; p = p->next)
            ++count;
    }
    for (p = absExports; p; p = p->next)
        ++count;

    fwrite("VOBJ", 4, 1, fp);
    putc(1, fp);                        /* big endian */
    vNumber(fp, 8);                     /* bits per byte */
    vNumber(fp, 2);                     /* bytes per address */
    writeName(fp, "6809");
    vNumber(fp, sectionCount);
    vNumber(fp, count);

    /* Symbol indices: sections first, then externals, then labels. */
    for (i = 0; i < sectionCount; ++i)
        vSymbol(fp, sections[i]->name, V_LABSYM, V_TYPE_SECTION, i + 1, 0);
    for (i = 0; i < externalCount; ++i)
        vSymbol(fp, externals[i], V_IMPORT, V_EXPORT, 0, 0);
    for (i = 0; i < sectionCount; ++i) {
        for (p = sections[i]->locals; p; p = p->next) {
            if (!hasName(sections[i]->exports, p->name))
                vSymbol(fp, p->name, V_LABSYM, 0, i + 1, p->value);
        }
        for (p = sections[i]->exports; p; p = p->next)
            vSymbol(fp, p->name, V_LABSYM, V_EXPORT, i + 1, p->value);
    }
    for (p = absExports; p; p = p->next)
        vSymbol(fp, p->name, V_EXPRESSION, V_EXPORT, 0, p->value);

    for (i = 0; i < sectionCount; ++i) {
        s = sections[i];
        if (s->size > 0xffff)
            error("Section exceeds 65535 bytes.");
        n = 0;
        for (r = s->relocs; r; r = r->next) {
            ++n;
            /* The field of a PC-relative relocation holds its addend. */
            if (r->pc && !(s->flags & SECTION_BSS) && r->offset + r->size <= s->capacity) {
                if (r->size == 2) {
                    s->code[r->offset]     = (uint8_t)(r->addend >> 8);
                    s->code[r->offset + 1] = (uint8_t)r->addend;
                } else {
                    s->code[r->offset]     = (uint8_t)r->addend;
                }
            }
        }
        writeName(fp, s->name);
        writeName(fp, s->attr);
        vNumber(fp, (s->locals || s->exports) ? 1 : 0);
        vNumber(fp, 1);                 /* alignment */
        vNumber(fp, s->size);
        vNumber(fp, n);
        vNumber(fp, (s->flags & SECTION_BSS) ? 0 : s->size);
        if (!(s->flags & SECTION_BSS)) {
            long stored = s->capacity < s->size ? s->capacity : s->size;
            if (stored > 0)
                fwrite(s->code, (size_t)stored, 1, fp);
            for (n = s->size - stored; n > 0; n -= (long)sizeof(zero))
                fwrite(zero, (size_t)(n < (long)sizeof(zero) ? n : (long)sizeof(zero)), 1, fp);
        }
        for (r = s->relocs; r; r = r->next) {
            vNumber(fp, r->pc ? V_REL_PC : V_REL_ABS);
            vNumber(fp, r->offset);
            vNumber(fp, 0);             /* bit offset */
            vNumber(fp, r->size * 8);
            vNumber(fp, r->mask ? (long)r->mask : -1L);
            vNumber(fp, r->addend);
            vNumber(fp, r->symbol >= REL_EXTERNAL
                        ? sectionCount + 1 + (r->symbol - REL_EXTERNAL)
                        : r->symbol + 1);
        }
    }
}

void objWrite(FILE *fp)
{
    if (gOutFmt == OB_VOBJ)
        vobjWrite(fp);
    else
        lwobjWrite(fp);
}
