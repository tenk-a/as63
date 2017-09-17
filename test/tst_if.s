*	èåè±æ›ÃﬁŸ
	if	1
		equ	1
	else
		equ -1
	endif
*
	if	1
		equ	1
	elsif 1
		equ -1
	elsif 0
		equ -1
	else
		equ -1
	endif
*
	if	0
		equ	-1
	elsif 1
		equ 1
	elsif 0
		equ -1
	else
		equ -1
	endif
* ==
	ifne	1
		equ 1
	endc
	ifne	0
		equ 1
	endc
	ifne	-1
		equ 1
	endc
* !=
	ifeq	1
		equ 1
	endc
	ifeq	0
		equ 1
	endc
	ifeq	-1
		equ 1
	endc

* <=
	ifle	1
		equ 1
	endc
	ifle	0
		equ 1
	endc
	ifle	-1
		equ 1
	endc
* <
	iflt	1
		equ 1
	endc
	iflt	0
		equ 1
	endc
	iflt	-1
		equ 1
	endc
* >=
	ifge	1
		equ 1
	endc
	ifge	0
		equ 1
	endc
	ifge	-1
		equ 1
	endc
* >
	ifgt	1
		equ 1
	endc
	ifgt	0
		equ 1
	endc
	ifgt	-1
		equ 1
	endc



