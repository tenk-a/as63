* Run in as63 and lwasm modes without --allmp.
* Expected bytes: 01 0a 23 02 07 20 fd 07 20 fd 0f 0f b6 10 0f 00
        org $1000
args    macro
        fcb \1,{10},{35},{L10}
        endm
local   macro
here?  fcb 7
        bra here?
        endm
        args 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35 ignored
        local
        local
        fcb $0f,15
        lda target
target  fcb 0
        end
