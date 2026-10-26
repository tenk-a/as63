* Run with -y2. Branches to the next instruction are removed (not BSR/LBSR).
* Expected bytes: 8D 00 8D 00 20 00 4C 39
	org $1000
	bra a
a	beq b
b	lbra c
c	bne d
d	bsr e
e	lbsr f
f	brn g
g	bra *+2
	inca
	bra h
	bra h
h	lbeq i
i	rts
