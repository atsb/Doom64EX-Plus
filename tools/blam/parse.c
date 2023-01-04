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

#include "types.h"
#include "common.h"
#include "macro.h"
#include "script.h"
#include "parse.h"

static void PS_ParseInner(void);
static void PS_ParseOuter(void);

static int ps_loopline;
static int ps_loopvalue;
static dboolean ps_enterloop;

char ps_userdirectory[256];

//
// PS_ParseIdentifier
//

void PS_ParseIdentifier(void)
{
    identifer_stack_t *stack;

    for(current_identifier = identifier_cap.next;
        current_identifier != &identifier_cap;
        current_identifier = current_identifier->next)
    {
        if(!strcmp(current_identifier->name, sc_parser->token))
        {
            SC_Find();

            if(current_identifier->has_args)
            {
                int i;

                SC_MustMatchToken(TK_LPAREN);
                stack = &id_stack[id_stack_count];

                for(i = 0; i < current_identifier->numargs; i++)
                {
                    SC_Find();
                    SC_PushIdStack(current_identifier->argnames[i]);

                    SC_Find();
                    if(sc_parser->tokentype == TK_RPAREN)
                    {
                        SC_Find();
                        break;
                    }
                    else
                    {
                        SC_MustMatchToken(TK_COMMA);
                    }
                }
            }
            else
            {
                stack = NULL;
            }

            SC_PushParser();

            sc_parser->buffsize         = current_identifier->bufsize;
            sc_parser->buffer           = current_identifier->buffer;
            sc_parser->pointer_start    = sc_parser->buffer;
            sc_parser->pointer_end      = sc_parser->buffer + sc_parser->buffsize;
            sc_parser->linepos          = 1;
            sc_parser->rowpos           = 1;
            sc_parser->buffpos          = 0;
            sc_parser->identifier       = current_identifier;
            sc_parser->stack            = stack;
            sc_parser->tokentype        = TK_NONE;
            sc_parser->isamacro         = true;
            sc_parser->name             = "identifier";

            return;
        }
        else if(sc_parser->stack != NULL || sc_parser->identifier != NULL)
        {
            int i;

            for(i = 0; i < current_identifier->numargs; i++)
            {
                if(!strcmp(sc_parser->token, sc_parser->stack[i].value))
                {
                    return;
                }
            }
        }
    }

    SC_Error("Unknown identifier: '%s'", sc_parser->token);
}

//
// PS_ParseInner
//

static void PS_ParseInner(void)
{
    while(SC_ReadTokens())
    {
        SC_Find();

        switch(sc_parser->tokentype)
        {
        case TK_NONE:
            break;
        case TK_EOF:
            return;
        case TK_NUMBER:
            {
                int special;
                int tag;

                special = SC_GetNumber();
                SC_ExpectNextToken(TK_COLON);
                SC_ExpectNextToken(TK_NUMBER);
                tag = SC_GetNumber();

                M_AppendAction(special, tag);

                SC_ExpectNextToken(TK_SEMICOLON);
            }
            break;
        case TK_RBRACK:
            if(ps_enterloop)
            {
                ps_enterloop = false;
                M_NewLine();
                M_AppendAction(204, ps_loopline);
                M_AppendAction(246, ps_loopvalue);
                M_NewLine();
            }
            return;
        case TK_IDENIFIER:
            PS_ParseIdentifier();
            break;
        case TK_WAIT:
            M_NewLine();
            SC_ExpectNextToken(TK_SEMICOLON);
            break;
        case TK_LOOP:
            if(ps_enterloop)
            {
                SC_Error("Nested loops is not allowed");
            }

            ps_enterloop = true;
            M_NewLine();
            ps_loopline = M_GetLine();
            SC_ExpectNextToken(TK_LPAREN);
            SC_ExpectNextToken(TK_NUMBER);
            ps_loopvalue = SC_GetNumber();
            SC_ExpectNextToken(TK_RPAREN);
            SC_ExpectNextToken(TK_LBRACK);
            PS_ParseInner();
            break;
        default:
            SC_Error("Unknown token in macro block: %s", sc_parser->token);
            break;
        }
    }
}

//
// PS_ParseOuter
//

static void PS_ParseOuter(void)
{
    while(SC_ReadTokens())
    {
        SC_Find();

        switch(sc_parser->tokentype)
        {
        case TK_NONE:
            break;
        case TK_EOF:
            return;
        case TK_IDENIFIER:
            PS_ParseIdentifier();
            break;
        case TK_POUND:
            {
                SC_Find();
                switch(sc_parser->tokentype)
                {
                case TK_INCLUDE:
                    {
                        static char includepath[256];

                        SC_GetString();
                        SC_PushParser();

                        sprintf(includepath, "%s%s",
                            ps_userdirectory, sc_stringbuffer);

                        SC_Open(includepath);
                        PS_ParseOuter();
                        SC_Close();
                    }
                    break;
                case TK_DEFINE:
                    SC_ExpectNextToken(TK_IDENIFIER);
                    SC_AddIdentifier();
                    break;
                case TK_UNDEF:
                    SC_ExpectNextToken(TK_IDENIFIER);
                    SC_RemoveIdentifier();
                    break;
                case TK_SETDIR:
                    SC_GetString();
                    sprintf(ps_userdirectory, "%s", sc_parser->token);
                    break;
                default:
                    break;
                }
            }
            break;
        case TK_MACRO:
            SC_ExpectNextToken(TK_NUMBER);
            M_NewMacro(SC_GetNumber());
            SC_ExpectNextToken(TK_LBRACK);
            PS_ParseInner();
            M_FinishMacro();
            break;
        default:
            SC_Error("Unknown token in outer block: %s", sc_parser->token);
            break;
        }
    }
}

//
// PS_ParseScript
//

void PS_ParseScript(void)
{
    ps_loopline = 0;
    ps_loopvalue = 0;
    ps_enterloop = false;

    PS_ParseOuter();
}
