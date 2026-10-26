* Run with -y2 under every -t mode. Forward references are optimized in all modes.
* Expected bytes: 96 40 8E 10 14 21 12 12 30 05 20 07 31 8C 03 DC 30 12 12 39 12
	org $1000
	setdp 0
	lda dpv
	ldx #fwd1
	bra near
	nop
near	nop
	leax small,x
	lbra later
	leay near2,pcr
	ldd fwdsmall
	nop
near2	nop
later	rts
dpv	equ $40
small	equ 5
fwdsmall equ $30
fwd1	nop
