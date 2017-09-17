*
* place for os9/09
*

         ifp1
         use  $INC/defsfile
EOL      equ  $0d
         endc

BUFSIZ   equ  256
ARGMAX   equ  30

         csect 0
argc     rmb  1
argptr   rmb  2
siz      rmb  2
bufptr   rmb  2
argv     rmb  ARGMAX*4
buf      rmb  BUFSIZ
         rmb  256
*MEMSIZE  rmb  0
	 endsect
MEMSIZE  equ  4096

*
         mod  ModSIZE,ModName,$11,$82,ENTRY,MEMSIZE
ModName  fcs  $modnam,2

ENTRY    equ  *
         leay buf,u
         sty  bufptr
         clr  argc
         leay argv,u
         sty  argptr

JJ1      equ  *
         clrb
         stx  ,y++

JJ2      incb
         lda  ,x+
         cmpa #EOL
         beq  RR0
         cmpa #'\
         beq  JJ3
         cmpa #'*
         bne  JJ2
         bsr  Place2

         ldd  #$ffff
         std  ,y++
         std  ,y++
         bsr  IncArgc
         bra  JJ1


JJ3      bsr  Place2
         ldb  ,x
         bsr  htoi1
         bcs  JJ1
         tfr  b,a
         ldb  1,x
         bsr  htoi1
         bcs  JJ4
         lsla
         lsla
         lsla
         lsla
         pshs b
         adda ,s+
         leax 1,x
JJ4      stx  ,y++
         sta  ,x+
         ldd  #1
         std  ,y++
         bsr  IncArgc
         bra  JJ1

Place2   decb
Place22  clra
         std  ,y++
IncArgc  inc  argc
         lda  argc
         cmpa #ARGMAX
         bhi  ERRARG
         rts

ERRARG   ldb  #215
         bra  ErrExit

RR0      bsr  Place22
RR1      ldy  #BUFSIZ
         ldx  bufptr
         clra
         os9  I$ReadLn
         bcs  Exit
         leay -1,y
         sty  siz
         ldu  argptr
         ldb  argc
RR2      ldx  ,u++
         ldy  ,u++
         beq  RR4
         bpl  RR3
         ldx  bufptr
         ldy  siz
RR3      lda  #1
         os9  I$WritLn
         bcs  Exit
RR4      decb
         bne  RR2
         bra  RR1

htoi1    subb #'0
         bcs  hh
         cmpb #9
         bls  hh2
         subb #7
         cmpb #10
         bcs  hh
         cmpb #15
         bhi  hh
hh2      andcc #$fe
         bra  hh3
hh       orcc #1
hh3      rts

Exit     equ  *
         cmpb #E$EOF
         beq  Exit1
ErrExit  coma
         fcb  $21
Exit1    clrb
Exit2    os9  F$Exit

         emod
ModSIZE  equ  *
         end
