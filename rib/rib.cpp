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

#include "rib.h"

extern FILE *yyin;
extern int yydebug;
extern int yyparse();

namespace rib {
  RIBParser *parser = nullptr;
}

int main(int ac, char **av)
{
  rib::parser = new rib::RIBParser();
  
  const std::string fileName = av[1];
  
  yydebug = 0; //1;
  yyin = fopen(fileName.c_str(),"r");
  yyparse();
}
