#include "as63.h"
#include <ctype.h>
#include <stdlib.h>

#define MAC_DEPTH       64
#define MAC_ARGS        35
#define MAC_SCOPE       256

typedef struct macro_def {
    struct macro_def *  next;
    char *              name;
    char *              body;
    int                 noexpand;
} MAC_DEF;

typedef unsigned long   id_t;
//typedef intmax_t      id_t;

typedef struct macro_frame {
    struct macro_frame* parent;
    char const *        body;
    char const *        cursor;
    char **             args;
    int                 argCapacity;
    char *              allArgs;
    char *              listingSource;
    int                 listingShown;
    int                 nargs;
    int                 carg;
    int                 repeat;
    int                 iteration;
    int                 fileDepth;
    int                 condDepth;
    int                 scopeDepth;
    int                 sourceLine;
    id_t                id;
    id_t                savedScope;
    int                 ownsBody;
} MAC_FRAME;

static MAC_DEF *        definitions;
static MAC_FRAME *      frames;
static int              frameDepth;
static int              scopeDepth;
static int              idDepth;
static id_t             serial;
static id_t             scope;
static id_t             scopes[MAC_SCOPE];
static id_t             ids[MAC_SCOPE];
static id_t             expandedLines;

static char *copyText(char const *s, size_t n)
{
    char *result = mallocE(n + 1);
    memcpy(result, s, n);
    result[n] = 0;
    return result;
}

static MAC_DEF *findMacro(char const *name)
{
    MAC_DEF *m = NULL;
    for (m = definitions; m; m = m->next) {
        if (gUpLo_f ? !strcasecmp(m->name, name) : !strcmp(m->name, name)) {
            return m;
        }
    }
    return NULL;
}

int macroDefined(char const *name)
{
    return findMacro(name) != NULL;
}

static MAC_FRAME *argumentFrame(void)
{
    MAC_FRAME *f = NULL;
    for (f = frames; f && f->repeat; f = f->parent) {
        ;
    }
    return f;
}

int macroValue(char const *name, val_t *value)
{
    MAC_FRAME *f = argumentFrame();
    if (!strcasecmp(name, "NARG")) {
        *value = f ? f->nargs : 0;
    } else if (!strcasecmp(name, "CARG")) {
        *value = f ? f->carg : 0;
    } else if (!strcasecmp(name, "REPTN")) {
        for (f = frames; f && !f->repeat; f = f->parent) {
            ;
        }
        *value = f ? f->iteration : -1;
    } else {
        return 0;
    }
    return 1;
}

int macroSetValue(char const *name, val_t value)
{
    MAC_FRAME *f = argumentFrame();
    if (strcasecmp(name, "CARG")) {
        return 0;
    }
    if (!f) {
        error("CARG assignment outside a macro.");
    } else {
        f->carg = value;
    }
    return 1;
}

void macroLocalLabel(char *name)
{
    char prefix[32];
    size_t n = strlen(name);
    size_t p = 0;
    if (!scope || !n) {
        return;
    }
    if (gCompatMode == COMPAT_LWASM) {
        if (!strchr(name, '@') && !strchr(name, '?')
            && !(pragmaEnabled("dollarlocal") && strchr(name, '$'))
            && !(scopeDepth && name[0] == '.')) {
            return;
        }
    } else if (name[0] != '.' && name[n - 1] != '$') {
        return;
    }
    sprintf(prefix, "__M%lu_", (long)scope);
    p = strlen(prefix);
    if (n + p > LBLSIZE) {
        error("Macro local label is too long.");
        return;
    }
    memmove(name + p, name, n + 1);
    memcpy(name, prefix, p);
}

void macroScopeBoundary(void)
{
    if (gCompatMode == COMPAT_LWASM) {
        scope = ++serial;
    }
}

int macroNumericLocal(uint8_t const *p)
{
    if (!scope || !isdigit(*p)) {
        return 0;
    }
    while (isdigit(*p)) {
        ++p;
    }
    return *p == '$';
}

static void popFrame(int exiting)
{
    MAC_FRAME *f = frames;
    int i = 0;
    if (!exiting && gCo_sp != f->condDepth) {
        error("Unclosed conditional block in macro or REPT.");
    }
    if (!exiting && scopeDepth != f->scopeDepth) {
        error("Unclosed INLINE block in macro or REPT.");
    }
    gCo_sp = f->condDepth;
    scopeDepth = f->scopeDepth;
    scope = f->savedScope;
    gSrcLine = f->sourceLine;
    frames = f->parent;
    --frameDepth;
    for (i = 0; i <= f->nargs; ++i) {
        free(f->args[i]);
    }
    free(f->args);
    free(f->allArgs);
    free(f->listingSource);
    if (f->ownsBody) {
        free((void *)f->body);
    }
    free(f);
}

