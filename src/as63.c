/************************************
 *									*
 *	HD6309 cross assembler			*
 *									*
 *		hack hack, more hack!		*
 *									*
 ************************************/

#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>
#define EXT
#include	"as63.h"


/*--------------------------------------------------------------------------*/

FILE	*fopenE(char *fname, char *atr)
{
	FILE   *fp;

	if ((fp = fopen(fname, atr)) == NULL) {
		fprintf(STDERR, "%s: %s をオープンできない\n", gCmdName, fname);
		exit(1);
	}
	return fp;
}

void   *mallocE(size_t siz)
{
	void   *p;

	if ((p = malloc(siz)) == NULL) {
		fprintf(STDERR, "%s:メモリが足りません\n", gCmdName);
		exit(1);
	}
	return p;
}

void	errPrg(char * s)
{
	fprintf(STDERR, "%s:BUGが発生(%s)\n", gCmdName, s);
	exit(1);
}

void	error(char *s)
{
	gErrors++;
	if (gPass != 2)
		return;
#ifdef OE
	if (gErrFName && gErrors - 1 == 0) {
		gErrFp = fopen(gErrFName, "w");
		if (gErrFp == NULL) {
			fprintf(STDERR, "%s: %s をオープンできない\n", gCmdName, gErrFName);
			gErrFp = STDERR;
		}
	}
#endif
	if (gList > 0) {
		fprintf(gLstFp, "*** %s\n", s);
	}
#ifdef FILSTK2
	fprintf(gErrFp, "%s %5d : %s\n", gSrcFName, gSrcLine, s);
#else
	fprintf(gErrFp, "%5d : %s\n", gLineNo, s);
#endif
}

static void errLbl(char *msg, char *lbl)
{
	char	buf[260];

	if (gPass != 2)
		return;
	sprintf(buf, "%s(%s)", msg, lbl);
	error(buf);
}

char	*FIL_BaseName(char *adr)
	/* ファイルパス名よりファイル名の位置を得る */
	/* パスの区切りは MS-DOS 依存. MS全角チェックなし */
{
	char *p;

	p = adr;
	while (*p != '\0') {
		if (*p == '/'
 #if 1	/* #ifdef MSDOS */
		 || *p == ':' || *p == '\\'
 #endif
		)
			adr = p + 1;
	  #if 0	/* MS全角チェック */
		if (isKanji((*(byte*)p)) && *(p+1) )
			p++;
	  #endif
		p++;
	}
	return adr;
}

char	*FIL_ChgExt(char filename[], char *ext)
	/* ファイル名の最後尾にある拡張子を付け替える */
{
	char *p;

	p = FIL_BaseName(filename);
	p = strrchr( p, '.');
	if (p == NULL) {
		strcat(filename,".");
		strcat( filename, ext);
	} else {
		strcpy(p+1, ext);
	}
	return filename;
}


/*---------------------------------------------------------------------------*/

static word oChkSum;			/* S-formatの各行ごとのチェック・サム */

static byte hexDigit(byte x)
{
	return ((x &= 0x0f) < 10) ? x + '0' : x - 10 + 'A';
}

static void put2hex(int b)
{
	if (!gObjct)
		return;
	b = (byte)b;
	putc(hexDigit(b >> 4), gObjFp);
	putc(hexDigit(b), gObjFp);
	oChkSum += b;
}

static void put4hex(int w)
{
	w = (word)w;
	put2hex(w >> 8);
	put2hex(w);
}

void	flushObj(void)
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
#ifdef OA
		} else if (gObjct == OB_ASM && gObjPos) {
			fprintf(gObjFp, "\tfcb $%02x", gObjBuf[0]);
			for (i = 1; i < gObjPos; i++)
				fprintf(gObjFp, ",$%02x", gObjBuf[i]);
			putc('\n', gObjFp);
#endif
		} else {
			fwrite(gObjBuf, gObjPos, 1, gObjFp);
		}
	}
	gObjPos = 0;
	gObjLc = gLc;
}

void	termObj(void)
{
	flushObj();
	if (gObjct == OB_SFMT) {
		fputs("S903", gObjFp);
		oChkSum = 3;
		put4hex(gEntryAddr);
		put2hex(~oChkSum);
		putc('\n', gObjFp);
	}
}

static void crc(int b)
{
	int 	w;

	b = (byte)b;
	w = b ^ gCrcBuf[0];
	gCrcBuf[0] =  gCrcBuf[1];
	gCrcBuf[1] =  gCrcBuf[2];
	gCrcBuf[1] ^= w >> 7;
	gCrcBuf[2] =  (byte)(w << 1);
	gCrcBuf[1] ^= w >> 2;
	gCrcBuf[2] ^= w << 6;
	w ^= w << 1;
	w ^= w << 2;
	w ^= w << 4;
	if (w & 0x80) {
		gCrcBuf[0] ^= 0x80;
		gCrcBuf[2] ^= 0x21;
	}
}

void	putObj(int b)
{
	if (gPass != 2)
		return;
	if (gObjPos >= gObjBufSz)
		flushObj();
	gObjBuf[gObjPos++] = (byte)b;
}

void	put2obj(int w)
{
	w = (word)w;
	putObj(w >> 8);
	putObj(w);
}

byte	putB(int b)
{
	if (gRmb_f && gFBasic_f && gObjct == OB_BIN && gRmb_sp){
		while(gRmb_sp-- > 0)
			putObj(0);
		gRmb_sp++;
	}
	b = (byte)b;
	putObj(b);
	++gObjCnt;
	++gLc;
	if (gOs9_f && gPass == 2)
		crc(b);
	return (byte)b;
}

int 	put2B(word w)
{
	putB((byte)(w >> 8));
	putB((byte)w);
	return w;
}

#ifdef OA
void	oa_putStr(char *s, int n)
{
	if (gPass != 2)
		return;
	flushObj();
	fputs(s, gObjFp);
	gObjCnt += n;
	gLc += n;
}
#endif

/*---------------------------------------------------------------------------*/

static int oPostf, oPos;

void	printByte(int b, int c)
{
	if (gPass != 2)
		return;
	b = (byte)b;
	gLineBuf[c] = hexDigit(b >> 4);
	gLineBuf[c + 1] = hexDigit(b);
}

void	printWord(int w, int c)
{
	if (gPass != 2)
		return;
	w = (word)w;
	printByte((w >> 8), c);
	printByte(w, c + 2);
}

void	putByte(int b)
{
	b = (byte)b;
	printByte(putB(b), oPostf);
}

void	postByte(int b)
{
	printByte(putB(b), oPostf);
	oPostf += 3;
}

void	putWord(int w)
{
	printWord(put2B(w), oPostf);
}

void	clearAddress(void)
{
	int i;

	if (gPass != 2)
		return;
	for (i = 5; i < 9; i++)
		gLineBuf[i] = ' ';
}

void	printAddress(int a)
{
	if (gPass != 2)
		return;
	printWord(a, 5);
}

void	printChar(int c, int p)
{
	gLineBuf[p] = (byte)c;
}

void	initLine(void)
{
	char *p;

	gLblPtr = NULL;
	oPostf = 15;
	oPos = 10;
	itoa(++gLineNo, gLineBuf, 10);
#ifdef FILSTK2
	++gSrcLine;
#endif
	p = gLineBuf;
	while (*p++) {}
	for (--p; p < gLinPtr; p++)
		*p = ' ';
	printAddress(gLinLc = gLc);
}

void	putLine(void)
{
	if (gPass == 2 && gList > 0)
		fputs(gLineBuf, gLstFp);
}

static void flushLine(void)
{
	char *p;

	putLine();
	for (p = gLineBuf; p < gLineBuf + LINEHEAD; p++)
		*p = ' ';
	*p++ = '\n';
	*p = '\0';
	printAddress(gLc);
	oPos = 10;
}

void	put1Byte(int b)
{
	if ((23 - 3) < oPos)
		flushLine();
	printByte(putB(b), oPos);
	oPos += 3;
}

void	put1Byt2(int b)
{
	if (23 < oPos)
		flushLine();
	printByte(putB(b), oPos);
	oPos += 2;
}

void	put1Word(int w)
{
	if (23 - 5 < oPos)
		flushLine();
	printWord(put2B(w), oPos);
	oPos += 5;
}

/*---------------------------------------------------------------------------*/

static LBLTBL_T *oLabel;
static int oLrf;

static void printNode(LBLTBL_T *lp)
{
	if (lp == NULL)
		return;
	printNode(lp->right);
	if (lp->line) {
		fprintf(gLstFp, "%15s %4d %04x", lp->name, lp->line, lp->value);
		fprintf(gLstFp, oLrf++ % 3 ? " " : "\n");
	}
	printNode(lp->left);
}

void	dumpSymbol(void)
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
	static int gi = 0;
	static LBLTBL_T *gp = NULL;

	if (gp == NULL || gi >= MAXLABEL) {
		gp = (LBLTBL_T *) mallocE(sizeof(LBLTBL_T) * MAXLABEL);
		gi = 0;
		DEBMSGF((STDERR, "alloc %d nodes\n", MAXLABEL));
	}
	return gp + gi++;
}

void	initNode(void)
{
	oLabel = getNode();
	oLabel->name[0] = '\0';
	oLabel->right = oLabel->left = NULL;
}

