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
#include <signal.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef MSIL
#    include "../version.h"
#else
#    include "../../version.h"
#endif
#if defined(_MSC_VER) || defined(BORLAND) || defined(__ORANGEC__)
#    include <io.h>
#endif

#ifdef __CCDL__
int _stklen = 100 * 1024;
#    ifdef MSDOS
int __dtabuflen = 32 * 1024;
#    endif
#endif

#if defined(WIN32) || defined(MICROSOFT)
extern "C" {
    char* __stdcall GetModuleFileNameA(void* handle, char* buf, int size);
}
#endif

extern COMPILER_PARAMS cparams;
extern int total_errors;
#ifndef CPREPROCESSOR
extern ARCH_ASM* chosenAssembler;
extern int diagcount;
extern NAMESPACEVALUES* globalNameSpace;
extern char infile[];

#endif

FILE* outputFile;
FILE* listFile;
char outfile[256];
char* prm_searchpath = 0;
char* sys_searchpath = 0;
char* prm_libpath = 0;
char version[256];
char copyright[256];
LIST* clist = 0;
int showBanner = true;
int showVersion = false;

static bool has_output_file;
static LIST *deflist = 0, *undeflist = 0;
static char** set_searchpath = &prm_searchpath;
static char** set_libpath = &prm_libpath;
void fatal(const char* fmt, ...)
{
    va_list argptr;

    va_start(argptr, fmt);
    fprintf(stderr, "Fatal error: ");
    vfprintf(stderr, fmt, argptr);
    va_end(argptr);
    extern void Cleanup();
    Cleanup();
    exit(1);
}
void banner(const char* fmt, ...)
{
    va_list argptr;

    va_start(argptr, fmt);
    vfprintf(stderr, fmt, argptr);
    va_end(argptr);

    putc('\n', stderr);
    putc('\n', stderr);
}

/* Print usage info */
void usage(char* prog_name)
{
    char* short_name;
    char* extension;

    short_name = strrchr(prog_name, '\\');
    if (short_name == NULL)
        short_name = strrchr(prog_name, '/');
    if (short_name == NULL)
        short_name = strrchr(prog_name, ':');
    if (short_name)
        short_name++;
    else
        short_name = prog_name;

    extension = strrchr(short_name, '.');
    if (extension != NULL)
        *extension = '\0';
    fprintf(stderr, "Usage: %s %s", short_name, getUsageText());
#ifndef CPREPROCESSOR
#    ifndef USE_LONGLONG
    fprintf(stderr, "   long long not supported");
#    endif
#endif

    exit(1);
}
int strcasecmp_internal(const char* left, const char* right)
{
    while (*left && *right)
    {
        if (toupper(*left) != toupper(*right))
            return true;
        left++, right++;
    }
    return *left != *right;
}
/*
 * If no extension, add the one specified
 */
void AddExt(char* buffer, const char* ext)
{
    char* pos = strrchr(buffer, '.');
    if (!pos || (*(pos - 1) == '.') || (*(pos + 1) == '\\'))
        strcat(buffer, ext);
}

/*
 * Strip extension, if it has one
 */
void StripExt(char* buffer)
{
    char* pos = strrchr(buffer, '.');
    if (pos && (*(pos - 1) != '.'))
        *pos = 0;
}

/*
 * Return path of EXE file
 */
void EXEPath(char* buffer, char* filename)
{
    char* temp;
    strcpy(buffer, filename);
    if ((temp = strrchr(buffer, '\\')) != 0)
        *(temp + 1) = 0;
    else
        buffer[0] = 0;
}

/*-------------------------------------------------------------------------*/

int HasExt(char* buffer, const char* ext)
{
    int l = strlen(buffer), l1 = strlen(ext);
    if (l1 < l)
    {
        return !strcasecmp_internal(buffer + l - l1, ext);
    }
    return 0;
}
/*
 * Pull the next path off the path search list
 */
