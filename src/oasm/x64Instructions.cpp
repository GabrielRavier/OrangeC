#include "x64Instructions.h"

const char* const opcodeTable[626] = {
    "",           "",           "",           "",         "",          "",           "",           "",          "",
    "",           "",           "",           "",         "",          "",           "",           "",          "",
    "",           "",           "",           "",         "",          "",           "",           "",          "",
    "",           "",           "",           "",         "",          "aaa",        "aad",        "aam",       "aas",
    "adc",        "add",        "and",        "arpl",     "bound",     "bsf",        "bsr",        "bswap",     "bt",
    "btc",        "btr",        "bts",        "call",     "cbw",       "cdq",        "cdqe",       "clc",       "cld",
    "cli",        "clts",       "cmc",        "cmova",    "cmovae",    "cmovb",      "cmovbe",     "cmovc",     "cmove",
    "cmovg",      "cmovge",     "cmovl",      "cmovle",   "cmovna",    "cmovnae",    "cmovnb",     "cmovnbe",   "cmovnc",
    "cmovne",     "cmovng",     "cmovnge",    "cmovnl",   "cmovnle",   "cmovno",     "cmovnp",     "cmovns",    "cmovnz",
    "cmovo",      "cmovp",      "cmovpe",     "cmovpo",   "cmovs",     "cmovz",      "cmp",        "cmps",      "cmpsb",
    "cmpsw",      "cmpsd",      "cmpsq",      "cmpxchg",  "cmpxchg8b", "cmpxchg16b", "cpuid",      "cqo",       "cwd",
    "cwde",       "daa",        "das",        "dec",      "div",       "enter",      "esc",        "f2xm1",     "fabs",
    "fadd",       "faddp",      "fbld",       "fbstp",    "fchs",      "fclex",      "fnclex",     "fcmovb",    "fcmovbe",
    "fcmove",     "fcmovnb",    "fcmovnbe",   "fcmovne",  "fcmovnu",   "fcmovu",     "fcom",       "fcomi",     "fcomip",
    "fcomp",      "fcompp",     "fcos",       "fdecstp",  "fdisi",     "fdiv",       "fdivp",      "fdivr",     "fdivrp",
    "feni",       "ffree",      "ffreep",     "fiadd",    "ficom",     "ficomp",     "fidiv",      "fidivr",    "fild",
    "fimul",      "fincstp",    "finit",      "fninit",   "fist",      "fistp",      "fisub",      "fisubr",    "fld",
    "fld1",       "fldcw",      "fldenv",     "fldl2e",   "fldl2t",    "fldlg2",     "fldln2",     "fldpi",     "fldz",
    "fmul",       "fmulp",      "fnop",       "fnsave",   "fpatan",    "fprem",      "fprem1",     "fptan",     "frndint",
    "frstor",     "fsave",      "fscale",     "fsetpm",   "fsin",      "fsincos",    "fsqrt",      "fst",       "fstcw",
    "fnstcw",     "fstenv",     "fnstenv",    "fstp",     "fstsw",     "fnstsw",     "fsub",       "fsubp",     "fsubr",
    "fsubrp",     "ftst",       "fucom",      "fucomi",   "fucomip",   "fucomp",     "fucompp",    "fwait",     "fxam",
    "fxch",       "fxch4",      "fxch7",      "fxrstor",  "fxsave",    "fxtract",    "fyl2x",      "fyl2xp1",   "hlt",
    "icebp",      "idiv",       "imul",       "in",       "ins",       "insb",       "insw",       "insd",      "inc",
    "int",        "int1",       "int3",       "into",     "invd",      "invlpg",     "iret",       "iretw",     "iretd",
    "iretq",      "ja",         "jae",        "jb",       "jbe",       "jc",         "jcxz",       "jecxz",     "je",
    "jg",         "jge",        "jl",         "jle",      "jmp",       "jna",        "jnae",       "jnb",       "jnbe",
    "jnc",        "jne",        "jng",        "jnge",     "jnl",       "jnle",       "jno",        "jnp",       "jns",
    "jnz",        "jo",         "jp",         "jpe",      "jpo",       "js",         "jz",         "lahf",      "lar",
    "lds",        "lea",        "leave",      "les",      "lfence",    "lfs",        "lgdt",       "lgs",       "lidt",
    "lldt",       "lmsw",       "lods",       "lodsb",    "lodsw",     "lodsd",      "lodsq",      "loop",      "loope",
    "loopne",     "loopnz",     "loopz",      "lsl",      "lss",       "ltr",        "mov",        "movs",      "movbe",
    "movsb",      "movsw",      "movsd",      "movsq",    "movsx",     "movzx",      "movsxd",     "mul",       "neg",
    "nop",        "not",        "or",         "out",      "outs",      "outsb",      "outsw",      "outsd",     "pop",
    "popa",       "popaw",      "popad",      "popcnt",   "popf",      "popfw",      "popfd",      "popfq",     "prefetchnta",
    "prefetcht0", "prefetcht1", "prefetcht2", "push",     "pusha",     "pushaw",     "pushad",     "pushf",     "pushfw",
    "pushfd",     "pushfq",     "rcl",        "rcr",      "rdmsr",     "rdpmc",      "rdtsc",      "ret",       "retf",
    "rol",        "ror",        "rsm",        "sahf",     "sal",       "sar",        "sbb",        "scas",      "scasb",
    "scasw",      "scasd",      "scasq",      "seta",     "setae",     "setb",       "setbe",      "setc",      "sete",
    "setg",       "setge",      "setl",       "setle",    "setna",     "setnae",     "setnb",      "setnbe",    "setnc",
    "setne",      "setng",      "setnge",     "setnl",    "setnle",    "setno",      "setnp",      "setns",     "setnz",
    "seto",       "setp",       "setpe",      "setpo",    "sets",      "setz",       "sfence",     "sgdt",      "shl",
    "shld",       "shr",        "shrd",       "sidt",     "sldt",      "smsw",       "stc",        "std",       "sti",
    "stos",       "stosb",      "stosw",      "stosd",    "stosq",     "str",        "sub",        "syscall",   "sysenter",
    "sysexit",    "sysret",     "test",       "ud2",      "verr",      "verw",       "wait",       "wbinvd",    "wrmsr",
    "xadd",       "xchg",       "xlat",       "xlatb",    "xor",       "xrstor",     "xrstor64",   "xsave",     "xsave64",
    "xsetbv",     "addpd",      "addps",      "addsd",    "addss",     "addsubpd",   "addsubps",   "andnpd",    "andnps",
    "andpd",      "andps",      "blendpd",    "blendps",  "cmppd",     "cmpps",      "cvtdq2pd",   "cvtdq2ps",  "cvtpd2dq",
    "cvtpd2pi",   "cvtpd2ps",   "cvtpi2pd",   "cvtpi2ps", "cvtps2dq",  "cvtps2pd",   "cvtps2pi",   "cvtsd2si",  "cvtsd2ss",
    "cvtsi2sd",   "cvtsi2ss",   "cvtss2sd",   "cvtss2si", "cvttpd2dq", "cvttpd2pi",  "cvttps2dq",  "cvttps2pi", "cvttsd2si",
    "cvttss2si",  "divpd",      "divps",      "divsd",    "divss",     "dppd",       "dpps",       "hsubpd",    "hsubps",
    "insertps",   "lddqu",      "maskmovdqu", "maskmovq", "maxpd",     "maxps",      "maxsd",      "maxss",     "mfence",
    "minpd",      "minps",      "minsd",      "minss",    "monitor",   "movapd",     "movaps",     "movd",      "movq",
    "movddup",    "movdq2q",    "movdqa",     "movdqu",   "movhlps",   "movhpd",     "movhps",     "movlhps",   "movlpd",
    "movlps",     "movmskpd",   "movmskps",   "movntdq",  "movnti",    "movntpd",    "movntd",     "movntq",    "movq2dq",
    "movshdup",   "movsldup",   "movss",      "movupd",   "movups",    "mpsadbw",    "mulpd",      "mulps",     "mulsd",
    "mulss",      "orpd",       "orps",       "packssdw", "packsswb",  "packusdw",   "paddb",      "paddd",     "paddq",
    "paddsw",     "paddusb",    "paddusw",    "paddw",    "palignr",   "pand",       "pandn",      "pavgb",     "pavgw",
    "pblendw",    "pcmpeqb",    "pcmpeqd",    "pcmpeqw",  "pcmpestri", "pcmpestrm",  "pcmpgtb",    "pcmpgtd",   "pcmpgtw",
    "pextrb",     "pextrd",     "pextrq",     "pextrw",   "pinsrb",    "pinsrd",     "pinsrq",     "pinsrw",    "pmaddwd",
    "pmaxsw",     "pmaxub",     "pminsw",     "pminub",   "pmovmskb",  "pmulhuw",    "pmulhw",     "pmullw",    "pmuludq",
    "psadbw",     "pshufd",     "pshufhw",    "pshuflw",  "pshufw",    "pslld",      "pslldq",     "psllq",     "psllw",
    "psrad",      "psraw",      "psrld",      "psrldq",   "psrlq",     "psrlw",      "psubb",      "psubd",     "psubq",
    "psubsb",     "psubsw",     "psubusb",    "psubusw",  "punpckhbw", "punpckhdq",  "punpckhqdq", "punpckhwd", "punpcklbw",
    "punpckldq",  "punpcklqdq", "punpcklwd",  "pxor",     "rcpps",     "rcpss",      "roundpd",    "roundps",   "roundsd",
    "roundss",    "rsqrtps",    "rsqrtss",    "shufpd",   "shufps",    "sqrtpd",     "sqrtps",     "sqrtsd",    "sqrtss",
    "subpd",      "subps",      "subsd",      "subss",    "unpckhpd",  "unpckhps",   "unpcklpd",   "unpcklps",  "xorpd",
    "xorps",      "invept",     "invvpid",    "vmcall",   "vmclear",   "vmlaunch",   "vmptrld",    "vmptrst",   "vmread",
    "vmresume",   "vmwrite",    "vmxoff",     "vmxon",    "a16",       "a32",        "lock",       "o16",       "o32",
    "rep",        "repe",       "repne",      "repnz",    "repz",
};

