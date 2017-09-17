* ’è”‰‰ŽZ
.	equ	1
..	equ	2
.$	equ	3
.@	equ	4
	org	0
	rmb	10
.A	equ	.+..+.$+.@	nomal:1+2+3+4=10($0a) os9mode:10+2+3+4=19($13)

	fdb	0		0
	fdb	1		1
	fdb	!0		1
	fdb	!10		0
AA	equ	*
AA2	equ	.
	fdb	1+2		3
	fdb	2-3		ffff	@
	fdb	3*4		c
	fdb	5/2		2
	fdb	5%2		1
	fdb	4>>1		2
	fdb	4<<1		8
	fdb	2>>0		2
	fdb	0>>1		0
	fdb	1&1		1
	fdb	1&2		0
	fdb	4|2		6
	fdb	2|2		2
	fdb	0^0		0
	fdb	0^$ff		ff
	fdb	1+3*2		7
	fdb	1+(3*2)		7
	fdb	(1+3)*2		8

	fdb	0==1		0
	fdb	0!=1		1
	fdb	-1==0xffff	1	@
	fdb	-1>$7fff	0
	fdb	$ffff>$7fff	0	@
	fdb	4>3		1
	fdb	4>=3		1
	fdb	4<3		0
	fdb	4<=3		0

	fdb	0&&0		0
	fdb	0&&2		0
	fdb	3&&0		0
	fdb	10&&5		1
	fdb	0||0		0
	fdb	0||3		1
	fdb	2||0		1
	fdb	10||5		1

*** set,equ
A0	equ	A2
	equ	A1
A1	set	1
	equ	A1
A1	set	2
	equ	A1
A1	set	3
	equ	A1
A2	equ	*
A2	set	*		error
A0	set	2		error

** used
	equ	defined(SUB1)	1
	equ	defined(SUB2)	0
	equ	used(SUB1)	1
	equ	used(SUB2)	0
SUB1
	fdb	defined(SUB1)	1
	fdb	defined(SUB2)	0
	fdb	used(SUB1)	1
	fdb	used(SUB2)	0

	equ used(JJJ1)		1
	equ used(JJJ2)		1
	equ used(JJJ3)		0
	equ used(JJJ4)		0

	bsr JJJ1
	bsr JJJ2		error
	rts

	equ used(JJJ1)		1
	equ used(JJJ2)		1
	equ used(JJJ3)		0
	equ used(JJJ4)		0
	
    if used(JJJ1)		1
JJJ1	rts
    endif

	equ used(JJJ1)		1
	equ used(JJJ2)		1
	equ used(JJJ3)		
	equ used(JJJ4)		

    if  defined(CHK)
CCC	equ CHK
    	bra JJJ3
    endif
    if  used(JJJ3)
	bra JJJ4		CHK‚ª’è‹`‚³‚ê‚é‚Æerror
JJJ3	rts
    endif

	equ used(JJJ1)		1
	equ used(JJJ2)		1
	equ used(JJJ3)		
	equ used(JJJ4)		
