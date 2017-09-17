* ???_i	
	lde_i	0
	ldf_i	1
	ldw_i	2
	ldq_i	3
	ldq_i	0,3
	ldq	#-1
	ldq	#-1,0


* 合成命令（生成される命令自体がただしいかどうかはチェックしてせん^^;）
* n
	tfr	n,x
	subr	n,u

* w
	negw
	comw
	incw

	aslw
	lslw
	andcc #$fe
	rolw
	
	asrw
	pshsw
	asl ,s++
	rorw

* psh,pul
	pshs	w,d,x,y
	pshs	d,x,y
	pshsw
	
	puls	w,d,x,y,pc
	pulsw
	puls	d,x,y,pc

	pshs	w
	pshsw
	
	puls	w
	pulsw

	pshu	w,d,x,y
	pshu	d,x,y
	pshuw
	
	pulu	w,d,x,y,pc
	puluw
	pulu	d,x,y,pc

	pshu	w
	pshuw
	
	pulu	w
	puluw

*q
	tstq
	stq	-4,s
	
	clrq
	clrw
	clrd
	
	comq
	comw
	comd
	
	negq
	comd
	comw
	incw
	bne *+4
	incd

	asrq
	asrd
	rorw
	
	lsrq
	lsrd
	rorw
	
	rorq
	rord
	rorw
	
	rolq
	rolw
	rold
	
	lslq		aslq
	andcc #$fe
	rolw
	rold
	
	incq
	incw
	bne *+4
	incd
	
	decq
	tstw
	bne LL1
	decd
LL1
	decw

* addq,subq
* imm
	addq	#2
	addw	#2
	adcd	#0
	
	subq	#1,2
	subw	#2
	sbcd	#1

* direct,extend
* ﾀﾞｲﾚｸﾄ･ｱﾃﾞﾚｯｼﾝｸﾞになるのは $0～$fdまで
	addq	$fd
	addw	$ff
	adcd	$fd

	addq	$fe
	addw	$100
	adcd	>$fe

	addq	$100
	addw	$102
	adcd	$100

* ｵｰﾄ･ｲﾝｸﾘﾒﾝﾄ,ﾃﾞｸﾘﾒﾝﾄ
	addq	,-x
	addw	1,x
	adcd	,-x
	
	addq	,--y
	addw	,y
	adcd	,--y

	subq	,---u
	subw	,-u
	sbcd	,--u
	
	subq	,----u
	subw	,--u
	sbcd	,--u

	addq	,x+
	addw	2,x
	adcd	,x+
	
	addq	,x++
	addw	2,x
	adcd	,x++
	
	addq	,x+++
	addw	2,x
	adcd	,x
	leax	3,x

	addq	,x++++
	addw	2,x
	adcd	,x
	leax	4,x

* zero|5bits offset
	addq	,x
	addw	2,x
	adcd	,x
	
	addq	1,x
	addw	3,x
	adcd	1,x
	
	subq	-2,x
	subw	,x
	sbcd	-2,x
	
	subq	-3,x
	subw	-1,x
	sbcd	-3,x
	
	addq	13,x
	addw	15,x
	adcd	13,x

	addq	14,x
	addw	16,x
	adcd	14,x
	
	addq	16,x
	addw	18,x
	adcd	16,x
	
	subq	-16,x
	subw	-14,x
	sbcd	-16,x

	subq	-18,x
	subw	-16,x
	sbcd	-18,x
	
	subq	-19,y
	subw	-17,y
	sbcd	-19,y

* 8|16bits
	addq	$7d,x
	addw	$7f,x
	adcd	$7d,x
	
	addq	$7f,x
	addw	$81,x
	adcd	$7f,x
	
	addq	$80,x
	addw	$82,x
	adcd	$80,x

	subq	-$80,x
	subw	-$7e,x
	sbcd	-$80,x
	
	subq	-$82,x
	subw	-$80,x
	sbcd	-$82,x

	subq	-$83,x
	subw	-$81,x
	sbcd	-$83,x
* pc
	addq	$7d,pc
	addw	$7f,pc
	adcd	$7d,pc
	
	addq	$7f,pc
	addw	$81,pc
	adcd	$7f,pc
	
	addq	$80,pc
	addw	$82,pc
	adcd	$80,pc

	subq	-$80,pc
	subw	-$7e,pc
	sbcd	-$80,pc

	subq	-$82,pc
	subw	-$80,pc
	sbcd	-$82,pc

	subq	-$83,pc
	subw	-$81,pc
	sbcd	-$83,pc

* pcr (pcﾘﾗﾃｨﾌﾞ)
J2	rzb	121
	subq	J2,pcr
J1	rzb	121
	subw	J1+2,pcr
	sbcd	J1,pcr

J4	rzb	126
	subq	J4,pcr
J3	rzb	126
	subw	J3+2,pcr
	sbcd	J3,pcr

J6	rzb	127
	subq	J6,pcr
J5	rzb	127
	subw	J5+2,pcr
	sbcd	J5,pcr

*　ﾚｼﾞｽﾀ･ｵﾌｾｯﾄ、ｲﾝﾀﾞｲﾚｸﾄ･ﾓｰﾄﾞは不可能