int macroConditionalFloor(void)
{
    return frames ? frames->condDepth : 0;
}

void macroReset(void)
{
    MAC_DEF *m = NULL;
    while (frames) {
        popFrame(1);
    }
    while (definitions) {
        m = definitions;
        definitions = m->next;
        free(m->name);
        free(m->body);
        free(m);
    }
    serial = scope = expandedLines = 0;
    scopeDepth = idDepth = frameDepth = 0;
}

void macroFinish(void)
{
    while (frames) {
        popFrame(1);
    }
    if (scopeDepth) {
        error("INLINE without EINLINE.");
    }
    scopeDepth = 0;
    scope = 0;
}

static MAC_FRAME *pushFrame(char const *body, int repeat, int ownsBody)
{
    MAC_FRAME *f = NULL;
    if (frameDepth >= MAC_DEPTH) {
        error("Macro or REPT nesting is too deep.");
        if (ownsBody) {
            free((void *)body);
        }
        return NULL;
    }
    f = mallocE(sizeof(*f));
    memset(f, 0, sizeof(*f));
    f->argCapacity  = 10;
    f->args         = mallocE(f->argCapacity * sizeof(*f->args));
    memset(f->args, 0, f->argCapacity * sizeof(*f->args));
    f->parent       = frames;
    f->body         = f->cursor = body;
    f->repeat       = repeat;
    f->ownsBody     = ownsBody;
    f->fileDepth    = gFile_sp;
    f->condDepth    = gCo_sp;
    f->scopeDepth   = scopeDepth;
    f->sourceLine   = gSrcLine;
    f->savedScope   = scope;
    f->id           = ++serial;
    f->carg         = 1;
    frames          = f;
    ++frameDepth;
    return f;
}

static int appendText(uint8_t *dst, size_t *n, size_t limit, char const *s)
{
    size_t count = strlen(s);
    if (count >= limit - *n) {
        error("Expanded macro line is too long.");
        return 0;
    }
    memcpy(dst + *n, s, count);
    *n += count;
    return 1;
}

static char const *argument(MAC_FRAME *f, int n)
{
    if (!f || n < 0 || n > f->nargs || !f->args[n]) {
        return "";
    }
    return f->args[n];
}

static void addArgument(MAC_FRAME *f, char const *text, size_t length)
{
    char **grown = NULL;
    if (f->nargs + 1 >= f->argCapacity) {
        f->argCapacity *= 2;
        grown = realloc(f->args, f->argCapacity * sizeof(*f->args));
        if (!grown) {
            errPrg("Macro allocation failed.");
        }
        f->args = grown;
    }
    f->args[++f->nargs] = copyText(text, length);
}

char const *macroListingSource(void)
{
    MAC_FRAME *f = frames;
    MAC_FRAME *outer = NULL;
    while (f) {
        if (f->listingSource) {
            outer = f;
        }
        f = f->parent;
    }
    if (!outer) {
        return NULL;
    }
    if (outer->listingShown) {
        return "";
    }
    outer->listingShown = 1;
    return outer->listingSource;
}

