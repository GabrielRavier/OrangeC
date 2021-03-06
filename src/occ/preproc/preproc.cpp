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
#include "sys/stat.h"
//#include "dir.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include <io.h>
extern "C" char* getcwd(char*, int);
#endif

#ifdef PARSER_ONLY
size_t ccReadFile(void* __ptr, size_t __size, size_t __n, FILE* __stream);
void ccSetFileLine(char* filename, int lineno);
void ccNewFile(char* fileName, bool main);
#endif
#ifndef CPREPROCESSOR
extern ARCH_ASM* chosenAssembler;
extern char infile[];
extern int ignore_global_init;
extern LINEDATA *linesHead, *linesTail;
#endif
extern COMPILER_PARAMS cparams;
extern char *prm_searchpath, *sys_searchpath;
extern char version[];

extern FILE* cppFile;

int preprocLine;
char* preprocFile;
LIST* nonSysIncludeFiles;

HASHTABLE* defsyms;
INCLUDES *includes, *inclData;
int stdpragmas;

FILELIST *incfiles = 0, *lastinc;
LIST* libincludes = 0;
void ppdefcheck(unsigned char* line);

MACROLIST* macroBuffers = NULL;

int cppprio;
int packdata[MAX_PACK_DATA] = {1};
int packlevel;

void filemac(char* string);
void datemac(char* string);
void dateisomac(char* string);
void timemac(char* string);
void linemac(char* string);
void countermac(char* string);

static int instr = 0;
static int commentlevel, commentline;
static char defkw[] = "defined";
static int currentfile;
static time_t source_date_epoch = (time_t)-1;
static int counter;

#ifndef CPREPROCESSOR
#    define ONCE_BUCKETS 32
typedef struct _once
{
    struct _once* next;
    long filesize;
    time_t filetime;
    unsigned crc;
} ONCE;
static ONCE* onceLists[ONCE_BUCKETS];
static int once;
#endif

/* List of standard macros */
#define INGROWNMACROS 6

struct inmac
{
    const char* s;
    void (*func)(char*);
} ingrownmacros[INGROWNMACROS] = {{"__FILE__", filemac},
                                  {
                                      "__DATE__",
                                      datemac,
                                  },
                                  {
                                      "__DATEISO__",
                                      dateisomac,
                                  },
                                  {"__TIME__", timemac},
                                  {"__LINE__", linemac},
                                  {"__COUNTER__", countermac}};

void pushif(void), popif(void);

/* Moudle init */
void preprocini(char* name, FILE* fil)
{
    INCLUDES* p = (INCLUDES *)globalAlloc(sizeof(INCLUDES));
    p->fname = litlate(name);
    p->handle = fil;
    p->first = true;
    p->realline = -1;
    p->line = -1;
    includes = p;
    inclData = NULL;
    incfiles = NULL;
    libincludes = NULL;
    nonSysIncludeFiles = NULL;
    stdpragmas = STD_PRAGMA_FCONTRACT;
    defsyms = CreateHashTable(GLOBALHASHSIZE);
    macroBuffers = NULL;
    counter = 0;
#ifndef CPREPROCESSOR
    once = 0;
    packlevel = 0;
    packdata[0] = chosenAssembler->arch->packSize;
    memset(onceLists, 0, sizeof(onceLists));
    cppprio = 0;
#endif
    char* sde = getenv("SOURCE_DATE_EPOCH");
    if (sde)
        source_date_epoch = (time_t)strtoul(sde, NULL, 10);
}
int defid(char* name, unsigned char** p)
/*
 * Get an identifier during macro replacement
 */
{
    int count = 0;
    while (issymchar(**p))
    {
        if (count < SYMBOL_NAME_LEN)
        {
            name[count++] = (char)*(*p);
        }
        (*p)++;
    }
    name[count] = 0;
    return (count);
}
void skipspace(void)
{
    while (isspace((unsigned char)*includes->lptr) || *includes->lptr == MACRO_PLACEHOLDER)
        includes->lptr++;
}
bool expectid(char* buf)
{
    if (isstartchar((unsigned char)*includes->lptr))
    {
        defid(buf, &includes->lptr);
        return true;
    }
    pperror(ERR_IDENTIFIER_EXPECTED, 0);
    return false;
}
bool expectstring(char* buf, unsigned char** in, bool angle)
{
    skipspace();
    if (**in == '"' || (angle && **in == '<'))
    {
        char s = *(*in)++;
        if (s == '<')
            s = '>';
        while (**in && s != **in)
            *buf++ = *(*in)++;
        *buf = 0;
        if (**in)
        {
            (*in)++;
            return true;
        }
    }
    pperror(ERR_NEEDSTRING, 0);
    return false;
}
PPINT expectnum(bool* uns)
{
    bool minus = false;
    while (isspace(*includes->lptr))
        includes->lptr++;
    if (*includes->lptr == '-')
    {
        includes->lptr++;
        minus = true;
    }
#ifdef USE_LONGLONG
    LLONG_TYPE rv = strtoull((char*)includes->lptr, (char**)&includes->lptr, 0);
#else
    LLONG_TYPE rv = strtoul((char*)includes->lptr, (char**)&includes->lptr, 0);
#endif
    if (minus)
        rv = -rv;
    if (uns)
    {
        *uns = false;
        if (tolower(includes->lptr[-1]) == 'u')
            *uns = true;
    }
    while (*includes->lptr == 'l' || *includes->lptr == 'L' || *includes->lptr == 'u' || *includes->lptr == 'U')
    {
        if (uns && tolower(*includes->lptr) == 'u')
            *uns = true;
        includes->lptr++;
    }
    return rv;
}

static void lineToCpp(void)
/*
 * line has been preprocessed, dump it to a file
 */
{
    if (cppFile)
    {
        unsigned char* p = includes->inputline;
        /* this isn't quite kosher, because, for example
         * #define FFLUSH -2
         * int a = -FFLUSH;
         * won't compile correctly from a file that has been generated from here
         */
        while (*p)
        {
            if (*p != MACRO_PLACEHOLDER)
                fputc(*p++, cppFile);
            else
                p++;
        }
    }
}
/* Strips comments */
static void stripcomment(unsigned char* line)
{
    unsigned char *s = line, *e = s;
    while (*e)
    {
        if (!instr)
        {
            if (!commentlevel)
            {
                if (*e == '/')
                {
                    if (*(e + 1) == '*')
                    {
                        e += 2;
                        *s++ = ' ';
                        commentlevel = 1;
                        commentline = includes->line;
                        continue;
                    }
                    else if (*(e + 1) == '/' && (!cparams.prm_ansi || cparams.prm_c99 || cparams.prm_cplusplus))
                    {
                        *s++ = '\n';
                        *s = 0;
                        return;
                    }
                }
                else if (*e == '"' || *e == '\'')
                    instr = *e;
            }
            else
            {
                if (*e == '/' && *(e + 1) == '*')
                {
                    error(ERR_NESTEDCOMMENTS);
                }
                if (*e == '*')
                {
                    if (*(e + 1) == '/')
                    {
                        commentlevel = 0;
                        e++;
                    }
                }
                e++;
                continue;
            }
        }
        else if (!commentlevel && *e == instr)
        {
            int count = 0;
            while (s - count > line && *(s - count - 1) == '\\')
                count++;
            if (!(count & 1))
                instr = 0;
        }
        *s++ = *e++;
    }
    *s = 0;
}

