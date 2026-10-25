* -x=obj (lwasm --obj compatible) test. Link: lwlink --format=decb -o out.bin tst_obj2.o tst_obj2_lib.o
	section code
	export ext1,ext2,dpvar
	import start
ext1	fdb start,start
ext2	jmp start
	endsection
	section dp,bss
dpvar	rmb 1
	endsection