static void expandLine(uint8_t *dst, size_t limit, char const *line, size_t length)
{
    char const* s           = line;
    char const* end         = line + length;
    char const* replacement = NULL;
    MAC_FRAME*  f           = argumentFrame();
    char        number[48];
    char        name[LBLSIZE + 2];
    uint8_t*    saved       = NULL;
    size_t      n           = 0;
    size_t      count       = 0;
    int         c           = 0;
    int         index       = 0;
    int         hex         = 0;
    val_t       value       = 0;
    id_t        id          = 0;
    while (s < end && n + 1 < limit) {
        if (f && *s == '{') {
            char const *next = s + 1;
            int argLength = *next == 'L';
            if (argLength) {
                ++next;
            }
            index = 0;
            if (isdigit(*(uint8_t const *)next)) {
                while (isdigit(*(uint8_t const *)next)) {
                    if (index <= 65535U) {
                        index = index * 10 + *next - '0';
                    }
                    ++next;
                }
                if (*next == '}') {
                    replacement = argument(f, index);
                    if (argLength) {
                        sprintf(number, "%lu", (unsigned long)strlen(replacement));
                        replacement = number;
                    }
                    if (!appendText(dst, &n, limit, replacement)) {
                        break;
                    }
                    s = next + 1;
                    continue;
                }
            }
        }
        if (*s != '\\' || !f) {
            dst[n++] = *(uint8_t const *)s++;
            continue;
        }
        ++s;
        if (s == end) {
            dst[n++] = '\\';
            break;
        }
        c = *(uint8_t const *)s++;
        replacement = NULL;
        if (c == '\\') {
            replacement = "\\";
        } else if (c >= '0' && c <= '9') {
            replacement = argument(f, c - '0');
        } else if (gAllMacroParams && gCompatMode != COMPAT_LWASM && tolower(c) >= 'a' &&
                   tolower(c) <= 'z') {
            replacement = argument(f, tolower(c) - 'a' + 10);
        } else if (c == '*') {
            replacement = f->allArgs ? f->allArgs : "";
        } else if (c == 'L' && s < end && isdigit(*(uint8_t const *)s)) {
            index = 0;
            while (s < end && isdigit(*(uint8_t const *)s)) {
                if (index <= 65535U) {
                    index = index * 10 + *s - '0';
                }
                ++s;
            }
            sprintf(number, "%lu", (unsigned long)strlen(argument(f, index)));
            replacement = number;
        } else if (c == '#' || c == '?') {
            if (c == '?') {
                if (s == end) {
                    error("Invalid macro argument length reference.");
                    break;
                }
                c = *(uint8_t const *)s++;
                if (isdigit(c)) {
                    index = c - '0';
                } else if (gAllMacroParams && tolower(c) >= 'a' && tolower(c) <= 'z') {
                    index = tolower(c) - 'a' + 10;
                } else {
                    error("Invalid macro argument length reference.");
                    break;
                }
                value = (val_t)strlen(argument(f, index));
            } else {
                value = f->nargs;
            }
            sprintf(number, "%ld", (long)value);
            replacement = number;
        } else if (c == '.' || c == '+' || c == '-') {
            replacement = argument(f, f->carg);
            if (c == '+') {
                ++f->carg;
            }
            if (c == '-') {
                --f->carg;
            }
        } else if (c == '@') {
            id = f->id;
            if (s < end && *s == '@') {
                ++s;
                if (!idDepth) {
                    error("Macro id stack is empty.");
                    break;
                }
                id = ids[--idDepth];
            } else if (s < end && (*s == '!' || *s == '?')) {
                c = *s++;
                if (idDepth == MAC_SCOPE) {
                    error("Macro id stack is full.");
                    break;
                }
                if (c == '?') {
                    if (!idDepth) {
                        error("Macro id stack is empty.");
                        break;
                    }
                    ids[idDepth]     = ids[idDepth - 1];
                    ids[idDepth - 1] = id;
                    ++idDepth;
                } else {
                    ids[idDepth++]  = id;
                }
            }
            sprintf(number, "_%06lu", (unsigned long)id);
            replacement = number;
        } else if (c == '<') {
            hex = s < end && *s == '$';
            if (hex) {
                ++s;
            }
            count = 0;
            while (s < end && *s != '>' && count < LBLSIZE) {
                name[count++] = *s++;
            }
            name[count] = '\n';
            name[count + 1] = 0;
            if (!count || s == end || *s++ != '>') {
                error("Invalid numeric macro expansion.");
                break;
            }
            saved = gLinPtr;
            gLinPtr = (uint8_t *)name;
            value = invExpr();
            gLinPtr = saved;
            if (hex) {
                sprintf(number, "%X", (unsigned int)value);
            } else {
                sprintf(number, "%u", (unsigned int)value);
            }
            replacement = number;
        } else {
            dst[n++] = '\\';
            if (n + 1 < limit) {
                dst[n++] = (uint8_t)c;
            }
        }
        if (replacement && !appendText(dst, &n, limit, replacement)) {
            break;
        }
    }
    if (s < end && n + 1 >= limit) {
        error("Expanded macro line is too long.");
    }
    if (!n || dst[n - 1] != '\n') {
        if (n + 1 >= limit) {
            n = limit - 2;
        }
        dst[n++] = '\n';
    }
    dst[n] = 0;
}

