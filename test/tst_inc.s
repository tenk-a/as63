	use	$INC\TST_INC.INC
	
START:
	pshs	d,x,y,u
	ldd	#10
	std	,x
	ldd	,y++
	muld	,u
	stw	,y
	lbsr	TST
	puls	d,x,y,u,pc

TST
	ldd_i	#1	error
	std_i	$ff20	error
	ldd	#"AB
	ldd	#'A*$100+'B
	rts
MSG:
	fcb	"Às‚Å‚«‚Ü‚¹‚ñ‚æ",'!,'?,$00
	fcc	'Às‚Å‚«‚Ü‚¹‚ñ‚æ','!','?',$00
	rmb	16
	fcb	100
