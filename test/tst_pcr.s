* Run with -p -y. Expected bytes: A6 8C 04 10 AE 8C 00 00
        lda target,pcr
        ldy target,pcr
target  fcb 0
        end
