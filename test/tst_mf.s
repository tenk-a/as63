* as63 -a -l tst_mf.a
* as63 -a -l tst_mf.a tst_mf2.a
test0
	lbra >test3
	rts
test:
	pshs d,x,y,u
	lda  >_TST1,y
	ldb  >_TST2,y
	mul
	pshs d
	lbsr >test2
	leas 2,s
	puls d,x,y,u,pc
	
	equ used(test2)