static const char* parsepath(const char* path, char* buffer)
{
    const char* pos = path;

    /* Quit if hit a ';' */
    while (*pos)
    {
        if (*pos == ';')
        {
            pos++;
            break;
        }
        *buffer++ = *pos++;
    }
    *buffer = 0;

    /* Return a null pointer if no more data */
    if (*pos)
        return (pos);

    return (0);
}
/*
 * For each library:
 * Search local directory and all directories in the search path
 *  until it is found or run out of directories
 */
FILE* SrchPth3(char* string, const char* searchpath, const char* mode)
{
    FILE* in;
    const char* newpath = searchpath;

    /* If no path specified we search along the search path */
    if (string[0] != '\\' && string[1] != ':')
    {
        char buffer[200];
        while (newpath)
        {
            int n;
            ;
            /* Create a file name along this path */
            newpath = parsepath(newpath, buffer);
            n = strlen(buffer);
            if (n && buffer[n - 1] != '\\' && buffer[n - 1] != '/')
                strcat(buffer, "\\");
            strcat(buffer, (char*)string);

            /* Check this path */
            in = fopen(buffer, mode);
            if (in != NULL)
            {
                strcpy(string, buffer);
                return (in);
            }
        }
    }
    else
    {
        in = fopen((char*)string, mode);
        if (in != NULL)
        {
            return (in);
        }
    }
    return (NULL);
}
/* this ditty takes care of the fact that on DOS
 * (and on dos shells under NT/XP)
 * the filenames are limited to 8.3 notation
 * which is a problem because the C++ runtime has long file names
 * while we could add RTL support for long filenamse on DOS, that doesn't help on XP/NT
 *
 * so first we search for the full filename, if that fails for the ~1 version, and if that
 * fails for the truncated 8.3 version
 */
FILE* ccOpenFile(const char* string, FILE* fil, const char* mode);
FILE* SrchPth2(char* name, const char* path, const char* attrib)
{
    FILE* rv = SrchPth3(name, path, attrib);
#ifdef PARSER_ONLY
    rv = ccOpenFile(name, rv, attrib);
#endif
#ifdef MSDOS
    char buf[256], *p;
    if (rv != -1)
        return rv;
    p = strrchr(name, '.');
    if (!p)
        p = name + strlen(name);
    if (p - name < 9)
        return rv;
    strcpy(buf, name);
    strcpy(buf + 6, "~1");
    strcpy(buf + 8, p);
    rv = SrchPth3(buf, path, attrib);
    if (rv != -1)
    {
        strcpy(name, buf);
        return rv;
    }
    strcpy(buf, name);
    strcpy(buf + 8, p);
    rv = SrchPth3(name, path, attrib);
    if (rv != -1)
    {
        strcpy(name, buf);
        return rv;
    }
    return -1;
#else
    return rv;
#endif
}

/*-------------------------------------------------------------------------*/

FILE* SrchPth(char* name, const char* path, const char* attrib, bool sys)
{
    FILE* rv = SrchPth2(name, path, attrib);
    char buf[265], *p;
    if (rv || !sys)
        return rv;
    strcpy(buf, name);
    p = strrchr(buf, '.');
    if (p && !strcasecmp_internal(p, ".h"))
    {
        *p = 0;
        rv = SrchPth2(buf, path, attrib);
        if (rv)
            strcpy(name, buf);
    }
    return rv;
}

extern CMDLIST* ArgList;
/*int parseParam(int bool, char *string); */

static int use_case; /* Gets set for case sensitivity */

/*
 * Function that unlinks the argument from che argv[] chain
 */
static void remove_arg(int pos, int* count, char* list[])
{
    int i;

    /* Decrement length of list */
    (*count)--;

    /* move items down */
    for (i = pos; i < *count; i++)
        list[i] = list[i + 1];
}

/*
 * ompare two characters, ignoring case if necessary
 */
static int cmatch(char t1, char t2)
{
    if (use_case)
        return (t1 == t2);

    return (toupper(t1) == toupper(t2));
}