/* strip digraphs */
void stripdigraph(unsigned char* buf)
{
    unsigned char* cp = buf;
    while (*cp)
    {
        if (*cp == '<' && *(cp + 1) == ':')
        {
            cp += 2;
            *buf++ = '[';
        }
        else if (*cp == ':' && *(cp + 1) == '>')
        {
            cp += 2;
            *buf++ = ']';
        }
        else if (*cp == '<' && *(cp + 1) == '%')
        {
            cp += 2;
            *buf++ = '{';
        }
        else if (*cp == '%' && *(cp + 1) == '>')
        {
            cp += 2;
            *buf++ = '}';
        }
        else if (*cp == '%' && *(cp + 1) == ':')
        {
            cp += 2;
            *buf++ = '#';
            if (*cp == '%' && *(cp + 1) == ':')
            {
                cp += 2;
                *buf++ = '#';
            }
        }
        else
            *buf++ = *cp++;
    }
    *buf = 0;
}

/* strip trigraphs */
void striptrigraph(unsigned char* buf)
{
    unsigned char* cp = buf;
    while (*cp)
    {
        if (*cp == '?' && *(cp + 1) == '?')
        {
            cp += 2;
            switch (*cp++)
            {
                case '=':
                    *buf++ = '#';
                    break;
                case '(':
                    *buf++ = '[';
                    break;
                case '/':
                    *buf++ = '\\';
                    break;
                case ')':
                    *buf++ = ']';
                    break;
                case '\'':
                    *buf++ = '^';
                    break;
                case '<':
                    *buf++ = '{';
                    break;
                case '!':
                    *buf++ = '|';
                    break;
                case '>':
                    *buf++ = '}';
                    break;
                case '-':
                    *buf++ = '~';
                    break;
                default:
                    cp -= 3;
                    *buf++ = *cp++;
                    break;
            }
        }
        else
            *buf++ = *cp++;
    }
    *buf = 0;
}
int getstring(unsigned char* s, int len, FILE* file)
{
    unsigned char* olds = s;
    if (!file)
    {
        *s = 0;
        includes->inputlen = 0;
        return s == olds;
    }
    while (true)
    {
        while (includes->inputlen--)
        {
            if (*includes->ibufPtr == 0x1a)
            {
                *s = 0;
                includes->inputlen = 0;
                return s == olds;
            }
            if (*includes->ibufPtr != '\r')
            {
                if ((*s++ = *includes->ibufPtr++) == '\n' || !--len)
                {
                    *s = 0;
                    return 0;
                }
            }
            else
                includes->ibufPtr++;
        }
        if (file == stdin)
        {
            char* p = fgets((char *)includes->inputbuffer, sizeof(includes->inputbuffer), file);
            if (p)
                includes->inputlen = strlen(p);
            else
                includes->inputlen = 0;
        }
        else
        {
#ifdef PARSER_ONLY
            includes->inputlen = ccReadFile(includes->inputbuffer, 1, sizeof(includes->inputbuffer), file);
#else
            includes->inputlen = fread(includes->inputbuffer, 1, sizeof(includes->inputbuffer), file);
#endif
        }
        includes->ibufPtr = includes->inputbuffer;
        if (includes->inputlen <= 0)
        {
            *s = 0;
            includes->inputlen = 0;
            return s == olds;
        }
        if (includes->first)
        {
            includes->first = false;
            if (includes->inputlen >= 3 && includes->ibufPtr[0] == 0xef && includes->ibufPtr[1] == 0xbb &&
                includes->ibufPtr[2] == 0xbf)
            {
                includes->ibufPtr += 3;
                includes->inputlen -= 3;
            }
        }
    }
}
void ccCloseFile(FILE*);
bool GetLine(void)
/*
 * Read in a line, preprocess it, and dump it to the list and preproc files
 * Also strip comments and alter trigraphs
 */
{
    bool rv, prepping;
    bool prepped = false;
    int rvc, lastTop;
    do
    {
        rv = false;
        prepping = false;
        rvc = 0;
#ifndef CPREPROCESSOR
        ErrorsToListFile();
#endif
    add:
        lastTop = rvc;
        while (rvc + 131 < MACRO_REPLACE_SIZE && !rv)
        {
            int temp;
            ++includes->line;
            ++includes->realline;
            preprocLine = includes->line;
            preprocFile = includes->fname;
#ifdef PARSER_ONLY
            if (!includes->ifskip)
                ccSetFileLine(includes->fname, includes->line);
#endif
            rv = getstring(includes->inputline + rvc, MACRO_REPLACE_SIZE - 132 - rvc, includes->handle);
#ifndef CPREPROCESSOR
            if (rv || once)
#else
            if (rv)
#endif
            {
#ifndef CPREPROCESSOR
                once = false;
                rv = true;
#endif
                break;
            }
            temp = strlen((char*)includes->inputline);
            if (temp && includes->inputline[temp - 1] != '\n')
                pperror(ERR_EOF_NONTERM, 0);
#ifndef CPREPROCESSOR
            if (!includes->sys_inc)
            {
                if (cparams.prm_listfile)
                {
                    printToListFile("%5d: %s", includes->line, includes->inputline + rvc);
                }
            }
#endif
            if (cparams.prm_trigraph)
                striptrigraph(includes->inputline + rvc);
            stripcomment(includes->inputline + rvc);
            /* for the back end*/
#ifndef CPREPROCESSOR
            InsertLineData(includes->realline, includes->fileindex, includes->fname, (char*)includes->inputline + rvc);
#endif
            rvc = strlen((char*)includes->inputline);
            while (rvc && isspace((unsigned char)includes->inputline[rvc - 1]))
                rvc--;

            if (!rvc || (!commentlevel && includes->inputline[rvc - 1] != '\\'))
                break;
            if (commentlevel)
                includes->inputline[rvc++] = ' ';
            else
                rvc--;
        }
        if (rvc)
            includes->inputline[rvc++] = '\n';
        includes->inputline[rvc] = 0;
        rvc = strlen((char*)includes->inputline);
        if (rvc && !prepped)
            rv = false;
        if (rv)
        {
            if (includes->ifs)
                pperror(ERR_EOF_PREPROC, includes->ifs->line);
            if (commentlevel)
                pperror(ERR_EOF_COMMENT, commentline);
            if (includes->next)
            {
                INCLUDES* p = includes;
#ifdef PARSER_ONLY
                ccCloseFile(includes->handle);
#else
                fclose(includes->handle);
#endif
#ifndef CPREPROCESSOR
                linesHead = (LINEDATA *)includes->linesHead;
                linesTail = (LINEDATA *)includes->linesTail;
#endif
                includes = includes->next;
                FreeInclData(p);
                commentlevel = 0;
                popif();
#ifndef CPREPROCESSOR
                browse_startfile(includes->fname, includes->fileindex);
                if (cparams.prm_listfile)
                    printToListFile("\n");
                if (chosenAssembler && chosenAssembler->enter_includename)
                    chosenAssembler->enter_includename(includes->fname, includes->fileindex);
#endif
                prepping = true;
                continue;
            }
#ifndef CPREPROCESSOR
            else
            {
                if (chosenAssembler && chosenAssembler->enter_includename)
                    chosenAssembler->enter_includename(infile, 0);
            }
#endif
        }
        if (rv)
            return 1;
        includes->lptr = includes->inputline + lastTop;

        while (*includes->lptr != '\n' && isspace((unsigned char)*includes->lptr))
            includes->lptr++;
        if (includes->lptr[0] == '#' || (cparams.prm_trigraph && (includes->lptr[0] == '%' && includes->lptr[1] == ':')))
        {
            if (cparams.prm_trigraph)
                stripdigraph(includes->lptr);
            if (!commentlevel)
            {
                preprocess();
                prepping = true;
            }
        }
        else if (lastTop)
        {
            includes->lptr = includes->inputline;
            while (*includes->lptr != '\n' && isspace((unsigned char)*includes->lptr))
                includes->lptr++;
        }
    } while (includes->ifskip || prepping);
    if (defcheck(includes->inputline) == INT_MIN + 1)
    {
        rvc = strlen((char*)includes->inputline);
        if (rvc + 131 < MACRO_REPLACE_SIZE)
        {
            if (includes->inputline[rvc - 1] == '\n')
                includes->inputline[rvc - 1] = ' ';
            prepped = true;
            goto add;
        }
    }
    lineToCpp();
    preprocLine = 0;
    preprocFile = 0;
    return 0;
}
/* Preprocessor dispatch */
void preprocess(void)
{
    char name[SYMBOL_NAME_LEN];
    if (includes->lptr[0] == '#')
        includes->lptr++;
    else
        includes->lptr += 2;
    skipspace();
    if (!*includes->lptr)
        return;
    if (!expectid(name))
        return;
    skipspace();
    if (strcmp(name, "include") == 0)
        doinclude();
    else if (strcmp(name, "define") == 0)
        dodefine();
    else if (strcmp(name, "endif") == 0)
        doendif();
    else if (strcmp(name, "else") == 0)
        doelse();
    else if (strcmp(name, "ifdef") == 0)
        doifdef(true);
    else if (strcmp(name, "ifndef") == 0)
        doifdef(false);
    else if (strcmp(name, "if") == 0)
    {
        doif();
    }
    else if (strcmp(name, "elif") == 0)
    {
        doelif();
    }
    else if (strcmp(name, "undef") == 0)
        (doundef());
    else if (strcmp(name, "error") == 0)
        (doerror());
    else if (strcmp(name, "warning") == 0)
        (dowarning());
    else if (strcmp(name, "pragma") == 0)
        (dopragma());
    else if (strcmp(name, "line") == 0)
        doline();
    else
    {
        if (!includes->ifskip)
            pperrorstr(ERR_UNKNOWNDIR, name);
    }
}

