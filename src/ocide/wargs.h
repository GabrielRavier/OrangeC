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

/* Mode values for ARGLIST */
#define ARG_CONCATSTRING 1
#define ARG_NOCONCATSTRING 2
#define ARG_BOOL 3
#define ARG_SWITCH 4
#define ARG_SWITCHSTRING 5

/* Valid arg separators */
#define ARG_SEPSWITCH '/'
#define ARG_SEPFALSE '-'
#define ARG_SEPTRUE '+'

/* Return values for dispatch routine */
#define ARG_NEXTCHAR 1
#define ARG_NEXTARG 2
#define ARG_NEXTNOCAT 3
#define ARG_NOMATCH 4
#define ARG_NOARG 5

typedef struct
{
    char id;
    int mode;
    void (*routine)(char, char*);
} ARGLIST;

int parse_args(int* argc, char* argv[], int case_sensitive);
char** CmdLineToC(int* count, char* cmd);