/* Routine scans a string to see if any of the characters match
 *  the arguments, then dispatches to the action routine if so.
 */
/* Callbacks of the form
 *   void boolcallback( char selectchar, int value)
 *   void switchcallback( char selectchar, int value)  ;; value always true
 *   void stringcallback( char selectchar, char *string)
 */
static int scan_args(char* string, int index, char* arg)
{
    int i = -1;
    bool legacyArguments = !!getenv("OCC_LEGACY_OPTIONS");
    while (ArgList[++i].id)
    {
        switch (ArgList[i].mode)
        {
            case ARG_SWITCHSTRING:
                if (cmatch(string[index], ArgList[i].id))
                {
                    (*ArgList[i].routine)(string[index], &string[index]);
                    return (ARG_NEXTCHAR);
                }
                break;
            case ARG_SWITCH:
                if (cmatch(string[index], ArgList[i].id))
                {
                    (*ArgList[i].routine)(string[index], (char*)true);
                    return (ARG_NEXTCHAR);
                }
                break;
            case ARG_BOOL:
                if (cmatch(string[index], ArgList[i].id))
                {
                    if (!legacyArguments || string[0] == ARG_SEPtrue || string[0] == '/')
                        (*ArgList[i].routine)(string[index], (char*)true);
                    else
                        (*ArgList[i].routine)(string[index], (char*)false);
                    return (ARG_NEXTCHAR);
                }
                break;
            case ARG_CONCATSTRING:
                if (cmatch(string[index], ArgList[i].id))
                {
                    (*ArgList[i].routine)(string[index], string + index + 1);
                    return (ARG_NEXTARG);
                }
                break;
            case ARG_NOCONCATSTRING:
                if (cmatch(string[index], ArgList[i].id))
                {
                    if (!arg)
                        return (ARG_NOARG);
                    (*ArgList[i].routine)(string[index], arg);
                    return (ARG_NEXTNOCAT);
                }
                break;
            case ARG_COMBINESTRING:
                if (cmatch(string[index], ArgList[i].id))
                {
                    if (string[index + 1])
                    {
                        (*ArgList[i].routine)(string[index], string + index + 1);
                        return (ARG_NEXTARG);
                    }
                    else
                    {
                        if (!arg)
                            return (ARG_NEXTARG);
                        (*ArgList[i].routine)(string[index], arg);
                        return (ARG_NEXTNOCAT);
                    }
                }
                break;
        }
    }
    return (ARG_NOMATCH);
}

/*
 * Main parse routine.  Scans for '-', then scan for arguments and
 * delete from the argv[] array if so.
 */
bool parse_args(int* argc, char* argv[], bool case_sensitive)
{

    int pos = 0;
    bool retval = true;
    use_case = case_sensitive;

    while (++pos < *argc)
    {
        if ((argv[pos][0] == ARG_SEPSWITCH) || (argv[pos][0] == ARG_SEPfalse) || (argv[pos][0] == ARG_SEPtrue))
        {
            if (argv[pos][1] == '!' || !strcmp(argv[pos], "--nologo"))
            {
                // skip the silence arg
            }
            else if (argv[pos][0] == ARG_SEPfalse && !argv[pos][1])
            {
                continue;
            }
            else
            {
                int argmode;
                int index = 1;
                bool done = false;
                do
                {
                    /* Scan the present arg */
                    if (pos < *argc - 1)
                        argmode = scan_args(argv[pos], index, argv[pos + 1]);
                    else
                        argmode = scan_args(argv[pos], index, 0);

                    switch (argmode)
                    {
                        case ARG_NEXTCHAR:
                            /* If it was a char, go to the next one */
                            if (!argv[pos][++index])
                                done = true;
                            break;
                        case ARG_NEXTNOCAT:
                            /* Otherwise if it was a nocat, remove the extra arg */
                            remove_arg(pos, argc, argv);
                            /* Fall through to NEXTARG */
                        case ARG_NEXTARG:
                            /* Just a next arg, go do it */
                            done = true;
                            break;
                        case ARG_NOMATCH:
                            /* No such arg, spit an error  */
#ifndef CPREPROCESSOR
#    ifdef XXXXX
                            switch (parseParam(argv[pos][index] != ARG_SEPfalse, &argv[pos][index + 1]))
                            {
                                case 0:
#    endif
#endif
                                    fprintf(stderr, "Invalid Arg: %s\n", argv[pos]);
                                    retval = false;
                                    done = true;
#ifndef CPREPROCESSORXX
#    ifdef XXXXX
                                    break;
                                case 1:
                                    if (!argv[pos][++index])
                                        done = true;
                                    break;
                                case 2:
                                    done = true;
                                    break;
                            }
#    endif
#endif
                            break;
                        case ARG_NOARG:
                            /* Missing the arg for a CONCAT type, spit the error */
                            fprintf(stderr, "Missing string for Arg %s\n", argv[pos]);
                            done = true;
                            retval = false;
                            break;
                    };

                } while (!done);
            }
            /* We'll always get rid of the present arg
             * And back up one
             */
            remove_arg(pos--, argc, argv);
        }
    }
    return (retval);
}
/*-------------------------------------------------------------------------*/

