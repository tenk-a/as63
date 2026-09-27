* 6309 追加命令.
        org $8000
start
        lbra main
* ---- 16bit 累算器 w / d (0x10 前置) ----
        subw #$1234
        subw <$12
        subw 5,x
        subw >$1234
        cmpw #$1234
        cmpw <$12
        cmpw 5,x
        cmpw >$1234
        sbcd #$1234
        sbcd <$12
        sbcd 5,x
        sbcd >$1234
        andd #$1234
        andd <$12
        andd 5,x
        andd >$1234
        bitd #$1234
        bitd <$12
        bitd 5,x
        bitd >$1234
        ldw #$1234
        ldw <$12
        ldw 5,x
        ldw >$1234
        eord #$1234
        eord <$12
        eord 5,x
        eord >$1234
        adcd #$1234
        adcd <$12
        adcd 5,x
        adcd >$1234
        ord #$1234
        ord <$12
        ord 5,x
        ord >$1234
        addw #$1234
        addw <$12
        addw 5,x
        addw >$1234
        stw <$12
        stw 5,x
        stw >$1234
        ldq #$12345678
        ldq <$12
        ldq 5,x
        ldq >$1234
        stq <$12
        stq 5,x
        stq >$1234
* ---- 8bit 累算器 e / f (0x11 前置) ----
        sube #$12
        sube <$12
        sube 5,x
        sube >$1234
        cmpe #$12
        cmpe <$12
        cmpe 5,x
        cmpe >$1234
        lde #$12
        lde <$12
        lde 5,x
        lde >$1234
        adde #$12
        adde <$12
        adde 5,x
        adde >$1234
        subf #$12
        subf <$12
        subf 5,x
        subf >$1234
        cmpf #$12
        cmpf <$12
        cmpf 5,x
        cmpf >$1234
        ldf #$12
        ldf <$12
        ldf 5,x
        ldf >$1234
        addf #$12
        addf <$12
        addf 5,x
        addf >$1234
        ste <$12
        ste 5,x
        ste >$1234
        stf <$12
        stf 5,x
        stf >$1234
        divd #$12
        divd <$12
        divd 5,x
        divd >$1234
        divq #$1234
        divq <$12
        divq 5,x
        divq >$1234
        muld #$1234
        muld <$12
        muld 5,x
        muld >$1234
* ---- メモリ即値演算 ----
        oim #$12,<$34
        oim #$12,5,x
        oim #$12,>$1234
        aim #$12,<$34
        aim #$12,5,x
        aim #$12,>$1234
        eim #$12,<$34
        eim #$12,5,x
        eim #$12,>$1234
        tim #$12,<$34
        tim #$12,5,x
        tim #$12,>$1234
* ---- 単項 (d / w / e / f) ----
        negd
        comd
        lsrd
        rord
        asrd
        asld
        rold
        decd
        incd
        tstd
        clrd
        clrw
        clre
        clrf
        sexw
        comw
        lsrw
        rorw
        rolw
        decw
        incw
        tstw
        come
        dece
        ince
        tste
        comf
        decf
        incf
        tstf
* ---- レジスタ間 ----
        addr a,b
        addr x,y
        addr w,d
        addr e,f
        addr v,u
        adcr a,b
        adcr x,y
        adcr w,d
        adcr e,f
        adcr v,u
        subr a,b
        subr x,y
        subr w,d
        subr e,f
        subr v,u
        sbcr a,b
        sbcr x,y
        sbcr w,d
        sbcr e,f
        sbcr v,u
        andr a,b
        andr x,y
        andr w,d
        andr e,f
        andr v,u
        orr a,b
        orr x,y
        orr w,d
        orr e,f
        orr v,u
        eorr a,b
        eorr x,y
        eorr w,d
        eorr e,f
        eorr v,u
        cmpr a,b
        cmpr x,y
        cmpr w,d
        cmpr e,f
        cmpr v,u
        exg w,v
        exg e,f
        tfr d,w
        tfr v,x
        pshsw
        pulsw
        pshuw
        puluw



* ---- ビット操作 / 転送 ----
        tfm x+,y+
        tfm x-,y-
        tfm x+,y
        tfm x,y+
        tfm u+,s+
        bitmd #$80
* ---- 6309 索引 ----
        lda e,x
        lda f,x
        lda w,x
        lda ,w
        lda >1000,w
        lda ,w++
        lda ,--w
        lda [,w]
        lda [>1000,w]
        lda [,w++]
        lda [,--w]
main
        nop
        rts

        end
