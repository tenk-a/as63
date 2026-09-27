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

* 68HC11 固有ニーモニックは、6809 命令列へ変換する。
        cpd #$1234
        cpd <$12
        cpd 5,x
        cpd 5,y
        cpd >$1234
        cpy #$1234
        cpy <$12
        cpy 5,x
        cpy 5,y
        cpy >$1234
        pshy
        puly
        xgdy
        iny
        dey
* ABY は B を符号なしで Y へ加算する。
        aby
        tsy
        tys
* 68HC12 / HCS12 の D スタック操作は 6809 の PSHS/PULS D と等価。
        pshd
        puld
* ビット設定・消去は 6309 では OIM/AIM、6809 では同じ結果となる命令列を生成する。
        bset <$12,$81
        bclr <$12,$81
        bset >$1234,$01
        bclr >$1234,$01
        bset 5,x,$01
        bclr 5,y,$01
* BRSET/BRCLR は A と CC を保存し、テスト結果だけで分岐する。
        brset <$12,$01,brset_ok
brset_ok
        brclr 5,x,$01,brclr_ok
brclr_ok
* HCS12 の MIN/MAX は indexed 形式を 6809 命令列へ展開する。
        maxa 5,x
        mina 5,y
        maxm 5,x
        minm 5,y
* EMULS は 6309 の MULD を使い、Y:D の 32-bit 符号付き積を作る。
        emuls
	end
