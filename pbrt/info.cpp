// ======================================================================== //
// Copyright 2015-2017 Ingo Wald                                            //
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
#include "pbrt/Parser.h"
// stl
#include <iostream>
#include <vector>

using namespace plib::pbrt;

int main(int ac, char **av)
{
  std::vector<std::string> fileName;
  bool dbg = false;
  bool summary = false;
  for (int i=1;i<ac;i++) {
    const std::string arg = av[i];
    if (arg[0] == '-') {
      if (arg == "-dbg" || arg == "--dbg")
        dbg = true;
      else if (arg == "--summary")
        summary = true;
      else
        THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
    } else {
      fileName.push_back(arg);
    }          
  }

  if (summary) {
    std::vector<std::string> successfulFiles;
    std::vector<std::string> failedFiles;
    for (int i=0;i<fileName.size();i++) {
      const std::string fn = fileName[i];
      std::cout << "parsing:" << fn << std::endl;
      plib::pbrt::Parser *parser = NULL;
      try {
        parser = new plib::pbrt::Parser(dbg);
        parser->parse(fn);
        successfulFiles.push_back(fn);
        std::cout << "==> parsing successful (grammar only for now)" << std::endl;
      } catch (std::runtime_error e) {
        if (parser) delete parser;
        failedFiles.push_back(fn);
        std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      }
    }
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "PARSING SUMMARY" << std::endl;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "Failed:" << std::endl;
    for (int i=0;i<failedFiles.size();i++)
      std::cout << " - " << failedFiles[i] << std::endl;
    std::cout << "Successful:" << std::endl;
    for (int i=0;i<successfulFiles.size();i++)
      std::cout << " - " << successfulFiles[i] << std::endl;
  } else {
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "parsing:";
    for (int i=0;i<fileName.size();i++)
      std::cout << " " << fileName[i];
    std::cout << std::endl;

    plib::pbrt::Parser *parser = new plib::pbrt::Parser(dbg);
    try {
      for (int i=0;i<fileName.size();i++)
        parser->parse(fileName[i]);
    
      std::cout << "==> parsing successful (grammar only for now)" << std::endl;

      embree::Ref<Scene> scene = parser->getScene();

      std::cout << "num cameras: " << scene->cameras.size() << std::endl;
      for (int i=0;i<scene->cameras.size();i++)
        std::cout << " - " << scene->cameras[i]->toString() << std::endl;

      std::cout << "num shapes: " << scene->shapes.size() << std::endl;
      for (int i=0;i<scene->shapes.size();i++)
        std::cout << " - " << scene->shapes[i]->toString() << std::endl;

    } catch (std::runtime_error e) {
      std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      exit(1);
    }
  }
}
