* Expected bytes: 01 12 34 01 00 00 02
*PRAGMA noforwardrefmax
        if later
        fcb 1
        else
        fcb 2
        endif
later   equ 1
        fcb >$1234
        org $1000
        fcb 1
        org $1003
        fcb 2
        end