void	defLabel(char *temp, byte f, byte gf)
{
	LBLTBL_T *lp;
	int 	i;

	lp = oLabel;
	for (;;) {
		if ((i = strcmp(temp, lp->name)) == 0) {
			if (lp->grp == 0 || lp->grp == gGrp) {
				if (lp->line != gLineNo && (f == 1 || lp->flg == 1))
					errLbl("ラベルの多重定義", temp);
				(gLblPtr = lp)->value = gLinLc;
				if (lp->flg == 0) {
					lp->flg = f;
					lp->line = gLineNo;
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
				lp->right = getNode();
				lp = lp->right;
				break;
			}
		} else {
			if (lp->left != NULL) {
				lp = lp->left;
			} else {
				lp->left = getNode();
				lp = lp->left;
				break;
			}
		}
	}
	if (lp == NULL)
		errPrg("defLabel()");
	gLabels++;
	gLblPtr = lp;
	lp->value = gLinLc;
	if (gf)
		lp->grp = 0;
	else
		lp->grp = gGrp;
	lp->flg = f;
	if (f)
		lp->line = gLineNo;
	else
		lp->line = 0x7fff;
	strcpy(lp->name, temp);
	lp->right = lp->left = NULL;
	return;
}

static LBLTBL_T *refLbl0(char *lbl)
{
	LBLTBL_T *lp;
	int 	i;

	lp = oLabel;
	while (lp != NULL) {
		if ((i = strcmp(lbl, lp->name)) == 0) {
			if (lp->grp == 0 || lp->grp == gGrp)
				break;
			i = (gGrp > lp->grp) ? 1 : -1;
		}
		lp = (i < 0) ? (lp->right) : (lp->left);
	}
	return lp;
}

static LBLTBL_T *refLabel(char *lbl)
{
	LBLTBL_T *lp;

	lp = refLbl0(lbl);
	if (lp && lp->flg == 0)
		return NULL;
	return lp;
}

/*---------------------------------------------------------------------------*/

int 	isSymbl(int c)
{
	c = (byte)c;
	return (isalnum(c) || (c == '_') || (c == '.') || (c == '@'));
}

int 	isSymbl2(int c)
{
	c = (byte)c;
	return (isalpha(c) || (c == '_') || (c == '.'));
}

int 	isSymbl3(int c)
{
	c = (byte)c;
	return (isalnum(c) || c == '_' || c == '.' || c == '@' || c == '$');
}

byte	getLabel(char *buf)
{
	byte    *p;
	byte	gf;

	gf = 0;
	if (!isSymbl2(*gLinPtr))
		error("ラベル名がおかしい");
	for (p = buf; p < buf + LBLSIZE; p++, gLinPtr++) {
		*p = *gLinPtr;
		if (!isSymbl3(*p))
			break;
		if (gUpLo_f)
			*p = toupper(*p);
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

void	skipSpace(void)
{
	while (isspace(*gLinPtr) && *gLinPtr != '\n')
		gLinPtr++;
}

int 	checkChar(byte c)
{
	if (toupper(*gLinPtr) == c) {
		gLinPtr++;
		return 1;
	}
	return 0;
}

int 	checkCh_e(byte c)
{
	static char buf[] = "必要な' 'がない";

	if (toupper(*gLinPtr) == c) {
		gLinPtr++;
		return 1;
	}
	buf[7] = c;
	error(buf);
	return 0;
}

/*---------------------------------------------------------------------------*/
val_t	expression(void);

static	val_t	term(void)
{
	val_t	expression();
	char	temp[LBLSIZE + 1];
	LBLTBL_T *lp;
	val_t	tv;
	word	c;

	switch (c = *gLinPtr++) {
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
		if (gOs9_f && !isSymbl3(*gLinPtr))
			return gCSectBase;
		break;
	case '\'':
		if (isKanji(*(byte*)gLinPtr))
			goto DC;
		return *gLinPtr++;
	case '"':
		/* if (!gOs9_f) break; */
	  DC:
		c = *gLinPtr++;
		c = c * 0x100 + *gLinPtr++;
		return c;
	case '$':
	  XDIG:
		for (tv = 0; c = *gLinPtr, isxdigit(c); gLinPtr++)
			tv = tv * 16 +
				(isdigit(c) ? (c - '0') : (toupper(c) - 'A' + 10));
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
		if (stricmp(temp, "defined") == 0 || stricmp(temp, "used") == 0) {
			checkCh_e('(');
			getLabel(temp);
			if (temp[4])
				lp = refLabel(temp);
			else
				lp = refLbl0(temp);
			checkCh_e(')');
			return (lp != NULL);
		} else if (gPass == 1 && refLbl0(temp) == NULL) {
			defLabel(temp, 0, 1);
		} else if ((lp = refLabel(temp)) != NULL) {
			if (gLineNo < lp->line)
				gValid_f = 0;
			return (lp->value);
		}
#ifdef OA
		if (gObjct == OB_ASM && refLbl0(temp))
			gOAchk_f = 1;
		else
#endif
			errLbl("ラベルが未定義", temp);
	/* DEBMSGF((STDERR,"LABEL:%s\n",temp)); */
		return (gValid_f = 0);
	} else if (isdigit(c)) {
#ifdef HE
		if (c == '0') {
			if (toupper(*(gLinPtr + 1)) == 'X') {
				gLinPtr += 2;
				goto XDIG;
			} else if (toupper(*(gLinPtr + 1)) == 'B') {
				gLinPtr += 2;
				goto BDIG;
			}
		}
#endif
		for (tv = 0; isdigit(*gLinPtr); ++gLinPtr)
			tv = (tv * 10) + *gLinPtr - '0';
		/* DEBMSGF((STDERR,"*gLinPtr : %c(%02x) *%lx\t[digit]\n", gLinPtr,*gLinPtr,gLinPtr)); */
		return tv;
	} else {
		error("式中に邪魔な文字がある");
		DEBMSGF((STDERR, "*gLinPtr : %c(%02x)\t[term()]\n", *gLinPtr, *gLinPtr));
		return (gValid_f = 0);
	}
}

static	val_t	expMUL(void)
{
	val_t	val;
	char	c;

	val = term();
	for (;;) {
		c = *gLinPtr;
		if (c == '*') {
			gLinPtr++;
			val *= term();
		} else if (c == '/' || c == '%') {
			val_t	v;
			gLinPtr++;
			v = term();
			if (v == 0) {
				error("0で割算を行った");
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

static	val_t	expADD(void)
{
	val_t	val;

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

static	val_t	expSHIFT(void)
{
	val_t	val;

	val = expADD();
	for (;;) {
		if (*gLinPtr == '<' && *(gLinPtr + 1) == '<') {
			gLinPtr += 2;
			val <<= expADD();
		} else if (*gLinPtr == '>' && *(gLinPtr + 1) == '>') {
			gLinPtr += 2;
			val >>= expADD();
		} else {
			break;
		}
	}
	return val;
}

static	val_t	expCO(void)
{
	val_t	val;
	byte	c;

	val = expSHIFT();
	for (;;) {
		c = *(gLinPtr + 1);
		switch (*gLinPtr) {
		case '<':
			if (c == '=') {
				gLinPtr += 2;
				val = (val <= expSHIFT());
			} else {
				gLinPtr++;
				val = (val < expSHIFT());
			}
			break;
		case '>':
			if (c == '=') {
				gLinPtr += 2;
				val = (val >= expSHIFT());
			} else {
				gLinPtr++;
				val = (val > expSHIFT());
			}
			break;
		default:
			goto J1;
		}
	}
  J1:
	return val;
}


static	val_t	expEQEQ(void)
{
	val_t	val;
	byte	c;

	val = expCO();
	for (;;) {
		c = *(gLinPtr + 1);
		switch (*gLinPtr) {
		case '!':
			if (c != '=')
				goto J1;
			gLinPtr += 2;
			val = (val != expCO());
			break;
		case '=':
			if (c != '=')
				goto J1;
			gLinPtr += 2;
			val = (val == expCO());
			break;
		default:
			goto J1;
		}
	}
  J1:
	return val;
}


static	val_t	expAND(void)
{
	val_t	val;

	val = expEQEQ();
	while (*gLinPtr == '&' && *(gLinPtr + 1) != '&') {
		gLinPtr++;
		val &= expEQEQ();
	}
	return val;
}

static	val_t	expEOR(void)
{
	val_t	val;

	val = expAND();
	while (*gLinPtr == '^' || (*gLinPtr == '?' /* && gOs9_f */ )) {
		gLinPtr++;
		val ^= expAND();
	}
	return val;
}

static	val_t	expOR(void)
{
	val_t	val;

	val = expEOR();
	while ((*gLinPtr == '|' && *(gLinPtr + 1) != '|')
		|| (*gLinPtr == '!' && *(gLinPtr + 1) != '=' /* && gOs9_f */ )) {
		gLinPtr++;
		val |= expEOR();
	}
	return val;
}

static	val_t	expLAND(void)
{
	val_t	val;

	val = expOR();
	while (*gLinPtr == '&' && *(gLinPtr + 1) == '&') {
		gLinPtr += 2;
		val = (expOR() && val);
	}
	return val;
}

static	val_t	expLOR(void)
{
	val_t	val;

	val = expLAND();
	while (*gLinPtr == '|' && *(gLinPtr + 1) == '|') {
		gLinPtr += 2;
		val = (expLAND() || val);
	}
	return val;
}

val_t	expression(void)
{
	val_t	val;

	gValid_f = 1;
	gByte_f = gWord_f = 0;
	if (checkChar('<'))
		gByte_f = 1;
	else if (checkChar('>'))
		gWord_f = 1;
	val = expLOR();
	switch (*gLinPtr) {
	case ' ':
	case '\t':
	case ',':
	case ')':
	case ']':
	case '\n':
		break;
	default:
		error("邪魔な文字がある");
		DEBMSGF((STDERR, "*gLinPtr : %c(%02x)\t[expression()]\n",
				 *gLinPtr, *gLinPtr));
	}
	return val;
}

byte	bytExpr(void)
{
	val_t	val;

	val = expression();
	if (val < -128 || 255 < val)
		error("値が1バイトに納まらない");
	return (byte)(val & 0xff);
}

val_t	invExpr(void)
{
	val_t r;

	r = expression();
	if (!gValid_f)
		error("定数式の値が定まらない");
	return r;
}

#ifndef M6809
void	imm4Expr(shrt *v1, shrt *v2)
{
	val_t	val;

	val = expression();
	if (checkChar(',')) {
		*v1 = (shrt) val;
		*v2 = (shrt) expression();
	} else {
		*v1 = (shrt) (val >> 16);
		*v2 = (shrt) val;
	}
}

#endif

/*---------------------------------------------------------------------------*/

void	putCode(int grp, int mode)
{
 /* addressing mode offset table 'gOffset[group][mode]' */
	static int gOffset[4][4] =	{
		{0x00, 0x00, 0x00, 0},
		{0x00, 0x00, 0x60, 0x70},
		{0x00, 0x10, 0x20, 0x30},
		{0x00, 0x01, 2,    3}
	};

	if (gOprPtr->prefix) {
		printByte(putB(gOprPtr->prefix), 10);
		printByte(putB(gOprPtr->opcode + gOffset[grp][mode]), 12);
	} else {
		printByte(putB(gOprPtr->opcode + gOffset[grp][mode]), 10);
	  #ifndef M6809
		if (gImVal != 512)
			postByte(gImVal);
	  #endif
	}
}

int 	getReg(int r)
{
	int reg;
	byte	c, b, d;
	byte   *l_p;

	reg = 0;
	l_p = gLinPtr;
	b = toupper(*gLinPtr);
	gLinPtr++;
	c = toupper(*gLinPtr);
	if (!isSymbl3(c)) {
		switch (b) {
		case 'A':	reg = A;	break;
		case 'B':	reg = B;	break;
		case 'D':	reg = D;	break;
		case 'X':	reg = X;	break;
		case 'Y':	reg = Y;	break;
		case 'U':	reg = U;	break;
		case 'S':	reg = S;	break;
#ifndef M6809
		case 'W':	reg = W;	break;
		case 'E':	reg = E;	break;
		case 'F':	reg = F;	break;
		case 'V':	reg = V;	break;
		case 'N':	reg = N;	break;
#endif
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
		} else if (b == 'P' && c == 'C' && d == 'R'
				   && !isSymbl3(*(gLinPtr + 1))) {
			++gLinPtr;
			reg = PCR;
		} else {
			while (isSymbl3(*gLinPtr))
				++gLinPtr;
		}
	}
#ifndef M6809
	if (gM68_f && (reg & X63REG))
		error("6809モードでE,F,W,Vレジスタが使われた");
#endif
	if (r & reg)
		return reg;
	if (r == OFFSETRG)
		gLinPtr = l_p;
	else
		error("レジスタの指定がおかしい");
	return 0;
}

int 	regNo(int r)
{
	switch (getReg(r)) {
	case D: return 0;
	case X: return 1;
	case Y: return 2;
	case U: return 3;
	case S: return 4;
	case PC:return 5;
#ifndef M6809
	case W: return 6;
	case V: return 7;
#endif
	case A: return 8;
	case B: return 9;
	case CC:return 10;
	case DP:return 11;
#ifndef M6809
	case N: return 12;
	case E: return 14;
	case F: return 15;
#endif
	default:
		error("レジスタの指定がおかしい");
		return -1;
	}
}


/*---------------------------------------------------------------------------*/

int 	checkByte(int b)
{
	return (gByte_f || (-128 <= b && b <= 127 && gValid_f && !gWord_f));
}

int 	index0(int frame, int reg)
{
	int 	xr;

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
#ifndef M6809
	case W:
		switch (frame) {
		case 0x83:				/* ,--W */
			frame = 0xef;
			break;
		case 0x81:				/* ,W++ */
			frame = 0xcf;
			break;
		case 0x84:				/* ,W */
			frame = 0x8f;
			break;
		case 0x89:				/* nnnn,W */
			frame = 0xaf;
			break;
		default:
			error("インデックス・モードの指定がおかしい");
		}
		return (frame ^ (gIndirect ? 0x1f : 0));
#endif
	default:
		xr = 0;
	}
	return (frame | xr | (gIndirect ? 0x10 : 0));
}


static void indexM(int frame, int reg)
{
	postByte(index0(frame, reg));
}

static void operand(int grp, int mode)
{
	val_t val;
	int 	reg;

	skipSpace();
	if ((mode & (IMMEDIATE | IMMEDIATE2)) && checkChar('#')) {
		putCode(grp, IMMEDIATE_MODE);
		if (mode & IMMEDIATE)
			putByte(bytExpr());
		else
			putWord(expression());
		return;
	}
	gIndirect = checkChar('[');
	if ((mode & INDEX) && checkChar(',')) {
		putCode(grp, INDEX_MODE);
		if (checkChar('-')) {
			if (checkChar('-'))
				indexM(0x83, getReg(INDEXREG | W));
			else if (gIndirect)
				error("[,-R] は指定できない");
			else
				indexM(0x82, getReg(INDEXREG));
		} else {
			reg = getReg(INDEXREG | W);
			if (checkChar('+')) {
				if (checkChar('+'))
					indexM(0x81, reg);
				else if (gIndirect)
					error("[,R+] は指定できない");
				else if (reg == W)
					error(",w+ という指定はできない");
				else
					indexM(0x80, reg);
			} else
				indexM(0x84, reg);
		}
	} else if ((mode & INDEX) && (reg = getReg(OFFSETRG)) > 0) {
		if (checkCh_e(',')) {
			putCode(grp, INDEX_MODE);
			switch (reg) {
			case A:
				val = 0x86;
				break;
			case B:
				val = 0x85;
				break;
			case D:
				val = 0x8b;
				break;
#ifndef M6809
			case E:
				val = 0x87;
				break;
			case F:
				val = 0x8a;
				break;
			case W:
				val = 0x8e;
				break;
#endif
			}
			indexM(val, getReg(INDEXREG));
		}
	} else {
		val = expression();
		if ((mode & INDEX) && checkChar(',')) {
			putCode(grp, INDEX_MODE);
			switch (reg = getReg(INDEXREG | W | PC | PCR)) {
			case X:
			case Y:
			case U:
			case S:
#ifndef M6809
			case W:
#endif
				if (gValid_f && val == 0) {
					indexM(0x84, reg);
				} else if (gValid_f && -16 <= val && val <= 15
						 && !gIndirect && reg != W) {
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
				if (checkByte(val -= gLinLc + 3 +
							  (gOprPtr->prefix ? 1 : 0))) {
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
		} else if ((mode & DIRECT) && (gByte_f
			|| ((word) (val - (gDp << 8)) <= 255 && gValid_f && !gWord_f))) {
			putCode(grp, DIRECT_MODE);
			putByte(val - (gDp << 8));
		} else if (mode & EXTEND) {
			putCode(grp, EXTEND_MODE);
			putWord(val);
		} else {
			error("アドレッシング・モードの指定がおかしい");
		}
	}
	if (gIndirect)
		checkCh_e(']');
}

/*---------------------------------------------------------------------------*/

void	load(void)
{
	operand(GROUP2, LOAD);
}

void	load2(void)
{
	operand(GROUP2, LOAD2);
}

void	store(void)
{
	operand(GROUP2, STORE);
}

void	memory(void)
{
	operand(GROUP1, MEMORY);
}

void	lea(void)
{
	operand(GROUP0, INDEX);
}

void	ccr(void)
{
	operand(GROUP0, IMMEDIATE);
}

#ifndef M6809
void	load4(void)
{
	shrt	val, val2;

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

void	immemory(void)
{
	skipSpace();
	if (checkCh_e('#')) {
		gImVal = bytExpr();
		checkCh_e(',');
		operand(GROUP1, MEMORY);
		gImVal = 512;
	}
}

#endif

/*----------------------------------*/

void	none(void)
{
	putCode(GROUP0, NO_MODE);
}

void	setdp(void)
{
	skipSpace();
	clearAddress();
	printByte(gDp = invExpr(), 15);
}


void	transfer(void)
{
	int 	r1, r2;

	skipSpace();
	putCode(GROUP0, NO_MODE);
	if ((r1 = regNo(ALLREG | X63REG)) < 0)
		goto ERR;
	checkCh_e(',');
	if ((r2 = regNo(ALLREG | X63REG)) < 0)
		goto ERR;
	if ((r1 ^ r2) & 0x08 && r1 != 0x0c && r2 != 0x0c)
		error("違うサイズのレジスタを組合せている");
	putByte((r1 << 4) | r2);
  ERR:
	return;
}

#ifndef M6809
void	tfm(void)
{
	int 	r1, r2;
	byte	md;

	md = ' ';
	skipSpace();
	if ((r1 = regNo(INDEXREG | D)) < 0)
		goto ER;
	if (*gLinPtr == '+')
		md = *gLinPtr++;
	else if (*gLinPtr == '-')
		md = *gLinPtr++;
	checkCh_e(',');
	if ((r2 = regNo(INDEXREG | D)) < 0)
		goto ER;
	if (*gLinPtr == '+') {
		gLinPtr++;
		if (md == '+')		/* tfm r+,r+ */
			putCode(GROUP3, 0);
		else if (md == ' ') /* tfm r+,r */
			putCode(GROUP3, 3);
		else
			goto ER;
	} else if (*gLinPtr == '-') {
		gLinPtr++;
		if (md == '-')		/* tfm r-,r- */
			putCode(GROUP3, 1);
		else
			goto ER;
	} else if (md == '+') {	/* tfm r,r+ */
		putCode(GROUP3, 2);
	} else {
		goto ER;
	}
	putByte((r1 << 4) | r2);
	return;
  ER:
	put1Word(0);
	put1Byte(0);
	error("tfm の指定がおかしい");
}

#endif

static void pshpul(byte u, byte h)
{
	word	m1, m2;

	m1 = 0;
	skipSpace();
	do {
#ifdef OPEQ
		switch (getReg(ALLREG | W))
#else
		switch (getReg(ALLREG))
#endif
		{
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
				error("pshs,pulsでsレジスタが指定されてる");
			m2 = 0x40;
			break;
		case U:
			if (u)
				error("pshu,puluでuレジスタが指定されてる");
			m2 = 0x40;
			break;
		case PC:
			m2 = 0x80;
			break;
#ifdef OPEQ
		case W:
			m2 = 0x100;
			break;
#endif
		}
		if (m1 & m2)
			error("同じレジスタが指定されている");
		m1 |= m2;
	} while (checkChar(','));
#ifdef OPEQ
	if (m1 & 0x100) {			/* w */
		if (h)
			put1Word(u ? 0x103b : 0x1039);		/* puluw pulsw */
		if (m1 & 0xff) {
			put1Byte(gOprPtr->opcode);
			put1Byte(m1 & 0xff);
		}
		if (!h) 				/* psh */
			put1Word(u ? 0x103a : 0x1038);		/* pshuw pshsw */
	} else {
		putCode(GROUP0, NO_MODE);
		putByte(m1);
	}
#else
	putCode(GROUP0, NO_MODE);
	putByte(m1);
#endif
}

void	pshs(void)
{
	pshpul(0, 0);
}

void	puls(void)
{
	pshpul(0, 1);
}

void	pshu(void)
{
	pshpul(1, 0);
}

void	pulu(void)
{
	pshpul(1, 1);
}

/*----------------------------------*/

void	branch(void)
{
	val_t	val;

	skipSpace();
	if ((val = expression() - gLinLc - 2) < -128 || 127 < val)
		error("ショート・ブランチがとどかない");
	putCode(GROUP0, NO_MODE);
	putByte(val);
}

void	lbranch(void)
{
#ifdef OPTIM
	OPTBL_T shortop;
	val_t	val;
#endif
	byte	f;

	skipSpace();
	f = checkChar('>');
	switch (gPass) {
	case 1:
#ifdef OPTIM
		if (gOpt_f && gOpt_sp < MAXOPTIM) {
			gOptStk[gOpt_sp++] = gLinLc;
		}
#endif
#ifdef OA
		if (gObjct == OB_ASM)
			expression();
#endif
		putCode(GROUP0, NO_MODE);
		putWord(0);
		return;
#ifdef OPTIM
	case -1:
		if (gOpt_f && gOpt_sp < MAXOPTIM) {
			if (gOptStk[gOpt_sp] == -1) {
				gOpt_sp++;
				putWord(0);
				return;
			}
			val = expression() - gOptStk[gOpt_sp] - 2;
			if (f == 0 && -128 <= val && val <= 127) {
				gOptChg++;
				gOptStk[gOpt_sp++] = -1;
				putWord(0);
				return;
			}
			gOptStk[gOpt_sp++] = gLinLc;
		}
		putCode(GROUP0, NO_MODE);
		putWord(0);
		return;
#endif
#ifdef OA
	case -2:
#endif
	case 2:
#ifdef OPTIM
		if (gOpt_f && gOpt_sp < MAXOPTIM && gOptStk[gOpt_sp++] == -1) {
			printChar('<', 4);
			gOptCount++;
			shortop.prefix = 0;
			shortop.opcode = (gOprPtr->opcode == 0x16 ? 0x20 :
					   gOprPtr->opcode == 0x17 ? 0x8d : gOprPtr->opcode);
			gOprPtr = &shortop;
			putCode(GROUP0, NO_MODE);
			putByte(expression() - gLinLc - 2);
			return;
		}
#endif
		putCode(GROUP0, NO_MODE);
		putWord(expression() - gLinLc - (gOprPtr->prefix ? 4 : 3));
		return;
	}
}

/*----------------------------------*/

void	os9svc(void)
{
	putCode(GROUP0, NO_MODE);
	skipSpace();
	printByte(putB(bytExpr()), 15);
}

void	mod(void)
{
	word	os9hdr[4];
	byte	sum, l;
	byte	*p;

	gOs9_f = 1;
	gObjLc = gLc = 0;
	gCrcBuf[0] = gCrcBuf[1] = gCrcBuf[2] = 0xff;
	skipSpace();
	put1Word(os9hdr[0] = 0x87cd);
	put1Word(os9hdr[1] = expression());
	checkCh_e(',');
	put1Word(os9hdr[2] = expression());
	checkCh_e(',');
	l = (expression() & 0xff);
	put1Byte(l);
	checkCh_e(',');
	put1Byte(os9hdr[3] = (expression() & 0xFF));
	os9hdr[3] |= l * 0x100;
	for (p = (byte *) os9hdr, sum = (byte)~0, l = 8; l-- > 0;) {
		sum ^= *p++;
	}
	put1Byte(sum & 0xff);
	if (checkChar(',')) {
		put1Word(expression());
		checkCh_e(',');
		put1Word(expression());
	}
	return;
}

void	emod(void)
{
 /* if (gOs9_f == 0) error("modがないのにemodがある"); */
	gOs9_f = 0;
	put1Byte(~gCrcBuf[0]);
	put1Byte(~gCrcBuf[1]);
	put1Byte(~gCrcBuf[2]);
	gOs9_f = 1;
}

void	fdb(void)
{
	skipSpace();
	do {
#ifdef HE
		if (checkChar('"')) {
			while (((*gLinPtr != '"') || (*++gLinPtr == '"'))
				   && (*gLinPtr != '\n')) {
				if (isKanji(*(byte*)gLinPtr)) {
					put1Byte(*gLinPtr++);
					put1Byte(*gLinPtr++);
				} else {
					put1Word(*gLinPtr++);
				}
			}
		} else {
			put1Word(expression());
		}
#else
		put1Word(expression());
#endif
	} while (checkChar(','));
}

void	fcb(void)
{
	skipSpace();
	do {
		if (checkChar('"')) {
			while (((*gLinPtr != '"') || (*++gLinPtr == '"'))
				   && (*gLinPtr != '\n')) {
				put1Byte(*gLinPtr++);
			}
#ifdef HE
		} else if (checkChar('#')) {
			for (;;) {
				byte	b, c;
				b = *gLinPtr;
				if (!isxdigit(b))
					break;
				c = *++gLinPtr;
				if (!isxdigit(c)) {
					error("fcbでの#指定は偶数桁でないといけない");
					break;
				}
				put1Byte(toXDigit(b) * 16 + toXDigit(c));
				gLinPtr++;
			}
#endif
		} else if (checkChar('>')) {
			put1Word(expression());
		} else {
			put1Byte(bytExpr());
		}
	} while (checkChar(','));
}

static void fccs(byte a)
{
	byte	temp[MNEMOSIZE + 1];
	byte *p;
	byte	b;
	byte	c;

	skipSpace();
	do {
		c = *gLinPtr++;
		if (c == '$' && toupper(*gLinPtr) == 'M') {
			getLabel(temp);
			if (stricmp(temp, "modnam") == 0 && gModName) {
				p = gModName;
				while ((b = *p++) != '\0') {
					if (a && *p == '\0')
						b |= 0x80;
					put1Byte(b);
				}
			} else {
				error("fccの指定がおかしい");
			}
		} else if (isSymbl3(c) || c == '%' || c == '(') {
			--gLinPtr;
			put1Byte(bytExpr());
		} else {
			while ((b = *gLinPtr++) != c) {
				if (b == '\n') {
					error("終わりのデリミタがない");
					return;
				} else {
					if (a && *gLinPtr == c)
						b |= 0x80;
					put1Byte(b);
				}
			}
		}
	} while (*gLinPtr++ == ',');
	--gLinPtr;
}

void	fcc(void)
{
	fccs(0);
}

void	fcs(void)
{
	fccs(1);
}

void	rzb(void)
{
	val_t	b;

	skipSpace();
	b = invExpr();
	printWord(gLc, 5);
	while (b-- > 0)
		putB(0);
}

/*----------------------------------*/

void	rmb(void)
{
	word	base;
	val_t	b;

	skipSpace();
	if (!gCSectSw) {
		if (gOrgSFmt_f && gObjct == OB_SFMT) {
			base = invExpr();
			flushObj();
			gLc += base;
		} else {
			if (gRmb_f && gFBasic_f){
				skipSpace();
				b = (int)(invExpr());
				printWord(gLc, 5);
				gRmb_sp += b;
				gObjCnt += b;
				gLc += b;
			} else {
				rzb();
			}
		}
	} else {
		base = invExpr();
		if (gLblPtr)
			gLblPtr->value = gCSectBase;
		clearAddress();
		printWord(gCSectBase, 5);
		gCSectBase += base;
	}
}

void	equ(void)
{
	val_t	val;

	skipSpace();
	val = expression(); 		/* invExpr() */
	if (gLblPtr)
		gLblPtr->value = val;
	clearAddress();
	printWord(val, 5);
}

void	set(void)
{
	val_t	val;

	skipSpace();
	val = invExpr();
	if (gLblPtr)
		gLblPtr->value = val;
	clearAddress();
	printWord(val, 5);
}

void	org(void)
{
	word	origin;

	skipSpace();
	origin = (word) invExpr();
	if (gStartAddr == 0xFFFF && gOrg_f == 0)
		gStartAddr = origin;
	if (gOs9_f) {
		gCSectSw = 1;
		gCSectBase = origin;
	} else {
		if ((gOrgSFmt_f && gObjct == OB_SFMT) || gOrg_f == 0) {
			flushObj();
			gObjLc = gLc = origin;
		} else if (gLc > origin) {
			error("orgの指定がおかしい");
			return;
		} else if (gLc < origin) {
			word	i;
			i = origin - gLc;
			while (i-- > 0)
				putB(0);
		}
	}
	printAddress(origin);
	gOrg_f = 1;
}

void	csct(void)
{
	gCSectSw = 2;
	skipSpace();
	if (*gLinPtr == '\n')
		gCSectBase = 0;
	else
		gCSectBase = invExpr();
	printWord(gCSectBase, 5);
}

void	endsct(void)
{
	if (gCSectSw > 1)
		gCSectSw = 0;
	else if (gPSect_f)
		gPSect_f = 0;
	else
		error("endsectがあまってる");
}

void	psct(void)
{
	gPSect_f = 1;
}

void	vsct(void)
{
	gCSectSw = 3;
}

/*----------------------------------*/

void	library(void)
{
	byte	fname[FNAMESZ + 1];
	FILE   *fp;
	byte	c;
	int 	i;
	byte *p;

	clearAddress();
	skipSpace();

	switch (c = *gLinPtr) {
	case '<':
		c = '>';
	case '\'':
	case '"':
		gLinPtr++;
		break;
	default:
		c = '\0';
	}
#ifdef INCLUDIR
	if (c == '>')
		p = stpcpy(fname, gIncDirName);
	else
#endif
		p = fname;
#ifdef INCLUDIR
	if (*gLinPtr == '$' && c != '>') {
		++gLinPtr;
		getLabel(fname);
		if (strcmp(fname, "INC") == 0) {
			p = stpcpy(fname, gIncDirName);
		} else {
			error("includeするファイル名がおかしい");
		}
	}
#endif
	for (i = FNAMESZ - 2; i--; p++, gLinPtr++) {
		*p = *gLinPtr;
		if (*p == '\n' || isspace(*p) || *p == '\0' || *p == c)
			break;
	}
	if (i == 0) {
		error("ファイル名が長すぎる");
		return;
	}
	*p = '\0';
	if (gVerbos_f)
		fprintf(STDERR, "[%s]\n", fname);
	DEBMSGF((STDERR, "include %s  (#%d)\n", fname, gFile_sp + 1));
	fp = fopenE(fname, "r");
	if (gFile_sp >= MAXLIB) {
		error("includeするファイルのネストが深する");
		exit(1);
	}
#ifdef FILSTK2
	strcpy(gFilStk2[gFile_sp].srcname, gSrcFName);
	gFilStk2[gFile_sp].srcline = gSrcLine;
	strcpy(gSrcFName, fname);
	gSrcLine = 0;
#endif
	gFileStk[gFile_sp++] = gSrcFp;
	gSrcFp = fp;
}

int 	popFile(void)
{
	if (gFile_sp <= 0)
		return 0;
	gSrcFp = gFileStk[--gFile_sp];
#ifdef FILSTK2
	strcpy(gSrcFName, gFilStk2[gFile_sp].srcname);
	gSrcLine = gFilStk2[gFile_sp].srcline;
#endif
	return 1;
}

void	endop(void)
{
	word	w;

	clearAddress();
	fclose(gSrcFp);
	if (popFile() != 0)
		return;
	gEOF_f = 1;
	skipSpace();
	w = (*gLinPtr != '\n') ? expression() : 0;
	if (gEntryAddr == 0xFFFF)
		gEntryAddr = w == 0 ? gStartAddr : w;
	printWord(gEntryAddr, 5);
	if(gRmb_f && gFBasic_f)
		gObjCnt -= gRmb_sp;
}

/*----------------------------------*/

void	opt(void)
{
	skipSpace();
	if (*gLinPtr == 'l') {
		++gList;
		++gLinPtr;
	} else if (*gLinPtr == '-' && *(gLinPtr + 1) == 'l') {
		--gList;
		gLinPtr += 2;
	}
}

void	spc(void)
{
}

void	nam(void)
{
}

void	page(void)
{
}


/*---------------------------------------------------------------------------*/

#ifdef OPED
void	none_d(void)
{
	byte *p;
	byte i,l;
	static byte tbl[16][5] = {
		{4,0x40,0x50,0x82,0x00},	/* negd 0x1040 */
		{0,0,0,0,0},
		{0,0,0,0,0},
		{2,0x53,0x43,0,0},			/* comd 0x1043 */
		{2,0x44,0x56,0,0},			/* lsrd 0x1044 */
		{0,0,0,0,0},
		{2,0x46,0x56,0,0},			/* rord 0x1046 */
		{2,0x47,0x56,0,0},			/* asrd 0x1047 */
		{2,0x58,0x49,0,0},			/* lsld 0x1048 */
		{2,0x59,0x49,0,0},			/* rold 0x1049 */
		{3,0x83,0x00,0x01,0},		/* decd 0x104a */
		{0,0,0,0,0},
		{3,0xc3,0x00,0x01,0},		/* incd 0x104c */
		{2,0xed,0x7e,0,0},			/* tstd 0x104d */
		{0,0,0,0,0},
		{2,0x4f,0x5f,0,0}			/* clrd 0x104f */
	};

 #ifndef M6809
	if (gM68_f == 0) {
		putCode(GROUP0, NO_MODE);	/*	none(); */
		return;
	}
 #endif
	p = (byte *)(tbl[gOprPtr->opcode - 0x40]);
	for (i = 10,l = *p++; l--; i += 2)
		printByte(putB(*p++),i);
}

static void putOped(byte o1, byte d1, byte o2, byte d2)
{
	put1Byte(gOprPtr->opcode + o1);
	put1Byte(d1);
	put1Byte(gOprPtr->opcode + o2);
	put1Byte(d2);
}

static void putOpedR(byte d1, byte d2, int reg)
{
	putOped(0x40+0x20, index0(d1,reg), 0x20, index0(d2,reg));
}

void	oped(void)
	/* andd  ord  eord	adcd  sbcd */
{
	int  val;
	int  reg, i, val2;

 #ifndef M6809
	if (gM68_f == 0) {
		load2();
		return;
	}
 #endif
	gIndirect = 0;
	skipSpace();
	if (checkChar('#')) {
		val = expression();
		putOped(0x40, val & 0xff, 0, val >> 8);
	} else if (checkChar(',')) {
		if (checkChar('-')) {
			if (checkChar('-'))
				putOpedR(0x82, 0x82,getReg(INDEXREG));
			else
				putOpedR(0x84, 0x82,getReg(INDEXREG));
		} else {
			reg = getReg(INDEXREG);
			if (checkChar('+')) {
				if (checkChar('+'))
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
			switch (reg = getReg(INDEXREG | PC | PCR)) {
			case X:
			case Y:
			case U:
			case S:
				for (++val, i = 1; i >= 0; --i, --val) {
					put1Byte(gOprPtr->opcode + 0x20 + i * 0x40);
					if (gValid_f && -16 <= val && val <= 15) {
						put1Byte(index0(val ? (val & 0x1f) : 0x84, reg));
					} else if (checkByte(val)) {
						put1Byte(index0(0x88, reg));
						put1Byte(val);
					} else {
						put1Byte(index0(0x89, reg));
						put1Word(val);
					}
				}
				break;
			case PC:
				for (++val, i = 1; i >= 0; --i, --val) {
					put1Byte(gOprPtr->opcode + 0x20 + i * 0x40);
					if (checkByte(val)) {
						put1Byte(index0(0x8c, 0));
						put1Byte(val);
					} else {
						put1Byte(index0(0x8d, 0));
						put1Word(val);
					}
				}
				break;
			case PCR:
				val2 = gLinLc + 3;
				for (++val, i = 1; i >= 0; --i, --val, val2 += 3) {
					put1Byte(gOprPtr->opcode + 0x20 + i * 0x40);
					if (checkByte(val - val2)) {
						put1Byte(index0(0x8c, 0));
						put1Byte(val - val2);
					} else {
						val2++;
						put1Byte(index0(0x8d, 0));
						put1Word(val - val2);
					}
				}
			}
		} else if ((gByte_f ||
				((word)(val - (gDp << 8)) <= 254 && gValid_f && !gWord_f))) {
			putOped(0x40+0x10, val - (gDp << 8) + 1, 0x10, val - (gDp << 8));
		} else {
			put1Byte(gOprPtr->opcode + 0x30 + 0x40);
			put1Word(val + 1);
			put1Byte(gOprPtr->opcode + 0x30);
			put1Word(val);
		}
	}
}
#endif

#ifndef M6809
#ifdef OPEQ
void	none_wq(void)
{
	static byte tbl[] = {
		0x10,0xed,0x7c,0,					/*	tstq  stq -4,s; */
		0x10,0x4f,0x10,0x5f,				/*	clrq  clrw;clrd */
		0x10,0x53,0x10,0x43,				/*	comq  comw;comd */
		0x10,0x44,0x10,0x56,				/*	lsrq  lsrd;rorw */
		0x10,0x47,0x10,0x56,				/*	asrq  asrd;rorw */
		0x10,0x46,0x10,0x56,				/*	rorq  rord;rorw */
		0x10,0x59,0x10,0x49,				/*	rolq  rolw;rold */
		0x1c,0xfe,0x10,0x59,				/*	lslw  andcc #$fe;rolw */
		0x10,0x53,0x10,0x5c,				/*	negw  comw;incw */
		0x10,0x38,0x68,0xe1,0x10,0x56,		/*	asrw  pshw;asl ,s++;rorw */
		0x1c,0xfe,0x10,0x59,0x10,0x49,		/*	lslq  andcc #$fe;rolw;rold */
		0x10,0x5c,0x26,0x02,0x10,0x4c,		/*	incq  incw;bne *+4;incd */
		0x10,0x5d,0x26,0x02,0x10,0x4a,0x10,0x5a,
											/* decq  tstw;bne *+4;decd;decw */
		0x10,0x43,0x10,0x53,0x10,0x5c,0x26,0x02,0x10,0x4c,
											/*	negq  comd;comw;incw;bne *+4;incd*/
		0
	};
	byte *p;
	byte l;

	p = tbl + gOprPtr->prefix;
	l = gOprPtr->opcode;
	while (l--)
		put1Byt2(*p++);
}

static void putOpeqR(word *op, byte d1, byte d2, int reg)
{
	put1Word(op[1] + 0x20);
	put1Byte(index0(d1,reg));
	put1Word(*op + 0x20);
	put1Byte(index0(d2,reg));
}

void		opeq(void)	/* addq  subq */
{
	shrt val, val2;
	shrt reg, i;
	word op[2];

	op[0] = gOprPtr->prefix + 0x1000;	/* d */
	op[1] = gOprPtr->opcode + 0x1000;	/* w */
	gIndirect = 0;
	skipSpace();
	if (checkChar('#')) {
		imm4Expr(&val, &val2);
		put1Word(op[1]);
		put1Word(val2);
		put1Word(op[0]);
		put1Word(val);
	} else if (checkChar(',')) {
		if (checkChar('-')) {
			if (checkChar('-'))
				if (checkChar('-'))
					if (checkChar('-'))
						putOpeqR(op,0x83,0x83,getReg(INDEXREG));
					else
						putOpeqR(op,0x82,0x83,getReg(INDEXREG));
				else
						putOpeqR(op,0x84,0x83,getReg(INDEXREG));
			else
						putOpeqR(op,0x01,0x82,getReg(INDEXREG));
		} else {
			reg = getReg(INDEXREG);
			if (checkChar('+')) {
				if (checkChar('+'))
					if (checkChar('+'))
						if (checkChar('+')) {
							putOpeqR(op,2,0x84,reg);
							put1Byte((reg == Y) ? 0x31 :
									 (reg == U) ? 0x32 :
									 (reg == S) ? 0x33 : 0x30 );
							put1Byte(index0(4,reg));
						} else {
							putOpeqR(op,2,0x84,reg);
							put1Byte((reg == Y) ? 0x31 :
									 (reg == U) ? 0x32 :
									 (reg == S) ? 0x33 : 0x30 );
							put1Byte(index0(3,reg));
					} else {
							putOpeqR(op,2,0x81,reg);
				} else {
							putOpeqR(op,2,0x80,reg);
				}
			} else {
					putOpeqR(op,2,0x84,reg);
			}
		}
	} else {
		val = expression();
		if (checkChar(',')) {
			switch (reg = getReg(INDEXREG | PC | PCR)) {
			case X:
			case Y:
			case U:
			case S:
				for (val+=2, i = 1; i >= 0; --i, val-= 2) {
					put1Word(op[i] + 0x20);
					if (gValid_f && -16 <= val && val <= 15) {
						put1Byte(index0(val ? (val & 0x1f) : 0x84, reg));
					} else if (checkByte(val)) {
						put1Byte(index0(0x88, reg));
						put1Byte(val);
					} else {
						put1Byte(index0(0x89, reg));
						put1Word(val);
					}
				}
				break;
			case PC:
				for (val += 2, i = 1; i >= 0; --i, val-= 2) {
					put1Word(op[i] + 0x20);
					if (checkByte(val)) {
						put1Byte(index0(0x8c, 0));
						put1Byte(val);
					} else {
						put1Byte(index0(0x8d, 0));
						put1Word(val);
					}
				}
				break;
			case PCR:
				val2 = gLinLc + 4;
				for (val += 2, i = 1; i >= 0; --i, val -= 2, val2 += 4) {
					put1Word(op[i] + 0x20);
					if (checkByte(val - val2)) {
						put1Byte(index0(0x8c, 0));
						put1Byte(val - val2);
					} else {
						val2++;
						put1Byte(index0(0x8d, 0));
						put1Word(val - val2);
					}
				}
			}
		} else if ((gByte_f || ((word)(val - (gDp << 8)) <= 253 && gValid_f && !gWord_f))) {
			put1Word(op[1] + 0x10);
			put1Byte(val - (gDp << 8) + 2);
			put1Word(op[0] + 0x10);
			put1Byte(val - (gDp << 8));
		} else {
			put1Word(op[1] + 0x30);
			put1Word(val + 2);
			put1Word(op[0] + 0x30);
			put1Word(val);
		}
	}
}
#endif
#endif


/*---------------------------------------------------------------------------*/
static OPTBL_T *oOpHash[256];

static int	hash(byte *s)
{
	int h;

	h = 0;
	while (*s)
		h = (h<<2) + h + *s++;
	return (h & 0xff);
}

OPTBL_T *srchOpTbl(byte *s)
{
	OPTBL_T *q;

	for (q = oOpHash[hash(s)]; q != NULL; q = q->nl) {
		if (strcmp(s, q->mnemonic) == 0)
			return q;
	}
	return NULL;
}

void	initOpTbl(void)
{
	OPTBL_T *p;
	OPTBL_T *q;
	int  h;

	for (p = gOpTab; *p->mnemonic; p++) {
		q = oOpHash[h = hash(p->mnemonic)];
		if (q) {
			while (q->nl != NULL)
				q = q->nl;
			q->nl = p;
		} else {
			oOpHash[h] = p;
		}
		p->nl = NULL;
	}
}




/*---------------------------------------------------------------------------*/
static void co_if(byte f)
{
	int val;

	val = 0;
	if (f >= CO_IF) {
		skipSpace();
		val = invExpr();
	}
	switch (f) {
	case CO_IFP1:
		gCoStk[++gCo_sp] = (gPass == 1) ? 2 : -3;
		break;
	case CO_IFN:
		val = !val;
		goto J1;
	case CO_IFGE:
		val = (val >= 0);
		goto J1;
	case CO_IFGT:
		val = (val > 0);
		goto J1;
	case CO_IFLE:
		val = (val <= 0);
		goto J1;
	case CO_IFLT:
		val = (val < 0);
	case CO_IF:
 J1:
		gCoStk[++gCo_sp] = (val) ? 1 : -1;
		break;
	case CO_ELIF:
		if (gCoStk[gCo_sp] == -1) {
			gCoStk[gCo_sp] = (val) ? 1 : -1;
			break;
		}
	case CO_ELSE:
		switch (gCoStk[gCo_sp]) {
		case  0: error("elseかelsifがあまっている");break;
		case -1: gCoStk[gCo_sp] = 1; break;
		case  1: gCoStk[gCo_sp] = -2; break;
		case  2: error("else,elsifがifp1の対になっている");
		}
		break;
	case CO_ENDC:
		if (gCoStk[gCo_sp] == 2) {
			if (gP1_sp > GP1_MAX)
				error("ifp1の数が多すぎます");
			gP1Stk[gP1_sp++] = gLineNo;
		} else if (gCoStk[gCo_sp] == -3) {
			gLineNo = gP1Stk[gP1_sp++];
		}
		gCoStk[gCo_sp] = 0;
		if (--gCo_sp < 0)
			error("endifがあまってる");
 #ifdef DEBUG
		break;
	default:
		error("co_if()の引数がおかしい");
 #endif
	}
}


static int	getMnemonic(void)
{
	static byte temp[MNEMOSIZE + 1];
	byte	*p;
	byte	*pp;
	OPTBL_T *q;

	skipSpace();
	if (*gLinPtr == '\n')
		return 0;
	if (!isSymbl2(*gLinPtr)) {
		error("ニーモニックの指定がおかしい");
		DEBMSGF((STDERR, "*gLinPtr : %c\n", *gLinPtr));
		return 0;
	}
	pp = (p = temp) + MNEMOSIZE;
	while (isSymbl(*p = toupper(*gLinPtr))) {
		if (p++ >= pp)
			goto ERR;
		gLinPtr++;
	}
	*p = '\0';
	if (p > temp+4 && *(p-2) == '_' && *(p-1) == 'I') {
		*(gLinPtr-2) = *(gLinPtr-1) = ' ';
		*(p-2) = '\0';
		skipSpace();
		if (*gLinPtr == '\n')
			goto ERR;
		*--gLinPtr = '#';
	}
	if ((q = srchOpTbl(temp)) != NULL) {
 #ifndef M6809
		if (gM68_f && (q->option & 0x01))
			error("6809モードで6309命令が使われた");
 #endif
		if (q->process == NULL) {
			co_if(q->prefix);
			return 0;
		} else if (gCoStk[gCo_sp] < 0)
			return 0;
		gOprPtr = q;
		return 1;
	}
 ERR:
	error("ニーモニックでない名前");
	return 0;
}

static byte *getLine(void)
{
	gLinPtr = gLineBuf + LINEHEAD;
	return fgets(gLinPtr, MAXCHAR - LINEHEAD, gSrcFp);
}

static byte oneLine(void)
{
	byte temp[LBLSIZE + 1];
	byte c,f,gf;

	while (getLine() == NULL) {
		fclose(gSrcFp);
		if (popFile() == 0)
			return 2;
	}
	initLine();
	c = *gLinPtr;
	if (c == '*' || c == '#')
		clearAddress();
	else {
		gf = temp[0] = '\0';
		if (!isspace(c) && c != '\n')
			gf = getLabel(temp);
 #ifdef OA
		if (gObjct == OB_ASM && gf && gPass == 2) {
			oa_putStr(gLineBuf+LINEHEAD,0);
		}
 #endif
		f = getMnemonic();
		if (temp[0] && gCoStk[gCo_sp] >= 0)
			defLabel(temp, strcmp(gOprPtr->mnemonic,"SET") ? 1 : 2, gf);
		return f;
	}
	return 0;
}

static void initPass(void)
{
 #ifndef M6809
	gImVal = 512;
 #endif
 #ifdef OA
	gOA_sp =
 #endif
 #ifdef FILSTK2
	gSrcLine =
 #endif
 #ifdef OPTIM
	gOptCount = gOpt_sp = gOptChg =
 #endif
	gFile_sp = gLineNo = gErrors = gP1_sp = gCo_sp = gObjPos = gRmb_sp =
	gObjCnt = gLc = gDp = gObjLc =
	gEOF_f = gGrp = gCSectSw = gPSect_f = gOrg_f = (byte)0;
 #ifdef OPTIM
	if (gVerbos_f)
		fprintf(STDERR, gPass == -1 ? "<pass 1.5>\n" : "<pass %d>\n", gPass);
 #else
	if (gVerbos_f)
		fprintf(STDERR, "<pass %d>\n", gPass);
 #endif
}

static void assemble(int argc, char **argv)
{
	int  i;
	byte f;

	initPass();
 #ifdef OPTS_FBAS
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
		strncpy(gSrcFName,argv[i],FNAMESZ);
		++gGrp;
		DEBMSGF((STDERR, "open %s\n", gSrcFName));
		gSrcFp = fopenE(gSrcFName, "r");
		if (gVerbos_f)
			fprintf(STDERR, "[%s]\n", gSrcFName);

		while (!gEOF_f) {
			if ((f = oneLine()) == 2)
				break;
			else if (f != 0) {
				if (gCSectSw > 1 &&
					strcmp(gOprPtr->mnemonic,"RMB")
					&& strcmp(gOprPtr->mnemonic,"ENDSECT")) {
					if (gCSectSw == 3) {
						if (strcmp(gOprPtr->mnemonic,"FDB")
							&& strcmp(gOprPtr->mnemonic,"FCB")
							&& strcmp(gOprPtr->mnemonic,"FCC")
							&& strcmp(gOprPtr->mnemonic,"FCS")
							&& strcmp(gOprPtr->mnemonic,"RZB"))
							error("vsect～endsectで使えない命令がある");
					} else {
						error("csect～endsectではrmb以外は使えない");
					}
				}
 #ifdef OA
				if (gObjct == OB_ASM && gPass == 2
					&& gOAStk[gOA_sp].ll == gLineNo) {
					DEBMSGF((STDERR,"OA#2:%d line=%d size=%d / gLineNo=%d\n"
						,gOA_sp,gOAStk[gOA_sp].ll,gOAStk[gOA_sp].nn,gLineNo));
					oa_putStr(gLineBuf+LINEHEAD,gOAStk[gOA_sp++].nn);
				} else if (gPass == -2) {
					word  bb;

					bb = gObjCnt;
					gOAchk_f = 0;
					gOprPtr->process();
					if (gOAchk_f) {
						gOAStk[gOA_sp].ll = gLineNo;
						gOAStk[gOA_sp].nn = gObjCnt - bb;
						DEBMSGF((STDERR,"OA#-2:line=%d	size=%d\n",
							gOAStk[gOA_sp].ll,gOAStk[gOA_sp].nn));
						if (++gOA_sp >= OA_MAX)
							error("-aオプション時のアセンブルしない行が多すぎる");
					}
				} else
 #endif
				{
					gOprPtr->process();
				}
				if (*gLinPtr != '\n' && !isspace(*gLinPtr)) {
					error("邪魔な文字がある");
					DEBMSGF((STDERR, "*gLinPtr : %c(%02x)\t[asemmble()]\n",
						*gLinPtr,*gLinPtr));
				}
			}
			putLine();
		}
	}
 #ifdef OPTS_FBAS
	if (gFBasic_f && gPass == 2) {
		putObj(0xff);
		put2obj(0x0000);
		if (gEntryAddr == 0xFFFF)
			gEntryAddr = gStartAddr;
		put2obj(gEntryAddr);
		putObj(0x1a);
	}
 #endif
	gObjSiz = gObjCnt;
}

/*---------------------------------------------------------------------------*/

static word xstrtoui(byte *p,byte **q)
{
	word w;

	for (w = 0; isxdigit(*p); p++)
		w = w * 16 +
			(isdigit(*p) ? (*p - '0') :
			(toupper(*p) - 'A' + 10));
	if (q)
		*q = p;
	return w;
}

static void getModNam(char *modnam, char *fnam)
{
	char *ep;
	int  i;
	char *s;

	for (ep = NULL, s = fnam, i = MODNAMSZ; *s != '\0' && i--; ++s) {
		if (isKanji(*(byte*)s)) {
			if (*(++s) == '\0')
				break;
		} else if (*s == '/'
 #ifdef MSDOS
			|| *s == ':' || *s == '\\'
 #endif
		){
			fnam = s + 1;
		} else if (*s == '.') {
			ep = s;
		}
	}
	if (ep == NULL || ep < fnam)
		ep = s;
	for (s = modnam; (*s = *fnam) != '\0' && fnam < ep; ++s, ++fnam) {}
	*s = '\0';
}


/*---------------------------------------------------------------------------*/
static char *oLstFName, *oObjFName;
static byte oList_f, oSymbol_f;

static void printLog(void)
{
	if (gList > 0) {
		fprintf(gLstFp, "\n    Total Errors %d\n", gErrors);
		if (gVerbos_f)
			fprintf(gLstFp, "    Total labels %d\n", gLabels);
	}
	fprintf(STDERR, "\n    Obj_Size = $%04x(%d)\n",gObjCnt,gObjCnt);
	fprintf(STDERR, "    Total Errors %d\n", gErrors);
	if (gVerbos_f)
		fprintf(STDERR, "    Total labels %d\n", gLabels);
  #ifdef OPTIM
	if (gOpt_f)
		fprintf(STDERR, "    Total Optimized Branch %d\n", gOptCount);
  #endif
}

static void usage(void)
{
	fprintf(STDERR,"usage: %s [-opts] src_file...\n",gCmdName);
	e_puts(" -?  ヘルプ\n");
	e_puts(" -9  os9標準asmモード            -8  6809モード\n");
	e_puts(" -s  シンボル・テーブルを表示    -v  途中経過の表示\n");
	e_puts(" -u  ラベルの大小文字を区別する  -n  ラベルの大小文字を区別しない\n");
	e_puts(" -q  org,rmbでアドレスが飛び飛びになるのを許す\n");
  #ifdef OPTIM
	e_puts(" -y  ロング・ブランチを可能ならショート・ブランチに置換\n");
  #endif
	e_puts(" -m<mod_name>  $modnam文字列変数を設定\n");
	e_puts(" -d<LBL>[=Val] ラベルLBLを値Valで定義.  LBL: set Val(省略:1)\n");
	e_puts(" -l[lst_file]  アセンブリ・リストをlst_fileに出力\n");
	e_puts(" -o[obj_file]  obj_fileにオブジェクトをバイナリで出力\n");
	e_puts(" -f[obj_file]  obj_fileにオブジェクトをS-Formatで出力\n");
  #ifdef OA
	e_puts(" -a[obj_file]  obj_fileにオブジェクトをFCBのデータの形で出力\n");
  #endif
  #ifdef OE
	e_puts(" -e[err_file]  err_fileにソースのエラーを出力\n");
  #endif
  #ifdef INCLUDIR
	e_puts(" -i[lib_file]  $INCで参照するディレクトリ\n");
  #endif
  #ifdef OPTS_FBAS
	e_puts(" -k[Start[,Enter]]  F-BASICマシン語ファイル形式で出力\n");
	e_puts(" -r  F-BASIC機械語ファイル形式時, rmbでその後にコードが無ければ0を出力しない\n");
  #endif
  #ifdef DEBUG
	e_puts(" -c  Debug mode\n");
  #endif
  #ifdef M6809
	DEBMSGF((STDERR,"6309に対応していない\n"));
  #endif
  #ifdef OPED
	DEBMSGF((STDERR,"6809拡張命令あり(D)\n"));
  #endif
  #ifdef OPEQ
	DEBMSGF((STDERR,"6309拡張命令あり(Q)\n"));
  #endif
	exit(0);
}

static void optsDefLbl(int argc,char **argv)
{
	char temp[LBLSIZE + 1];
	char *p;
	int  i;

	for (i = 1; i < argc; ++i) {
		p = argv[i];
		if (*p++ != '-')
			continue;
		if (*p++ != 'd' && *(p-1) != 'D')
			continue;
		if (*p == '\0') {
			e_puts("-d でラベル名が指定されていない\n");
			continue;
		}
		gLinPtr = strncpy(gLineBuf+LINEHEAD,p,MAXCHAR-LINEHEAD-2);
		getLabel(temp);
		defLabel(temp,2,1); /* set,グローバルラベル */
		gLblPtr->line = 1;
		gLblPtr->grp = 0;
		if (*gLinPtr++ == '=') {
			if (*gLinPtr == '$') {
				gLinPtr++;
				gLblPtr->value = (int)xstrtoui(gLinPtr,NULL);
			} else {
				gLblPtr->value = atoi(gLinPtr);
			}
		} else {
			gLblPtr->value = 1;
		}
		DEBMSGF((STDERR,"-d opts : %s  (temp %s) : %s = $%x\n", p, temp, gLblPtr->name, gLblPtr->value));
	}
}

static void options(byte *p)
{
	byte *pp;
	byte c;

	while ((c = *p) != '\0') {
		if (*++p == '=')
			++p;
		switch (toupper(c)) {
	  #ifdef DEBUG
		case 'C':
			gDebug_f = 1;
			break;
	  #endif
		case '?':
			usage();
		case 'D':
			goto LOOPOUT;
		case 'Q':
			gOrgSFmt_f = 1;
			break;
		case 'U':
			gUpLo_f = 0;
			break;
		case 'N':
			gUpLo_f = 1;
			break;
		case 'V':
			gVerbos_f = 1;
			break;
	  #ifndef M6809
		case '8':
			gM68_f = 1;
			break;
	  #endif
		case '9':
			gOs9_f = gUpLo_f = 1;
			break;
		case 'S':
			oSymbol_f = 1;
			break;
		case 'L':
			if (*p) {
				oLstFName = p;
				oList_f = 1;
			} else {
				oList_f = 0;
			}
			gList = 1;
			goto LOOPOUT;
		case 'M':
			if (*p) {
				strncpy(gModName, p, MODNAMSZ);
			}
			goto LOOPOUT;
	  #ifdef OE
		case 'E':
			if (*p)
				gErrFName = p;
			else
				gErrFName = (byte *)(~0);
			goto LOOPOUT;
	  #endif
	  #ifdef OA
		case 'A':
			if ((gOAStk = (OATBL_T *) calloc(OA_MAX,sizeof(OATBL_T))) == NULL){
				e_puts("-aオプションを行うためのメモリが足りない\n");
				break;
			}
			gObjBufSz = 16;
			gObjct = OB_ASM;
			goto OB;
	  #endif
		case 'O':
			gObjct = OB_BIN;
			goto OB;
		case 'F':
			gObjct = OB_SFMT;
		 OB:
			if (*p)
				oObjFName = p;
			goto LOOPOUT;
	  #ifdef OPTIM
		case 'Y':
			if ((gOptStk = (int *)malloc(sizeof(int) * MAXOPTIM))
				== NULL) {
				e_puts("オプティマイズを行うためのメモリが足りない\n");
				break;
			}
			gOpt_f = 1;
			break;
	  #endif
	  #ifdef OPTS_FBAS
		case 'K':
			gFBasic_f = 1;
			gRmb_sp = 0;
			if (*p) {
				gStartAddr = xstrtoui(p,&pp);
				p = pp;
				if (gStartAddr == 0)
					gStartAddr = 0xFFFF;
				if (*p == ',') {
					gEntryAddr = xstrtoui(p+1,&pp);
					/*p = pp;*/
				}
				if (gEntryAddr == 0)
					gEntryAddr = 0xFFFF;
			}
			goto LOOPOUT;
		case 'R':
			gRmb_f  = 1;
			break;
	  #endif
	  #ifdef INCLUDIR
		case 'I':
			gIncDirName = ".";
			if (*p)
				gIncDirName = p;
			if (strlen(gIncDirName) >= FNAMESZ-10) {
				e_puts("ファイル名が長すぎる\n");
				exit(1);
			}
			goto LOOPOUT;
	  #endif
		default:
			fprintf(STDERR,"%s: オプションが間違っている(-%c)\n", gCmdName,c);
			exit(1);
		}
	}
 LOOPOUT:;
}

int main(int argc, char *argv[])
{
	static char *title = "HD6309 cross assembler version 01.20T\n";
	char *p;
	int  i;

	gCmdName   = argv[0];
	gObjBufSz  = OBJSIZE;
	gEntryAddr = gStartAddr = 0xFFFF;
	gObjFp     = NULL;
	gLstFp     = stdout;
  #ifdef INCLUDIR
	gIncDirName= INCLUDIR;
  #endif
  #ifdef OE
	gErrFp     = STDERR;
	gErrFName  =
  #endif
	oLstFName  = oObjFName = NULL;
	oList_f    = gModName[0] = gSrcFName[0] = gSrcFName[FNAMESZ] = '\0';
	gUpLo_f    = 1;

	e_puts(title);
	for (i = 1; i < argc; i++) {
		p = argv[i];
		if (*p != '-') {
			if (gSrcFName[0] == '\0')
				strncpy(gSrcFName,p,FNAMESZ);
		} else {
			if (*++p == '\0')
				usage();
			options(p);
		}
	}
	if (*gSrcFName == '\0') {
		fprintf(STDERR, "usage: %s [-opts] src_file...(-?でオプション説明)\n",
			gCmdName);
		exit(1);
	}
	if (oObjFName == NULL && gObjct) {
		oObjFName = mallocE(FNAMESZ+1);
		FIL_ChgExt(strcpy(oObjFName, gSrcFName),
	  #ifdef OA
			(gObjct == OB_ASM) ? "oa" :
	  #endif
			(gObjct == OB_SFMT) ? "s" : "o");
	}
  #ifdef OE
	if (gErrFName == (byte *)(~0)) {
		gErrFName = mallocE(FNAMESZ+1);
		FIL_ChgExt(strcpy(gErrFName, gSrcFName),"err");
	}
  #endif
	if (*gModName == '\0') {
		getModNam(gModName,gSrcFName);
	}
	DEBMSGF((STDERR, "gModName = %s\n",gModName));
	initNode();
	initOpTbl();
	optsDefLbl(argc,argv);

	gPass = 1;
	DEBMSGF((STDERR, "enter pass 1\n"));
	assemble(argc,argv);

  #ifdef OPTIM
	if (gOpt_f) {
		gPass = -1;
		DEBMSGF((STDERR, "enter pass 1.5\n"));
		do {
			assemble(argc, argv);
		} while (gOptChg);
	}
  #endif

  #ifdef OA
	if (gObjct == OB_ASM) {
		gPass = -2;
		DEBMSGF((STDERR, "enter pass 1.9\n"));
		assemble(argc, argv);
	}
  #endif

	gPass = 2;
	DEBMSGF((STDERR, "enter pass 2\n"));
	if (gList || oSymbol_f) {
		if (oList_f) {
			DEBMSGF((STDERR, "open %s\n", oLstFName));
			gLstFp = fopenE(oLstFName, "w");
		} else {
			gLstFp = stdout;
		}
	}
	if (gObjct) {
		DEBMSGF((STDERR, "open %s\n", oObjFName));
		gObjFp = fopenE(oObjFName, (gObjct == OB_BIN) ? "wb" : "w");
	}
	assemble(argc,argv);
	if (gObjct) {
		termObj();
	  #if 0 /*def OS9*/
		if (gObjct == OB_BIN) {
			chmod(oObjFName,0x07/*S_EXEC|S_IWRITE|S_IREAD*/);
		}
	  #endif
	}
	if (oSymbol_f)
		dumpSymbol();
	printLog();

	DEBMSGF((STDERR, "close all files\n"));
	if (gObjct)
		fclose(gObjFp);
	if (oList_f)
		fclose(gLstFp);
	if (gErrFName == NULL)
		fclose(gErrFp);
	return (gErrors ? 1 : 0);
}

