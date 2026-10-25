* -x=vobj test. as63 -t=vasm -x=vobj -o=NAME.vo NAME.s; link: vlink -b rawbin1 -T tst_vobj.ld -o out.bin tst_vobj1.vo tst_vobj1_lib.vo [tst_vobj2.vo]
	section code,"acrx"
	xdef ext1,ext2,dpx,ext3
dpx	equ $12
ext1	db 1,2,3
ext2	rts
	section dat,"adrw"
ext3	dw 7
