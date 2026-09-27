* M6800 互換ニーモニックから生成する 6809 命令列の確認用。
* HD6301 / HD6303 の追加命令も M6800 系互換として扱う。
        xgdx
        slp
	clc
	sec
	cli
	sei
	clv
	sev
	clf
	sef
	clz
	sez
	psha
	pshb
	pula
	pulb
	pshx
	pulx
	des
	dex
	ins
	inx
	wai
	tab
	tba
	tap
	tpa
	tsx
	txs
	aba
	cba
	sba

* 6800 固有の別名も通常の 6809 命令と同じコードを生成する。
	ldaa	#$12
	ldab	#$34
	oraa	#$56
	orab	#$78
	staa	<$80
	stab	>$1234
	cpx	#$5678
	end