void err_setup(char select, char* string)
/*
 * activation for the max errs argument
 */
{
    int n;
    (void)select;
    if (*string == '+')
    {
        cparams.prm_extwarning = true;
        string++;
    }
    else if (*string == '-')
    {
        cparams.prm_warning = false;
        string++;
    }
    n = atoi(string);
    if (n > 0)
        cparams.prm_maxerr = n;
    DisableTrivialWarnings();
}
void warning_setup(char select, char* string)
{
    if (string[0] == 0)
        AllWarningsDisable();
    else
        switch (string[0])
        {
            case '+':
                cparams.prm_extwarning = true;
                DisableTrivialWarnings();
                break;
            case 'd':
                DisableWarning(atoi(string + 1));
                break;
            case 'o':
                WarningOnlyOnce(atoi(string + 1));
                break;
            case 'x':
                AllWarningsAsError();
                break;
            case 'e':
                if (!strcmp(string, "error"))
                    AllWarningsAsError();
                else
                    WarningAsError(atoi(string + 1));
                break;
            default:
                EnableWarning(atoi(string));
                break;
        }
}

/*-------------------------------------------------------------------------*/

void sysincl_setup(char select, char* string)
{
    (void)select;
    if (sys_searchpath)
    {
        sys_searchpath = (char *)realloc(sys_searchpath, strlen(string) + strlen(sys_searchpath) + 2);
        strcat(sys_searchpath, ";");
    }
    else
    {
        sys_searchpath = (char *)malloc(strlen(string) + 1);
        sys_searchpath[0] = 0;
    }
    fflush(stdout);
    strcat(sys_searchpath, string);
}
void incl_setup(char select, char* string)
/*
 * activation for include paths
 */
{
    (void)select;
    if (*set_searchpath)
    {
        *set_searchpath = (char *)realloc(*set_searchpath, strlen(string) + strlen(*set_searchpath) + 2);
        strcat(*set_searchpath, ";");
    }
    else
    {
        *set_searchpath = (char *)malloc(strlen(string) + 1);
        *set_searchpath[0] = 0;
    }
    fflush(stdout);
    strcat(*set_searchpath, string);
}
void libpath_setup(char select, char* string)
{
    (void)select;
    if (*set_libpath)
    {
        *set_libpath = (char *)realloc(*set_libpath, strlen(string) + strlen(*set_libpath) + 2);
        strcat(*set_libpath, ";");
    }
    else
    {
        *set_libpath = (char *)malloc(strlen(string) + 1);
        *set_libpath[0] = 0;
    }
    fflush(stdout);
    strcat(*set_libpath, string);
}
void tool_setup(char select, char* string)
{
    char buf[2048];
    buf[0] = '$';
    strcpy(buf + 1, string);
    InsertAnyFile(buf, 0, -1, false);
}
/*-------------------------------------------------------------------------*/

