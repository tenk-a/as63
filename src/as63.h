/********************************
 *								*
 *		HD6309 cross assembler	*
 *								*
 ********************************/

#ifdef EXT
 #define EXTERN
#else
 #define EXTERN extern
#endif

typedef unsigned char  byte;/* 1ﾊﾞｲﾄ符号無整数型 */
typedef unsigned short word;/* 2ﾊﾞｲﾄ符号無整数型 */
typedef short	shrt;		/* 2ﾊﾞｲﾄ符号付整数型 */
#define __(x)	x

#define STDERR		stderr
#define isKanji(c)	((unsigned)((c)^0x20) - 0xa1 < 0x3cU)
#define isKanji2(c) ((byte)(c) >= 0x40 && (byte)(c) <= 0xfc && (c) != 0x7f)
#define toXDigit(c) (isdigit(c) ? (c - '0') : (toupper(c) - 'A' + 10))
#define e_puts(s)	fprintf(STDERR,s)
#ifndef OS9
#define stpcpy(d,s)	(strcpy((d),(s)),(d)+strlen(d))
#endif

#ifdef	DEBUG
 #define DEBMSGF(x) 	if (gDebug_f) (fprintf x)
#else
 #define DEBMSGF(x)
#endif

/*---------------------------------------------------------------------------*/

/* buffer size */
#define LINEHEAD		24		/* ﾘｽﾄ表示でのｿｰｽ行の表示位置           */
#define OBJSIZE 		32		/* objの出力ﾊﾞｯﾌｧのｻｲｽﾞ(byte):S式での１行分 */
#define MNEMOSIZE		8		/* ﾆｰﾓﾆｯｸの最大文字数                   */
#define MAXCHAR 		1024	/* 入力する行の最大文字数               */
#define MAXLABEL		256 	/* 名前表用のﾒﾓﾘ拾得のﾌﾞﾛｯｸ･ｻｲｽﾞ(node数)*/
#define MODNAMSZ		29		/* module name の最大文字数             */

typedef int  val_t; 			/* int<->long  定数演算のint/long化     */
/*#define M6809 */				/* 定義すると6309の命令を生成しない     */
#define HE						/* 定義すると余計なものを生成する       */
#define OE						/* 定義すると-eｵﾌﾟｼｮﾝ関係を生成         */
#define OA						/* 定義すると-aｵﾌﾟｼｮﾝ関係を生成         */
#define OPTIM					/* 定義するとｵﾌﾟﾃｨﾏｲｽﾞ(-y)を行える      */
#define OPTS_FBAS				/* 定義すると-k(FBASICﾏｼﾝ語ﾌｧｲﾙ出力)を生成*/
#define FILSTK2 				/* 定義するとｴﾗｰ時にﾌｧｲﾙ名,行番号を表示 */
#define OPED					/* 6809用合成命令を使用可能にする       */
#define OPEQ					/* 6309用合成命令を使用可能にする       */
#define FNAMESZ 		127 	/* ﾌｧｲﾙ名(ﾊﾟｽﾘｽﾄ)の最大文字数           */
#define MAXLIB			16		/* includeでネストできる深さ            */
#define LBLSIZE 		21		/* ラベル名の文字数                     */
#define MAXOPTIM		3000	/* longﾌﾞﾗﾝﾁをｼｮｰﾄに変更できる最大数    */
#define GCO_MAX 		40		/* ifﾈｽﾄの最大の深さ */
#define GP1_MAX 		100 	/* ifp1の使える回数 */
#ifdef OA
# define OA_MAX 500
#endif
#define INCLUDIR		"."

#ifdef M6809
 #ifdef OPEQ
  #undef OPEQ
 #endif
#endif

/* register notation */
#define NONE			0
#define CC				0x01
#define A				0x02
#define B				0x04
#define D				0x06
#define DP				0x08
#define X				0x10
#define Y				0x20
#define U				0x40
#define S				0x80
#define PC				0x100
#define PCR 			0x200
#define E				0x400
#define F				0x800
#define W				0xC00
#define V				0x1000
#define N				0x2000
#define INDEXREG		(X|Y|U|S)
#define OFFSETRG		(A|B|E|F)
#define ALLREG			(CC|A|B|D|DP|X|Y|U|S|PC)
#define X63REG			(E|F|W|V|N)

/* addressing mode */
#define IMMEDIATE		0x01
#define IMMEDIATE2		0x02
#define DIRECT			0x04
#define INDEX			0x08
#define EXTEND			0x10
#define LOAD			(IMMEDIATE|DIRECT|INDEX|EXTEND)
#define LOAD2			(IMMEDIATE2|DIRECT|INDEX|EXTEND)
#define STORE			(DIRECT|INDEX|EXTEND)
#define MEMORY			(DIRECT|INDEX|EXTEND)

/* addressing mode variation group */
#define GROUP0			0
#define GROUP1			1
#define GROUP2			2
#define GROUP3			3		/* tfm */

/* mode offset */
#define NO_MODE 		0
#define IMMEDIATE_MODE	0
#define DIRECT_MODE 	1
#define INDEX_MODE		2
#define EXTEND_MODE 	3


/* 条件アセンブラ */
	#define CO_ELSE 	1
	#define CO_ENDC 	2
	#define CO_IFP1 	3
	#define CO_IF		4
	#define CO_IFN		5
	#define CO_ELIF 	6
	#define CO_IFGE 	7
	#define CO_IFGT 	8
	#define CO_IFLE 	9
	#define CO_IFLT 	10

