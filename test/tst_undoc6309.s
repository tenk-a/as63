* 6309 未定義動作.
* as63 -9 -mtst_undoc6309 -o tst_undoc6309.s
* 警告メッセージ
* [WARNING] Registers have different sizes.
* がでる.

	org	0
mem_size	equ	.
	org	0

F$Exit	equ	$06

	mod	mod_size,mod_name,$11,$81,entry,mem_size
mod_name:
	fcs	$modnam,2

entry:
	pshs	cc,d,x,y,u
	tfr	e,w
	tfr	w,e
* V は OS-9 が使うため、実行経路では転送元にするだけで変更しない。
	tfr	v,f
	exg	e,w
	exg	w,e

	puls	cc,d,x,y,u
	clrb
	os9	F$Exit
	bra	entry_error

* V を転送先にする形式と EXG はコード生成だけを確認する。
v_register_code:
	tfr	f,v
	exg	f,v
	exg	v,f

entry_error:
	bra	entry_error

	emod
mod_size	equ	*
	end
