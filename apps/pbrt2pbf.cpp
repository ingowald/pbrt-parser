// ======================================================================== //
// Copyright 2015-2019 Ingo Wald                                            //
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
#include "pbrtParser/Scene.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>
#include <assert.h>
// std
#include <set>
#include <stack>

namespace pbrt {
  namespace semantic {
    
    using std::cout;
    using std::endl;

    void usage(const std::string &msg)
    {
      if (msg != "") std::cerr << "Error: " << msg << std::endl << std::endl;
      std::cout << "./pbrt2pbf inFile.pbrt|inFile.pbf <args>" << std::endl;
      std::cout << std::endl;
      std::cout << "  -o <out.pbf>   : where to write the output to" << std::endl;
      std::cout << "                   (tris to quads, removing reundant fields, etc)" << std::endl;
      std::cout << std::endl;
      exit(msg == "" ? 0 : 1);
    }
  
    inline bool endsWith(const std::string &s, const std::string &suffix)
    {
      return s.substr(s.size()-suffix.size(),suffix.size()) == suffix;
    }

    void pbrt2pbf(int ac, char **av)
    {
      std::string inFileName;
      std::string outFileName;
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "-o") {
          assert(i+1 < ac);
          outFileName = av[++i];
        } else if (arg[0] != '-') {
          inFileName = arg;
        } else {
          usage("invalid argument '"+arg+"'");
        }          
      }

      if (outFileName == "")
        usage("no output file specified (-o <file.pbff>)");
      if (inFileName == "")
        usage("no input pbrt file specified");

      if (!endsWith(outFileName,".pbf")) {
        std::cout << "output file name missing '.pbsf' extension - adding it ..." << std::endl;
        outFileName = outFileName+".pbf";
      }
      
      std::cout << "-------------------------------------------------------" << std::endl;
      std::cout << "parsing pbrt file " << inFileName << std::endl;
      // try {
        Scene::SP scene = importPBRT(inFileName);
        std::cout << "\033[1;32m done importing scene.\033[0m" << std::endl;
        std::cout << "writing to binary file " << outFileName << std::endl;
        scene->saveTo(outFileName);
        std::cout << "\033[1;32m => yay! writing successful...\033[0m" << std::endl;
      // } catch (std::runtime_error &e) {
      //   cout << "\033[1;31mError in parsing: " << e.what() << "\033[0m\n";
      //   throw e;
      // }
    }

    extern "C" int main(int ac, char **av)
    {
      pbrt2pbf(ac,av);
      return 0;
    }

  } // ::pbrt::semantic
} // ::pbrt
