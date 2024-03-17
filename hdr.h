#ifndef HDR_H
#define HDR_H

const char *ehdr_type[] = {
    "ET_NONE",
    "ET_REL",
    "ET_EXEC",
    "ET_DYN",
    "ET_CORE"
};

const char *ehdr_machine[] = {
    "EM_NONE",
    "EM_M32",
    "EM_SPARC",
    "EM_386",
    "EM_68k",
    "EM_88k",
    "EM_486",
    "EM_860",
    "EM_MIPS",
    "EM_UNKNOWN9",
    "EM_MIPS_RS3_LE",
    "EM_RS6000",
    "EM_UNKNOWN12",
    "EM_UNKNOWN13",
    "EM_UNKNOWN14",
    "EM_PA_RISC",
    "EM_nCUBE",
    "EM_VPP500",
    "EM_SPARC32PLUS",
    "EM_UNKNOWN19",
    "EM_PPC",
    "EM_UNKNOWN21",
    "EM_UNKNOWN22",
    "EM_UNKNOWN23",
    "EM_UNKNOWN24",
    "EM_UNKNOWN25",
    "EM_UNKNOWN26",
    "EM_UNKNOWN27",
    "EM_UNKNOWN28",
    "EM_UNKNOWN29",
    "EM_UNKNOWN30",
    "EM_UNKNOWN31",
    "EM_UNKNOWN32",
    "EM_UNKNOWN33",
    "EM_UNKNOWN34",
    "EM_UNKNOWN35",
    "EM_UNKNOWN36",
    "EM_UNKNOWN37",
    "EM_UNKNOWN38",
    "EM_UNKNOWN39",
    "EM_ARM",
    "EM_UNKNOWN41",
    "EM_SH",
};

const char *phdr_type[] = {
    "PT_NULL",					/* Program header table entry unused */
    "PT_LOAD",					/* Loadable program segment */
    "PT_DYNAMIC",				/* Dynamic linking information */
    "PT_INTERP",				/* Program interpreter */
    "PT_NOTE",					/* Auxiliary information */
    "PT_SHLIB",					/* Reserved, unspecified semantics */
    "PT_PHDR",					/* Entry for header table itself */
};

const char *phdr_flags[] = {
    "---",
    "--X",
    "-W-",
    "-WX",
    "R--",
    "R-X",
    "RW-",
    "RWX"
};

#endif // HDR_H
