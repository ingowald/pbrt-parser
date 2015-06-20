%{

  /*
// ======================================================================== //
// Copyright 2015 Ingo Wald                                                 //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //
*/ 

  /* iw, 6/2015: adapted this code from similar lexer.ll in
     ospray.github.io/, MIT license */

#define YY_MAIN 1
#define YY_NEVER_INTERACTIVE 0

#include "common/Loc.h"
#include "pbrt/Parser.h"
#include "parser_y.hpp" // (iw) use auto-generated one, not checked-in one
#include <string>

static void lCComment();
static void lCppComment();
static void lHandleCppHash();

 using namespace plib;
 using namespace plib::pbrt;

#define register /**/
%}

%option nounput
%option noyywrap

WHITESPACE [ \t\r]+
INT_NUMBER (([0-9]+)|(0x[0-9a-fA-F]+))
FLOAT_NUMBER (([0-9]+|(([0-9]+\.[0-9]*f?)|(\.[0-9]+)))([eE][-+]?[0-9]+)?f?)|([-]?0x[01]\.?[0-9a-fA-F]+p[-+]?[0-9]+f?)

IDENT [a-zA-Z_][a-zA-Z_0-9\-]*

%%
Scale            { return TOKEN_Scale; }

{IDENT} { 
  yylval.identifierVal = strdup(yytext);
  return TOKEN_IDENTIFIER; 
}

{INT_NUMBER} { 
    char *endPtr = NULL;
    unsigned long long val = strtoull(yytext, &endPtr, 0);
    yylval.intVal = (int32_t)val;
    if (val != (unsigned int)yylval.intVal)
      Warning(plib::Loc::current, "32-bit integer has insufficient bits to represent value %s (%llx %llx)",
              yytext, yylval.intVal, (unsigned long long)val);
    return TOKEN_INT_CONSTANT; 
}
-{INT_NUMBER} { 
    char *endPtr = NULL;
    unsigned long long val = strtoull(yytext, &endPtr, 0);
    yylval.intVal = (int32_t)val;
    return TOKEN_INT_CONSTANT; 
}



{FLOAT_NUMBER} { yylval.floatVal = atof(yytext); return TOKEN_FLOAT_CONSTANT; }
-{FLOAT_NUMBER} { yylval.floatVal = atof(yytext); return TOKEN_FLOAT_CONSTANT; }

":" { return ':'; }
"," { return ','; }
"{" { return '{'; }
"}" { return '}'; }
"[" { return '['; }
"]" { return ']'; }

{WHITESPACE} { /* nothing */ }
\n { Loc::current.line++; }

^#.*\n { 
  /*lHandleCppHash(); */ 
  Loc::current.line++;
   }

. { Error(Loc::current, "Illegal character: %c (0x%x)", yytext[0], int(yytext[0])); }

%%