/*-------------------------------------------------------------------------*/

void doerror(void)
{
    char temp[4096];
    if (includes->ifskip)
        return;
    strcpy(temp, (char*)includes->lptr);

    if (temp[strlen(temp) - 1] == '\n')
        temp[strlen(temp) - 1] = 0;
    pperrorstr(ERR_PRAGERROR, temp);
}

void dowarning(void)
{
    char temp[4096];
    if (includes->ifskip)
        return;
    strcpy(temp, (char*)includes->lptr);

    if (temp[strlen(temp) - 1] == '\n')
        temp[strlen(temp) - 1] = 0;
    pperrorstr(ERR_PRAGWARN, temp);
}

/*-------------------------------------------------------------------------*/

unsigned char* getauxname(unsigned char* ptr, char** bufp)
{
    char buf[512], *bp = buf;
    while (isspace((unsigned char)*ptr))
        ptr++;
    if (!expectstring(bp, &ptr, true))
        return 0;
    IncGlobalFlag();
    *bufp = litlate(buf);
    DecGlobalFlag();
    return ptr;
}
#ifndef CPREPROCESSOR
unsigned onceCRC(FILE* handle)
{
    unsigned crc = 0;
    unsigned PartialCRC32(unsigned crc, unsigned char* data, size_t len);
#    ifdef PARSER_ONLY
    crc = PartialCRC32(crc, (unsigned char *)includes->handle, includes->filesize);
#    else
    int hnd = dup(fileno(handle));
    unsigned char buf[8192];
    int n;
    lseek(hnd, 0, SEEK_SET);
    while ((n = read(hnd, buf, sizeof(buf))) > 0)
    {
        crc = PartialCRC32(crc, buf, n);
    }
    close(hnd);
#    endif
    return crc;
}
void pragonce(void)
{
    time_t filetime = 0;
    struct stat statbuf;
    stat(includes->fname, &statbuf);
    filetime = statbuf.st_mtime;
    int bucket = ((includes->filesize >> 8) + (includes->filesize)) & (ONCE_BUCKETS - 1);
    ONCE* oncePos = onceLists[bucket];
    for (; oncePos; oncePos = oncePos->next)
        if (oncePos->filesize == includes->filesize)
            break;
    if (oncePos && filetime == oncePos->filetime)
    {
        if (oncePos->crc == onceCRC(includes->handle))
        {
            once = true;
            return;
        }
    }
    IncGlobalFlag();
    oncePos = (ONCE *)Alloc(sizeof(ONCE));
    DecGlobalFlag();
    oncePos->filesize = includes->filesize;
    oncePos->filetime = filetime;
    oncePos->crc = onceCRC(includes->handle);
    oncePos->next = onceLists[bucket];
    onceLists[bucket] = oncePos;
}
#endif
/*-------------------------------------------------------------------------*/

static void pragerror(int error)
{
    char buf[100], *p = buf;
    int i = 99;
    unsigned char* s;
    skipspace();
    s = includes->lptr;
    if (*s == '(')
    {
        // warning control
#ifndef CPREPROCESSOR
        if (error == ERR_PRAGWARN)
        {
            do
            {
                void (*func)(int) = NULL;
                s++;
                char name[256];
                name[0] = 0;
                while (isspace(*s))
                    s++;
                defid(name, &s);
                if (!strcmp(name, "push"))
                {
                    PushWarnings();
                }
                else if (!strcmp(name, "pop"))
                {
                    PopWarnings();
                }
                else if (!strcmp(name, "enable"))
                {
                    func = EnableWarning;
                }
                else if (!strcmp(name, "disable"))
                {
                    func = DisableWarning;
                }
                else
                {
                    break;
                }
                if (func)
                {
                    while (isspace(*s))
                        s++;
                    if (*s != ':')
                        break;
                    s++;
                    while (isspace(*s))
                        s++;
                    while (isdigit(*s))
                    {
                        func(atoi((char *)s));
                        while (isdigit(*s))
                            s++;
                        while (isspace(*s))
                            s++;
                    }
                }
                while (isspace(*s))
                    s++;
            } while (*s == ',');
        }
#endif
        return;
    }
    // else a warning message
    while (i-- && *s && *s != '\n')
        *p++ = (char)*s++;
    *p = 0;
    pperrorstr(error, buf);
}

/*-------------------------------------------------------------------------*/

