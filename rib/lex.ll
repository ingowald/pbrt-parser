%{

#define YY_MAIN 1
#define YY_NEVER_INTERACTIVE 1

#include "rib.h"
#include "parse_y.hpp"
#include <string>

  rib::FilePos rib::FilePos::current;
%}

%option nounput
%option noyywrap

WHITESPACE [ \t\r]+
INT_NUMBER (([0-9]+)|(0x[0-9a-fA-F]+))
FLOAT_NUMBER (([0-9]+|(([0-9]+\.[0-9]*f?)|(\.[0-9]+)))([eE][-+]?[0-9]+)?f?)|([-]?0x[01]\.?[0-9a-fA-F]+p[-+]?[0-9]+f?)

IDENT [a-zA-Z_()][a-zA-Z_0-9\-()\:]*

%%
Resource        { return TOKEN_Resource; }
AttributeBegin  { return TOKEN_AttributeBegin; }
AttributeEnd    { return TOKEN_AttributeEnd; }
Attribute       { return TOKEN_Attribute; }
TransformBegin  { return TOKEN_TransformBegin; }
TransformEnd    { return TOKEN_TransformEnd; }
MotionBegin     { return TOKEN_MotionBegin; }
MotionEnd       { return TOKEN_MotionEnd; }
Identity        { return TOKEN_Identity; }
ConcatTransform { return TOKEN_ConcatTransform; }
ScopedCoordinateSystem { return TOKEN_ScopedCoordinateSystem; }
AreaLightSource { return TOKEN_AreaLightSource; }
Translate       { return TOKEN_Translate; }
Orientation     { return TOKEN_Orientation; }
Rotate          { return TOKEN_Rotate; }
Scale           { return TOKEN_Scale; }
Sphere          { return TOKEN_Sphere; }
Shader          { return TOKEN_Shader; }
LightFilter     { return TOKEN_LightFilter; }
Basis           { return TOKEN_Basis; }
SubdivisionMesh { return TOKEN_SubdivisionMesh; }
PointsGeneralPolygons { return TOKEN_PointsGeneralPolygons; }
ObjectBegin     { return TOKEN_ObjectBegin; }
ObjectEnd       { return TOKEN_ObjectEnd; }
ObjectInstance  { return TOKEN_ObjectInstance; }


{IDENT} { 
  yylval.charPtr = strdup(yytext);
  return TOKEN_IDENTIFIER; 
}

{INT_NUMBER} { 
    char *endPtr = NULL;
    unsigned long long val = strtoull(yytext, &endPtr, 0);
    yylval.intVal = (int32_t)val;
    // if (val != (unsigned int)yylval.intVal)
        // Warning(FilePos::current, "32-bit integer has insufficient bits to represent value %s (%llx %llx)",
        //         yytext, yylval.intVal, (unsigned long long)val);
    return TOKEN_INT_CONSTANT; 
}

-{INT_NUMBER} { 
    char *endPtr = NULL;
    unsigned long long val = strtoull(yytext, &endPtr, 0);
    yylval.intVal = (int32_t)val;
    return TOKEN_INT_CONSTANT; 
}

L?\"(\\.|[^\\"])*\" {
    return TOKEN_STRING;
}



{FLOAT_NUMBER} { yylval.floatVal = atof(yytext); return TOKEN_FLOAT_CONSTANT; }
-{FLOAT_NUMBER} { yylval.floatVal = atof(yytext); return TOKEN_FLOAT_CONSTANT; }

"[" { return '['; }
"]" { return ']'; }

{WHITESPACE} { /* nothing */ }
\n { rib::FilePos::current.line++; }

#.* { /* do nothing */ }

. { throw std::runtime_error("illegal character..."); } //Error(FilePos::current, "Illegal character: %c (0x%x)", yytext[0], int(yytext[0])); }

%%