uint8_t *macroReadLine(uint8_t *dst, size_t limit)
{
    MAC_FRAME*  f   = NULL;
    char const* end = NULL;
    while ((f = frames) != NULL && gFile_sp <= f->fileDepth) {
        if (!*f->body) {
            popFrame(0);
            continue;
        }
        if (!*f->cursor) {
            if (f->repeat && ++f->iteration < f->repeat) {
                if (gCo_sp != f->condDepth || scopeDepth != f->scopeDepth) {
                    error("Unclosed block in REPT.");
                    popFrame(1);
                    continue;
                }
                f->cursor = f->body;
            } else {
                popFrame(0);
                continue;
            }
        }
        end = strchr(f->cursor, '\n');
        if (!end) {
            end = f->cursor + strlen(f->cursor);
        } else {
            ++end;
        }
        if (++expandedLines > 1000000UL) {
            error("Macro expansion limit exceeded.");
            while (frames) {
                popFrame(1);
            }
            return (uint8_t *)fgets((char *)dst, (int)limit, gSrcFp);
        }
        expandLine(dst, limit, f->cursor, (size_t)(end - f->cursor));
        f->cursor = end;
        return dst;
    }
    return (uint8_t *)fgets((char *)dst, (int)limit, gSrcFp);
}

static void directiveName(char const *line, char *word)
{
    uint8_t const*  p       = (uint8_t const *)line;
    size_t          n       = 0;
    uint8_t const*  first   = p;
    while (isspace(*first) && *first != '\n') {
        ++first;
    }
    if (isCommentChar(*first)) {
        *word = 0;
        return;
    }
    if (*p && !isspace(*p)) {
        while (*p && !isspace(*p)) {
            ++p;
        }
    }
    while (*p && *p != '\n' && isspace(*p)) {
        ++p;
    }
    while (isSymbl(*p) && n < LBLSIZE) {
        word[n++] = (char)toupper(*p++);
    }
    word[n] = 0;
}

static char *captureBlock(int macro)
{
    char    word[LBLSIZE + 1];
    int     stack[MAC_DEPTH];
    int     depth   = 1;
    int     comment = 0;
    size_t  n       = 0;
    size_t  capacity= 256;
    size_t  length  = 0;
    char *  body    = mallocE(capacity);
    char *  grown   = NULL;
    stack[0]        = macro;
    body[0]         = 0;
    putLine();
    while (getLine() != NULL) {
        initLine();
        directiveName(gLineBuf + LINEHEAD, word);
        if (!strcmp(word, "REM")) {
            comment = 1;
        } else if (comment) {
            if (!strcmp(word, "EREM")) {
                comment = 0;
            }
        } else if (!strcmp(word, "MACRO") || !strcmp(word, "REPT")) {
            if (depth == MAC_DEPTH) {
                error("Macro definition nesting is too deep.");
                break;
            }
            stack[depth++] = !strcmp(word, "MACRO");
        } else if (!strcmp(word, "ENDM") || !strcmp(word, "ENDR")) {
            if (stack[depth - 1] != !strcmp(word, "ENDM")) {
                error("Mismatched ENDM or ENDR.");
                break;
            }
            if (!--depth) {
                return body;
            }
        }
        length = strlen(gLineBuf + LINEHEAD);
        if (n + length > MACRO_BUF_SIZE) {
            error("Macro definition is too large.");
            break;
        }
        if (n + length + 1 > capacity) {
            while (n + length + 1 > capacity) {
                capacity *= 2;
            }
            grown = realloc(body, capacity);
            if (!grown) {
                free(body);
                errPrg("Macro allocation failed.");
            }
            body = grown;
        }
        memcpy(body + n, gLineBuf + LINEHEAD, length + 1);
        n += length;
        putLine();
    }
    error(macro ? "MACRO without ENDM." : "REPT without ENDR.");
    free(body);
    return NULL;
}

