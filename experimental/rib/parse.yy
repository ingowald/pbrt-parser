// ======================================================================== //
// Copyright 2019 Ingo Wald                                                 //
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

/*! \file lex.ll experimental RIB parser loosely based off of ISPC's
    flex/bison framework for ints, floats, strings, and cmake rules,
    all else from scratch... */

%error-verbose

%{

#include "Parser.h"

  using namespace rib;

  namespace yacc_state {
    rib::RIBParser *parser;
  }
  using yacc_state::parser;
  
extern int yylex();
void yyerror(const char *s);

 extern char *yytext;


%}

%union {
  float               floatVal;
  int                 intVal;
  pbrt::semantic::QuadMesh  *quadMesh;
  char                *charPtr;
  ospcommon::affine3f *xfm;
  std::vector<float>  *floatArray;
  std::vector<int>    *intArray;
  std::vector<std::string> *stringArray;
  rib::Params              *params;
}

%token TOKEN_IDENTIFIER TOKEN_INT_CONSTANT TOKEN_FLOAT_CONSTANT TOKEN_STRING
%token TOKEN_Resource
%token TOKEN_ObjectBegin
%token TOKEN_ObjectEnd
%token TOKEN_ObjectInstance
%token TOKEN_AttributeBegin
%token TOKEN_AttributeEnd
%token TOKEN_Attribute
%token TOKEN_TransformBegin
%token TOKEN_TransformEnd
%token TOKEN_MotionBegin
%token TOKEN_MotionEnd
%token TOKEN_Identity
%token TOKEN_ConcatTransform
%token TOKEN_ScopedCoordinateSystem
%token TOKEN_AreaLightSource
%token TOKEN_Shader
%token TOKEN_Translate
%token TOKEN_Rotate
%token TOKEN_Scale
%token TOKEN_Sphere
%token TOKEN_LightFilter
%token TOKEN_Basis
%token TOKEN_Orientation
%token TOKEN_SubdivisionMesh
%token TOKEN_PointsGeneralPolygons

%type <quadMesh>    subdiv_mesh polygon_mesh
%type <xfm>         transformation
%type <floatVal>    float
%type <intVal>      int
%type <charPtr>     string
%type <floatArray>  floats float_array
%type <intArray>    ints int_array
%type <stringArray> strings string_array
%type <params> other_params

%start startSymbol
%%

startSymbol
: file_blocks
;

file_blocks
: file_blocks attribute_block
| file_blocks resource
|
;

resource
: TOKEN_Resource string string other_params
;

attribute_block_body
: attribute_block_body block_item
| 
;

transform_block_body
: transform_block_body block_item
| 
;

block_item
: TOKEN_Identity
| object_definition
| TOKEN_ObjectInstance string other_params
| TOKEN_Sphere float float float float
{
  parser->ignore("Sphere");
}
| TOKEN_Orientation string
{
  parser->ignore("Orientation");
  free($2);
}
| TOKEN_ScopedCoordinateSystem string
{
  parser->ignore("ScopedCoordinateSystem");
  free($2);
}
| transformation
{
  parser->applyXfm(*$1);
  delete $1;
}
| attribute
| attribute_block
| transform_block
| motion_block
{ /* does its own thing */ }
| area_light_source
| shader
| light_filter
| TOKEN_Basis string float string float
{
  parser->ignore("Basis");
  free($2);
  free($4);
}
| subdiv_mesh
{
  parser->add($1);
  /* no delete here, the parser takes ownership via sharedptr */
}
| polygon_mesh
{
  parser->add($1);
}
| resource
{ parser->ignore("Resource"); }
;

transformation
: TOKEN_Rotate float float float float
{
  vec3f axis($3,$4,$5);
  float angle = $2 * float(2*M_PI / 360.f);
  $$ = new affine3f;
  *$$ = ospcommon::affine3f::rotate(axis,angle);
}
| TOKEN_ConcatTransform '[' float float float float float float float float float float float float float float float float ']'
{
  affine3f xfm;
  xfm.l.vx = vec3f($3,$4,$5);
  xfm.l.vy = vec3f($7,$8,$9);
  xfm.l.vz = vec3f($11,$12,$13);
  xfm.p    = vec3f($15,$16,$17);
  $$ = new affine3f;
  *$$ = xfm;
}
| TOKEN_Translate float float float
{
  $$ = new affine3f;
  *$$ = ospcommon::affine3f::translate(vec3f($2,$3,$4));
}
| TOKEN_Scale float float float
{
  $$ = new affine3f;
  *$$ = ospcommon::affine3f::scale(vec3f($2,$3,$4));
}
;

