A0	equ	-1
	if defined(A0)
	bra	error
	endif

	equ	T1
	equ	T2
	equ	T3
	if defined(T1)
A1	equ	T1
	endif
	if defined(T2)
A2	set	T2
	endif
	if defined(T3)
A3	equ	T3
	endif