void def_setup(char select, char* string)
/*
 * activation for command line #defines
 */
{
    char* s = (char *)malloc(strlen(string) + 1);
    LIST* l = (LIST *)malloc(sizeof(LIST));
    (void)select;
    strcpy(s, string);
    l->next = deflist;
    deflist = l;
    l->data = s;
}

void undef_setup(char select, char* string)
{
    char* s = (char *)malloc(strlen(string) + 1);
    LIST* l = (LIST *)malloc(sizeof(LIST));
    (void)select;
    strcpy(s, string);
    l->next = undeflist;
    undeflist = l;
    l->data = s;
}

/*-------------------------------------------------------------------------*/

void output_setup(char select, char* string)
{
    (void)select;
    strcpy(outfile, string);
    has_output_file = true;
}

/*-------------------------------------------------------------------------*/

void setglbdefs(void)
/*
 * function declares any global defines from the command line and also
 * declares a couple of other macros so we can tell what the compiler is
 * doing
 */
{
#ifndef CPREPROCESSOR
    ARCH_DEFINES* a = chosenAssembler->defines;
#endif
    LIST* l = deflist;
    char buf[256];
    int major, temp, minor, build;
    while (l)
    {
        char* s = (char *)l->data;
        char* n = s;
        while (*s && *s != '=')
            s++;
        if (*s == '=')
            *s++ = 0;
        if (*s)
        {
            char* q = (char *)calloc(1, strlen(s) + 3);
            q[0] = MACRO_PLACEHOLDER;
            strcpy(q + 1, s);
            q[strlen(s) + 1] = MACRO_PLACEHOLDER;
            glbdefine(n, q, false);
            free(q);
        }
        else
        {
            glbdefine(n, s, false);
        }
        if (*s)
            s[-1] = '=';
        l = l->next;
    }
    l = undeflist;
    while (l)
    {
        char* s = (char *)l->data;
        char* n = s;
        while (*s && *s != '=')
            s++;
        if (*s == '=')
            *s = 0;
        glbUndefine(n);
        l = l->next;
    }
    sscanf(STRING_VERSION, "%d.%d.%d.%d", &major, &temp, &minor, &build);
    my_sprintf(buf, "%d", major * 100 + minor);
    glbdefine("__ORANGEC__", buf, true);
    my_sprintf(buf, "%d", major);
    glbdefine("__ORANGEC_MAJOR__", buf, true);
    my_sprintf(buf, "%d", minor);
    glbdefine("__ORANGEC_MINOR__", buf, true);
    my_sprintf(buf, "%d", build);
    glbdefine("__ORANGEC_PATCHLEVEL__", buf, true);
    sprintf(buf, "\"%s\"", STRING_VERSION);
    glbdefine("__VERSION__", buf, true);
    glbdefine("__CHAR_BIT__", "8", true);
    if (cparams.prm_cplusplus)
    {
        glbdefine("__cplusplus", "201402", true);
        if (cparams.prm_xcept)
            glbdefine("__RTTI__", "1", true);
    }
    glbdefine("__STDC__", "1", true);

    if (cparams.prm_c99 || cparams.prm_c1x)
    {
#ifndef CPREPROCESSOR
        glbdefine("__STDC_HOSTED__", chosenAssembler->hosted, true);  // hosted compiler, not embedded
#endif
    }
    if (cparams.prm_c1x)
    {
        glbdefine("__STDC_VERSION__", "201112L", true);
    }
    else if (cparams.prm_c99)
    {
        glbdefine("__STDC_VERSION__", "199901L", true);
    }
    /*   glbdefine("__STDC_IEC_599__","1");*/
    /*   glbdefine("__STDC_IEC_599_COMPLEX__","1");*/
    /*   glbdefine("__STDC_ISO_10646__","199712L");*/
/*    glbdefine(GLBDEFINE, "");*/
#ifndef CPREPROCESSOR
    if (a)
    {
        while (a->define)
        {
            if (a->respect)
            {
                glbdefine(a->define, a->value, a->permanent);
            }
            a++;
        }
    }
#endif
}

