* Run in as63 and vasm modes.
* Expected bytes: 11 22 33 44 55 66 a6 05 c6 12 61 20 62
emit3   macro
        fcb \1,\2,\3
        endm
load    macro
        lda \1
        ldb \2
        endm
text    macro
        fcc \1
        endm
        emit3 $11,$22,$33
        emit3 <$44>,<$55>,<$66>
        load <5,x>,#$12
        text <"a b">
        end