void dopragma(void)
{
    char buf[40], *p = buf;
    bool sflag;
    int val;
    char name[SYMBOL_NAME_LEN];

    if (includes->ifskip)
        return;
    lineToCpp();
    if (!expectid(name))
        return;
    if (!strncmp(name, "PRIORITYCPP", 11))
    {
        cppprio++;
        return;
    }
    else if (!strncmp(name, "STDC", 4))
    {
        int on = 0;
        skipspace();
        defid(name, &includes->lptr);
        if (!strncmp(name, "FENV_ACCESS", 11))
            val = STD_PRAGMA_FENV;
        else if (!strncmp(name, "CX_LIMITED_RANGE", 16))
            val = STD_PRAGMA_CXLIMITED;
        else if (!strncmp(name, "FP_CONTRACT", 11))
            val = STD_PRAGMA_FCONTRACT;
        else
            return;
        skipspace();
        defid(name, &includes->lptr);
        if (strncmp(name, "ON", 2) == 0)
            on = 1;
        else if (strncmp(name, "OFF", 3) != 0)
            return;
        if (on)
            stdpragmas |= val;
        else
            stdpragmas &= ~val;
        return;
    }
#ifndef CPREPROCESSOR
    else if (!strcmp(name, "once"))
    {
        pragonce();
        return;
    }
#endif
    else if (!strcmp(name, "error"))
    {
        pragerror(ERR_PRAGERROR);
        return;
    }
    else if (!strcmp(name, "warning"))
    {
        pragerror(ERR_PRAGWARN);
        return;
    }
#ifndef CPREPROCESSOR
    else if (!strcmp(name, "ignore_global_init"))
    {
        skipspace();
        ignore_global_init = expectnum(NULL);
        return;
    }
    else if (!strcmp(name, "startup"))
        sflag = true;
    else if (!strcmp(name, "rundown"))
        sflag = false;
    else if (!strncmp(name, "library", 7))
    {
        {
            LIST* l;
            char* f;
            char buf[256], *p = buf;
            skipspace();
            if (*includes->lptr++ != '(')
                return;
            skipspace();
            while (*includes->lptr && *includes->lptr != ')' && !isspace((unsigned char)*includes->lptr))
                *p++ = *includes->lptr++;
            skipspace();
            if (*includes->lptr++ != ')')
                return;
            IncGlobalFlag();
            f = litlate(buf);
            l = (LIST*)(LIST *)Alloc(sizeof(LIST));
            l->data = f;
            l->next = libincludes;
            libincludes = l;
            DecGlobalFlag();
        }
        return;
    }
    else if (!strncmp(name, "pack", 4))
    {
        skipspace();
        if (*includes->lptr++ != '(')
        {
            return;
        }
        skipspace();
        if (!strncmp((char*)includes->lptr, "pop", 3))
        {
            includes->lptr += 3;
            skipspace();
            if (*includes->lptr == ')')
            {
                if (packlevel)
                    packlevel--;
            }
        }
        else if (*includes->lptr == ')')
        {
            if (packlevel)
                packlevel--;
        }
        else
        {
            if (!strncmp((char*)includes->lptr, "push", 4))
            {
                includes->lptr += 4;
                skipspace();
                if (*includes->lptr != ',')
                    return;
                includes->lptr++;
                skipspace();
            }
            if (isdigit(*includes->lptr))
            {
                if (packlevel < sizeof(packdata) - 1)
                {
                    packdata[++packlevel] = expectnum(NULL);
                    if (packdata[packlevel] < 1)
                        packdata[packlevel] = 1;
                    skipspace();
                    if (*includes->lptr != ')')
                    {
                        packlevel--;
                        return;
                    }
                }
            }
        }
        return;
    }
    else if (!strcmp(name, "aux"))
    {
        char* name;
        char* alias;
        includes->lptr = getauxname(includes->lptr, &name);
        if (!includes->lptr)
            return;
        if (*includes->lptr++ != '=')
        {
            return;
        }
        IncGlobalFlag();
        if (!getauxname(includes->lptr, &alias))
        {
            DecGlobalFlag();
            return;
        }
        insertAlias(name, alias);

        return;
    }
    else if (!strcmp(name, "farkeyword"))
    {
        skipspace();
        cparams.prm_farkeyword = *includes->lptr == '1';
        return;
    }
    else
#endif
    {
#ifndef CPREPROCESSOR
        if (chosenAssembler && chosenAssembler->doPragma)
            chosenAssembler->doPragma(name, (char*)includes->lptr);
#endif
        return;
    }
#ifndef CPREPROCESSOR
    skipspace();
    /* if we get here it was either a startup or rundown pragma */
    defid(p, &includes->lptr);
    skipspace();
    if (*includes->lptr && !isdigit(*includes->lptr))
    {
        return;
    }

    if (isdigit(*includes->lptr))
    {
        val = expectnum(NULL);
    }
    else
        val = 64;
    insertStartup(sflag, buf, val);
#endif
}

unsigned char* getMacroBuffer()
{
    unsigned char* rv;
    if (macroBuffers)
    {
        rv = (unsigned char *)macroBuffers;
        macroBuffers = macroBuffers->next;
    }
    else
    {
        rv = (unsigned char*)Alloc(sizeof(MACROLIST));
    }
    return rv;
}
void freeMacroBuffer(unsigned char* buf)
{
    MACROLIST* item = (MACROLIST*)buf;
    item->next = macroBuffers;
    macroBuffers = item;
}
#ifndef CPREPROCESSOR
/* parses the _Pragma directive*/
void Compile_Pragma(void)
{
    /* fixme, save context */
    unsigned char *buf = getMacroBuffer(), *q = buf;
    unsigned char* last;
    skipspace();
    if (*includes->lptr != '(')
        pperror(ERR_NEEDY, '(');
    includes->lptr++;
    skipspace();

    if ((*includes->lptr == 'L' || *includes->lptr == 'l') && includes->lptr[1] == '"')
        includes->lptr++;
    if (*includes->lptr == '"')
    {
        unsigned char* p = includes->lptr;
        while (*p)
        {
            if (*p == '\\' && (*(p + 1) == '"' || *(p + 1) == '\\'))
                p++;
            else if (*p == '"')
                break;
            *q++ = *p++;
        }
        *q = 0;
    }
    else
    {
        pperror(ERR_NEEDSTRING, 0);
        freeMacroBuffer(buf);
        return;
    }
    if (*includes->lptr != ')')
        pperror(ERR_NEEDY, ')');
    else
        includes->lptr++;
    last = includes->lptr;
    includes->lptr = buf;
    skipspace();
    dopragma();
    skipspace();
    includes->lptr = last;
    freeMacroBuffer(buf);
}
#endif
/*-------------------------------------------------------------------------*/

void doline(void)
/*
 * Handle #line directive
 */
{
    char buf[260];
    if (includes->ifskip)
        return;
    ppdefcheck(includes->lptr);
    skipspace();
    includes->line = expectnum(NULL) - 1;
    skipspace();
    if (*includes->lptr)
    {
        expectstring(buf, &includes->lptr, true);
        includes->linename = litlate(buf);
    }
}

INCLUDES* GetIncludeData(void)
{
    INCLUDES* rv;
    if (inclData)
    {
        rv = inclData;
        inclData = inclData->next;
        memset(rv, 0, sizeof(*rv));
    }
    else
    {
        rv = (INCLUDES *)globalAlloc(sizeof(INCLUDES));
    }
    rv->anonymousid = 1;
    return rv;
}
void FreeInclData(INCLUDES* data)
{
    data->next = inclData;
    inclData = data;
}
/*-------------------------------------------------------------------------*/

void doinclude(void)
/*
 * HAndle include files
 */
{
    INCLUDES* inc;
    bool nonSys = false;
    char name[260], name_orig[260], *p, *q;
    if (includes->ifskip)
        return;
    inc = GetIncludeData();
    inc->sys_inc = false;
    inc->first = true;
    if (*includes->lptr != '"' && *includes->lptr != '<')
    {
        ppdefcheck(includes->lptr);
    }
    if (*includes->lptr == '<')
        inc->sys_inc = true;
    if (!expectstring(name, &includes->lptr, true))
    {
        pperror(ERR_INCL_FILE_NAME, 0);
        return;
    }
    p = name, q = name;
    while (*p)
    {
        if ((unsigned char)*p != MACRO_PLACEHOLDER)
            *q++ = *p++;
        else
            p++;
    }
    *q = 0;
    strcpy(name_orig, name);
    if (inc->sys_inc)
        inc->handle = SrchPth(name, sys_searchpath, "r", true);
    if (inc->handle == NULL && includes)
    {
        char buf[260], *p, *q;
        strcpy(buf, (char*)includes->fname);
        p = strrchr(buf, '\\');
        q = strrchr(buf, '/');
        if (p < q)
            p = q;
        if (p)
        {
            *p = 0;
            inc->handle = SrchPth(name, buf, "r", false);
        }
    }
    if (inc->handle == NULL)
    {
        inc->handle = SrchPth(name, ".", "r", false);
        if (inc->handle)
            nonSys = true;
    }
    if (inc->handle == NULL)
    {
        inc->handle = SrchPth(name, prm_searchpath, "r", false);
        if (inc->handle)
            nonSys = true;
    }
    if (!inc->sys_inc && inc->handle == NULL)
        inc->handle = SrchPth(name, sys_searchpath, "r", true);

    IncGlobalFlag();
    inc->fname = litlate(name);
    if (nonSys)
    {
        LIST* fil = (LIST*)(LIST *)Alloc(sizeof(LIST));
        fil->data = inc->fname;
        fil->next = nonSysIncludeFiles;
        nonSysIncludeFiles = fil;
    }
    DecGlobalFlag();
    if (inc->handle == NULL)
    {
        pperrorstr(ERR_INCL_CANT_OPEN, inc->fname);
        return;
    }
    else
    {
        FILELIST* list;
        int i;
        ansieol();
        pushif();
        IncGlobalFlag();
        for (i = 1, list = incfiles; list; i++, list = list->next)
            if (!strcmp(list->data, inc->fname))
                break;
        if (!list)
        {
            list = (FILELIST*)Alloc(sizeof(FILELIST));
            list->data = inc->fname;
            list->next = 0;
            if (incfiles)
                lastinc = lastinc->next = list;
            else
                incfiles = lastinc = list;
        }
#ifndef CPREPROCESSOR
        inc->linesHead = linesHead;
        inc->linesTail = linesTail;

        linesHead = linesTail = NULL;
#    ifdef PARSER_ONLY
        inc->filesize = strlen((char *)inc->handle);
#    else
        fseek(inc->handle, 0, SEEK_END);
        inc->filesize = ftell(inc->handle);
        fseek(inc->handle, 0, SEEK_SET);
#    endif
#endif
        inc->fileindex = i;
#ifndef CPREPROCESSOR
        if (chosenAssembler && chosenAssembler->enter_includename)
            chosenAssembler->enter_includename(inc->fname, i);
        browse_startfile(inc->fname, i);
#endif
#ifdef PARSER_ONLY
        ccNewFile(inc->fname, false);
#endif
        DecGlobalFlag();
        inc->next = includes;
        includes = inc;
    }
}

