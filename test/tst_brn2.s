* Run with -y2. Absolute and *-relative targets keep BRA; chained label skips all use BRN.
* Expected bytes: 20 01 12 12 20 01 4C 12 21 4C 21 4C 21 5C 39
	org $2000
	bra $2003
	nop
	nop
	bra *+3
	inca
	nop
a1	bra a2
	inca
a2	bra a3
	inca
a3	bra a4
	incb
a4	rts
