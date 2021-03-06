/* Software License Agreement
 *
 *     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
 *
 *     This file is part of the Orange C Compiler package.
 *
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version, with the addition of the
 *     Orange C "Target Code" exception.
 *
 *     The Orange C Compiler package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 *
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 *
 */

#include "compiler.h"
#include <setjmp.h>
extern ARCH_DEBUG* chosenDebugger;
extern ARCH_ASM* chosenAssembler;
extern NAMESPACEVALUES* globalNameSpace;
extern LIST* clist;
extern char outfile[];
extern FILE* outputFile;
extern int total_errors;
/*extern int prm_peepopt; */
extern FILE* listFile;
extern char version[256];
extern int optflags;
extern LIST* nonSysIncludeFiles;

#ifdef _WIN32
extern "C" {
    char* __stdcall GetModuleFileNameA(int handle, char* buf, int size);
}
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef PARSER_ONLY
void ccDumpSymbols(void);
void ccNewFile(char* fileName, bool main);
void ccCloseFile(FILE* handle);
int ccDBOpen(const char* name);
#endif

int verbosity = 0;

int maxBlocks, maxTemps, maxAllocationSpills, maxAllocationPasses, maxAllocationAccesses;
char cppfile[256];
FILE *cppFile, *browseFile;

FILE *errFile, *icdFile;
char infile[256];

static char tempOutFile[260];
char realOutFile[260];
static char oldOutFile[260];

static FILE* inputFile = 0;
static int stoponerr = 0;

COMPILER_PARAMS cparams = {
    25,    /* int  prm_maxerr;*/
    0,     /* prm_stackalign */
    true,  /* optimize_for_speed */
    false, /* optimize_for_size */
    false, /* optimize_for_float_access */
    false, /* char prm_quieterrors;*/
    true,  /* char prm_warning;*/
    false, /* char prm_extwarning;*/
    false, /* char prm_diag;*/
    false, /* char prm_ansi;*/
    true,  /* char prm_cmangle;*/
    true,  /* char prm_c99;*/
    true,  /* char prm_c1x;*/
    false, /* char prm_cplusplus;*/
    true,  /* char prm_xcept;*/
    false, /* char prm_icdfile;*/
    true,  /* char prm_asmfile;*/
    false, /* char prm_compileonly;*/
    false, /* char prm_debug;*/
    false, /* char prm_listfile;*/
    false, /* char prm_cppfile;*/
    false, /* char prm_errfile;*/
    false, /* char prm_browse;*/
    false, /* char prm_trigraph;*/
    false, /* char prm_oldfor;*/
    false, /* char prm_stackcheck;*/
    true,  /* char prm_allowinline;*/
    false, /* char prm_profiler;*/
    true,  /* char prm_mergstrings;*/
    false, /* char prm_revbits;*/
    true,  /* char prm_lines;*/
    true,  /* char prm_bss;*/
    false, /* char prm_intrinsic;*/
    false, /* char prm_smartframes;*/
    false, /* char prm_farkeyword;*/
    false, /* char prm_linkreg;*/
    false, /* char prm_charisunsigned;*/
};

