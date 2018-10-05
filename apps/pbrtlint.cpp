// ======================================================================== //
// Copyright 2015-2018 Ingo Wald                                            //
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

// pbrt_parser
#include "pbrt_parser/Scene.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>

namespace pbrt_parser {

  using std::cout;
  using std::endl;

  void pbrtLint(int ac, char **av)
  {
    std::vector<std::string> fileName;
    bool dbg = false;
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg[0] == '-') {
        if (arg == "-dbg" || arg == "--dbg")
          dbg = true;
        else
          THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
      } else {
        fileName.push_back(arg);
      }          
    }
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "pbrtlint - checking valid parsing(-only) on a pbrt file ..." << std::endl;
    for (int i=0;i<fileName.size();i++) {
      std::cout << " - " << fileName[i] << std::endl;
      try {
        std::shared_ptr<Scene> scene = pbrt_parser::Scene::parseFromFile(fileName[i],dbg);
        std::cout << " => yay! parsing successful..." << std::endl;
      } catch (std::runtime_error e) {
        std::cerr << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
        std::cerr << "(this means that either there's something wrong with that PBRT file, or that the parser can't handle it)" << std::endl;
        exit(1);
      }
    }
  }

} // ::pbrt_parser

int main(int ac, char **av)
{
  pbrt_parser::pbrtLint(ac,av);
  return 0;
}
