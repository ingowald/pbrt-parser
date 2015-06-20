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

  /* iw, 6/2015: adapted this code from similar lexer.ll in
     ospray.github.io/, MIT license */


%error-verbose

%{

#include "common/Loc.h"
#include "pbrt/Parser.h"
  
  extern int yydebug;
  
extern int yylex();
void yyerror(const char *s);

extern char *yytext;
 namespace plib {
   namespace pbrt {
     extern Parser *parser;
   }
 }
%}

%union {
  int                intVal;
  float              floatVal;
  char              *identifierVal;
}

// DUMMY
%token TOKEN_INT_CONSTANT TOKEN_IDENTIFIER TOKEN_FLOAT_CONSTANT TOKEN_OTHER_STRING
%token TOKEN_Scale
;


%type <intVal>    Int
%type <floatVal>  Float
%type <identifierVal> Identifier

%start world
%%

world: Float Float Float { 
 }
;

Float
: TOKEN_FLOAT_CONSTANT { $$ = yylval.floatVal; }
| Int                  { $$ = $1; }
;

Int: TOKEN_INT_CONSTANT { $$ = yylval.intVal; }
;

Identifier: TOKEN_IDENTIFIER { $$ = yylval.identifierVal; }
;

%%
#include <stdio.h>

void yyerror(const char *s)
{
  std::cout << "=======================================================" << std::endl;
  PRINT(yytext);
  Error(plib::Loc::current, s);
}