void bool_setup(char select, char* string);
void err_setup(char select, char* string);
void incl_setup(char select, char* string);
void libpath_setup(char select, char* string);
void def_setup(char select, char* string);
void codegen_setup(char select, char* string);
void optimize_setup(char select, char* string);
void parsefile(char select, char* string);
void output_setup(char select, char* string);
void stackalign_setup(char select, char* string);
void verbose_setup(char select, char* string);
void library_setup(char select, char* string);
void tool_setup(char select, char* string);
void warning_setup(char select, char* string);
void sysincl_setup(char select, char* string);
/* setup for ARGS.C */
static CMDLIST Args[] = {{'8', ARG_BOOL, bool_setup},
                         {'9', ARG_BOOL, bool_setup},
                         {'1', ARG_BOOL, bool_setup},
                         {'A', ARG_BOOL, bool_setup},
                         {'E', ARG_CONCATSTRING, err_setup},
                         {'I', ARG_COMBINESTRING, incl_setup},
                         {'z', ARG_COMBINESTRING, sysincl_setup},
                         {'L', ARG_COMBINESTRING, libpath_setup},
                         {'D', ARG_CONCATSTRING, def_setup},
                         {'U', ARG_CONCATSTRING, undef_setup},
                         {'e', ARG_BOOL, bool_setup},
                         {'l', ARG_CONCATSTRING, library_setup},
                         {'i', ARG_BOOL, bool_setup},
                         {'Q', ARG_BOOL, bool_setup},
                         {'Y', ARG_BOOL, bool_setup},
                         {'T', ARG_BOOL, bool_setup},
                         {'v', ARG_BOOL, bool_setup},
                         {'M', ARG_BOOL, bool_setup},
                         {'c', ARG_BOOL, bool_setup},
                         {'X', ARG_BOOL, bool_setup},
                         {'@', ARG_CONCATSTRING, parsefile},
                         {'o', ARG_COMBINESTRING, output_setup},
                         {'C', ARG_CONCATSTRING, codegen_setup},
                         {'O', ARG_CONCATSTRING, optimize_setup},
                         {'s', ARG_CONCATSTRING, stackalign_setup},
                         {'#', ARG_BOOL, bool_setup},
                         {'y', ARG_CONCATSTRING, verbose_setup},
                         {'p', ARG_CONCATSTRING, tool_setup},
                         {'w', ARG_CONCATSTRING, warning_setup},
                         {0, 0, 0}};

CMDLIST* ArgList = &Args[0];

void library_setup(char select, char* string)
{
    (void)select;
    if (string[0] == 0)
    {
        cparams.prm_listfile = true;
    }
    else
    {
        char buf[260];
        strcpy(buf, string);
        StripExt(buf);
        AddExt(buf, ".l");
        InsertAnyFile(buf, 0, -1, false);
    }
}
void bool_setup(char select, char* string)
/*
 * activation routine (callback) for boolean command line arguments
 */
{
    bool v = (bool)string;
    if (select == '1')
        cparams.prm_c99 = cparams.prm_c1x = v;
    if (select == '9')
    {
        cparams.prm_c99 = v;
        cparams.prm_c1x = !v;
    }
    if (select == '8')
        cparams.prm_c99 = cparams.prm_c1x = !v;
    if (select == 'M')
        cparams.prm_makestubs = v;
    if (select == 'A')
        cparams.prm_ansi = v;
    if (select == 'e')
        cparams.prm_errfile = v;
    //    if (select == 'l')
    //        cparams.prm_listfile = v;
    if (select == 'i')
        cparams.prm_cppfile = v;
    if (select == 'Q')
        cparams.prm_quiet = v;
    if (select == 'T')
        cparams.prm_trigraph = v;
    if (select == 'Y')
        cparams.prm_icdfile = v;
    if (select == '#')
    {
        cparams.prm_assemble = true;
        cparams.prm_asmfile = false;
    }
    if (select == 'v')
    {
        cparams.prm_debug = v;
        if (v)
        {
            cparams.prm_optimize_for_speed = cparams.prm_optimize_for_size = 0;
        }
    }
    if (select == 'c')
    {
        if (chosenAssembler->objext)
        {
            cparams.prm_compileonly = v;
            cparams.prm_asmfile = false;
        }
    }
    if (select == 'X')
    {
        cparams.prm_xcept = v;
    }
}
void verbose_setup(char select, char* string)
{
    (void)select;
    verbosity = 1 + strlen(string);
}
void optimize_setup(char select, char* string)
{
    (void)select;
    if (!*string || (*string == '+' && string[1] == '\0'))
    {
        cparams.prm_optimize_for_speed = true;
        cparams.prm_optimize_for_size = false;
        cparams.prm_debug = false;
    }
    else if (*string == '-' && string[1] == '\0')
        cparams.prm_optimize_for_speed = cparams.prm_optimize_for_size = cparams.prm_optimize_float_access = false;
    else
    {
        cparams.prm_debug = false;
        if (*string == '0')
        {
            cparams.prm_optimize_for_speed = cparams.prm_optimize_for_size = 0;
        }
        else if (*string == 'f')
            cparams.prm_optimize_float_access = true;
        else if (*string == '2')
        {
            cparams.prm_optimize_for_speed = true;
            cparams.prm_optimize_for_size = false;
        }
        else if (*string == '1')
        {
            cparams.prm_optimize_for_speed = false;
            cparams.prm_optimize_for_size = true;
        }
    }
}
/*-------------------------------------------------------------------------*/