/*-------------------------------------------------------------------------*/

void InsertOneFile(char* filename, char* path, int drive, bool primary)
/*
 * Insert a file name onto the list of files to process
 */

{
    char a = 0;
    char *newbuffer, buffer[260], *p = buffer;
    bool inserted;
    LIST **r = &clist, *s;

    if (drive != -1)
    {
        *p++ = (char)(drive + 'A');
        *p++ = ':';
    }
    if (path)
    {
        strcpy(p, path);
        //        strcat(p, "\\");
    }
    else
        *p = 0;
    /* Allocate buffer and make .C if no extension */
    strcat(buffer, filename);
#ifndef CPREPROCESSOR
    if (buffer[0] == '-')
    {
        a = buffer[0];
        buffer[0] = 'a';
    }
    inserted = chosenAssembler->insert_noncompile_file && chosenAssembler->insert_noncompile_file(buffer, primary);
    if (a)
        buffer[0] = a;
    if (!inserted)
#endif
    {
        AddExt(buffer, ".c");
        newbuffer = (char*)malloc(strlen(buffer) + 1);
        if (!newbuffer)
            return;
        strcpy(newbuffer, buffer);

        while ((*r))
            r = &(*r)->next;
        (*r) = (LIST *) malloc(sizeof(LIST));
        s = (*r);
        if (!s)
            return;
        s->next = 0;
        s->data = newbuffer;
    }
}
void InsertAnyFile(char* filename, char* path, int drive, bool primary)
{
    char drv[256], dir[256], name[256], ext[256];
#if defined(_MSC_VER) || defined(BORLAND) || defined(__ORANGEC__)
    struct _finddata_t findbuf;
    size_t n;
    _splitpath(filename, drv, dir, name, ext);
    n = _findfirst(filename, &findbuf);
    if (n != -1)
    {
        do
        {
            InsertOneFile(findbuf.name, dir[0] ? dir : 0, drv[0] ? tolower(drv[0]) - 'a' : -1, primary);
        } while (_findnext(n, &findbuf) != -1);
        _findclose(n);
    }
    else
    {
        InsertOneFile(filename, path, drive, primary);
    }
#else
    InsertOneFile(filename, path, drive, primary);
#endif
}
/*-------------------------------------------------------------------------*/

void dumperrs(FILE* file);
void setfile(char* buf, const char* orgbuf, const char* ext)
/*
 * Get rid of a file path an add an extension to the file name
 */
{
    const char* p = strrchr(orgbuf, '\\');
    const char* p1 = strrchr(orgbuf, '/');
    if (p1 > p)
        p = p1;
    else if (!p)
        p = p1;
    if (!p)
        p = orgbuf;
    else
        p++;
    strcpy(buf, p);
    StripExt(buf);
    strcat(buf, ext);
}

/*-------------------------------------------------------------------------*/

void outputfile(char* buf, const char* orgbuf, const char* ext)
{

    if (buf[strlen(buf) - 1] == '\\')
    {
        const char* p = strrchr(orgbuf, '\\');
        if (p)
            p++;
        else
            p = orgbuf;
        strcat(buf, p);
        StripExt(buf);
        AddExt(buf, ext);
    }
    else if (has_output_file)
    {
        AddExt(buf, ext);
    }
    else
    {
        setfile(buf, orgbuf, ext);
    }
}

/*-------------------------------------------------------------------------*/

void scan_env(char* output, char* string)
{
    char name[256], *p;
    while (*string)
    {
        if (*string == '%')
        {
            p = name;
            string++;
            while (*string && *string != '%')
                *p++ = *string++;
            if (*string)
                string++;
            *p = 0;
            p = getenv(name);
            if (p)
            {
                strcpy(output, p);
                output += strlen(output);
            }
        }
        else
            *output++ = *string++;
    }
    *output = 0;
}

