* -x=vobj test. as63 -t=vasm -x=vobj -o=NAME.vo NAME.s; link: vlink -b rawbin1 -T tst_vobj.ld -o out.bin tst_vobj1.vo tst_vobj1_lib.vo [tst_vobj2.vo]
	section code,"acrx"
	xref ext1,ext2,dpx,ext3
lbl	rts
start	ldd ext1,x
	ldd [ext1,x]
	ldd [ext1]
	lda ext1,pcr
	lda [ext1,pcr]
	leax lbl,pcr
	leax ext3,pcr
	lda ext1+3,y
	ldb ext1-2,u
	std <dpx
	sta ext3
	stx dlab
	leax dlab,y
	jsr [ext2]
	jmp [dlab]
	ldy #dlab+2
	ldu #lbl-start
	cmpx #ext1
	lbsr lbl
	lbsr dlab
	lbeq ext2
	lbne start
	bra lbl
	leas lbl-start,s
	section dat,"adrw"
dlab	dw lbl,start,ext2
	db 1