void codegen_setup(char select, char* string)
/*
 * activation for code-gen type command line arguments
 */
{
    char v = true;
    (void)select;
    while (*string)
    {
        switch (*string)
        {
            /*               case 'f':*/
            /*                  cparams.prm_smartframes = bool ;*/
            /*                  break ;*/
            case 'u':
                cparams.prm_charisunsigned = v;
                break;
            case 'd':
                cparams.prm_diag = v;
                break;
            case 'r':
                cparams.prm_revbits = v;
                break;
            case 'b':
                cparams.prm_bss = v;
                break;
            case 'l':
                cparams.prm_lines = v;
                break;
            case 'm':
                cparams.prm_cmangle = v;
                break;
#ifndef i386
                /*                case 'R':*/
                /*                    cparams.prm_linkreg = v;*/
                /*                    break;*/
#endif
            case 'S':
                cparams.prm_stackcheck = v;
                break;
            case 'O':
                cparams.prm_oldfor = v;
                break;
            case 'Z':
                cparams.prm_profiler = v;
                break;
            case 'i':
                cparams.prm_allowinline = v;
                break;
            case '-':
                v = false;
                break;
            case '+':
                v = true;
                break;
            default:
                if (chosenAssembler->parse_codegen)
                {
                    switch (chosenAssembler->parse_codegen(v, string))
                    {
                        case 1:
                            break;
                        case 2:
                            return;
                        case 0:
                        default:
                            fatal("Invalid codegen parameter ");
                            break;
                    }
                }
                else
                    fatal("Invalid codegen parameter ");
        }
        string++;
    }
}
void stackalign_setup(char select, char* string)
{
    (void)select;
    int n = 16;
    if (string[0])
        n = atoi(string);
    if (!n || (n % chosenAssembler->arch->stackalign))
        fatal("Invalid stack alignment parameter ");
    cparams.prm_stackalign = n;
}
static void debug_dumptypedefs(NAMESPACEVALUES* nameSpace)
{
    int i;
    HASHTABLE* syms = nameSpace->syms;
    for (i = 0; i < syms->size; i++)
    {
        HASHREC* h = syms->table[i];
        if (h != 0)
        {
            while (h)
            {

                SYMBOL* sp = (SYMBOL*)h->p;
                if (sp->storage_class == sc_namespace)
                {
                    debug_dumptypedefs(sp->nameSpaceValues);
                }
                else if (istype(sp))
                    chosenDebugger->outputtypedef(sp);
                h = h->next;
            }
        }
    }
}
void MakeStubs(void)
{
    LIST* list;
    // parse the file, only gets the macro expansions
    errorinit();
    syminit();
    preprocini(infile, inputFile);
    lexini();
    setglbdefs();
    while (getsym() != NULL)
        ;
    printf("%s:\\\n", infile);
    list = nonSysIncludeFiles;
    while (list)
    {
        printf("    %s \\\n", (char*)list->data);
        list = list->next;
    }
    printf("\n");
}
void compile(bool global)
{
    LEXEME* lex = NULL;
    SetGlobalFlag(true);
    helpinit();
    mangleInit();
    errorinit();
    constoptinit();
    declare_init();
    init_init();
    inlineinit();
    lambda_init();
    rtti_init();
    expr_init();
    libcxx_init();
    statement_ini(global);
    syminit();
    preprocini(infile, inputFile);
    lexini();
    setglbdefs();
    templateInit();
#ifndef PARSER_ONLY
    SSAInit();
    outcodeini();
    conflictini();
    iexpr_init();
    iinlineInit();
    flow_init();
    genstmtini();
#endif
    ParseBuiltins();
    if (chosenAssembler->intrinsicInit)
        chosenAssembler->intrinsicInit();
    if (chosenAssembler->inlineAsmInit)
        chosenAssembler->inlineAsmInit();
    if (chosenAssembler->outcode_init)
        chosenAssembler->outcode_init();
    if (chosenAssembler->enter_filename)
        chosenAssembler->enter_filename((char *)clist->data);
    if (cparams.prm_debug && chosenDebugger && chosenDebugger->init)
        chosenDebugger->init();
    if (cparams.prm_browse && chosenDebugger && chosenDebugger->init_browsedata)
        chosenDebugger->init_browsedata((char *)clist->data);
    browse_init();
    browse_startfile(infile, 0);
    if (cparams.prm_assemble)
    {
        lex = getsym();
        if (lex)
        {
            BLOCKDATA block;
            memset(&block, 0, sizeof(block));
            block.type = begin;
            while ((lex = statement_asm(lex, NULL, &block)) != NULL)
                ;
#ifndef PARSER_ONLY
            genASM(block.head);
#endif
        }
    }
    else
    {
#ifndef PARSER_ONLY
        asm_header((char *)clist->data, version);
#endif
        lex = getsym();
        if (lex)
        {
            while ((lex = declare(lex, NULL, NULL, sc_global, lk_none, NULL, true, false, false, ac_public)) != NULL)
                ;
        }
    }
#ifdef PARSER_ONLY
    ccDumpSymbols();
#endif
    if (!total_errors)
    {
        dumpInlines();
        dumpInitializers();
        dumpInlines();
        dumpStartups();
#ifndef PARSER_ONLY
        dumpLits();
#endif
        /*        rewrite_icode(); */
        if (chosenAssembler->gen->finalGen)
            chosenAssembler->gen->finalGen();
        if (!cparams.prm_assemble && cparams.prm_debug)
            if (chosenDebugger && chosenDebugger->outputtypedef)
                debug_dumptypedefs(globalNameSpace);
#ifndef PARSER_ONLY
        putexterns();
#endif
        if (!cparams.prm_asmfile)
            if (chosenAssembler->output_obj_file)
                chosenAssembler->output_obj_file();
    }
    findUnusedStatics(globalNameSpace);
    dumperrs(stdout);
    if (cparams.prm_debug && chosenDebugger && chosenDebugger->rundown)
        chosenDebugger->rundown();
    if (cparams.prm_browse && chosenDebugger && chosenDebugger->rundown_browsedata)
        chosenDebugger->rundown_browsedata();
#ifndef PARSER_ONLY
    if (!cparams.prm_assemble)
        asm_trailer();
#endif
}
/*-------------------------------------------------------------------------*/

