* -x=obj (lwasm --obj compatible) test. Link: lwlink --format=decb -o out.bin tst_obj1_lib.o tst_obj1.o
* library module
	section code
	export puts,libinit,table
	import counter
puts	pshs a,x
loop@	lda ,x+
	beq done@
	inc counter+1
	bra loop@
done@	puls a,x,pc
libinit	clr counter
	clr counter+1
	ldx #table
	rts
table	fdb puts,libinit,1,2,counter
	endsection

	section data
	export counter
counter	fdb 0
	fcb 1,2,3
	endsection
