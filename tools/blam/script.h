// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------

#ifndef __BLAM_SCRIPT__
#define __BLAM_SCRIPT__

#define MAX_NESTED_PARSERS  128

typedef enum
{
    TK_NONE,
    TK_NUMBER,
    TK_STRING,
    TK_POUND,
    TK_COLON,
    TK_SEMICOLON,
    TK_LBRACK,
    TK_RBRACK,
    TK_LPAREN,
    TK_RPAREN,
    TK_COMMA,
    TK_IDENIFIER,
    TK_DEFINE,
    TK_UNDEF,
    TK_INCLUDE,
    TK_SETDIR,
    TK_WAIT,
    TK_MACRO,
    TK_LOOP,
    TK_EOF
} tokentype_t;

#define MAX_IDENTIFIER_LEN  16
#define MAX_IDENTIFIER_ARGS 5

typedef struct identifier_s
{
    struct identifier_s* prev;
    struct identifier_s* next;
    char name[64];
    dboolean has_args;
    char argnames[MAX_IDENTIFIER_ARGS][MAX_IDENTIFIER_LEN];
    int numargs;
    char *buffer;
    int bufsize;
} identifier_t;

extern identifier_t identifier_cap;
extern identifier_t *current_identifier;

#define MAX_IDENTIFER_VALUE_LEN 16
#define MAX_IDENTIFER_STACK_LEN 128

typedef struct
{
    char *argname;
    char value[MAX_IDENTIFER_VALUE_LEN];
    int token_type;
} identifer_stack_t;

extern identifer_stack_t id_stack[MAX_IDENTIFER_STACK_LEN];
extern int id_stack_count;

typedef struct
{
    char                token[512];
    char*               buffer;
    char*               pointer_start;
    char*               pointer_end;
    int                 linepos;
    int                 rowpos;
    int                 buffpos;
    int                 buffsize;
    tokentype_t         tokentype;
    dboolean            isamacro;
    identifier_t        *identifier;
    identifer_stack_t   *stack;
    const char          *name;
} scparser_t;

extern scparser_t *sc_parsers[MAX_NESTED_PARSERS];
extern scparser_t *sc_parser;
extern char sc_stringbuffer[256];
extern dboolean verbose;
extern FILE *debugfile;

void SC_Open(char* name);
void SC_Close(void);
int SC_ReadTokens(void);
void SC_CheckKeywords(void);
void SC_MustMatchToken(int type);
void SC_ExpectNextToken(int type);
int SC_Find(void);
char SC_GetChar(void);
void SC_Rewind(void);
void SC_Error(const char *msg, ...);
void SC_PushParser(void);
void SC_PopParser(void);
int SC_GetNumber(void);
void SC_GetString(void);
void SC_AddIdentifier(void);
void SC_RemoveIdentifier(void);
void SC_PushIdStack(char *name);
void SC_Init(void);

#endif // __BLAM_SCRIPT__