std::map<enum e_tk, const char*> tokenNames = {
    {tk_star, "*"},      {tk_plus, "+"},      {tk_comma, ","},   {tk_colon, ":"},   {tk_openbr, "["},    {tk_closebr, "]"},
    {tk_byte, "byte"},   {tk_dword, "dword"}, {tk_far, "far"},   {tk_near, "near"}, {tk_qword, "qword"}, {tk_short, "short"},
    {tk_tword, "tword"}, {tk_word, "word"},   {tk_al, "al"},     {tk_ah, "ah"},     {tk_ax, "ax"},       {tk_eax, "eax"},
    {tk_rax, "rax"},     {tk_r8b, "r8b"},     {tk_r8w, "r8w"},   {tk_r8d, "r8d"},   {tk_r8, "r8"},       {tk_cl, "cl"},
    {tk_ch, "ch"},       {tk_cx, "cx"},       {tk_ecx, "ecx"},   {tk_rcx, "rcx"},   {tk_r9b, "r9b"},     {tk_r9w, "r9w"},
    {tk_r9d, "r9d"},     {tk_r9, "r9"},       {tk_dl, "dl"},     {tk_dh, "dh"},     {tk_dx, "dx"},       {tk_edx, "edx"},
    {tk_rdx, "rdx"},     {tk_r10b, "r10b"},   {tk_r10w, "r10w"}, {tk_r10d, "r10d"}, {tk_r10, "r10"},     {tk_bl, "bl"},
    {tk_bh, "bh"},       {tk_bx, "bx"},       {tk_ebx, "ebx"},   {tk_rbx, "rbx"},   {tk_r11b, "r11b"},   {tk_r11w, "r11w"},
    {tk_r11d, "r11d"},   {tk_r11, "r11"},     {tk_spl, "spl"},   {tk_sp, "sp"},     {tk_esp, "esp"},     {tk_rsp, "rsp"},
    {tk_r12b, "r12b"},   {tk_r12w, "r12w"},   {tk_r12d, "r12d"}, {tk_r12, "r12"},   {tk_bpl, "bpl"},     {tk_bp, "bp"},
    {tk_ebp, "ebp"},     {tk_rbp, "rbp"},     {tk_r13b, "r13b"}, {tk_r13w, "r13w"}, {tk_r13d, "r13d"},   {tk_r13, "r13"},
    {tk_sil, "sil"},     {tk_si, "si"},       {tk_esi, "esi"},   {tk_rsi, "rsi"},   {tk_r14b, "r14b"},   {tk_r14w, "r14w"},
    {tk_r14d, "r14d"},   {tk_r14, "r14"},     {tk_dil, "dil"},   {tk_di, "di"},     {tk_edi, "edi"},     {tk_rdi, "rdi"},
    {tk_r15b, "r15b"},   {tk_r15w, "r15w"},   {tk_r15d, "r15d"}, {tk_r15, "r15"},   {tk_rip, "rip"},     {tk_mm0, "mm0"},
    {tk_xmm0, "xmm0"},   {tk_xmm8, "xmm8"},   {tk_mm1, "mm1"},   {tk_xmm1, "xmm1"}, {tk_xmm9, "xmm9"},   {tk_mm2, "mm2"},
    {tk_xmm2, "xmm2"},   {tk_xmm10, "xmm10"}, {tk_mm3, "mm3"},   {tk_xmm3, "xmm3"}, {tk_xmm11, "xmm11"}, {tk_mm4, "mm4"},
    {tk_xmm4, "xmm4"},   {tk_xmm12, "xmm12"}, {tk_mm5, "mm5"},   {tk_xmm5, "xmm5"}, {tk_xmm13, "xmm13"}, {tk_mm6, "mm6"},
    {tk_xmm6, "xmm6"},   {tk_xmm14, "xmm14"}, {tk_mm7, "mm7"},   {tk_xmm7, "xmm7"}, {tk_xmm15, "xmm15"}, {tk_es, "es"},
    {tk_cs, "cs"},       {tk_ss, "ss"},       {tk_ds, "ds"},     {tk_fs, "fs"},     {tk_gs, "gs"},       {tk_st0, "st0"},
    {tk_st1, "st1"},     {tk_st2, "st2"},     {tk_st3, "st3"},   {tk_st4, "st4"},   {tk_st5, "st5"},     {tk_st6, "st6"},
    {tk_st7, "st7"},     {tk_cr0, "cr0"},     {tk_cr1, "cr1"},   {tk_cr2, "cr2"},   {tk_cr3, "cr3"},     {tk_cr4, "cr4"},
    {tk_cr5, "cr5"},     {tk_cr6, "cr6"},     {tk_cr7, "cr7"},   {tk_dr0, "dr0"},   {tk_dr1, "dr1"},     {tk_dr2, "dr2"},
    {tk_dr3, "dr3"},     {tk_dr4, "dr4"},     {tk_dr5, "dr5"},   {tk_dr6, "dr6"},   {tk_dr7, "dr7"},     {tk_tr0, "tr0"},
    {tk_tr1, "tr1"},     {tk_tr2, "tr2"},     {tk_tr3, "tr3"},   {tk_tr4, "tr4"},   {tk_tr5, "tr5"},     {tk_tr6, "tr6"},
    {tk_tr7, "tr7"},
};

