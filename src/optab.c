#include <stdio.h>
#include <string.h>
#include "as63.h"

OPTBL_T const gOpTab[] = {
    /* abx */
    { "ABX",      0x00,       0x3a,    0x00,           none        },
    /* adc */
    { "ADCA",     0x00,       0x89,    0x00,           load        },
    { "ADCB",     0x00,       0xc9,    0x00,           load        },
    /* add */
    { "ADDA",     0x00,       0x8b,    0x00,           load        },
    { "ADDB",     0x00,       0xcb,    0x00,           load        },
    { "ADDD",     0x00,       0xc3,    0x00,           load2       },
    /* and */
    { "ANDA",     0x00,       0x84,    0x00,           load        },
    { "ANDB",     0x00,       0xc4,    0x00,           load        },
    { "ANDC",     0x00,       0x1c,    0x00,           ccr         },
    { "ANDCC",    0x00,       0x1c,    0x00,           ccr         },
    /* asl */
    { "ASL",      0x00,       0x08,    0x00,           memory      },
    { "ASLA",     0x00,       0x48,    0x00,           none        },
    { "ASLB",     0x00,       0x58,    0x00,           none        },
    /* asr */
    { "ASR",      0x00,       0x07,    0x00,           memory      },
    { "ASRA",     0x00,       0x47,    0x00,           none        },
    { "ASRB",     0x00,       0x57,    0x00,           none        },
    /* br */
    { "BCC",      0x00,       0x24,    0x00,           branch      },
    { "BCS",      0x00,       0x25,    0x00,           branch      },
    { "BEQ",      0x00,       0x27,    0x00,           branch      },
    { "BGE",      0x00,       0x2c,    0x00,           branch      },
    { "BGT",      0x00,       0x2e,    0x00,           branch      },
    { "BHI",      0x00,       0x22,    0x00,           branch      },
    { "BHS",      0x00,       0x24,    0x00,           branch      },
    { "BLE",      0x00,       0x2f,    0x00,           branch      },
    { "BLO",      0x00,       0x25,    0x00,           branch      },
    { "BLS",      0x00,       0x23,    0x00,           branch      },
    { "BLT",      0x00,       0x2d,    0x00,           branch      },
    { "BMI",      0x00,       0x2b,    0x00,           branch      },
    { "BNE",      0x00,       0x26,    0x00,           branch      },
    { "BPL",      0x00,       0x2a,    0x00,           branch      },
    { "BRA",      0x00,       0x20,    0x00,           branch      },
    { "BRN",      0x00,       0x21,    0x00,           branch      },
    { "BSR",      0x00,       0x8d,    0x00,           branch      },
    { "BVC",      0x00,       0x28,    0x00,           branch      },
    { "BVS",      0x00,       0x29,    0x00,           branch      },
    /* bit */
    { "BITA",     0x00,       0x85,    0x00,           load        },
    { "BITB",     0x00,       0xc5,    0x00,           load        },
    /* clr */
    { "CLR",      0x00,       0x0f,    0x00,           memory      },
    { "CLRA",     0x00,       0x4f,    0x00,           none        },
    { "CLRB",     0x00,       0x5f,    0x00,           none        },
    /* cmp */
    { "CMPA",     0x00,       0x81,    0x00,           load        },
    { "CMPB",     0x00,       0xc1,    0x00,           load        },
    { "CMPD",     0x10,       0x83,    0x00,           load2       },
    { "CMPS",     0x11,       0x8c,    0x00,           load2       },
    { "CMPU",     0x11,       0x83,    0x00,           load2       },
    { "CMPX",     0x00,       0x8c,    0x00,           load2       },
    { "CMPY",     0x10,       0x8c,    0x00,           load2       },
    /* com */
    { "COM",      0x00,       0x03,    0x00,           memory      },
    { "COMA",     0x00,       0x43,    0x00,           none        },
    { "COMB",     0x00,       0x53,    0x00,           none        },
    /* cwai */
    { "CWAI",     0x00,       0x3c,    0x00,           ccr         },
    /* daa */
    { "DAA",      0x00,       0x19,    0x00,           none        },
    /* dec */
    { "DEC",      0x00,       0x0a,    0x00,           memory      },
    { "DECA",     0x00,       0x4a,    0x00,           none        },
    { "DECB",     0x00,       0x5a,    0x00,           none        },
    /* eor */
    { "EORA",     0x00,       0x88,    0x00,           load        },
    { "EORB",     0x00,       0xc8,    0x00,           load        },
    /* exg */
    { "EXG",      0x00,       0x1e,    0x00,           transfer    },
    /* inc */
    { "INC",      0x00,       0x0c,    0x00,           memory      },
    { "INCA",     0x00,       0x4c,    0x00,           none        },
    { "INCB",     0x00,       0x5c,    0x00,           none        },
    /* jmp */
    { "JMP",      0x00,       0x0e,    0x00,           memory      },
    { "JSR",      0x00,       0x8d,    0x00,           store       },
    /* lb** */
    { "LBCC",     0x10,       0x24,    0x00,           lbranch     },
    { "LBCS",     0x10,       0x25,    0x00,           lbranch     },
    { "LBEQ",     0x10,       0x27,    0x00,           lbranch     },
    { "LBGE",     0x10,       0x2c,    0x00,           lbranch     },
    { "LBGT",     0x10,       0x2e,    0x00,           lbranch     },
    { "LBHI",     0x10,       0x22,    0x00,           lbranch     },
    { "LBHS",     0x10,       0x24,    0x00,           lbranch     },
    { "LBLE",     0x10,       0x2f,    0x00,           lbranch     },
    { "LBLO",     0x10,       0x25,    0x00,           lbranch     },
    { "LBLS",     0x10,       0x23,    0x00,           lbranch     },
    { "LBLT",     0x10,       0x2d,    0x00,           lbranch     },
    { "LBMI",     0x10,       0x2b,    0x00,           lbranch     },
    { "LBNE",     0x10,       0x26,    0x00,           lbranch     },
    { "LBPL",     0x10,       0x2a,    0x00,           lbranch     },
    { "LBRA",     0x00,       0x16,    0x00,           lbranch     },
    { "LBRN",     0x10,       0x21,    0x00,           lbranch     },
    { "LBSR",     0x00,       0x17,    0x00,           lbranch     },
    { "LBVC",     0x10,       0x28,    0x00,           lbranch     },
    { "LBVS",     0x10,       0x29,    0x00,           lbranch     },
    /* ld */
    { "LDA",      0x00,       0x86,    0x00,           load        },
    { "LDB",      0x00,       0xc6,    0x00,           load        },
    { "LDD",      0x00,       0xcc,    0x00,           load2       },
    { "LDS",      0x10,       0xce,    0x00,           load2       },
    { "LDU",      0x00,       0xce,    0x00,           load2       },
    { "LDX",      0x00,       0x8e,    0x00,           load2       },
    { "LDY",      0x10,       0x8e,    0x00,           load2       },
    /* lea */
    { "LEAS",     0x00,       0x32,    0x00,           lea         },
    { "LEAU",     0x00,       0x33,    0x00,           lea         },
    { "LEAX",     0x00,       0x30,    0x00,           lea         },
    { "LEAY",     0x00,       0x31,    0x00,           lea         },
    /* lsl */
    { "LSL",      0x00,       0x08,    0x00,           memory      },
    { "LSLA",     0x00,       0x48,    0x00,           none        },
    { "LSLB",     0x00,       0x58,    0x00,           none        },
    /* lsr */
    { "LSR",      0x00,       0x04,    0x00,           memory      },
    { "LSRA",     0x00,       0x44,    0x00,           none        },
    { "LSRB",     0x00,       0x54,    0x00,           none        },
    /* mul */
    { "MUL",      0x00,       0x3d,    0x00,           none        },
    /* neg */
    { "NEG",      0x00,       0x00,    0x00,           memory      },
    { "NEGA",     0x00,       0x40,    0x00,           none        },
    { "NEGB",     0x00,       0x50,    0x00,           none        },
    /* nop */
    { "NOP",      0x00,       0x12,    0x00,           none        },
    /* or */
    { "ORA",      0x00,       0x8a,    0x00,           load        },
    { "ORB",      0x00,       0xca,    0x00,           load        },
    { "ORCC",     0x00,       0x1a,    0x00,           ccr         },
    /* psh,pul */
    { "PSHS",     0x00,       0x34,    0x00,           pshs        },
    { "PSHU",     0x00,       0x36,    0x00,           pshu        },
    { "PULS",     0x00,       0x35,    0x00,           puls        },
    { "PULU",     0x00,       0x37,    0x00,           pulu        },
    /* rol */
    { "ROL",      0x00,       0x09,    0x00,           memory      },
    { "ROLA",     0x00,       0x49,    0x00,           none        },
    { "ROLB",     0x00,       0x59,    0x00,           none        },
    /* ror */
    { "ROR",      0x00,       0x06,    0x00,           memory      },
    { "RORA",     0x00,       0x46,    0x00,           none        },
    { "RORB",     0x00,       0x56,    0x00,           none        },
    /* rts */
    { "RTI",      0x00,       0x3b,    0x00,           none        },
    { "RTS",      0x00,       0x39,    0x00,           none        },
    /* sbc */
    { "SBCA",     0x00,       0x82,    0x00,           load        },
    { "SBCB",     0x00,       0xc2,    0x00,           load        },
    /* sex */
    { "SEX",      0x00,       0x1d,    0x00,           none        },
    /* st */
    { "STA",      0x00,       0x87,    0x00,           store       },
    { "STB",      0x00,       0xc7,    0x00,           store       },
    { "STD",      0x00,       0xcd,    0x00,           store       },
    { "STS",      0x10,       0xcf,    0x00,           store       },
    { "STU",      0x00,       0xcf,    0x00,           store       },
    { "STX",      0x00,       0x8f,    0x00,           store       },
    { "STY",      0x10,       0x8f,    0x00,           store       },
    /* sub */
    { "SUBA",     0x00,       0x80,    0x00,           load        },
    { "SUBB",     0x00,       0xc0,    0x00,           load        },
    { "SUBD",     0x00,       0x83,    0x00,           load2       },
    /* swi */
    { "SWI",      0x00,       0x3f,    0x00,           none        },
    { "SWI2",     0x10,       0x3f,    0x00,           none        },
    { "SWI3",     0x11,       0x3f,    0x00,           none        },
    { "SYNC",     0x00,       0x13,    0x00,           none        },
    /* tfr */
    { "TFR",      0x00,       0x1f,    0x00,           transfer    },
    /* tst */
    { "TST",      0x00,       0x0d,    0x00,           memory      },
    { "TSTA",     0x00,       0x4d,    0x00,           none        },
    { "TSTB",     0x00,       0x5d,    0x00,           none        },

    /* undocumented 6809 opcodes (-8 -z) */
    { "XNEG",     0x00,       0x01,    OPR_UNDOC6809,  memory      },
    { "XNEGA",    0x00,       0x41,    OPR_UNDOC6809,  none        },
    { "XNEGB",    0x00,       0x51,    OPR_UNDOC6809,  none        },
    { "XLSR",     0x00,       0x05,    OPR_UNDOC6809,  memory      },
    { "XLSRA",    0x00,       0x45,    OPR_UNDOC6809,  none        },
    { "XLSRB",    0x00,       0x55,    OPR_UNDOC6809,  none        },
    { "XNOP",     0x00,       0x1b,    OPR_UNDOC6809,  none        },
    { "XNC",      0x00,       0x02,    OPR_UNDOC6809,  memory      },
    { "NGC",      0x00,       0x02,    OPR_UNDOC6809,  memory      },
    { "XNCA",     0x00,       0x42,    OPR_UNDOC6809,  none        },
    { "NGCA",     0x00,       0x42,    OPR_UNDOC6809,  none        },
    { "XNCB",     0x00,       0x52,    OPR_UNDOC6809,  none        },
    { "NGCB",     0x00,       0x52,    OPR_UNDOC6809,  none        },
    { "XDEC",     0x00,       0x0b,    OPR_UNDOC6809,  memory      },
    { "DCC",      0x00,       0x0b,    OPR_UNDOC6809,  memory      },
    { "XDECA",    0x00,       0x4b,    OPR_UNDOC6809,  none        },
    { "DCCA",     0x00,       0x4b,    OPR_UNDOC6809,  none        },
    { "XDECB",    0x00,       0x5b,    OPR_UNDOC6809,  none        },
    { "DCCB",     0x00,       0x5b,    OPR_UNDOC6809,  none        },
    { "XCLRA",    0x00,       0x4e,    OPR_UNDOC6809,  none        },
    { "CLCA",     0x00,       0x4e,    OPR_UNDOC6809,  none        },
    { "XCLRB",    0x00,       0x5e,    OPR_UNDOC6809,  none        },
    { "CLCB",     0x00,       0x5e,    OPR_UNDOC6809,  none        },
    { "X18",      0x00,       0x18,    OPR_UNDOC6809,  none        },
    { "ASLCC",    0x00,       0x18,    OPR_UNDOC6809,  none        },
    { "XHCF",     0x00,       0x14,    OPR_UNDOC6809,  none        },
    { "XHCF15",   0x00,       0x15,    OPR_UNDOC6809,  none        },
    { "XHCFCD",   0x00,       0xcd,    OPR_UNDOC6809,  none        },
    { "HALT",     0x00,       0x14,    OPR_UNDOC6809,  none        },
    { "XANDCC",   0x00,       0x38,    OPR_UNDOC6809,  ccr         },
    { "XRES",     0x00,       0x3e,    OPR_UNDOC6809,  none        },
    { "RST",      0x00,       0x3e,    OPR_UNDOC6809,  none        },
    { "XSTA",     0x00,       0x87,    OPR_UNDOC6809,  undoc_imm8  },
    { "XSTB",     0x00,       0xc7,    OPR_UNDOC6809,  undoc_imm8  },
    { "XSTX",     0x00,       0x8f,    OPR_UNDOC6809,  undoc_imm16 },
    { "XSTU",     0x00,       0xcf,    OPR_UNDOC6809,  undoc_imm16 },
    { "XSTY",     0x10,       0x8f,    OPR_UNDOC6809,  undoc_imm16 },
    { "XSTS",     0x10,       0xcf,    OPR_UNDOC6809,  undoc_imm16 },
    { "FLAG",     0x00,       0x87,    OPR_UNDOC6809,  undoc_flag  },
    { "XLBRA",    0x10,       0x20,    OPR_UNDOC6809,  lbranch     },
    { "XSWI2",    0x10,       0x3e,    OPR_UNDOC6809,  none        },
    { "XADDD",    0x10,       0xc3,    OPR_UNDOC6809,  load2       },
    { "XSTA10",   0x10,       0x87,    OPR_UNDOC6809,  undoc_imm8  },
    { "XSTB10",   0x10,       0xc7,    OPR_UNDOC6809,  undoc_imm8  },
    { "XSTX10",   0x10,       0x8f,    OPR_UNDOC6809,  undoc_imm16 },
    { "XSTY10",   0x10,       0x8f,    OPR_UNDOC6809,  undoc_imm16 },
    { "XSTU10",   0x10,       0xcf,    OPR_UNDOC6809,  undoc_imm16 },
    { "XSTS10",   0x10,       0xcf,    OPR_UNDOC6809,  undoc_imm16 },
    { "XFIRQ",    0x11,       0x3e,    OPR_UNDOC6809,  none        },
    { "XADDU",    0x11,       0xc3,    OPR_UNDOC6809,  load2       },
    { "XSTA11",   0x11,       0x87,    OPR_UNDOC6809,  undoc_imm8  },
    { "XSTB11",   0x11,       0xc7,    OPR_UNDOC6809,  undoc_imm8  },
    { "XSTX11",   0x11,       0x8f,    OPR_UNDOC6809,  undoc_imm16 },
    { "XSTU11",   0x11,       0xcf,    OPR_UNDOC6809,  undoc_imm16 },
    { "ADCD",     0x10,       0x89,    0x00,           oped        },
    { "ANDD",     0x10,       0x84,    0x00,           oped        },
    { "EORD",     0x10,       0x88,    0x00,           oped        },
    { "ORD",      0x10,       0x8a,    0x00,           oped        },
    { "SBCD",     0x10,       0x82,    0x00,           oped        },
    { "ASLD",     0x10,       0x48,    0x00,           none_d      },
    { "ASRD",     0x10,       0x47,    0x00,           none_d      },
    { "CLRD",     0x10,       0x4f,    0x00,           none_d      },
    { "COMD",     0x10,       0x43,    0x00,           none_d      },
    { "DECD",     0x10,       0x4a,    0x00,           none_d      },
    { "INCD",     0x10,       0x4c,    0x00,           none_d      },
    { "LSLD",     0x10,       0x48,    0x00,           none_d      },
    { "LSRD",     0x10,       0x44,    0x00,           none_d      },
    { "NEGD",     0x10,       0x40,    0x00,           none_d      },
    { "ROLD",     0x10,       0x49,    0x00,           none_d      },
    { "RORD",     0x10,       0x46,    0x00,           none_d      },
    { "TSTD",     0x10,       0x4d,    0x00,           none_d      },

 /* HD6309         */
    { "MULD",     0x11,       0x8f,    0x01,           load2       },
    { "BITD",     0x10,       0x85,    0x01,           load2       },
    { "DIVD",     0x11,       0x8d,    0x01,           load        },
    /* W */
    { "ADDW",     0x10,       0x8b,    0x01,           load2       },
    { "CMPW",     0x10,       0x81,    0x01,           load2       },
    { "LDW",      0x10,       0x86,    0x01,           load2       },
    { "SUBW",     0x10,       0x80,    0x01,           load2       },
    { "STW",      0x10,       0x87,    0x01,           store       },
    { "CLRW",     0x10,       0x5f,    0x01,           none        },
    { "COMW",     0x10,       0x53,    0x01,           none        },
    { "DECW",     0x10,       0x5a,    0x01,           none        },
    { "INCW",     0x10,       0x5c,    0x01,           none        },
    { "LSRW",     0x10,       0x54,    0x01,           none        },
    { "PSHSW",    0x10,       0x38,    0x01,           none        },
    { "PSHUW",    0x10,       0x3a,    0x01,           none        },
    { "PULSW",    0x10,       0x39,    0x01,           none        },
    { "PULUW",    0x10,       0x3b,    0x01,           none        },
    { "ROLW",     0x10,       0x59,    0x01,           none        },
    { "RORW",     0x10,       0x56,    0x01,           none        },
    { "SEXW",     0x00,       0x14,    0x01,           none        },
    { "TSTW",     0x10,       0x5d,    0x01,           none        },
    { "ASLW",     WQ_LSLW,    4,       0x01,           none_wq     },
    { "ASRW",     WQ_ASRW,    6,       0x01,           none_wq     },
    { "LSLW",     WQ_LSLW,    4,       0x01,           none_wq     },
    { "NEGW",     WQ_NEGW,    4,       0x01,           none_wq     },
    /* r,r */
    { "ADCR",     0x10,       0x31,    0x01,           transfer    },
    { "ADDR",     0x10,       0x30,    0x01,           transfer    },
    { "ANDR",     0x10,       0x34,    0x01,           transfer    },
    { "CMPR",     0x10,       0x37,    0x01,           transfer    },
    { "EORR",     0x10,       0x36,    0x01,           transfer    },
    { "ORR",      0x10,       0x35,    0x01,           transfer    },
    { "SBCR",     0x10,       0x33,    0x01,           transfer    },
    { "SUBR",     0x10,       0x32,    0x01,           transfer    },
    { "TFM",      0x11,       0x38,    0x01,           tfm         },
    /* imm */
    { "AIM",      0x00,       0x02,    0x01,           immemory    },
    { "BIM",      0x00,       0x0B,    0x01,           immemory    },
    { "TIM",      0x00,       0x0B,    0x01,           immemory    },
    { "EIM",      0x00,       0x05,    0x01,           immemory    },
    { "OIM",      0x00,       0x01,    0x01,           immemory    },
    /* DP bit operations: reg,src_bit,dst_bit,address. */
    { "BAND",     0x11,       0x30,    0x01,           bitTransfer },
    { "BIAND",    0x11,       0x31,    0x01,           bitTransfer },
    { "BOR",      0x11,       0x32,    0x01,           bitTransfer },
    { "BIOR",     0x11,       0x33,    0x01,           bitTransfer },
    { "BEOR",     0x11,       0x34,    0x01,           bitTransfer },
    { "BIEOR",    0x11,       0x35,    0x01,           bitTransfer },
    { "LDBT",     0x11,       0x36,    0x01,           bitTransfer },
    { "STBT",     0x11,       0x37,    0x01,           bitTransfer },
    /* E */
    { "ADDE",     0x11,       0x8b,    0x01,           load        },
    { "CLRE",     0x11,       0x4f,    0x01,           none        },
    { "CMPE",     0x11,       0x81,    0x01,           load        },
    { "COME",     0x11,       0x43,    0x01,           none        },
    { "DECE",     0x11,       0x4a,    0x01,           none        },
    { "INCE",     0x11,       0x4c,    0x01,           none        },
    { "LDE",      0x11,       0x86,    0x01,           load        },
    { "STE",      0x11,       0x87,    0x01,           store       },
    { "SUBE",     0x11,       0x80,    0x01,           load        },
    { "TSTE",     0x11,       0x4d,    0x01,           none        },
    /* F */
    { "ADDF",     0x11,       0xcb,    0x01,           load        },
    { "CLRF",     0x11,       0x5f,    0x01,           none        },
    { "CMPF",     0x11,       0xc1,    0x01,           load        },
    { "COMF",     0x11,       0x53,    0x01,           none        },
    { "DECF",     0x11,       0x5a,    0x01,           none        },
    { "INCF",     0x11,       0x5c,    0x01,           none        },
    { "LDF",      0x11,       0xc6,    0x01,           load        },
    { "STF",      0x11,       0xc7,    0x01,           store       },
    { "SUBF",     0x11,       0xc0,    0x01,           load        },
    { "TSTF",     0x11,       0x5d,    0x01,           none        },
    /*Q*/
    { "DIVQ",     0x11,       0x8e,    0x01,           load2       },
    { "LDQ",      0x10,       0xcc,    0x01,           load4       },
    { "STQ",      0x10,       0xcd,    0x01,           store       },
    { "ASLQ",     WQ_LSLQ,    6,       0x01,           none_wq     },
    { "ASRQ",     WQ_ASRQ,    4,       0x01,           none_wq     },
    { "CLRQ",     WQ_CLRQ,    4,       0x01,           none_wq     },
    { "COMQ",     WQ_COMQ,    4,       0x01,           none_wq     },
    { "DECQ",     WQ_DECQ,    8,       0x01,           none_wq     },
    { "INCQ",     WQ_INCQ,    6,       0x01,           none_wq     },
    { "LSLQ",     WQ_LSLQ,    6,       0x01,           none_wq     },
    { "LSRQ",     WQ_LSRQ,    4,       0x01,           none_wq     },
    { "NEGQ",     WQ_NEGQ,    10,      0x01,           none_wq     },
    { "ROLQ",     WQ_ROLQ,    4,       0x01,           none_wq     },
    { "RORQ",     WQ_RORQ,    4,       0x01,           none_wq     },
    { "TSTQ",     WQ_TSTQ,    3,       0x01,           none_wq     },
    { "ADDQ",     0x89,       0x8b,    0x01,           opeq        },
    { "SUBQ",     0x82,       0x80,    0x01,           opeq        },
    /* md */
    { "BITMD",    0x11,       0x3c,    0x01,           ccr         },
    { "LDMD",     0x11,       0x3d,    0x01,           ccr         },

    /* M6800 aliases with an identical 6809 encoding. */
    { "CPX",      0x00,       0x8c,    OPR_M6800,      load2       },  /* cmpx */
    { "LDAA",     0x00,       0x86,    OPR_M6800,      load        },  /* lda */
    { "LDAB",     0x00,       0xc6,    OPR_M6800,      load        },  /* ldb */
    { "ORAA",     0x00,       0x8a,    OPR_M6800,      load        },  /* ora */
    { "ORAB",     0x00,       0xca,    OPR_M6800,      load        },  /* orb */
    { "STAA",     0x00,       0x87,    OPR_M6800,      store       },  /* sta */
    { "STAB",     0x00,       0xc7,    OPR_M6800,      store       },  /* stb */
    /* M6800 mnemonics translated to 6809 sequences. */
    { "CLC",      0,          0,       0,              mnm6800     },  /* andcc #$fe */
    { "SEC",      0,          1,       0,              mnm6800     },  /* orcc #$01 */
    { "CLI",      0,          2,       0,              mnm6800     },  /* andcc #$ef */
    { "SEI",      0,          3,       0,              mnm6800     },  /* orcc #$10 */
    { "CLV",      0,          4,       0,              mnm6800     },  /* andcc #$fd */
    { "SEV",      0,          5,       0,              mnm6800     },  /* orcc #$02 */
    { "CLF",      0,          6,       0,              mnm6800     },  /* andcc #$bf */
    { "SEF",      0,          7,       0,              mnm6800     },  /* orcc #$40 */
    { "CLZ",      0,          8,       0,              mnm6800     },  /* andcc #$fb */
    { "SEZ",      0,          9,       0,              mnm6800     },  /* orcc #$04 */
    { "PSHA",     0,          10,      0,              mnm6800     },  /* pshs a */
    { "PSHB",     0,          11,      0,              mnm6800     },  /* pshs b */
    { "PULA",     0,          12,      0,              mnm6800     },  /* puls a */
    { "PULB",     0,          13,      0,              mnm6800     },  /* puls b */
    { "PSHX",     0,          14,      0,              mnm6800     },  /* pshs x */
    { "PULX",     0,          15,      0,              mnm6800     },  /* puls x */
    { "DES",      0,          16,      0,              mnm6800     },  /* leas -1,s */
    { "DEX",      0,          17,      0,              mnm6800     },  /* leax -1,x */
    { "INS",      0,          18,      0,              mnm6800     },  /* leas 1,s */
    { "INX",      0,          19,      0,              mnm6800     },  /* leax 1,x */
    { "WAI",      0,          20,      0,              mnm6800     },  /* cwai #$ff */
    { "TAB",      0,          21,      0,              mnm6800     },  /* tfr a,b;tsta */
    { "TBA",      0,          22,      0,              mnm6800     },  /* tfr b,a;tsta */
    { "TAP",      0,          23,      0,              mnm6800     },  /* tfr a,cc */
    { "TPA",      0,          24,      0,              mnm6800     },  /* tfr cc,a */
    { "TSX",      0,          25,      0,              mnm6800     },  /* tfr s,x */
    { "TXS",      0,          26,      0,              mnm6800     },  /* tfr x,s */
    { "ABA",      0,          27,      0,              mnm6800     },  /* 6309: addr b,a; 6809: pshs b; adda ,s+ */
    { "CBA",      0,          28,      0,              mnm6800     },  /* 6309: cmpr b,a; 6809: pshs b; cmpa ,s+ */
    { "SBA",      0,          29,      0,              mnm6800     },  /* 6309: subr b,a; 6809: pshs b; suba ,s+ */
    /* HD6301/HD6303 compatibility */
    { "XGDX",     0x1e,       0x01,    0x01,           none        },  /* exg d,x */
    { "SLP",      0x00,       0x13,    0x01,           none        },  /* sync */
    /* 68HC11/HCS12 compatibility */
    { "PSHD",     0x34,       0x06,    OPR_M6800,      none        },  /* pshs d */
    { "PULD",     0x35,       0x06,    OPR_M6800,      none        },  /* puls d */
    { "BSET",     0,          0,       OPR_M6800,      hc11BitOp   },  /* 6309: oim #m,ea; 6809: pshs a; lda ea; ora #m; sta ea; puls a */
    { "BCLR",     0,          1,       OPR_M6800,      hc11BitOp   },  /* 6309: aim #~m,ea; 6809: pshs a; lda ea; anda #~m; sta ea; puls a */
    { "BRSET",    0,          0,       OPR_M6800,      hc11BitBranch}, /* pshs a,cc; lda ea; anda #m; cmpa #m; bne; puls a,cc; bra target */
    { "BRCLR",    0,          1,       OPR_M6800,      hc11BitBranch}, /* pshs a,cc; lda ea; anda #m; bne; puls a,cc; bra target */
    { "MAXA",     0,          0,       OPR_M6800,      hc11MinMax  },  /* pshs b; ldb ea; cba; bcc; tfr b,a; puls b */
    { "MINA",     0,          1,       OPR_M6800,      hc11MinMax  },  /* pshs b; ldb ea; cba; bcs; tfr b,a; puls b */
    { "MAXM",     0,          2,       OPR_M6800,      hc11MinMax  },  /* pshs b; ldb ea; cba; bcc; pshs cc; sta ea; puls cc,b */
    { "MINM",     0,          3,       OPR_M6800,      hc11MinMax  },  /* pshs b; ldb ea; cba; bcs; pshs cc; sta ea; puls cc,b */
    { "EMULS",    0,          0,       OPR_M6800,      hc11Emuls   },  /* pshs w,y; muld ,s++; tfr d,y; tfr w,d; puls w */
    { "CPD",      0x10,       0x83,    OPR_M6800,      load2       },  /* cmpd */
    { "CPY",      0x10,       0x8c,    OPR_M6800,      load2       },  /* cmpy */
    { "PSHY",     0x34,       0x20,    OPR_M6800,      none        },  /* pshs y */
    { "PULY",     0x35,       0x20,    OPR_M6800,      none        },  /* puls y */
    { "XGDY",     0x1e,       0x02,    OPR_M6800,      none        },  /* exg d,y */
    { "INY",      0x31,       0x21,    OPR_M6800,      none        },  /* leay 1,y */
    { "DEY",      0x31,       0x3f,    OPR_M6800,      none        },  /* leay -1,y */
    { "ABY",      0,          0,       OPR_M6800,      mnm68hc11   },  /* 6309: addr d,y; 6809: leay d,y with a,cc saved */
    { "TSY",      0,          1,       OPR_M6800,      mnm68hc11   },  /* pshs cc; leay 2,s; puls cc */
    { "TYS",      0x32,       0x3f,    OPR_M6800,      none        },  /* leas -1,y */

    /* 疑似命令 */
    { "OS9",      0x10,       0x3f,    0x00,           os9svc      },
    { "MOD",      0x00,       0x00,    0x00,           mod         },
    { "EMOD",     0x00,       0x00,    0x00,           emod        },

    { "REORG",    1,          0,          0,           org         },

    { "FCB",      0x00,       0x00,    0x00,           fcb         },
    { "DC.B",     0x00,       0x00,    0x00,           fcb         },
    { ".DB",      0,          0,          0,           fcb         },
    { ".BYTE",    0,          0,          0,           fcb         },
    { "FDB",      0x00,       0x00,    0x00,           fdb         },
    { "DC.W",     0x00,       0x00,    0x00,           fdb         },
    { ".DW",      0,          0,          0,           fdb         },
    { ".WORD",    0,          0,          0,           fdb         },
    { "FQB",      0,          0,          0,           fqb         },
    { "DC.L",     0x00,       0x00,    0x00,           fqb         },
    { ".QUAD",    0,          0,          0,           fqb         },
    { ".4BYTE",   0,          0,          0,           fqb         },

    { "FCC",      0x00,       0x00,    0x00,           fcc         },
    { ".ASCII",   0,          0,          0,           fcc         },
    { ".STR",     0,          0,          0,           fcc         },
    { "FCS",      0x00,       0x00,    0x00,           fcs         },
    { ".ASCIS",   0,          0,          0,           fcs         },
    { ".STRS",    0,          0,          0,           fcs         },
    { "FCN",      0,          0,          0,           fcn         },
    { ".ASCIZ",   0,          0,          0,           fcn         },
    { ".STRZ",    0,          0,          0,           fcn         },

    { "RMB",      0,          0,          0,           rmb         },
    { "RMD",      2, BLOCK_CHECK,         0,           rmb         },
    { "RMQ",      4, BLOCK_CHECK,         0,           rmb         },
    { ".BLKB",    1, BLOCK_CHECK,         0,           rmb         },
    { ".DS",      1, BLOCK_CHECK,         0,           rmb         },
    { ".RS",      1, BLOCK_CHECK,         0,           rmb         },

    { "RZB",      0,           0,            0,        rzb         },
    { "ZMB",      1, BLOCK_CHECK,            0,        rzb         },
    { "ZMD",      2, BLOCK_CHECK,            0,        rzb         },
    { "ZMQ",      4, BLOCK_CHECK,            0,        rzb         },
    { "DS",       2, BLOCK_CHECK,            0,        rzb         },
    { "DS.B",     1, BLOCK_CHECK,            0,        rzb         },
    { "DS.W",     2, BLOCK_CHECK,            0,        rzb         },
    { "DS.L",     4, BLOCK_CHECK,            0,        rzb         },
    { "DCB",      2, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "DCB.B",    1, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "DCB.W",    2, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "DCB.L",    4, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "BLK",      2, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "BLK.B",    1, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "BLK.W",    2, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "BLK.L",    4, BLOCK_CHECK|BLOCK_FILL, 0,        rzb         },
    { "FILL",     1, BLOCK_CHECK|BLOCK_REVERSED, 0,    rzb         },

    { "RS",       2,          0,       0,              rsOffset    },
    { "RS.B",     1,          0,       0,              rsOffset    },
    { "RS.W",     2,          0,       0,              rsOffset    },
    { "RS.L",     4,          0,       0,              rsOffset    },
    { "RSRESET",  0,         RS_RESET, 0,              rsOffset    },
    { "RSSET",    0,         RS_SET,   0,              rsOffset    },
    { "SO",       2,          16,      0,              rsOffset    },
    { "SO.B",     1,          16,      0,              rsOffset    },
    { "SO.W",     2,          16,      0,              rsOffset    },
    { "SO.L",     4,          16,      0,              rsOffset    },
    { "CLRSO",    0,      16|RS_RESET, 0,              rsOffset    },
    { "SETSO",    0,      16|RS_SET,   0,              rsOffset    },
    { "FO",       2,          32,      0,              rsOffset    },
    { "FO.B",     1,          32,      0,              rsOffset    },
    { "FO.W",     2,          32,      0,              rsOffset    },
    { "FO.L",     4,          32,      0,              rsOffset    },
    { "CLRFO",    0,      32|RS_RESET, 0,              rsOffset    },
    { "SETFO",    0,      32|RS_SET,   0,              rsOffset    },

    { "RORG",     0,          0,       0,              relativeOrg },
    { "DR.B",     1,          0,       0,              relativeData},
    { "DR.W",     2,          0,       0,              relativeData},
    { "DR.L",     4,          0,       0,              relativeData},

    { "CARGS",    0,          0,       0,              argumentOffsets },
    { "OFFSET",   0,          0,       0,              offsetSection   },

    { "EVEN",     0,          0,       0,              alignData   },
    { "ODD",      3,          0,       0,              alignData   },
    { "ALIGN",    1,          0,       0,              alignData   },
    { "CNOP",     2,          0,       0,              alignData   },

    { "EQU",      0x00,       0x00,    0x00,           equ         },
    { "=",        0,          0,       0,              equ         },
    { "SET",      0x00,  EQU_RESOLVED, 0x00,           equ         },
    { "CSECT",    0x00,       0x00,    0x00,           csct        },
    { "ENDSECT",  0x00,       0x00,    0x00,           endsct      },
    { "VSECT",    0x00,       0x00,    0x00,           vsct        },
    { "PSECT",    0x00,       0x00,    0x00,           psct        },

    { "ORG",      0x00,       0x00,    0x00,           org         },
    { "END",      0x00,       0x00,    0x00,           endop       },

    { "SETDP",    0x00,       0x00,    0x00,           setdp       },

    { "LIB",      0x00,       0x00,    0x00,           library     },
    { "USE",      0x00,       0x00,    0x00,           library     },
    { "INCLUDE",  0x00,       0x00,    0x00,           library     },
    { ".INCLUDE", 0x00,       0x00,    0x00,           library     },
    { "INCBIN",   0,          0,       0,              incbin      },
    { "INCLUDEBIN",0,         0,          0          , incbin      },
    { "INCDIR",   0,          0,       0,              incdir      },

    /* if */
    { "IF",       CO_IF,      0x00,    0x00,           NULL        },
    { "IFNE",     CO_IF,      0x00,    0x00,           NULL        },
    { "IFN",      CO_IFN,     0x00,    0x00,           NULL        },
    { "IFEQ",     CO_IFN,     0x00,    0x00,           NULL        },
    { "IFGE",     CO_IFGE,    0x00,    0x00,           NULL        },
    { "IFGT",     CO_IFGT,    0x00,    0x00,           NULL        },
    { "IFLE",     CO_IFLE,    0x00,    0x00,           NULL        },
    { "IFLT",     CO_IFLT,    0x00,    0x00,           NULL        },
    { "IFP1",     CO_IFP1,    0x00,    0x00,           NULL        },
    { "ELSE",     CO_ELSE,    0x00,    0x00,           NULL        },
    { "ELSIF",    CO_ELIF,    0x00,    0x00,           NULL        },
    { "ENDIF",    CO_ENDC,    0x00,    0x00,           NULL        },
    { "ENDC",     CO_ENDC,    0x00,    0x00,           NULL        },
    { "IFDEF",    CO_IFD,     0x00,    0x00,           NULL        },
    { "IFNDEF",   CO_IFND,    0x00,    0x00,           NULL        },
    { "IFD",      CO_IFD,     0x00,    0x00,           NULL        },
    { "IFND",     CO_IFND,    0x00,    0x00,           NULL        },
    { "IFC",      CO_IFC,     0x00,    0x00,           NULL        },
    { "IFNC",     CO_IFNC,    0x00,    0x00,           NULL        },
    { ".IF",      CO_IF,      0x00,    0x00,           NULL        },
    { ".ELSE",    CO_ELSE,    0x00,    0x00,           NULL        },
    { ".ELSIF",   CO_ELIF,    0x00,    0x00,           NULL        },
    { ".ENDIF",   CO_ENDC,    0x00,    0x00,           NULL        },
    { "IFPRAGMA", CO_IFPRAGMA,0,       0,              NULL        },
    { "IFB",      CO_IFB,     0,       0,              NULL        },
    { "IFNB",     CO_IFNB,    0,       0,              NULL        },
    { "IFMACROD", CO_IFMACROD,0,       0,              NULL        },
    { "IFMACROND",CO_IFMACROND,0,      0,              NULL        },
    { "IIF",      0,          0,       0,              ignoreOperand },

    /* global label */
    { "XDEF",     1,          0,       0,              symbolDirective },
    { "EXPORT",   1,          0,       0,              symbolDirective },
    { ".GLOBAL",  1,          0,       0,              symbolDirective },
    { ".GLOBL",   1,          0,       0,              symbolDirective },
    { "GLOBAL",   1,          0,       0,              symbolDirective },
    { "PUBLIC",   1,          0,       0,              symbolDirective },
    { "WEAK",     1,          0,       0,              symbolDirective },
    { "XREF",     2,          0,       0,              symbolDirective },
    { "EXTERN",   2,          0,       0,              symbolDirective },
    { "EXTERNAL", 2,          0,       0,              symbolDirective },
    { "IMPORT",   2,          0,       0,              symbolDirective },

    /* etc */
    { "PRAGMA",   0,          0,       0,              pragmaDirective },
    { "OPT",      0x00,       0x00,    0x00,           opt         },
    { "LIST",  LIST_ABSOLUTE, 1,       0,              opt         },
    { "NOLIST",LIST_ABSOLUTE, 0,       0,              opt         },
    { "REM",      1,          0,       0,              commentBlock},
    { "EREM",     0,          0,       0,              commentBlock},
    { "ECHO",     0,          0,       0,              printText   },
    { "PRINTT",   0,          0,       0,              printText   },
    { "PRINTV",   0,          0,       0,              printValue  },

    { "FAIL",     0,          0,       0,              failDirective },
    { "ERROR",    1,          0,       0,              failDirective },
    { "WARNING",  2,          0,       0,              failDirective },

    { "SPC",      0x00,       0x00,    0x00,           ignoreOperand },
    { "NAM",      0x00,       0x00,    0x00,           ignoreOperand },
    { "TTL",      0x00,       0x00,    0x00,           ignoreOperand },
    { "PAG",      0x00,       0x00,    0x00,           ignoreOperand },
    { "PAGE",     0,          0,       0,              ignoreOperand },
    { "LLEN",     0,          0,       0,              ignoreOperand },
    { "PLEN",     0,          0,       0,              ignoreOperand },
    { "COMMENT",  0,          0,       0,              ignoreOperand },
    { "OUTPUT",   0,          0,       0,              ignoreOperand },
    { ".MODULE",  0,          0,       0,              ignoreOperand },

    /* end of table */
    { "",         0x00,       0x00,    0x00,           none        }
};