/*-------------------------------------------------------------------------*/

void glbdefine(const char* name, const char* value, bool permanent)
{
    DEFSTRUCT* def;
    if ((DEFSTRUCT*)search(name, defsyms) != 0)
        return;
    IncGlobalFlag();
    def = (DEFSTRUCT*)Alloc(sizeof(DEFSTRUCT));
    def->name = litlate(name);
    def->string = litlate(value);
    def->permanent = permanent;
    def->line = 1;
    def->file = "COMMAND LINE";
    insert((SYMBOL*)def, defsyms);
    DecGlobalFlag();
}
void glbUndefine(const char* name)
{
    DEFSTRUCT* hr;
    hr = (DEFSTRUCT*)search(name, defsyms);
    if (hr == NULL)
    {
        IncGlobalFlag();
        hr = (DEFSTRUCT*)Alloc(sizeof(DEFSTRUCT));
        hr->name = litlate(name);
        hr->string = "";
        insert((SYMBOL*)hr, defsyms);
        DecGlobalFlag();
    }
    hr->undefined = true;
    hr->permanent = true;
}
int undef2(const char* name)
{
    {
        HASHREC** p = LookupName(name, defsyms);
        if (p)
        {
            DEFSTRUCT* d = (DEFSTRUCT*)(*p)->p;
            if (!d->permanent)
                *p = (*p)->next;
            else
                return 0;
        }
    }
    return 1;
}
/* Handle #defines
 * Doesn't check for redefine with different value
 * Does handle ANSI macros
 */
#define MAX_MACRO_ARGS 128
void dodefine(void)
{
    char name[SYMBOL_NAME_LEN];
    DEFSTRUCT* hr;
    DEFSTRUCT* def;
    const char* args[128];
    int count = 0;
    char* ps;
    int p, i, j;
    int charindex;
    if (includes->ifskip)
        return;
    skipspace();
    charindex = includes->lptr - includes->inputline;
    if (!expectid(name))
    {
        pperror(ERR_IDENTIFIER_EXPECTED, 0);
        return;
    }
    if ((hr = (DEFSTRUCT*)search(name, defsyms)) != 0)
        if (!undef2(name))
            return;

    IncGlobalFlag();
    def = (DEFSTRUCT *) globalAlloc(sizeof(DEFSTRUCT));
    def->name = litlate(name);
    def->args = 0;
    def->argcount = 0;
    def->line = includes->line;
    def->file = includes->fname;
    if (*includes->lptr == '(')
    {
        bool gotcomma = false, nullargs = true;
        includes->lptr++;
        skipspace();
        while (isstartchar((unsigned char)*includes->lptr))
        {
            int j;
            gotcomma = false;
            nullargs = false;
            expectid(name);
            args[count++] = litlate(name);
            if (count >= MAX_MACRO_ARGS)
                fatal("Macro arg count > %d", MAX_MACRO_ARGS);
            for (j = 0; j < count - 1; j++)
                if (!strcmp(args[count - 1], args[j]))
                {
                    pperrorstr(ERR_DUPLICATE_IDENTIFIER, name);
                    break;
                }
            skipspace();
            if (*includes->lptr != ',')
                break;
            gotcomma = true;
            includes->lptr++;
            skipspace();
        }
        if ((cparams.prm_c99 || cparams.prm_cplusplus) && (gotcomma || nullargs))
        {
            if (includes->lptr[0] == '.' && includes->lptr[1] == '.' && includes->lptr[2] == '.')
            {
                includes->lptr += 3;
                def->varargs = true;
                gotcomma = false;
                skipspace();
            }
        }
        if (*includes->lptr != ')' || gotcomma)
            pperror(ERR_NEEDY, ')');
        if (*includes->lptr == ')')
            includes->lptr++;
        def->args = (char**)Alloc(count * sizeof(char*));
        memcpy(def->args, args, count * sizeof(char*));
        def->argcount = count + 1;
    }
    skipspace();
    *(--includes->lptr) = MACRO_PLACEHOLDER;
    p = strlen((char*)includes->lptr);
    while (isspace((unsigned char)includes->lptr[p - 1]))
        p--;
    includes->lptr[p++] = MACRO_PLACEHOLDER;
    includes->lptr[p] = 0;
    for (i = 0, j = 0; i < p + 1; i++, j++)
        if (!strncmp((char*)includes->lptr + i, "##", 2))
        {
            includes->lptr[j] = REPLACED_TOKENIZING;
            i++;
        }
        else if (!strncmp((char*)includes->lptr + i, "%:%:", 4))
        {
            includes->lptr[j] = REPLACED_TOKENIZING;
            i += 3;
        }
        else
            includes->lptr[j] = includes->lptr[i];
    p = strlen((char*)includes->lptr);
    for (i = 0; i < p && isspace((unsigned char)includes->lptr[i]); i++)
        if (i[includes->lptr] == REPLACED_TOKENIZING)
            pperror(ERR_PP_INV_DEFINITION, 0);
    for (i = p - 1; i >= 0 && isspace((unsigned char)includes->lptr[i]); i--)
        ;
    if (i[includes->lptr] == '#' || i[includes->lptr] == REPLACED_TOKENIZING)
        pperror(ERR_PP_INV_DEFINITION, 0);

    ps = (char *)Alloc((p = strlen((char*)includes->lptr)) + 1);
    strcpy(ps, (char*)includes->lptr);
    def->string = ps;
    if (hr)
    {
        bool same = true;
        if (def->argcount != hr->argcount)
            same = false;
        else
        {
            int i;
            const char *p, *q;
            for (i = 0; i < def->argcount - 1 && same; i++)
                if (strcmp(def->args[i], hr->args[i]) == 0)
                    same = false;
            p = def->string;
            q = hr->string;
            while (*p && *q)
            {
                if (isspace((unsigned char)*p))
                    if (isspace((unsigned char)*q))
                    {
                        while (isspace((unsigned char)*p))
                            p++;
                        while (isspace((unsigned char)*q))
                            q++;
                    }
                    else
                    {
                        break;
                    }
                else if (isspace((unsigned char)*q))
                {
                    break;
                }
                else if (*p != *q)
                    break;
                else
                    p++, q++;
            }
            if (*p)
                while (isspace((unsigned char)*p))
                    p++;
            if (*q)
                while (isspace((unsigned char)*q))
                    q++;
            if (*p || *q)
                same = false;
        }
        if (!same)
        {
            preverror(ERR_PP_REDEFINE_NOT_SAME, hr->name, hr->file, hr->line);
        }
    }
    insert((SYMBOL*)def, defsyms);
    DecGlobalFlag();
#ifndef CPREPROCESSOR
    browse_define(name, def->line, charindex);
#endif
}