/*-------------------------------------------------------------------------*/

int parse_arbitrary(char* string)
/*
 * take a C string and and convert it to ARGC, ARGV format and then run
 * it through the argument parser
 */
{
    char* argv[40];
    char output[1024];
    int rv, i;
    int argc = 1;
    if (!string || !*string)
        return 1;
    scan_env(output, string);
    string = output;
    while (1)
    {
        int quoted = ' ';
        while (*string == ' ')
            string++;
        if (!*string)
            break;
        if (*string == '\"')
            quoted = *string++;
        argv[argc++] = string;
        while (*string && *string != quoted)
            string++;
        if (!*string)
            break;
        *string = 0;
        string++;
    }
    rv = parse_args(&argc, argv, true);
    for (i = 1; i < argc; i++)
        InsertAnyFile(argv[i], 0, -1, true);
    return rv;
}

/*-------------------------------------------------------------------------*/

void parsefile(char select, char* string)
/*
 * parse arguments from an input file
 */
{
    FILE* temp = fopen(string, "r");
    (void)select;
    if (!temp)
        fatal("Response file not found");
    while (!feof(temp))
    {
        char buf[256];
        buf[0] = 0;
        fgets(buf, 256, temp);
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0;
        if (!parse_arbitrary(buf))
            break;
    }
    fclose(temp);
}

/*-------------------------------------------------------------------------*/

void addinclude(void)
/*
 * Look up the INCLUDE environment variable and append it to the
 * search path
 */
{
#ifdef COLDFIRE
    char* string = getenv("cfccincl");
#else
    char* string = getenv("CCINCL");
#endif
    if (string && string[0])
    {
        char temp[1000];
        strcpy(temp, string);
        if (*set_searchpath)
        {
            strcat(temp, ";");
            strcat(temp, *set_searchpath);
            free(*set_searchpath);
        }
        *set_searchpath = (char *)malloc(strlen(temp) + 1);
        strcpy(*set_searchpath, temp);
    }
    string = getenv("CPATH");
    if (string && string[0])
    {
        char temp[1000];
        strcpy(temp, string);
        if (*set_searchpath)
        {
            strcat(temp, ";");
            strcat(temp, *set_searchpath);
            free(*set_searchpath);
        }
        *set_searchpath = (char *)(strlen(temp) + 1);
        strcpy(*set_searchpath, temp);
    }
}

/*-------------------------------------------------------------------------*/

int parseenv(const char* name)
/*
 * Parse the environment argument string
 */
{
    char* string = getenv(name);
    return parse_arbitrary(string);
}

/*-------------------------------------------------------------------------*/

int parseconfigfile(char* name)
{
    char buf[256], *p;
#ifndef CPREPROCESSOR
    if (!chosenAssembler->cfgname)
        return 0;
#endif
    strcpy(buf, name);
    p = strrchr(buf, '\\');
    if (p)
    {
        FILE* temp;
#ifdef CPREPROCESSOR
        strcpy(p + 1, "CPP");
#else
        strcpy(p + 1, "CC");
        strcpy(p + 1, chosenAssembler->cfgname);
#endif
        strcat(p, ".CFG");
        temp = fopen(buf, "r");
        if (!temp)
            return 0;
        set_searchpath = &sys_searchpath;
        while (!feof(temp))
        {
            buf[0] = 0;
            fgets(buf, 256, temp);
            if (buf[strlen(buf) - 1] == '\n')
                buf[strlen(buf) - 1] = 0;
            if (!parse_arbitrary(buf))
                break;
        }
        set_searchpath = &prm_searchpath;
        fclose(temp);
    }
    return 0;
}

/*-------------------------------------------------------------------------*/

