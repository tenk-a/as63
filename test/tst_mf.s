* as63 -a -l tst_mf.s
* as63 -a -l tst_mf.s tst_mf2.s
test0
	lbra >test3
	rts
	pshs d,x,y,u
test:
	lda  >_TST1,y
	ldb  >_TST2,y
	mul
	pshs d
	lbsr >test2
	leas 2,s
	puls d,x,y,u,pc
	
	equ used(test2)

* Expected error when assembled with tst_mf2.s:
*   Undefined label(test3)
