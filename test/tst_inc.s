*[SJIS] This file is encoded in SJIS to store SJIS strings in the fcb.
	use	$INC\TST_INC.INC
	
START:
	pshs	d,x,y,u
	ldd	#10
	std	,x
	ldd	,y++
	muld	,u
	stw	,y
	lbsr	TST
	puls	d,x,y,u,pc

TST
	ldd_i	#1	error
	std_i	$ff20	error
	ldd	#"AB
	ldd	#'A*$100+'B
	rts
MSG:
	fcb	"é¿çsÇ≈Ç´Ç‹ÇπÇÒÇÊ",'!,'?,$00
	fcc	'é¿çsÇ≈Ç´Ç‹ÇπÇÒÇÊ','!','?',$00
	rmb	16
	fcb	100

* Expected errors:
*   Duplicate label definition(LBL1) (from tst_inc.inc)
*   Invalid character in expression (2 occurrences)
*   Unexpected character (4 occurrences)