polygon_mesh
:TOKEN_PointsGeneralPolygons int_array int_array /*< vertex count */ int_array /*<faceindices*/ other_params
{
  $$ = parser->makePolygonMesh(*$3,*$4,*$5);
  delete($2);
  delete($3);
  delete($4);
  delete($5);
}
;


subdiv_mesh
: TOKEN_SubdivisionMesh string int_array int_array string_array int_array int_array float_array other_params {
  $$ = parser->makeSubdivMesh($2,*$3,*$4,*$6,*$7,*$8,*$9);
  free($2);
  delete($3);
  delete($4);
  delete($6);
  delete($7);
  delete($8);
  delete($9);
 }
;

other_params
: other_params string string_array
{
  $$ = $1;
  $$->add(std::make_shared<ParamT<std::string>>($2, std::move(*$3)));
  free($2);
  delete $3;
}
| other_params string float_array
{
  $$ = $1;
  $$->add(std::make_shared<ParamT<float>>($2, std::move(*$3)));
  free($2);
  delete $3;
}
| /* new list */
{ $$ = new rib::Params(); }
;

floats
: floats float { $$ = $1; $$->push_back($2); }
|              { $$ = new std::vector<float>(); }
;

ints
: ints int { $$ = $1; $$->push_back($2); }
|          { $$ = new std::vector<int>(); }
;

strings
: strings string { $$ = $1; $$->push_back($2); free($2); }
|                { $$ = new std::vector<std::string>(); }
;

float_array
: '[' floats ']' { $$ = $2; }
;

int_array
: '[' ints ']' { $$ = $2; }
;

string_array
: '[' strings ']' { $$ = $2; }
;

// strings_and_values
// : strings_and_values string
// | strings_and_values string_array
// | strings_and_values float_array
// |
// ;

area_light_source
: TOKEN_AreaLightSource string string other_params { parser->ignore("AreaLightSource"); }
;

shader
: TOKEN_Shader string string other_params { parser->ignore("Shader"); }
;

light_filter
: TOKEN_LightFilter string string other_params { parser->ignore("LightFilter"); }
;

attribute
: TOKEN_Attribute string other_params
;

motion_block_body
: transformation transformation
{
  /*! we don't do real motion transforms, so use only the first and
      ignore the second */
  parser->applyXfm(*$1);
  delete $1;
  delete $2;
  parser->ignore("2nd arg in MotionBlock::transform"); 
}
| subdiv_mesh subdiv_mesh
{
  parser->add($1);
  /* no delete of $1 here, the parser takes ownership via sharedptr */
  
  /* since we don't do 'real' motion blocks, $2 is no longer needed
     here - kill it */
  delete $2;
  parser->ignore("2nd arg in MotionBlock::subdiv"); 
}
;

// motion_block_body
// : motion_block_body block_item
// | 
// ;

attribute_block
: TOKEN_AttributeBegin {
  parser->pushXfm();
}
attribute_block_body TOKEN_AttributeEnd {
  parser->popXfm();
}
;

motion_block
: TOKEN_MotionBegin '[' float float ']'
{
  parser->ignore("time values in MotionBlock"); 
} motion_block_body TOKEN_MotionEnd
;

transform_block
: TOKEN_TransformBegin
{
  parser->pushXfm();
}
transform_block_body TOKEN_TransformEnd
{
  parser->popXfm();
}
;

object_definition 
: TOKEN_ObjectBegin string transform_block_body TOKEN_ObjectEnd
;

float
: TOKEN_FLOAT_CONSTANT { $$ = yylval.floatVal; }
| int                  { $$ = $1; }
;

int
: TOKEN_INT_CONSTANT { $$ = yylval.intVal; }
;

string
: TOKEN_STRING { $$ = strdup(yytext+1); $$[strlen($$)-1]=0; }
;

%%
#include <stdio.h>

void yyerror(const char *s)
{
  throw std::runtime_error(FilePos::current.toString()+": general error "+std::string(s)+", yytext="+std::string(yytext));
}

