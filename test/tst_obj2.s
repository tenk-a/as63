* -x=obj (lwasm --obj compatible) test. Link: lwlink --format=decb -o out.bin tst_obj2.o tst_obj2_lib.o
	section code
	export start,tabend
	import ext1,ext2,dpvar
start	lda <dpvar
	sta <dpvar
	ldd ext1,pcr
	ldx [ext2,pcr]
	jmp [ext1]
	leax ext1,x
	ldy ext2,s
	ldq #$12345678
	aim #$fe,ext1
	oim #$01,start
	eim #$80,ptr
	tim #$40,<dpvar
	ldx #ptr
	ldd ptr
	ldw #ext1
	addr x,y
	muld #ext2
	fdb ext1-ext2,ext2-start,start-ext1
	bsr fwd
	lbsr fwd
	lbra far
	ldd #(*-start)
	fcb 1,2
fwd	rts
	endsection

	section data
ptr	equ buf+4
msg	fcc "abc"
	rmb 3
	fcb 9
	zmb 5
	fdb msg
	endsection

	section code
far	ldx #msg
	align 4
	nop
tabend	equ *
	rts
	endsection

	section bss,bss
buf	rmb 16
	endsection
	section tail
	fdb tabend,ptr