/*
 * Undefine
 */
void doundef(void)
{
    char name[SYMBOL_NAME_LEN];
    if (!includes->ifskip)
    {
        if (!expectid(name))
            pperror(ERR_IDENTIFIER_EXPECTED, 0);
        else
            undef2(name);
    }
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

/*
 * Insert a replacement string
 */
int definsert(unsigned char* macro, unsigned char* end, unsigned char* begin, unsigned char* text, unsigned char* etext, int len,
              int replen)
{
    unsigned char* q;
    static unsigned char NULLTOKEN[] = {TOKENIZING_PLACEHOLDER, 0};
    int p, r;
    int val;
    int stringizing = false;
    q = end;
    while (*q >= 0 && isspace((unsigned char)*q))
        q++;  //*q unsigned >= 0 always
    if (*q == REPLACED_TOKENIZING)
    {
        if (text[0] == 0)
        {
            text = NULLTOKEN;
        }
    }
    else
    {
        q = begin - 1;
        while (q > macro && *q >= 0 && isspace((unsigned char)*q))
            q--;  //*q unsigned >= 0 always
        if (*q == REPLACED_TOKENIZING)
        {
            if (text[0] == 0)
            {
                text = NULLTOKEN;
            }
        }
        else if (*q == '#')
        {
            stringizing = true;
        }
        else
        {
            text = etext;
        }
    }
    p = strlen((char*)text);
    if (stringizing)
        p += 1;
    val = p - replen;
    r = strlen((char*)begin);
    if (val + r + 1 >= len)
    {
        pperror(ERR_MACROSUBS, 0);
        return (INT_MIN);
    }
    if (val > 0)
        for (q = begin + r + 1; q >= end; q--)
            *(q + val) = *q;
    else if (val < 0)
    {
        r = strlen((char*)end) + 1;
        for (q = end; q < end + r; q++)
            *(q + val) = *q;
    }
    while (*text)
        *begin++ = *text++;
    if (stringizing)
        *begin++ = STRINGIZING_PLACEHOLDER;
    return (p);
}
static int oddslash(unsigned char* start, unsigned char* p)
{
    int v = 0;
    while (p >= start && *p-- == '\\')
    {
        v++;
    }
    return v & 1;
}
void defstringizing(unsigned char* macro)
{
    unsigned char* replmac = getMacroBuffer();
    unsigned char *p = replmac, *q = macro;
    int waiting = 0;
    while (*q)
    {
        if (!waiting && (*q == '"' || *q == '\'') && !oddslash(macro, q - 1))
        {
            waiting = *q;
            *p++ = *q++;
        }
        else if (waiting)
        {
            if (*q == waiting && !oddslash(macro, q - 1))
                waiting = 0;
            *p++ = *q++;
        }
        else if (*q == '#' && *(q - 1) != '#' && *(q + 1) != '#') /* # ## # */
        {
            unsigned char *s, *r;
            q++;
            while (isspace(*q))
                q++;
            *p++ = '"';
            s = r = p;
            while (*q && *q != STRINGIZING_PLACEHOLDER)
            {
                while (*q == MACRO_PLACEHOLDER)
                    q++;
                if (*q == '\\' || *q == '"')
                {
                    *p++ = '\\';
                }
                if (*q != STRINGIZING_PLACEHOLDER)
                    *p++ = *q++;
            }
            *p++ = '"';
            if (*q)
                q++;
            while (s < p)
            {
                while (isspace(*s) && isspace(s[1]))
                    s++;
                *r++ = *s++;
            }
            p = r;
        }
        else
        {
            *p++ = *q++;
        }
    }
    *p = 0;
    strcpy((char*)macro, (char*)replmac);
    freeMacroBuffer(replmac);
}
/* replace macro args */
int defreplaceargs(unsigned char* macro, int count, unsigned char** oldargs, unsigned char** newargs, unsigned char** expandedargs,
                   unsigned char* varargs)
{
    int i, rv;
    char name[256];
    unsigned char *p = macro, *q;
    int waiting = 0;
    while (*p)
    {
        if (!waiting && (*p == '"' || *p == '\'') && !oddslash(macro, p - 1))
        {
            waiting = *p;
        }
        else if (waiting)
        {
            if (*p == waiting && !oddslash(macro, p - 1))
                waiting = 0;
        }
        else if (issymchar((unsigned char)*p))
        {
            q = p;
            defid(name, &p);
            if ((cparams.prm_c99 || cparams.prm_cplusplus) && !strcmp(name, "__VA_ARGS__"))
            {
                if (varargs[0])
                {
                    if ((rv = definsert(macro, p, q, varargs, varargs, MACRO_REPLACE_SIZE - (q - macro), p - q)) <
                        -MACRO_REPLACE_SIZE)
                        return (false);
                    else
                    {
                        p = q + rv - 1;
                    }
                }
                else
                {
                    if ((rv = definsert(macro, p, q, (UBYTE*)"", (UBYTE *)"", MACRO_REPLACE_SIZE - (q - macro), p - q)) < -MACRO_REPLACE_SIZE)
                        return (false);
                    else
                    {
                        p = q + rv - 1;
                    }
                }
            }
            else
                for (i = 0; i < count; i++)
                {
                    if (!strcmp(name, (char*)oldargs[i]))
                    {
                        if ((rv = definsert(macro, p, q, newargs[i], expandedargs[i], MACRO_REPLACE_SIZE - (q - macro), p - q)) <
                            -MACRO_REPLACE_SIZE)
                            return (false);
                        else
                        {
                            p = q + rv - 1;
                            break;
                        }
                    }
                }
        }
        if (*p)
            p++;
    }
    return (true);
}
void deftokenizing(unsigned char* macro)
{
    unsigned char *b = macro, *e = macro;
    int waiting = 0;
    while (*e)
    {
        if (!waiting && (*e == '"' || *e == '\'') && !oddslash(macro, e - 1))
        {
            waiting = *e;
            *b++ = *e++;
        }
        else if (waiting)
        {
            if (*e == waiting && !oddslash(macro, e - 1))
                waiting = 0;
            *b++ = *e++;
        }
        else if (*e == REPLACED_TOKENIZING)
        {
            while (b != macro && (isspace((unsigned char)*(b - 1)) || b[-1] == MACRO_PLACEHOLDER))
                b--;
            while (*++e != 0 && (isspace((unsigned char)*e) || *e == MACRO_PLACEHOLDER))
                ;
            if (b != macro && b[-1] == TOKENIZING_PLACEHOLDER && *e != TOKENIZING_PLACEHOLDER)
                b--;
            if (*e == TOKENIZING_PLACEHOLDER)
            {
                e++;
            }
        }
        else
            *b++ = *e++;
    }
    *b = 0;
}

/*-------------------------------------------------------------------------*/

char* fullqualify(char* string)
{
    static char buf[265];
    if (string[0] == '\\')
    {
        getcwd(buf, 265);
        strcpy(buf + 2, string);
        return buf;
    }
    else if (string[1] != ':')
    {
        char *p, *q = string;
        getcwd(buf, 265);
        p = buf + strlen(buf);
        if (!strncmp(q, ".\\", 2))
            q += 2;
        p--;
        while (!strncmp(q, "..\\", 3))
        {
            q += 3;
            while (p > buf && *p != '\\')
                p--;
            if (p > buf)
                p--;
        }
        p++;
        *p++ = '\\';
        strcpy(p, q);
        return buf;
    }
    return string;
}

/*-------------------------------------------------------------------------*/

void filemac(char* string)
{
    char* p = string;
    char* q = includes->fname;
    *p++ = '"';
    while (*q)
    {
        if (*q == '\\')
            *p++ = *q;
        *p++ = *q++;
    }
    *p++ = '"';
    *p = 0;
}

/*-------------------------------------------------------------------------*/

void datemac(char* string)
{
    struct tm* t1;
    time_t t2;
    if (source_date_epoch != (time_t)-1)
        t2 = source_date_epoch;
    else
        time(&t2);
    t1 = localtime(&t2);
    strftime(string, 40, "\"%b %d %Y\"", t1);
    if (string[5] == '0')
        string[5] = ' '; /* as asctime() */
}

void dateisomac(char* string)
{
    struct tm* t1;
    time_t t2;
    if (source_date_epoch != (time_t)-1)
        t2 = source_date_epoch;
    else
        time(&t2);
    t1 = localtime(&t2);
    strftime(string, 40, "\"%Y-%m-%d\"", t1);
}

/*-------------------------------------------------------------------------*/

void timemac(char* string)
{
    struct tm* t1;
    time_t t2;
    if (source_date_epoch != (time_t)-1)
        t2 = source_date_epoch;
    else
        time(&t2);
    t1 = localtime(&t2);
    strftime(string, 40, "\"%H:%M:%S\"", t1);
}

/*-------------------------------------------------------------------------*/

void linemac(char* string) { sprintf(string, "%d", includes->line); }

/*-------------------------------------------------------------------------*/

void countermac(char* string)
{
    sprintf(string, "%d", counter);
    counter++;
}

/* Scan for default macros and replace them */
void defmacroreplace(char* macro, char* name)
{
    int i;
    macro[0] = 0;
    for (i = 0; i < INGROWNMACROS; i++)
        if (!strcmp(name, ingrownmacros[i].s))
        {
            (ingrownmacros[i].func)(macro);
            break;
        }
}

/*-------------------------------------------------------------------------*/
void SetupAlreadyReplaced(unsigned char* macro)
{
    unsigned char *nn = getMacroBuffer(), *src = nn;
    char name[256];
    int instr = false;
    strcpy((char*)nn, (char*)macro);
    while (*src)
    {
        if ((*src == '"' || *src == '\'') && !oddslash(macro, src - 1))
            instr = !instr;
        if (isstartchar((unsigned char)*src) && !instr)
        {
            DEFSTRUCT* sp;
            defid(name, &src);
            if ((sp = (DEFSTRUCT*)search(name, defsyms)) != 0 && sp->preprocessing)
            {
                *macro++ = REPLACED_ALREADY;
            }
            strcpy((char*)macro, name);
            macro += strlen((char*)macro);
        }
        else
        {
            *macro++ = *src++;
        }
    }
    *macro = 0;
    freeMacroBuffer(nn);
}
//
// a preprocessing number starts with a digit or '.' followed by a digit, then
// has any number of alphanumeric charactors or any of the sequences
// E+ e+ P+ p+
int ppNumber(unsigned char* start, unsigned char* pos)
{
    unsigned char* x = pos;
    if (*pos == '+' || *pos == '-' || isdigit(*pos))  // we would get here with the first alpha char following the number
    {
        // backtrack through all characters that could possibly be part of the number
        while (pos >= start &&
               (issymchar(*pos) || *pos == '.' ||
                ((*pos == '-' || *pos == '+') && (pos[-1] == 'e' || pos[-1] == 'E' || pos[-1] == 'p' || pos[-1] == 'P'))))
        {
            if (*pos == '-' || *pos == '+')
                pos--;
            pos--;
        }
        // go forward, skipping sequences that couldn't actually start a number
        pos++;
        if (!isdigit(*pos))
        {
            while (pos < x && (*pos != '.' || isdigit(pos[-1]) || !isdigit(pos[1])))
                pos++;
        }
        // if we didn't get back where we started we have a number
        return pos < x && (pos[0] != '0' || (pos[1] != 'x' && pos[1] != 'X'));
    }
    return false;
}
int replacesegment(unsigned char* start, unsigned char* end, int* inbuffer, int totallen, unsigned char** pptr)
{
    unsigned char *args[MAX_MACRO_ARGS], *expandedargs[MAX_MACRO_ARGS];
    unsigned char *macro = getMacroBuffer(), varargs[4096];
    char name[256];
    char waiting = 0;
    int rv;
    int size;
    unsigned char *p, *q;
    DEFSTRUCT* sp;
    int insize, rv1;
    unsigned char* orig_end = end;
    p = start;
    while (p < end)
    {
        q = p;
        if (!waiting && (*p == '"' || *p == '\'') && !oddslash(start, p - 1))
        {
            waiting = *p;
            p++;
        }
        else if (waiting)
        {
            if (*p == waiting && !oddslash(start, p - 1))
                waiting = 0;
            p++;
        }
        else if (isstartchar((unsigned char)*p) && (p == start || !issymchar((unsigned char)p[-1])))
        {
            name[0] = 0;
            defid(name, &p);
            if ((!cparams.prm_cplusplus || name[0] != 'R' || name[1] != '\0' || *p != '"') &&
                (sp = (DEFSTRUCT*)search(name, defsyms)) != 0 && !sp->undefined && q[-1] != REPLACED_ALREADY &&
                !ppNumber(start, q - 1))
            {
                if (sp->argcount)
                {
                    unsigned char *r, *s;
                    int count = 0;
                    unsigned char* q = p;

                    varargs[0] = 0;

                    while (isspace((unsigned char)*q) || *q == MACRO_PLACEHOLDER)
                        q++;
                    if (q > start && *(q - 1) == '\n')
                    {
                        freeMacroBuffer(macro);
                        return INT_MIN + 1;
                    }
                    if (*q++ != '(')
                    {
                        goto join;
                    }
                    p = q;
                    if (sp->argcount > 1)
                    {
                        do
                        {
                            unsigned char* nm = macro;
                            int nestedparen = 0, nestedstring = 0;
                            while (isspace((unsigned char)*p))
                                p++;
                            while (*p && (((*p != ',' && *p != ')') || nestedparen || nestedstring) && *p != '\n'))
                            {
                                if (nestedstring)
                                {
                                    if (*p == nestedstring && !oddslash(start, p - 1))
                                        nestedstring = 0;
                                }
                                else if ((*p == '\'' || *p == '"') && !oddslash(macro, p - 1))
                                    nestedstring = *p;
                                else if (*p == '(')
                                    nestedparen++;
                                else if (*p == ')' && nestedparen)
                                    nestedparen--;
                                *nm++ = *p++;
                            }
                            while (nm > macro && isspace((unsigned char)nm[-1]))
                                nm--;
                            *nm = 0;
                            args[count] = (unsigned char*)litlate((char*)macro);
                            insize = 0;
                            size = strlen((char*)macro);
                            rv = replacesegment(macro, macro + size, &insize, totallen, 0);
                            if (rv < -MACRO_REPLACE_SIZE)
                            {
                                freeMacroBuffer(macro);
                                return rv;
                            }
                            macro[rv + size] = 0;
                            expandedargs[count++] = (unsigned char*)litlate((char*)macro);
                        } while (*p && *p++ == ',' && count != sp->argcount - 1);
                    }
                    else
                    {
                        count = 0;
                        while (isspace((unsigned char)*p))
                            p++;
                        if (*p == ')')
                            p++;
                    }
                    if (*(p - 1) != ')' || count != sp->argcount - 1)
                    {
                        if (count == sp->argcount - 1 && (cparams.prm_c99 || cparams.prm_cplusplus) && (sp->varargs))
                        {
                            unsigned char* q = varargs;
                            int nestedparen = 0;
                            if (!(sp->varargs))
                            {
                                freeMacroBuffer(macro);
                                return INT_MIN;
                            }
                            while (*p != '\n' && *p && (*p != ')' || nestedparen))
                            {
                                if (*p == '(')
                                    nestedparen++;
                                if (*p == ')' && nestedparen)
                                    nestedparen--;
                                *q++ = *p++;
                            }
                            *q = 0;
                            p++;
                        }
                        if (*(p - 1) != ')' || count != sp->argcount - 1)
                        {
                            if (!*(p) || !(*(p - 1)))
                            {
                                freeMacroBuffer(macro);
                                return INT_MIN + 1;
                            }
                            pperrorstr(ERR_WRONGMACROARGS, name);
                            freeMacroBuffer(macro);
                            return INT_MIN;
                        }
                    }
                    strcpy((char*)macro, sp->string);
                    if (count != 0 || varargs[0])
                        if (!defreplaceargs(macro, count, (unsigned char**)sp->args, args, expandedargs, varargs))
                        {
                            freeMacroBuffer(macro);
                            return INT_MIN;
                        }
                    deftokenizing(macro);
                    defstringizing(macro);
                    r = macro, s = macro;
                    while (*r)
                    {
                        if (*r != TOKENIZING_PLACEHOLDER)
                            *s++ = *r++;
                        else
                            r++;
                    }
                    *s = 0;
                }
                else
                {
                    strcpy((char*)macro, sp->string);
                }
                sp->preprocessing = true;
                SetupAlreadyReplaced(macro);
                size = strlen((char*)macro);
                if ((rv1 = definsert(start, p, q, macro, macro, totallen - *inbuffer, p - q)) < -MACRO_REPLACE_SIZE)
                {
                    sp->preprocessing = false;
                    freeMacroBuffer(macro);
                    return rv1;
                }
                insize = rv1 - (p - q);
                *inbuffer += insize;
                end += insize;
                p += insize;
                insize = 0;
                rv = replacesegment(q, p, &insize, totallen, &p);
                sp->preprocessing = false;
                if (rv < -MACRO_REPLACE_SIZE)
                {
                    freeMacroBuffer(macro);
                    return rv;
                }
                *inbuffer += rv;
                end += rv;
                insize = 0;
            }
            else
            {
            join:
                defmacroreplace((char*)macro, name);
                if (macro[0])
                {
                    if ((rv = definsert(start, p, q, macro, macro, totallen - *inbuffer, p - q)) < -MACRO_REPLACE_SIZE)
                    {

                        freeMacroBuffer(macro);
                        return rv;
                    }
                    end += rv - (p - q);
                    *inbuffer += rv - (p - q);
                    p += rv - (p - q);
                }
            }
        }
        else
            p++;
    }
    if (pptr)
        *pptr = p;
    freeMacroBuffer(macro);
    return end - orig_end;
}
void ppdefcheck(unsigned char* line) { defcheck(line); }
/* Scan line for macros and do replacements */
int defcheck(unsigned char* line)
{
    int inbuffer = strlen((char*)line);
    int rv = replacesegment(line, line + inbuffer, &inbuffer, MACRO_REPLACE_SIZE, 0);
    unsigned char* p = line;
    while (*p)
    {
        if (*p != REPLACED_ALREADY)
        {
            *line++ = *p++;
        }
        else
        {
            p++;
            if (rv >= -MACRO_REPLACE_SIZE)
                rv--;
        }
    }
    *line = 0;
    return rv;
}

/*-------------------------------------------------------------------------*/

static void repdefines(unsigned char* lptr)
/*
 * replace 'defined' keyword in #IF and #ELIF statements
 */
{
    unsigned char* q = lptr;
    char name[512];
    while (*lptr)
    {
        if (!strncmp((char*)lptr, defkw, 7))
        {
            bool needend = false;
            lptr += 7;
            while (isspace(*lptr))
                lptr++;
            if (*lptr == '(')
            {
                lptr++;
                needend = true;
            }
            while (isspace(*lptr))
                lptr++;
            defid(name, &lptr);
            while (isspace(*lptr))
                lptr++;
            if (needend)
            {
                if (*lptr == ')')
                    lptr++;
                else
                    pperror(ERR_NEEDY, ')');
            }
            if (search(name, defsyms) != 0)
                *q++ = '1';
            else
                *q++ = '0';
            *q++ = ' ';
        }
        else
        {
            *q++ = *lptr++;
        }
    }
    *q = 0;
}

/*-------------------------------------------------------------------------*/

void pushif(void)
/* Push an if context */
{
    IFSTRUCT* p;
    p = (IFSTRUCT*)globalAlloc(sizeof(IFSTRUCT));
    p->next = includes->ifs;
    p->iflevel = includes->ifskip;
    p->elsetaken = includes->elsetaken;
    includes->elsetaken = false;
    includes->ifs = p;
}

/*-------------------------------------------------------------------------*/

void popif(void)
/* Pop an if context */
{
    if (includes->ifs)
    {
        includes->ifskip = includes->ifs->iflevel;
        includes->elsetaken = includes->ifs->elsetaken;
        includes->ifs = includes->ifs->next;
    }
    else
    {
        includes->ifskip = 0;
        includes->elsetaken = 0;
    }
#ifdef PARSER_ONLY
    if (!includes->ifskip)
        ccSetFileLine(includes->fname, includes->line);
#endif
}

/*-------------------------------------------------------------------------*/

void ansieol(void)
{
    if (cparams.prm_ansi)
    {
        skipspace();
        if (*includes->lptr)
        {
            pperror(ERR_EOL_UNEXPECTED, 0);
        }
    }
}

/*-------------------------------------------------------------------------*/

void doifdef(bool flag)
/* Handle IFDEF */
{
    DEFSTRUCT* hr;
    char name[SYMBOL_NAME_LEN];
    if (includes->ifskip)
    {
        includes->skiplevel++;
        return;
    }
    skipspace();
    if (!expectid(name))
    {
        pperror(ERR_IDENTIFIER_EXPECTED, 0);
        return;
    }
    hr = (DEFSTRUCT*)search(name, defsyms);
    pushif();
    includes->ifs->line = includes->line;
    if ((hr && ((!hr->undefined && !flag) || (hr->undefined && flag))) || (!hr && flag))
        includes->ifskip = true;
    ansieol();
}

/*-------------------------------------------------------------------------*/

void doif(void)
/* Handle #if */
{
    if (includes->ifskip)
    {
        includes->skiplevel++;
        return;
    }
    repdefines(includes->lptr);
    ppdefcheck(includes->lptr);
    pushif();
    includes->ifs->line = includes->line;
    if (!ppexpr())
        includes->ifskip = true;
    else
        includes->elsetaken = true;
    ansieol();
}

/*-------------------------------------------------------------------------*/

void doelif(void)
/* Handle #elif */
{
    PPINT is;
    if (includes->skiplevel)
    {
        return;
    }
    repdefines(includes->lptr);
    ppdefcheck(includes->lptr);

    is = !ppexpr();
    if (!includes->elsetaken)
    {
        if (includes->ifs)
        {
            int oldifskip = includes->ifskip;
            includes->ifskip = !includes->ifskip || is || includes->elsetaken;
            if (!oldifskip || !includes->ifskip)
                includes->elsetaken = true;
        }
        else
            pperror(ERR_PPELIF_NO_IF, 0);
    }
    else
    {
        includes->ifskip = true;
        includes->elsetaken = true;
    }
    if (includes->ifs)
        includes->ifs->line = includes->line;
    ansieol();
}

/* handle else */
void doelse(void)
{
    if (includes->skiplevel)
    {
        return;
    }
    if (includes->ifs)
    {
        if (!includes->ifs->iflevel)
            includes->ifskip = !includes->ifskip || includes->elsetaken;
    }
    else
        pperror(ERR_PPELSE_NO_IF, includes->line);
    ansieol();
}

/* HAndle endif */
void doendif(void)
{
    if (includes->skiplevel)
    {
        includes->skiplevel--;
        return;
    }
    if (!includes->ifs)
        pperror(ERR_PPENDIF_NO_IF, includes->line);
    popif();
    ansieol();
}