void dumperrs(FILE* file)
{
#ifndef CPREPROCESSOR
    if (cparams.prm_listfile)
    {
        fprintf(listFile, "******** Global Symbols ********\n");
        list_table(globalNameSpace->syms, 0);
        fprintf(listFile, "******** Global Tags ********\n");
        list_table(globalNameSpace->tags, 0);
    }
    if (diagcount && !total_errors)
        fprintf(file, "%d Diagnostics\n", diagcount);
#endif
    if (total_errors)
        fprintf(file, "%d Errors\n", total_errors);
}

/*-------------------------------------------------------------------------*/

void ctrlchandler(int aa)
{
    fprintf(stderr, "^C");
    extern void Cleanup();
    Cleanup();
    exit(1);
}

/*-------------------------------------------------------------------------*/

void internalError(int a)
{
    (void)a;
    extern void Cleanup();
    Cleanup();
    fprintf(stderr, "Internal Error - Aborting compile");
    exit(1);
}

/*-------------------------------------------------------------------------*/
void ccinit(int argc, char* argv[])
{
    char buffer[260];
    char* p;
    int rv;
    int i;

    strcpy(copyright, COPYRIGHT);
    strcpy(version, STRING_VERSION);

    outfile[0] = 0;
    for (i = 1; i < argc; i++)
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            if (argv[i][1] == '!' || !strcmp(argv[i], "--nologo"))
            {
                showBanner = false;
            }
            else if ((argv[i][1] == 'V' && argv[i][2] == 0) || !strcmp(argv[i], "--version"))
            {
                showVersion = true;
            }
        }

    if (showBanner || showVersion)
    {
#ifdef CPREPROCESSOR
        banner("CPP Version %s %s", version, copyright);
#else
        banner("%s Version %s %s", chosenAssembler->progname, version, copyright);
#endif
    }
    if (showVersion)
    {
        fprintf(stderr, "Compile date: " __DATE__ ", time: " __TIME__ "\n");
        exit(0);
    }
#if defined(WIN32) || defined(MICROSOFT)
    GetModuleFileNameA(NULL, buffer, sizeof(buffer));
#else
    strcpy(buffer, argv[0]);
#endif

    if (!getenv("ORANGEC"))
    {
        char* p = strrchr(buffer, '\\');
        if (p)
        {
            *p = 0;
            char* q = strrchr(buffer, '\\');
            if (q)
            {
                *q = 0;
                char* buf1 = (char*)calloc(1, strlen("ORANGEC") + strlen(buffer) + 2);
                strcpy(buf1, "ORANGEC");
                strcat(buf1, "=");
                strcat(buf1, buffer);
                putenv(buf1);
                *q = '\\';
            }
            *p = '\\';
        }
    }
    DisableTrivialWarnings();
    /* parse the environment and command line */
#ifndef CPREPROCESSOR
    if (chosenAssembler->envname && !parseenv(chosenAssembler->envname))
        usage(argv[0]);
#endif

    parseconfigfile(buffer);
    if (!parse_args(&argc, argv, true) || (!clist && argc == 1))
        usage(argv[0]);

    /* tack the environment includes in */
    addinclude();

    /* Scan the command line for file names or response files */
    {
        int i;
        for (i = 1; i < argc; i++)
            if (argv[i][0] == '@')
                parsefile(0, argv[i] + 1);
            else
                InsertAnyFile(argv[i], 0, -1, true);
    }

#ifndef PARSER_ONLY

    if (has_output_file)
    {
#    ifndef CPREPROCESSOR
        if (chosenAssembler->insert_output_file)
            chosenAssembler->insert_output_file(outfile);
#    endif
        if (!cparams.prm_compileonly)
        {
            has_output_file = false;
        }
        else
        {
            if (clist && clist->next && outfile[strlen(outfile) - 1] != '\\')
                fatal("Cannot specify output file for multiple input files\n");
        }
    }
#else
    {
        LIST* t = clist;
        while (t)
        {
            t->data = litlate(fullqualify((char *)t->data));
            t = t->next;
        }
    }
#endif

    /* Set up a ctrl-C handler so we can exit the prog with cleanup */
    signal(SIGINT, ctrlchandler);
    //    signal(SIGSEGV, internalError);
}
