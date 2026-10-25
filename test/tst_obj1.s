* -x=obj (lwasm --obj compatible) test. Link: lwlink --format=decb -o out.bin tst_obj1_lib.o tst_obj1.o
* main module: references across sections and modules
	section init
	export __start
	import puts,table,counter,libinit
__start	lds #stack
	jsr libinit
	lbsr puts
	leax msg,pcr
	jsr [vec]
	ldd table+4
	ldx #table
	leay table,pcr
	lda #counter&$ff
	ldb #counter/256
	ldu #(msg-main2)/2
	ldd #counter-table
	lbra main2
	lbeq main2
	bsr local1
local1	rts
	endsection

	section code
main2	ldx #msg
	ldd ,x
	ldd 2,x
	ldd main2,y
	ldd counter,u
	stx counter
	jmp __start
	clr counter
	leax buf,pcr
	ldd #buf+10
	cmpx #stack-1
	fdb main2,__start,buf,msg+1,table-1
	fcb msg&255,msg/256,-msg
	fcc "hello"
msg	fcn "msg"
vec	fdb main2
here	fdb *,*-main2,here+2
	ldx #~counter
	ldd #counter*2
	ldd #2*counter+1
	endsection

	section bss,bss
buf	rmb 32
area	rmb 128
stack	equ *
	endsection
