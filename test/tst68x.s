* 6809 拡張命令のチェック　　as63 -8l で表示して確認
* 1
	tstd
	std  -2,s

	clrd
	clra
	clrb

	comd
	comb
	coma

	negd
	nega
	negb
	sbca #0

	lsrd
	lsra
	rorb

	rord
	rora
	rorb

	asrd
	asra
	rorb

	lsld
	lslb
	rola

	rold
	rolb
	rola

	incd
	addd #1

	decd
	subd #1


* andd,ord,eord,adcd,sbcd
* imm
	andd	#$0ff0
	andb	#$f0
	anda	#$0f

* direct,extend
* ﾀﾞｲﾚｸﾄ･ｱﾃﾞﾚｯｼﾝｸﾞになるのは $0～$feまで
	sbcd	$fe
	sbcb	$ff
	sbca	$fe

	sbcd	$ff
	sbcb	$100
	sbca	>$ff

	adcd	$100
	adcb	$101
	adca	$100

* ｵｰﾄ･ｲﾝｸﾘﾒﾝﾄ,ｵｰﾄ･ﾃﾞｸﾘﾒﾝﾄ
	andd	,-x
	andb	,x
	anda	,-x

	ord	,--x
	orb	,-x
	ora	,-x

	andd	,x+
	andb	1,x
	anda	,x+

	ord	,x++
	orb	1,x
	ora	,x++

* zero,5bit
	ord	,x
	orb	1,x
	ora	,x

	andd	-1,x
	andb	,x
	anda	-1,x

	eord	14,s
	eorb	15,s
	eora	14,s

	adcd	15,u
	adcb	16,u
	adca	15,u

	sbcd	16,y
	sbcb	17,y
	sbca	16,y

	eord	-16,s
	eorb	-15,s
	eora	-16,s

	adcd	-17,u
	adcb	-16,u
	adca	-17,u

	sbcd	-18,y
	sbcb	-17,y
	sbca	-18,y


* 8bit|16bit ｵﾌｾｯﾄ
	andd	$7e,x
	andb	$7f,x
	anda	$7e,x

	andd	$7f,x
	andb	$80,x
	anda	$7f,x

	andd	$80,x
	andb	$81,x
	anda	$80,x

	eord	-$80,s
	eorb	-$7f,s
	eora	-$80,s

	eord	-$81,s
	eorb	-$80,s
	eora	-$81,s

	eord	-$82,s
	eorb	-$81,s
	eora	-$82,s

* pc
	sbcd	$7f,pc
	sbcb	$80,pc
	sbca	$7f,pc

	adcd	$80,pc
	adcb	$81,pc
	adca	$80,pc

	eord	-$80,pc
	eorb	-$7f,pc
	eora	-$80,pc

	eord	-$81,pc
	eorb	-$80,pc
	eora	-$81,pc

* pcﾘﾗﾃｨﾌﾞ
J2	set	*
	rzb	120
	andd	J2,pcr
J1	set	*
	rzb	120
	andb	J1+1,pcr
	anda	J1,pcr

J2	set	*
	rzb	125
	andd	J2,pcr
J1	set	*
	rzb	125
	andb	J1+1,pcr
	anda	J1,pcr

J2	set	*
	rzb	127
	andd	J2,pcr
J1	set	*
	rzb	127
	andb	J1+1,pcr
	anda	J1,pcr

*　ﾚｼﾞｽﾀ･ｵﾌｾｯﾄ、ｲﾝﾀﾞｲﾚｸﾄ･ﾓｰﾄﾞは不可能
	adcd	a,x	error
	sbcd	d,y	error
	andd	[,x]	error

	eord	,w	error
	eord	e,x	error