void Cleanup()
{
    if (outputFile)
        fclose(outputFile);
    unlink(realOutFile);
    unlink(tempOutFile);
    rename(oldOutFile, realOutFile);
}
int main(int argc, char* argv[])
{
    char buffer[256];
    char* p;
    bool multipleFiles = false;
    bool openOutput = true;
    int rv;

    srand(time(0));

    /*   signal(SIGSEGV,internalError) ;*/
    /*   signal(SIGFPE, internalError) ;*/

    /* initialize back end */
    if (!init_backend(&argc, argv))
        fatal("Could not initialize back end");

    if (chosenAssembler->Args)
    {
        CMDLIST* newArgs = (CMDLIST *)calloc(sizeof(Args) + sizeof(Args[0]) * chosenAssembler->ArgCount, 1);
        if (newArgs)
        {
            memcpy(&newArgs[0], chosenAssembler->Args, chosenAssembler->ArgCount * sizeof(Args[0]));
            memcpy(&newArgs[chosenAssembler->ArgCount], &Args[0], sizeof(Args));
            ArgList = newArgs;
        }
    }
    /* parse environment variables, command lines, and config files  */
    ccinit(argc, argv);

    /* loop through and preprocess all the files on the file list */
    if (clist && clist->next)
        multipleFiles = true;
#ifdef PARSER_ONLY
    strcpy(buffer, (char *)clist->data);
    strcpy(realOutFile, outfile);
    outputfile(realOutFile, buffer, ".ods");
    if (!ccDBOpen(realOutFile))
        fatal("Cannot open database file %s", realOutFile);
#else
    BitInit();
    regInit();
#endif
    if (chosenAssembler->main_preprocess)
        openOutput = chosenAssembler->main_preprocess();
    while (clist)
    {
        cparams.prm_cplusplus = false;
        strcpy(buffer, (char *)clist->data);
#ifndef PARSER_ONLY
        if (buffer[0] == '-')
            strcpy(buffer, "a.c");
        strcpy(realOutFile, outfile);
        strcpy(tempOutFile, outfile);
        outputfile(tempOutFile, buffer, ".oo");
        if (cparams.prm_asmfile)
        {
            outputfile(realOutFile, buffer, chosenAssembler->asmext);
        }
        else
        {
            outputfile(realOutFile, buffer, chosenAssembler->objext);
        }
        strcpy(oldOutFile, realOutFile);
        StripExt(oldOutFile);
        AddExt(oldOutFile, ".tmp");
#else
        ccNewFile(buffer, true);
#endif
        AddExt(buffer, ".C");
        p = strrchr(buffer, '.');
        if (*(p - 1) != '.')
        {
            if (p[1] == 'h' || p[1] == 'H')  // compile H files as C++ for the IDE
                cparams.prm_cplusplus = true;
            if (p[1] == 'c' || p[1] == 'C')
                if (p[2] == 'p' || p[2] == 'P')
                {
                    if (p[3] == 'p' || p[3] == 'P')
                        cparams.prm_cplusplus = true;
                }
                else
                {
                    if (p[2] == 'x' || p[2] == 'X')
                    {
                        if (p[3] == 'x' || p[3] == 'X')
                            cparams.prm_cplusplus = true;
                    }
                }
            else if ((p[2] == 'c' || p[2] == 'C') && !p[3])
            {
                cparams.prm_cplusplus = true;
            }
            else
            {
                if (p[2] == '+')
                {
                    if (p[3] == '+')
                        cparams.prm_cplusplus = true;
                }
            }
        }
        if (cparams.prm_cplusplus)
            cparams.prm_c99 = cparams.prm_c1x = false;
        if (cparams.prm_cplusplus && chosenAssembler->msil)
            fatal("MSIL compiler does not compile C++ files at this time");
        if (*(char*)clist->data == '-')
            inputFile = stdin;
        else
            inputFile = SrchPth2(buffer, "", "r");
        if (!inputFile)
            fatal("Cannot open input file %s", buffer);
        strcpy(infile, buffer);
        if (cparams.prm_makestubs)
        {
            preprocini(infile, inputFile);
            if (chosenAssembler->enter_filename)
                chosenAssembler->enter_filename((char *)clist->data);
            MakeStubs();
        }
        else
        {
#ifndef PARSER_ONLY
            if (openOutput)
            {
                unlink(oldOutFile);
                rename(realOutFile, oldOutFile);
                outputFile = fopen(tempOutFile, cparams.prm_asmfile ? "w" : "wb");
                if (!outputFile)
                {
                    if (inputFile != stdin)
                        fclose(inputFile);
                    fatal("Cannot open output file %s", tempOutFile);
                }
                setvbuf(outputFile, 0, _IOFBF, 32768);
            }
#endif
            if (cparams.prm_cppfile)
            {
                StripExt(buffer);
                AddExt(buffer, ".i");
                strcpy(cppfile, buffer);
                cppFile = fopen(buffer, "w");
                if (!cppFile)
                {
                    if (inputFile != stdin)
                        fclose(inputFile);
                    fclose(outputFile);
                    fatal("Cannot open preprocessor output file %s", buffer);
                }
            }
            if (cparams.prm_listfile)
            {
                StripExt(buffer);
                AddExt(buffer, ".lst");
                listFile = fopen(buffer, "w");
                if (!listFile)
                {
                    if (inputFile != stdin)
                        fclose(inputFile);
                    fclose(cppFile);
                    fclose(outputFile);
                    fatal("Cannot open list file %s", buffer);
                }
            }
            if (cparams.prm_errfile)
            {
                StripExt(buffer);
                AddExt(buffer, ".err");
                errFile = fopen(buffer, "w");
                if (!errFile)
                {
                    if (inputFile != stdin)
                        fclose(inputFile);
                    fclose(cppFile);
                    fclose(listFile);
                    fclose(outputFile);
                    fatal("Cannot open error file %s", buffer);
                }
            }
            if (cparams.prm_browse)
            {
                char name[260];
                strcpy(name, realOutFile);
                StripExt(name);
                AddExt(name, ".cbr");
                browseFile = fopen(name, "wb");
                if (!browseFile)
                {
                    fclose(errFile);
                    if (inputFile != stdin)
                        fclose(inputFile);
                    fclose(cppFile);
                    fclose(listFile);
                    fclose(outputFile);
                    fatal("Cannot open browse file %s", buffer);
                }
                setvbuf(browseFile, 0, _IOFBF, 32768);
            }
            if (cparams.prm_icdfile)
            {
                StripExt(buffer);
                AddExt(buffer, ".icd");
                icdFile = fopen(buffer, "w");
                if (!icdFile)
                {
                    fclose(browseFile);
                    fclose(errFile);
                    if (inputFile != stdin)
                        fclose(inputFile);
                    fclose(cppFile);
                    fclose(listFile);
                    fclose(outputFile);
                    fatal("Cannot open error file %s", buffer);
                }
                setvbuf(icdFile, 0, _IOFBF, 32768);
            }

            if (multipleFiles && !cparams.prm_quiet)
                printf("%s\n", clist->data);

            compile(!openOutput);
        }
#ifdef PARSER_ONLY
        localFree();
#endif
        globalFree();
        if (cparams.prm_diag)
        {
            mem_summary();
            printf("Intermediate stats:\n");
            printf("  Block peak:          %d\n", maxBlocks);
            printf("  Temp peak:           %d\n", maxTemps);
            printf("  Allocation Spills:   %d\n", maxAllocationSpills);
            printf("  Allocation Passes:   %d\n", maxAllocationPasses);
            printf("  Allocation Accesses: %d\n", maxAllocationAccesses);
        }
        maxBlocks = maxTemps = maxAllocationSpills = maxAllocationPasses = maxAllocationAccesses = 0;
        if (inputFile != stdin)
#ifdef PARSER_ONLY
            ccCloseFile(inputFile);
#else
            fclose(inputFile);
#endif
        if (outputFile && openOutput)
            fclose(outputFile);
        outputFile = NULL;
        if (cppFile)
            fclose(cppFile);
        if (listFile)
            fclose(listFile);
        if (errFile)
            fclose(errFile);
        if (browseFile)
            fclose(browseFile);
        if (icdFile)
            fclose(icdFile);

        if (openOutput)
        {
            if (total_errors)
            {
                Cleanup();
            }
            else
            {
                unlink(oldOutFile);
                rename(tempOutFile, realOutFile);
            }
        }
        /* Flag to stop if there are any errors */
        stoponerr |= total_errors;

        clist = clist->next;
    }
    if (chosenAssembler->main_postprocess)
        chosenAssembler->main_postprocess(stoponerr);
    rv = !!stoponerr;
    if (!cparams.prm_makestubs)
    {
        if (!stoponerr)
        {
            rv = 0;
            if (chosenAssembler->compiler_postprocess)
            {
#ifdef _WIN32
                GetModuleFileNameA(NULL, buffer, sizeof(buffer));
#else
                strcpy(buffer, argv[0]);
#endif
                rv = chosenAssembler->compiler_postprocess(buffer);
            }
        }
        if (chosenAssembler->rundown)
            chosenAssembler->rundown();
    }
    return rv;
}
