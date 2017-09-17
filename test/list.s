*
* list for os9/09
*
	ifp1
	use	$INC/defs
	endc

BUFSIZ	equ	200

	org	0
IPATH	rmb	1
PRMPTR	rmb	2
BUFFER	rmb	BUFSIZ
	rmb	200
	rmb	200
MSIZ	equ 	.
	org	0

	mod	SIZE,NAME,$11,$81,ENT,MSIZ
NAME	fcs	"List"

ENT	equ	*
	stx	<PRMPTR
	lda	#1
	os9	I$OPEN
	bcs	L50
	sta	<IPATH
	stx	<PRMPTR
L20	lda	<IPATH
	leax	BUFFER,u
	ldy	#BUFSIZ
	os9	I$READLN
	bcs	L30
	lda	#1
	os9	I$WRITLN
	bcc	L20
	bra	L50

L30	cmpb	#E$EOF
	bne	L50
	lda	IPATH
	os9	I$CLOSE
	bcs	L50
	ldx	<PRMPTR
	lda	0,x
	cmpa	#$0d
	bne	ENT
	clrb
L50	os9	F$EXIT

	emod
SIZE
	end
