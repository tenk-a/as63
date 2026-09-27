* 6809 公式全命令.
        org $8000
start
* ---- 累算器 (imm / dir / idx / ext) ----
        suba #$12
        suba <$12
        suba 5,x
        suba >$1234
        cmpa #$12
        cmpa <$12
        cmpa 5,x
        cmpa >$1234
        sbca #$12
        sbca <$12
        sbca 5,x
        sbca >$1234
        anda #$12
        anda <$12
        anda 5,x
        anda >$1234
        bita #$12
        bita <$12
        bita 5,x
        bita >$1234
        lda #$12
        lda <$12
        lda 5,x
        lda >$1234
        eora #$12
        eora <$12
        eora 5,x
        eora >$1234
        adca #$12
        adca <$12
        adca 5,x
        adca >$1234
        ora #$12
        ora <$12
        ora 5,x
        ora >$1234
        adda #$12
        adda <$12
        adda 5,x
        adda >$1234
        subb #$12
        subb <$12
        subb 5,x
        subb >$1234
        cmpb #$12
        cmpb <$12
        cmpb 5,x
        cmpb >$1234
        sbcb #$12
        sbcb <$12
        sbcb 5,x
        sbcb >$1234
        andb #$12
        andb <$12
        andb 5,x
        andb >$1234
        bitb #$12
        bitb <$12
        bitb 5,x
        bitb >$1234
        ldb #$12
        ldb <$12
        ldb 5,x
        ldb >$1234
        eorb #$12
        eorb <$12
        eorb 5,x
        eorb >$1234
        adcb #$12
        adcb <$12
        adcb 5,x
        adcb >$1234
        orb #$12
        orb <$12
        orb 5,x
        orb >$1234
        addb #$12
        addb <$12
        addb 5,x
        addb >$1234
        subd #$1234
        subd <$12
        subd 5,y
        subd >$1234
        addd #$1234
        addd <$12
        addd 5,y
        addd >$1234
        cmpx #$1234
        cmpx <$12
        cmpx 5,y
        cmpx >$1234
        ldx #$1234
        ldx <$12
        ldx 5,y
        ldx >$1234
        ldd #$1234
        ldd <$12
        ldd 5,y
        ldd >$1234
        ldu #$1234
        ldu <$12
        ldu 5,y
        ldu >$1234
        cmpd #$1234
        cmpd <$12
        cmpd 5,y
        cmpd >$1234
        cmpy #$1234
        cmpy <$12
        cmpy 5,y
        cmpy >$1234
        ldy #$1234
        ldy <$12
        ldy 5,y
        ldy >$1234
        lds #$1234
        lds <$12
        lds 5,y
        lds >$1234
        cmpu #$1234
        cmpu <$12
        cmpu 5,y
        cmpu >$1234
        cmps #$1234
        cmps <$12
        cmps 5,y
        cmps >$1234
        sta <$12
        sta 5,u
        sta >$1234
        stb <$12
        stb 5,u
        stb >$1234
        stx <$12
        stx 5,u
        stx >$1234
        std <$12
        std 5,u
        std >$1234
        stu <$12
        stu 5,u
        stu >$1234
        sty <$12
        sty 5,u
        sty >$1234
        sts <$12
        sts 5,u
        sts >$1234
* ---- 索引の全形 ----
        lda ,x
        lda ,x+
        lda ,x++
        lda ,-x
        lda ,--x
        lda -16,x
        lda 15,x
        lda <5,x
        lda <-100,x
        lda >5,x
        lda >1000,x
        lda a,x
        lda b,x
        lda d,x
        lda [,x++]
        lda [,--x]
        lda [,x]
        lda [<5,x]
        lda [>1000,x]
        lda [a,x]
        lda [b,x]
        lda [d,x]
        lda [$1234]
        lda ,y
        lda ,u+
        lda ,--s
        lda 5,s
        lda b,y
        lda [d,u]
        lda <near,pcr
        lda >lit,pcr
        lda [<near,pcr]
        lda [>lit,pcr]
near
* ---- メモリ RMW ----
        neg <$12
        neg 5,x
        neg >$1234
        nega
        negb
        com <$12
        com 5,x
        com >$1234
        coma
        comb
        lsr <$12
        lsr 5,x
        lsr >$1234
        lsra
        lsrb
        ror <$12
        ror 5,x
        ror >$1234
        rora
        rorb
        asr <$12
        asr 5,x
        asr >$1234
        asra
        asrb
        asl <$12
        asl 5,x
        asl >$1234
        asla
        aslb
        rol <$12
        rol 5,x
        rol >$1234
        rola
        rolb
        dec <$12
        dec 5,x
        dec >$1234
        deca
        decb
        inc <$12
        inc 5,x
        inc >$1234
        inca
        incb
        tst <$12
        tst 5,x
        tst >$1234
        tsta
        tstb
        clr <$12
        clr 5,x
        clr >$1234
        clra
        clrb
        jmp <$12
        jmp 5,x
        jmp >$1234
* ---- その他 ----
        nop
        sync
        daa
        orcc #$12
        andcc #$fe
        sex
        abx
        mul
        exg a,b
        exg d,x
        exg x,y
        exg u,s
        exg cc,dp
        exg pc,x
        tfr a,b
        tfr d,x
        tfr x,y
        tfr u,s
        tfr cc,dp
        tfr x,pc
        leax 5,x
        leay <$40,y
        leas -2,s
        leau [,u]
        pshs a
        pshs a,b,x
        pshs cc,a,b,dp,x,y,u,pc
        puls a,b
        puls pc
        pshu a,b,y,s
        pulu cc,a,b,dp,x,y,s,pc
        lbra main
* ---- 分岐 (前へ) ----
main
        bra fwd0
fwd0
        brn fwd1
fwd1
        bhi fwd2
fwd2
        bls fwd3
fwd3
        bcc fwd4
fwd4
        bcs fwd5
fwd5
        bne fwd6
fwd6
        beq fwd7
fwd7
        bvc fwd8
fwd8
        bvs fwd9
fwd9
        bpl fwd10
fwd10
        bmi fwd11
fwd11
        bge fwd12
fwd12
        blt fwd13
fwd13
        bgt fwd14
fwd14
        ble fwd15
fwd15
        lbra fwd16
fwd16
        lbrn fwd17
fwd17
        lbhi fwd18
fwd18
        lbls fwd19
fwd19
        lbcc fwd20
fwd20
        lbcs fwd21
fwd21
        lbne fwd22
fwd22
        lbeq fwd23
fwd23
        lbvc fwd24
fwd24
        lbvs fwd25
fwd25
        lbpl fwd26
fwd26
        lbmi fwd27
fwd27
        lbge fwd28
fwd28
        lblt fwd29
fwd29
        lbgt fwd30
fwd30
        lble fwd31
fwd31
        bsr sub1
        lbsr sub1
        jsr <$12
        jsr 5,x
        jsr >sub1
        jsr [>lit,pcr]
        rts
sub1
        rts

* ---- 生成確認 (実行しない) ----
        rti
        cwai #$ef
        swi
        swi2
        swi3
lit
        dc.b 1,2,3,4
        end
