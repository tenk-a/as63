        ORG     $C100

PSTRNG  EQU     $CD1E
WARMS   EQU     $CD03

HELLO   LDX     #MESSAGE
        JSR     PSTRNG
        JMP     WARMS

MESSAGE FCC     "Hello, world!"
        FCB     $04

        END     HELLO
