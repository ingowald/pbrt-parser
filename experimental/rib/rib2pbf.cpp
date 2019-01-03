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

#include "Parser.h"

namespace rib {
  RIBParser *parser = nullptr;

  void usage(const std::string &err = "")
  {
    if (err != "")
      std::cerr << "Error: " << err << std::endl << std::endl;
    std::cout << "Usage: ./exp_rib2pbf inFile.rib -o outFile.pbf" << std::endl;
    exit(err != "");
  }
  
  extern "C" int main(int ac, char **av)
  {
    std::string inFileName = "";
    std::string outFileName = "";

    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg[0] != '-')
        inFileName = arg;
      else if (arg == "-o")
        outFileName = av[++i];
      else usage("unknown argument '"+arg+"'");
    }
    if (inFileName == "")
      usage("no input file name specified");
    if (outFileName == "")
      usage("no output file name specified");
  
    RIBParser parser(inFileName);
    parser.scene->saveTo(outFileName);
  }
}
