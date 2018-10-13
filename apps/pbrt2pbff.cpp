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

  void pbrt2pbff(int ac, char **av)
  {
    std::string fileName;
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg[0] == '-') {
          throw std::runtime_error("invalid argument '"+arg+"'");
      } else {
        fileName = arg;
      }          
    }
    
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "parsing pbrt file " << fileName << std::endl;
    std::shared_ptr<Scene> scene;
    try {
      scene = pbrt_parser::Scene::parseFromFile(fileName);
      std::cout << " => yay! parsing successful..." << std::endl;
    } catch (std::runtime_error e) {
      std::cerr << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      std::cerr << "(this means that either there's something wrong with that PBRT file, or that the parser can't handle it)" << std::endl;
      exit(1);
    }
    const std::string outFileName = "binary.pbff";
    std::cout << "writing to binary file " << outFileName << std::endl;
    pbrtParser_saveToBinary(scene,outFileName);
    std::cout << " => yay! writing successful..." << std::endl;
  }

} // ::pbrt_parser

int main(int ac, char **av)
{
  pbrt_parser::pbrt2pbff(ac,av);
  return 0;
}
