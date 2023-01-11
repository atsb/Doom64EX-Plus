//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//

#ifndef __PARSER_H__
#define __PARSER_H__

#define MAX_NESTED_PARSERS      128
#define MAX_NESTED_FILENAMES    128

typedef enum
{
    TK_NONE,
    TK_NUMBER,
    TK_STRING,
    TK_POUND,
    TK_COLON,
    TK_SEMICOLON,
    TK_PERIOD,
    TK_QUOTE,
    TK_FORWARDSLASH,
    TK_EQUAL,
    TK_LBRACK,
    TK_RBRACK,
    TK_LPAREN,
    TK_RPAREN,
    TK_LSQBRACK,
    TK_RSQBRACK,
    TK_COMMA,
    TK_IDENIFIER,
    TK_DEFINE,
    TK_UNDEF,
    TK_INCLUDE,
    TK_SETDIR,
    TK_EOF
} tokentype_t;

typedef enum
{
    AT_SHORT,
    AT_INTEGER,
    AT_FLOAT,
    AT_DOUBLE,
    AT_VECTOR
} arraytype_t;

#define SC_TOKEN_LEN    512

typedef struct
{
    int         id;
    const char  *token;
} sctokens_t;

class kexLexer
{
public:
    kexLexer(const char *filename, char *buf, int bufSize);
    ~kexLexer(void);

    bool                CheckState(void);
    void                CheckKeywords(void);
    void                MustMatchToken(int type);
    void                ExpectNextToken(int type);
    bool                Find(void);
    char                GetChar(void);
    void                Rewind(void);
    void                SkipLine(void);
    bool                Matches(const char *string);
    int                 GetNumber(void);
    double              GetFloat(void);
    kexVec3             GetVector3(void);
    kexVec4             GetVector4(void);
    kexVec3             GetVectorString3(void);
    kexVec4             GetVectorString4(void);
    void                GetString(void);
    int                 GetIDForTokenList(const sctokens_t *tokenlist, const char *token);
    void                ExpectTokenListID(const sctokens_t *tokenlist, int id);
    void                AssignFromTokenList(const sctokens_t *tokenlist,
                                            char *str, int id, bool expect);
    void                AssignFromTokenList(const sctokens_t *tokenlist,
                                            unsigned int *var, int id, bool expect);
    void                AssignFromTokenList(const sctokens_t *tokenlist,
                                            unsigned short *var, int id, bool expect);
    void                AssignFromTokenList(const sctokens_t *tokenlist,
                                            float *var, int id, bool expect);
    void                AssignVectorFromTokenList(const sctokens_t *tokenlist,
            float *var, int id, bool expect);
    void                AssignFromTokenList(const sctokens_t *tokenlist,
                                            arraytype_t type, void **data, int count,
                                            int id, bool expect, kexHeapBlock &hb);

    int                 LinePos(void) { return linepos; }
    int                 RowPos(void) { return rowpos; }
    int                 BufferPos(void) { return buffpos; }
    int                 BufferSize(void) { return buffsize; }
    char                *Buffer(void) { return buffer; }
    char                *StringToken(void) { return stringToken; }
    const char          *Token(void) const { return token; }
    const int           TokenType(void) const { return tokentype; }

private:
    void                ClearToken(void);
    void                GetNumberToken(char initial);
    void                GetLetterToken(char initial);
    void                GetSymbolToken(char c);
    void                GetStringToken(void);

    char                token[SC_TOKEN_LEN];
    char                stringToken[MAX_FILEPATH];
    char*               buffer;
    char*               pointer_start;
    char*               pointer_end;
    int                 linepos;
    int                 rowpos;
    int                 buffpos;
    int                 buffsize;
    int                 tokentype;
    const char          *name;
};

class kexParser
{
public:
    kexParser(void);
    ~kexParser(void);

    kexLexer            *Open(const char *filename);
    void                Close(void);
    void                HandleError(const char *msg, ...);
    void                PushLexer(const char *filename, char *buf, int bufSize);
    void                PopLexer(void);
    void                PushFileName(const char *name);
    void                PopFileName(void);
    byte                *CharCode(void) { return charcode; }
    const kexLexer      *CurrentLexer(void) const { return currentLexer; }

private:
    int                 OpenExternalFile(const char *name, byte **buffer) const;
    const char          *GetNestedFileName(void) const;

    kexLexer            *currentLexer;
    kexLexer            *lexers[MAX_NESTED_PARSERS];
    int                 numLexers;
    byte                charcode[256];
    char                nestedFilenames[MAX_NESTED_FILENAMES][MAX_FILEPATH];
    int                 numNestedFilenames;
};

extern kexParser *parser;

#endif
