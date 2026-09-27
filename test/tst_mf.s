* as63 -a -l tst_mf2.s
* as63 -l tst_mf.s tst_mf2.oa
	org $100
test0
	bsr test
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

