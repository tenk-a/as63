* Run with -y2. Skips of exactly one byte by label become one-byte BRN ($21),
* and a branch to the next instruction is removed.
* Expected bytes: 21 4C 12 20 02 4A 12 27 01 5C 21 5C 20 02 C6 01 20 01 12 20 02 12
*                 20 02 12 12 39 (-y1 gives 20 01 for the two BRN forms and 20 00 before the RTS)
	org $1000
s1	bra l1
	inca
l1	nop
s2	bra l2
	deca
	nop
l2	beq l3
	incb
l3	lbra l4
	incb
l4	bra l5
	ldb #1
l5	bra *+3
	nop
s6	bra $1017
	nop
	bra l6
	nop
	nop
l6	bra l7
l7	rts