/* none_wq */
#define WQ_TSTQ 0
#define WQ_CLRQ (1*4)
#define WQ_COMQ (2*4)
#define WQ_LSRQ (3*4)
#define WQ_ASRQ (4*4)
#define WQ_RORQ (5*4)
#define WQ_ROLQ (6*4)
#define WQ_LSLW (7*4)
#define WQ_NEGW (8*4)
#define WQ_ASRW (9*4)
#define WQ_LSLQ (10*4+2)
#define WQ_INCQ (12*4)
#define WQ_DECQ (13*4+2)
#define WQ_NEGQ (15*4+2)

/* 出力するファイルの種類 */
#define OB_BIN		1
#define OB_SFMT 	2
/*#define OBJ_ROF	*/
#define OB_ASM		4

/* opcode table */
typedef struct optbl_t {
		char *mnemonic;
		byte prefix;
		byte opcode;
		byte option;
		void (*process)(void);
		struct optbl_t *nl;
} OPTBL_T;

/* label table */
typedef struct lbltbl_t {
		int    line;
		val_t  value;
		struct lbltbl_t *left,*right;
		char   flg;			/* 0:used  1:EQU  2:SET */
		byte   grp;
		char   name[LBLSIZE + 1];
} LBLTBL_T;


/*-------------------------------- var -------------------------------------*/
#ifdef DEBUG
 EXTERN byte gDebug_f;
#endif

/* アセンブル */
EXTERN FILE *gSrcFp;
EXTERN OPTBL_T *gOprPtr;
EXTERN int  gErrors, gPass;
EXTERN int  gImVal, gIndirect;
EXTERN word gDp;
EXTERN word gLc, gLinLc, gObjLc;
EXTERN byte gValid_f, gEOF_f;
EXTERN byte gOs9_f, gOrg_f, gOrgSFmt_f;
EXTERN byte gByte_f, gWord_f;
EXTERN char gModName[MODNAMSZ+1];
#ifndef M6809
 EXTERN byte gM68_f;
#endif

/* object出力 */
EXTERN FILE *gObjFp;
EXTERN word gObjSiz, gObjCnt;
EXTERN word gStartAddr, gEntryAddr;
EXTERN byte gObjct;
EXTERN int  gObjPos;
EXTERN int  gObjBufSz;
EXTERN byte gObjBuf[OBJSIZE];
EXTERN byte gCrcBuf[3];
EXTERN int  gRmb_sp,gRmb_f;
#ifdef OPTS_FBAS
 EXTERN byte gFBasic_f;
#endif

/* ラベル */
EXTERN LBLTBL_T *gLblPtr;
EXTERN int  gLabels, gLineNo;
EXTERN word gCSectBase;
EXTERN byte gGrp, gCSectSw;
EXTERN byte gUpLo_f, gPSect_f;

/* リスト、メッセージ表示 */
EXTERN char *gCmdName;
EXTERN int  gList;
EXTERN FILE *gLstFp;
EXTERN char *gLinPtr;
EXTERN byte gVerbos_f;
EXTERN char gLineBuf[MAXCHAR+2];
#ifdef OE
 EXTERN FILE *gErrFp;
 EXTERN char *gErrFName;
#else
 #define gErrFp STDERR
#endif

#ifdef OPTIM
 /* ｵﾌﾟﾃｨﾏｲｽﾞ(long->short branch)用*/
 EXTERN int *gOptStk, gOpt_sp, gOptChg, gOptCount;
 EXTERN byte gOpt_f;
#endif

/* 'if'(条件ｱｾﾝﾌﾞﾙ)の管理 */
EXTERN int	gCo_sp;
EXTERN int	gCoStk[GCO_MAX+1];
EXTERN int	gP1_sp;
EXTERN int	gP1Stk[GP1_MAX+1];

/* use for library inclusion */
EXTERN FILE *gFileStk[MAXLIB];
EXTERN int	gFile_sp;
EXTERN char gSrcFName[FNAMESZ+1];
#ifdef FILSTK2
 EXTERN int  gSrcLine;
 EXTERN struct FILSTK2_tag {
			int  srcline;
			char srcname[FNAMESZ+1];
		} gFilStk2[MAXLIB];
#endif
#ifdef INCLUDIR
 EXTERN char *gIncDirName;
#endif

#ifdef OA  /* -a ｵﾌﾟｼｮﾝ */
 typedef struct {
	 int  ll;
	 byte nn;
 } OATBL_T;
 EXTERN OATBL_T *gOAStk;
 EXTERN byte gOAchk_f;
 EXTERN int  gOA_sp;
 void oa_putStr(char *, int);
#endif

extern OPTBL_T gOpTab[];


/*-- Function --*/
#if 10
void
	none(void), load(void), load2(void),	store(void),
	ccr(void),	lea(void),	memory(void),	transfer(void),
	pshs(void), puls(void), pshu(void), 	pulu(void),
	mod(void),	emod(void), branch(void),	lbranch(void),
	equ(void),	set(void),	rmb(void),		os9svc(void),
	rzb(void),	fcb(void),	fdb(void),		library(void),
	fcc(void),	fcs(void),	org(void),		setdp(void),
	vsct(void), psct(void), csct(void), 	endsct(void),
	opt(void),	nam(void),	page(void), 	spc(void),
	endop(void);
#ifndef M6809
  void	tfm(void),	load4(void),immemory(void);
 #ifdef OPEQ
  void	opeq(void), none_wq(void);
 #endif
#endif
#ifdef OPED
  void	oped(void),	none_d(void);
#endif
#endif
