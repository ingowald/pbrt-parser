// ======================================================================== //
// Copyright 2015 Ingo Wald
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

// pbrt
#include "pbrt/parser.h"
// stl
#include <iostream>

int main(int ac, char **av)
{
  plib::pbrt::Parser *parser = new plib::pbrt::Parser;
  for (int i=1;i<ac;i++) {
    const std::string arg = av[i];
    if (arg[0] == '-') {
      THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
    } else {
      parser->parse(arg);
    }          
  }
  std::cout << "done parsing" << std::endl;
}
