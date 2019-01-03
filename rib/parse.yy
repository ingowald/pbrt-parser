%error-verbose

%{

#include "Parser.h"

  using namespace rib;

  namespace rib {
  extern RIBParser *parser;
  }
  
extern int yylex();
void yyerror(const char *s);

 extern char *yytext;


%}

%union {
  float               floatVal;
  int                 intVal;
  char                *charPtr;
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
| TOKEN_ScopedCoordinateSystem string
| TOKEN_Translate float float float
| TOKEN_Rotate float float float float
| TOKEN_Scale float float float
| concat_transform
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
| subdiv_mesh
| TOKEN_PointsGeneralPolygons int_array int_array /*< vertex count */ int_array /*<faceindices*/ other_params
| resource
;

concat_transform
: TOKEN_ConcatTransform '[' float float float float float float float float float float float float float float float float ']'
;

subdiv_mesh
: TOKEN_SubdivisionMesh string int_array int_array string_array int_array int_array float_array other_params {
  parser->makeSubdivMesh($2,*$3,*$4,*$6,*$7,*$8,*$9);
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
: TOKEN_AreaLightSource string string other_params
;

shader
: TOKEN_Shader string string other_params
;

light_filter
: TOKEN_LightFilter string string other_params
;

attribute
: TOKEN_Attribute string other_params
;

motion_block_body
: concat_transform concat_transform
| subdiv_mesh subdiv_mesh
;

// motion_block_body
// : motion_block_body block_item
// | 
// ;

attribute_block
: TOKEN_AttributeBegin { parser->pushXfm(); } attribute_block_body TOKEN_AttributeEnd { parser->popXfm(); }
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
: TOKEN_STRING { $$ = strdup(yytext+1); $$[strlen($$)-1]=0; }
;

%%
#include <stdio.h>

void yyerror(const char *s)
{
  throw std::runtime_error(FilePos::current.toString()+": general error "+std::string(s)+", yytext="+std::string(yytext));
}

