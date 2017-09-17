*	org,rmb の s-format(-f,-q指定)出力時とそうでないときのチェック
	org	$200
	fcb	1
	org	$220
	fcb	2
	rmb	31
	rzb	16
T1	fcb	"test fileだよ～ん",$00
T2	fcc	"test だってば",$00
T3	fcc	/test object/,$00
T4	fcs	'test object',$00
	end	
