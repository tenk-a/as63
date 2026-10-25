* -x=vobj test. as63 -t=vasm -x=vobj -o=NAME.vo NAME.s; link: vlink -b rawbin1 -T tst_vobj.ld -o out.bin tst_vobj1.vo tst_vobj1_lib.vo [tst_vobj2.vo]
	section code,"acrx"
	xdef start,cst,off4,tail
	xref ext1,ext2,dpx,ext3
cst	equ 5
off4	equ buf+4
start	bra ext1
	beq ext1
	lbsr ext2
	lda <dpx
	lda #dpx
	ldx #off4
	ldd #cst
	fdb off4,buf
	ldx #cst*2
	fdb ext1-start
	lda #start&255
	lda #start>>8
	ldb #table&255
	ldb #table>>8
	ldy #ext1+$100
	ldu #start+2
	ldd ext3-2
	jsr table
	bra dlab
	leax table,pcr
	leay ext2,pcr
	lbra ext3
	ldx #tail-start
	fdb tail,start,ext1,ext2+7
	db ext3&255
	db ext3>>8
	db cst,dpx,cst+1
tail	rts
	section dat,"adrw"
	xdef table
table	dw ext1,dlab,start+1,buf,table
dlab	db 9,8
	dw ext3+1,ext3-1
	section bss,"aurw"
buf	rmb 8
	xdef bufend
bufend	rmb 1