int macroControl(char const *label, uint8_t global)
{
    uint8_t *   saved         = gLinPtr;
    uint8_t *   counterStart  = NULL;
    size_t      counterLength = 0;
    char        word[LBLSIZE + 1];
    char        name[LBLSIZE + 1];
    size_t      n       = 0;
    int         active  = gCoStk[gCo_sp] >= 0;
    int         kind    = 0;
    int         noexpand= 0;
    val_t       count   = 0;
    char *      body    = NULL;
    char *      expandedBody = NULL;
    MAC_DEF *   m       = NULL;
    MAC_FRAME * f       = NULL;
    skipSpace();
    while (isSymbl(*gLinPtr) && n < LBLSIZE) {
        word[n++] = (char)toupper(*gLinPtr++);
    }
    word[n] = 0;
    if (!strcmp(word, "MACRO") || !strcmp(word, "REPT")) {
        kind = !strcmp(word, "MACRO");
        name[0] = 0;
        if (active) {
            if (kind) {
                if (*label) {
                    strcpy(name, label);
                } else {
                    skipSpace();
                    getLabel(name);
                }
                skipSpace();
                noexpand = !strncasecmp((char const *)gLinPtr, "NOEXPAND", 8) &&
                           (!gLinPtr[8] || isspace(gLinPtr[8]) || gLinPtr[8] == ';');
            } else {
                if (*label) {
                    defLabel(label, 1, global);
                }
                skipSpace();
                count = invExpr();
                if (count < 0 && gCompatMode == COMPAT_VASM)
                    count = 0;
                if (count < 0 || count > 65535) {
                    error("Invalid REPT count.");
                }
                skipSpace();
                if (*gLinPtr == ',') {
                    ++gLinPtr;
                    skipSpace();
                    counterStart  = gLinPtr;
                    getLabel(name);
                    counterLength = (size_t)(gLinPtr - counterStart);
                    if (!*name || counterLength > LBLSIZE) {
                        error("Missing or invalid REPT counter name.");
                    } else {
                        memcpy(name, counterStart, counterLength);
                        name[counterLength] = 0;
                    }
                }
            }
        }
        body = captureBlock(kind);
        if (!body) {
            return 1;
        }
        if (!active) {
            free(body);
            return 1;
        }
        if (kind) {
            if (!*name || findMacro(name)) {
                error("Missing or duplicate macro name.");
                free(body);
                return 1;
            }
            m = mallocE(sizeof(*m));
            m->name     = copyText(name, strlen(name));
            m->body     = body;
            m->noexpand = noexpand;
            m->next     = definitions;
            definitions = m;
        } else if (count > 0 && count <= 65535) {
            if (*name) {
                expandedBody = mallocE(strlen(body) + strlen(name) + 16);
                sprintf(expandedBody, "%s SET REPTN\n%s", name, body);
                free(body);
                body = expandedBody;
            }
            pushFrame(body, count, 1);
        } else {
            free(body);
        }
        return 1;
    }
    if (!strcmp(word, "ENDM")  || !strcmp(word, "ENDR") || !strcmp(word, "MEXIT")  ||
        !strcmp(word, "EXITM") || !strcmp(word, "EXIT") || !strcmp(word, "INLINE") ||
        !strcmp(word, "EINLINE")) {
        if (active) {
            if (!strcmp(word, "INLINE")) {
                if (scopeDepth == MAC_SCOPE) {
                    error("INLINE nesting is too deep.");
                } else {
                    scopes[scopeDepth++] = scope;
                    scope = ++serial;
                }
            } else if (!strcmp(word, "EINLINE")) {
                if (!scopeDepth || (frames && scopeDepth <= frames->scopeDepth)) {
                    error("EINLINE without INLINE.");
                } else {
                    scope = scopes[--scopeDepth];
                }
            } else if (!strcmp(word, "MEXIT") || !strcmp(word, "EXITM") || !strcmp(word, "EXIT")) {
                f = argumentFrame();
                if (!f) {
                    error("MEXIT outside a macro.");
                } else {
                    kind = f->fileDepth;
                    while (gFile_sp > kind) {
                        fclose(gSrcFp);
                        popFile();
                    }
                    while (frames != f) {
                        popFrame(1);
                    }
                    popFrame(1);
                }
            } else {
                error("ENDM or ENDR without matching block.");
            }
        }
        clearAddress();
        return 1;
    }
    gLinPtr = saved;
    return 0;
}

