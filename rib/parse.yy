%error-verbose

%{

#include "rib.h"

  using namespace rib;
  
extern int yylex();
void yyerror(const char *s);

 extern char *yytext;


%}

%union {
  float              floatVal;
  int                intVal;
  char               *charPtr;
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

%type <floatVal>  float
%type <intVal>    int

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
| TOKEN_ScopedCoordinateSystem string
| TOKEN_Translate float float float
| TOKEN_Rotate float float float float
| TOKEN_Scale float float float
| TOKEN_ConcatTransform '[' float float float float float float float float float float float float float float float float ']'
| TOKEN_Sphere float float float float
| TOKEN_Orientation string
| attribute
| attribute_block
| transform_block
| motion_block
| area_light_source
| shader
| light_filter
| TOKEN_Basis string float string float
| TOKEN_SubdivisionMesh string int_array int_array string_array int_array int_array float_array other_params
| TOKEN_PointsGeneralPolygons int_array int_array /*< vertex count */ int_array /*<faceindices*/ other_params
| resource
;


other_param
: string string_array
| string float_array
;

other_params
: other_params other_param
|
;

floats
: floats float
|
;

ints
: ints int
|
;

strings
: strings string
|
;

float_array
: '[' floats ']'
;

int_array
: '[' ints ']'
;

string_array
: '[' strings ']'
;

strings_and_values
: strings_and_values string
| strings_and_values string_array
| strings_and_values float_array
|
;

area_light_source
: TOKEN_AreaLightSource strings_and_values
;

shader
: TOKEN_Shader strings_and_values
;

light_filter
: TOKEN_LightFilter strings_and_values
;

attribute
: TOKEN_Attribute strings_and_values
;

motion_block_body
: motion_block_body block_item
| 
;

attribute_block
: TOKEN_AttributeBegin attribute_block_body TOKEN_AttributeEnd
;

motion_block
: TOKEN_MotionBegin '[' float float ']' motion_block_body TOKEN_MotionEnd
;

transform_block
: TOKEN_TransformBegin transform_block_body TOKEN_TransformEnd
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
: TOKEN_STRING { }
;

%%
#include <stdio.h>

void yyerror(const char *s)
{
  throw std::runtime_error(FilePos::current.toString()+": general error "+std::string(s)+", yytext="+std::string(yytext));
}