InputToken Tokenstar{InputToken::TOKEN, new AsmExprNode(tk_star)};
InputToken Tokenplus{InputToken::TOKEN, new AsmExprNode(tk_plus)};
InputToken Tokencomma{InputToken::TOKEN, new AsmExprNode(tk_comma)};
InputToken Tokencolon{InputToken::TOKEN, new AsmExprNode(tk_colon)};
InputToken Tokenopenbr{InputToken::TOKEN, new AsmExprNode(tk_openbr)};
InputToken Tokenclosebr{InputToken::TOKEN, new AsmExprNode(tk_closebr)};
InputToken Tokenbyte{InputToken::TOKEN, new AsmExprNode(tk_byte)};
InputToken Tokendword{InputToken::TOKEN, new AsmExprNode(tk_dword)};
InputToken Tokenfar{InputToken::TOKEN, new AsmExprNode(tk_far)};
InputToken Tokennear{InputToken::TOKEN, new AsmExprNode(tk_near)};
InputToken Tokenqword{InputToken::TOKEN, new AsmExprNode(tk_qword)};
InputToken Tokenshort{InputToken::TOKEN, new AsmExprNode(tk_short)};
InputToken Tokentword{InputToken::TOKEN, new AsmExprNode(tk_tword)};
InputToken Tokenword{InputToken::TOKEN, new AsmExprNode(tk_word)};
InputToken Tokenal{InputToken::REGISTER, new AsmExprNode(tk_al - 1000, true)};
InputToken Tokenah{InputToken::REGISTER, new AsmExprNode(tk_ah - 1000, true)};
InputToken Tokenax{InputToken::REGISTER, new AsmExprNode(tk_ax - 1000, true)};
InputToken Tokeneax{InputToken::REGISTER, new AsmExprNode(tk_eax - 1000, true)};
InputToken Tokenrax{InputToken::REGISTER, new AsmExprNode(tk_rax - 1000, true)};
InputToken Tokenr8b{InputToken::REGISTER, new AsmExprNode(tk_r8b - 1000, true)};
InputToken Tokenr8w{InputToken::REGISTER, new AsmExprNode(tk_r8w - 1000, true)};
InputToken Tokenr8d{InputToken::REGISTER, new AsmExprNode(tk_r8d - 1000, true)};
InputToken Tokenr8{InputToken::REGISTER, new AsmExprNode(tk_r8 - 1000, true)};
InputToken Tokencl{InputToken::REGISTER, new AsmExprNode(tk_cl - 1000, true)};
InputToken Tokench{InputToken::REGISTER, new AsmExprNode(tk_ch - 1000, true)};
InputToken Tokencx{InputToken::REGISTER, new AsmExprNode(tk_cx - 1000, true)};
InputToken Tokenecx{InputToken::REGISTER, new AsmExprNode(tk_ecx - 1000, true)};
InputToken Tokenrcx{InputToken::REGISTER, new AsmExprNode(tk_rcx - 1000, true)};
InputToken Tokenr9b{InputToken::REGISTER, new AsmExprNode(tk_r9b - 1000, true)};
InputToken Tokenr9w{InputToken::REGISTER, new AsmExprNode(tk_r9w - 1000, true)};
InputToken Tokenr9d{InputToken::REGISTER, new AsmExprNode(tk_r9d - 1000, true)};
InputToken Tokenr9{InputToken::REGISTER, new AsmExprNode(tk_r9 - 1000, true)};
InputToken Tokendl{InputToken::REGISTER, new AsmExprNode(tk_dl - 1000, true)};
InputToken Tokendh{InputToken::REGISTER, new AsmExprNode(tk_dh - 1000, true)};
InputToken Tokendx{InputToken::REGISTER, new AsmExprNode(tk_dx - 1000, true)};
InputToken Tokenedx{InputToken::REGISTER, new AsmExprNode(tk_edx - 1000, true)};
InputToken Tokenrdx{InputToken::REGISTER, new AsmExprNode(tk_rdx - 1000, true)};
InputToken Tokenr10b{InputToken::REGISTER, new AsmExprNode(tk_r10b - 1000, true)};
InputToken Tokenr10w{InputToken::REGISTER, new AsmExprNode(tk_r10w - 1000, true)};
InputToken Tokenr10d{InputToken::REGISTER, new AsmExprNode(tk_r10d - 1000, true)};
InputToken Tokenr10{InputToken::REGISTER, new AsmExprNode(tk_r10 - 1000, true)};
InputToken Tokenbl{InputToken::REGISTER, new AsmExprNode(tk_bl - 1000, true)};
InputToken Tokenbh{InputToken::REGISTER, new AsmExprNode(tk_bh - 1000, true)};
InputToken Tokenbx{InputToken::REGISTER, new AsmExprNode(tk_bx - 1000, true)};
InputToken Tokenebx{InputToken::REGISTER, new AsmExprNode(tk_ebx - 1000, true)};
InputToken Tokenrbx{InputToken::REGISTER, new AsmExprNode(tk_rbx - 1000, true)};
InputToken Tokenr11b{InputToken::REGISTER, new AsmExprNode(tk_r11b - 1000, true)};
InputToken Tokenr11w{InputToken::REGISTER, new AsmExprNode(tk_r11w - 1000, true)};
InputToken Tokenr11d{InputToken::REGISTER, new AsmExprNode(tk_r11d - 1000, true)};
InputToken Tokenr11{InputToken::REGISTER, new AsmExprNode(tk_r11 - 1000, true)};
InputToken Tokenspl{InputToken::REGISTER, new AsmExprNode(tk_spl - 1000, true)};
InputToken Tokensp{InputToken::REGISTER, new AsmExprNode(tk_sp - 1000, true)};
InputToken Tokenesp{InputToken::REGISTER, new AsmExprNode(tk_esp - 1000, true)};
InputToken Tokenrsp{InputToken::REGISTER, new AsmExprNode(tk_rsp - 1000, true)};
InputToken Tokenr12b{InputToken::REGISTER, new AsmExprNode(tk_r12b - 1000, true)};
InputToken Tokenr12w{InputToken::REGISTER, new AsmExprNode(tk_r12w - 1000, true)};
InputToken Tokenr12d{InputToken::REGISTER, new AsmExprNode(tk_r12d - 1000, true)};
InputToken Tokenr12{InputToken::REGISTER, new AsmExprNode(tk_r12 - 1000, true)};
InputToken Tokenbpl{InputToken::REGISTER, new AsmExprNode(tk_bpl - 1000, true)};
InputToken Tokenbp{InputToken::REGISTER, new AsmExprNode(tk_bp - 1000, true)};
InputToken Tokenebp{InputToken::REGISTER, new AsmExprNode(tk_ebp - 1000, true)};
InputToken Tokenrbp{InputToken::REGISTER, new AsmExprNode(tk_rbp - 1000, true)};
InputToken Tokenr13b{InputToken::REGISTER, new AsmExprNode(tk_r13b - 1000, true)};
InputToken Tokenr13w{InputToken::REGISTER, new AsmExprNode(tk_r13w - 1000, true)};
InputToken Tokenr13d{InputToken::REGISTER, new AsmExprNode(tk_r13d - 1000, true)};
InputToken Tokenr13{InputToken::REGISTER, new AsmExprNode(tk_r13 - 1000, true)};
InputToken Tokensil{InputToken::REGISTER, new AsmExprNode(tk_sil - 1000, true)};
InputToken Tokensi{InputToken::REGISTER, new AsmExprNode(tk_si - 1000, true)};
InputToken Tokenesi{InputToken::REGISTER, new AsmExprNode(tk_esi - 1000, true)};
InputToken Tokenrsi{InputToken::REGISTER, new AsmExprNode(tk_rsi - 1000, true)};
InputToken Tokenr14b{InputToken::REGISTER, new AsmExprNode(tk_r14b - 1000, true)};
InputToken Tokenr14w{InputToken::REGISTER, new AsmExprNode(tk_r14w - 1000, true)};
InputToken Tokenr14d{InputToken::REGISTER, new AsmExprNode(tk_r14d - 1000, true)};
InputToken Tokenr14{InputToken::REGISTER, new AsmExprNode(tk_r14 - 1000, true)};
InputToken Tokendil{InputToken::REGISTER, new AsmExprNode(tk_dil - 1000, true)};
InputToken Tokendi{InputToken::REGISTER, new AsmExprNode(tk_di - 1000, true)};
InputToken Tokenedi{InputToken::REGISTER, new AsmExprNode(tk_edi - 1000, true)};
InputToken Tokenrdi{InputToken::REGISTER, new AsmExprNode(tk_rdi - 1000, true)};
InputToken Tokenr15b{InputToken::REGISTER, new AsmExprNode(tk_r15b - 1000, true)};
InputToken Tokenr15w{InputToken::REGISTER, new AsmExprNode(tk_r15w - 1000, true)};
InputToken Tokenr15d{InputToken::REGISTER, new AsmExprNode(tk_r15d - 1000, true)};
InputToken Tokenr15{InputToken::REGISTER, new AsmExprNode(tk_r15 - 1000, true)};
InputToken Tokenrip{InputToken::REGISTER, new AsmExprNode(tk_rip - 1000, true)};
InputToken Tokenmm0{InputToken::REGISTER, new AsmExprNode(tk_mm0 - 1000, true)};
InputToken Tokenxmm0{InputToken::REGISTER, new AsmExprNode(tk_xmm0 - 1000, true)};
InputToken Tokenxmm8{InputToken::REGISTER, new AsmExprNode(tk_xmm8 - 1000, true)};
InputToken Tokenmm1{InputToken::REGISTER, new AsmExprNode(tk_mm1 - 1000, true)};
InputToken Tokenxmm1{InputToken::REGISTER, new AsmExprNode(tk_xmm1 - 1000, true)};
InputToken Tokenxmm9{InputToken::REGISTER, new AsmExprNode(tk_xmm9 - 1000, true)};
InputToken Tokenmm2{InputToken::REGISTER, new AsmExprNode(tk_mm2 - 1000, true)};
InputToken Tokenxmm2{InputToken::REGISTER, new AsmExprNode(tk_xmm2 - 1000, true)};
InputToken Tokenxmm10{InputToken::REGISTER, new AsmExprNode(tk_xmm10 - 1000, true)};
InputToken Tokenmm3{InputToken::REGISTER, new AsmExprNode(tk_mm3 - 1000, true)};
InputToken Tokenxmm3{InputToken::REGISTER, new AsmExprNode(tk_xmm3 - 1000, true)};
InputToken Tokenxmm11{InputToken::REGISTER, new AsmExprNode(tk_xmm11 - 1000, true)};
InputToken Tokenmm4{InputToken::REGISTER, new AsmExprNode(tk_mm4 - 1000, true)};
InputToken Tokenxmm4{InputToken::REGISTER, new AsmExprNode(tk_xmm4 - 1000, true)};
InputToken Tokenxmm12{InputToken::REGISTER, new AsmExprNode(tk_xmm12 - 1000, true)};
InputToken Tokenmm5{InputToken::REGISTER, new AsmExprNode(tk_mm5 - 1000, true)};
InputToken Tokenxmm5{InputToken::REGISTER, new AsmExprNode(tk_xmm5 - 1000, true)};
InputToken Tokenxmm13{InputToken::REGISTER, new AsmExprNode(tk_xmm13 - 1000, true)};
InputToken Tokenmm6{InputToken::REGISTER, new AsmExprNode(tk_mm6 - 1000, true)};
InputToken Tokenxmm6{InputToken::REGISTER, new AsmExprNode(tk_xmm6 - 1000, true)};
InputToken Tokenxmm14{InputToken::REGISTER, new AsmExprNode(tk_xmm14 - 1000, true)};
InputToken Tokenmm7{InputToken::REGISTER, new AsmExprNode(tk_mm7 - 1000, true)};
InputToken Tokenxmm7{InputToken::REGISTER, new AsmExprNode(tk_xmm7 - 1000, true)};
InputToken Tokenxmm15{InputToken::REGISTER, new AsmExprNode(tk_xmm15 - 1000, true)};
InputToken Tokenes{InputToken::REGISTER, new AsmExprNode(tk_es - 1000, true)};
InputToken Tokencs{InputToken::REGISTER, new AsmExprNode(tk_cs - 1000, true)};
InputToken Tokenss{InputToken::REGISTER, new AsmExprNode(tk_ss - 1000, true)};
InputToken Tokends{InputToken::REGISTER, new AsmExprNode(tk_ds - 1000, true)};
InputToken Tokenfs{InputToken::REGISTER, new AsmExprNode(tk_fs - 1000, true)};
InputToken Tokengs{InputToken::REGISTER, new AsmExprNode(tk_gs - 1000, true)};
InputToken Tokenst0{InputToken::REGISTER, new AsmExprNode(tk_st0 - 1000, true)};
InputToken Tokenst1{InputToken::REGISTER, new AsmExprNode(tk_st1 - 1000, true)};
InputToken Tokenst2{InputToken::REGISTER, new AsmExprNode(tk_st2 - 1000, true)};
InputToken Tokenst3{InputToken::REGISTER, new AsmExprNode(tk_st3 - 1000, true)};
InputToken Tokenst4{InputToken::REGISTER, new AsmExprNode(tk_st4 - 1000, true)};
InputToken Tokenst5{InputToken::REGISTER, new AsmExprNode(tk_st5 - 1000, true)};
InputToken Tokenst6{InputToken::REGISTER, new AsmExprNode(tk_st6 - 1000, true)};
InputToken Tokenst7{InputToken::REGISTER, new AsmExprNode(tk_st7 - 1000, true)};
InputToken Tokencr0{InputToken::REGISTER, new AsmExprNode(tk_cr0 - 1000, true)};
InputToken Tokencr1{InputToken::REGISTER, new AsmExprNode(tk_cr1 - 1000, true)};
InputToken Tokencr2{InputToken::REGISTER, new AsmExprNode(tk_cr2 - 1000, true)};
InputToken Tokencr3{InputToken::REGISTER, new AsmExprNode(tk_cr3 - 1000, true)};
InputToken Tokencr4{InputToken::REGISTER, new AsmExprNode(tk_cr4 - 1000, true)};
InputToken Tokencr5{InputToken::REGISTER, new AsmExprNode(tk_cr5 - 1000, true)};
InputToken Tokencr6{InputToken::REGISTER, new AsmExprNode(tk_cr6 - 1000, true)};
InputToken Tokencr7{InputToken::REGISTER, new AsmExprNode(tk_cr7 - 1000, true)};
InputToken Tokendr0{InputToken::REGISTER, new AsmExprNode(tk_dr0 - 1000, true)};
InputToken Tokendr1{InputToken::REGISTER, new AsmExprNode(tk_dr1 - 1000, true)};
InputToken Tokendr2{InputToken::REGISTER, new AsmExprNode(tk_dr2 - 1000, true)};
InputToken Tokendr3{InputToken::REGISTER, new AsmExprNode(tk_dr3 - 1000, true)};
InputToken Tokendr4{InputToken::REGISTER, new AsmExprNode(tk_dr4 - 1000, true)};
InputToken Tokendr5{InputToken::REGISTER, new AsmExprNode(tk_dr5 - 1000, true)};
InputToken Tokendr6{InputToken::REGISTER, new AsmExprNode(tk_dr6 - 1000, true)};
InputToken Tokendr7{InputToken::REGISTER, new AsmExprNode(tk_dr7 - 1000, true)};
InputToken Tokentr0{InputToken::REGISTER, new AsmExprNode(tk_tr0 - 1000, true)};
InputToken Tokentr1{InputToken::REGISTER, new AsmExprNode(tk_tr1 - 1000, true)};
InputToken Tokentr2{InputToken::REGISTER, new AsmExprNode(tk_tr2 - 1000, true)};
InputToken Tokentr3{InputToken::REGISTER, new AsmExprNode(tk_tr3 - 1000, true)};
InputToken Tokentr4{InputToken::REGISTER, new AsmExprNode(tk_tr4 - 1000, true)};
InputToken Tokentr5{InputToken::REGISTER, new AsmExprNode(tk_tr5 - 1000, true)};
InputToken Tokentr6{InputToken::REGISTER, new AsmExprNode(tk_tr6 - 1000, true)};
InputToken Tokentr7{InputToken::REGISTER, new AsmExprNode(tk_tr7 - 1000, true)};