int macroInvoke(char const *token)
{
    char        name[LBLSIZE + 1];
    char const *suffix  = NULL;
    char const *start   = NULL;
    uint8_t *   p       = NULL;
    int         quote   = 0;
    int         level   = 0;
    int         bracket = 0;
    int         angle   = 0;
    int         strictArgs = gCompatMode == COMPAT_LWASM && !pragmaEnabled("asm09");
    int         maxArgs = gCompatMode == COMPAT_LWASM ? 65535 : (gAllMacroParams ? MAC_ARGS : 9);
    uint8_t *   allStart= NULL;
    uint8_t *   allEnd  = NULL;
    size_t      n       = 0;
    MAC_DEF *   m       = NULL;
    MAC_FRAME * f       = NULL;
    m       = findMacro(token);
    suffix  = m ? NULL : strchr(token, '.');
    n       = suffix ? (size_t)(suffix - token) : strlen(token);
    if (n > LBLSIZE) {
        return 0;
    }
    memcpy(name, token, n);
    name[n] = 0;
    if (!m) {
        m   = findMacro(name);
    }
    if (!m) {
        return 0;
    }
    skipSpace();
    allStart    = gLinPtr;
    allEnd      = allStart;
    if (strictArgs) {
        while (*allEnd && !isspace(*allEnd) && *allEnd != ';') {
            ++allEnd;
        }
    }
    f = pushFrame(m->body, 0, 0);
    if (!f) {
        while (*gLinPtr && *gLinPtr != '\n') {
            ++gLinPtr;
        }
        return 1;
    }
    if (gCompatMode == COMPAT_LWASM) {
        scope = f->id;
    }
    if (m->noexpand) {
        f->listingSource = copyText(gLineBuf + LINEHEAD, strlen(gLineBuf + LINEHEAD));
    }
    f->args[0] = copyText(suffix ? suffix + 1 : "", suffix ? strlen(suffix + 1) : 0);
    while (*gLinPtr && *gLinPtr != '\n' && *gLinPtr != ';' &&
           (!strictArgs || gLinPtr < allEnd)) {
        if (f->nargs == maxArgs) {
            error("Too many macro arguments (use --allmp for 35).");
            break;
        }
        skipSpace();
        start = (char const *)gLinPtr;
        p = gLinPtr;
        angle = gCompatMode != COMPAT_LWASM && *p == '<' && p[1] && p[1] != '\n' &&
                strchr((char const *)p + 1, '>') != NULL;
        if (angle) {
            ++p;
            ++start;
        }
        quote = level = bracket = 0;
        while (*p && *p != '\n') {
            if (strictArgs && (isspace(*p) || *p == ',' || *p == ';')) {
                break;
            }
            if (strictArgs) {
                ++p;
                continue;
            }
            if (!quote && !angle && !level && !bracket && isspace(*p)) {
                break;
            }
            if (quote) {
                if (*p == quote) {
                    quote = 0;
                }
            } else if (*p == '"' || *p == '\'') {
                if (strchr((char const *)p + 1, *p)) {
                    quote = *p;
                }
            } else if (angle && *p == '>') {
                if (p[1] == '>') {
                    p += 2;
                    continue;
                }
                break;
            } else if (*p == '(') {
                ++level;
            } else if (*p == ')' && level) {
                --level;
            } else if (*p == '[') {
                ++bracket;
            } else if (*p == ']' && bracket) {
                --bracket;
            } else if (!angle && !level && !bracket && (*p == ',' || *p == ';')) {
                break;
            }
            ++p;
        }
        n = (size_t)((char const *)p - start);
        while (n && isspace(*(uint8_t const *)(start + n - 1))) {
            --n;
        }
        if (gCompatMode == COMPAT_LWASM && pragmaEnabled("asm09")
            && n >= 2 && start[0] == '(' && start[n - 1] == ')') {
            ++start;
            n -= 2;
        }
        addArgument(f, start, n);
        if (angle) {
            char *read = f->args[f->nargs];
            char *write = read;
            while (*read) {
                *write++ = *read;
                if (*read == '>' && read[1] == '>') {
                    ++read;
                }
                ++read;
            }
            *write = 0;
        }
        if (quote || level || bracket || (angle && *p != '>')) {
            error("Unclosed macro argument.");
        }
        if (angle && *p == '>') {
            ++p;
        }
        gLinPtr = p;
        if (gCompatMode == COMPAT_LWASM && isspace(*gLinPtr)) {
            break;
        }
        skipSpace();
        if (*gLinPtr != ',') {
            break;
        }
        ++gLinPtr;
        if (gCompatMode != COMPAT_LWASM) {
            skipSpace();
        }
        if (!*gLinPtr || isspace(*gLinPtr) || *gLinPtr == ';') {
            if (f->nargs < maxArgs) {
                addArgument(f, "", 0);
            } else {
                error("Too many macro arguments.");
            }
            break;
        }
    }
    if (!strictArgs) {
        allEnd = gLinPtr;
    }
    f->allArgs = copyText((char const *)allStart, (size_t)(allEnd - allStart));
    while (*gLinPtr && *gLinPtr != '\n') {
        ++gLinPtr;
    }
    return 1;
}
