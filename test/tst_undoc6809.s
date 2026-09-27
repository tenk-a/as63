* 6809 未定義命令 OS-9 実行テスト
* as63 -8 -9 -z -mtst_undoc6809 -o tst_undoc6809.s

	org	0
work:
	rmb	$0200
mem_size	equ	.
	org	0

F$Exit	equ	$06

	mod	mod_size,mod_name,$11,$81,entry,mem_size
mod_name:
	fcs	$modnam,2

entry:
	pshs	cc,d,x,y,u
	leax	work,u
	leay	4,x
	sty	,x
	lda	#$81
	sta	<work+4
	sta	1,y
	sta	<$80,y
	sta	>$0100,y

*
	neg	<work+4
	nega
	negb
	lsr	,y
	lsra
	lsrb
	nop
	andcc	#$ff

* メモリ命令: direct、indexed 0/auto/5/8/16 bit、A/B/D offset、indirect
	xneg	<work+4
	xneg	,y
	xneg	,y+
	xneg	,-y
	xneg	1,y
	xneg	<$80,y
	xneg	>$0100,y
	clra
	xneg	a,y
	clrb
	xneg	b,y
	ldd	#$0000
	xneg	d,y
	xneg	[,x]
	xlsr	<work+4
	xnc	1,y
	ngc	2,y
	xdec	<$10,y
	dcc	>$0100,y

* アキュムレータ命令.
	xnega
	xnegb
	xlsra
	xlsrb
	xnop
	xnca
	ngca
	xncb
	ngcb
	xdeca
	dcca
	xdecb
	dccb
	xclra
	clca
	xclrb
	clcb
	x18
	aslcc
	xandcc	#$ff

* 即値ストア命令.
	xsta	#$01
	xstb	#$02
	xstx	#$1234
	xstu	#$5678
	xsty	#$9abc
	xsts	#$def0
	flag	#$12
	flag	#$1234

* 異幅 TFR/EXG. V は未定義レジスタの代表として使用.
	tfr	a,d
	tfr	d,a
	exg	b,x
	exg	x,b
	tfr	v,a
	tfr	a,v
	exg	v,a
	exg	a,v

* $10/$11 prefix
	xlbra	after_lbra
	xnop
after_lbra:
	xaddd	#$1234
	xaddd	<work+4
	xaddd	,y
	xsta10	#$01
	xstb10	#$02
	xstx10	#$1234
	xsty10	#$9abc
	xstu10	#$5678
	xsts10	#$def0
	xaddu	#$1234
	xaddu	<work+4
	xaddu	,y
	xsta11	#$01
	xstb11	#$02
	xstx11	#$1234
	xstu11	#$5678

* OS-9 exit
	puls	cc,d,x,y,u
	clrb
	os9	F$Exit
	bra	entry_error

* コード生成のみ.
system_opcodes:
	xhcf
	xhcf15
	xhcfcd
	halt
	xres
	rst
	xswi2
	xfirq

entry_error:
	bra	entry_error

	emod
mod_size	equ	*
	end
