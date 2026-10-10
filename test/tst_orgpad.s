* --orgpad: AA 00 00 BB; --no-orgpad: AA BB.
* Nothing is emitted before the first ORG.
        org $1000
        fcb $aa
        org $1003
        fcb $bb
        end $1000
