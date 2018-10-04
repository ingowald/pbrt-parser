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

// pbrt
#include "pbrt/Parser.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>

namespace pbrt_parser {

  using std::cout;
  using std::endl;

  FileName basePath = "";

  std::string exportMaterial(std::shared_ptr<Material> material)
  {
  }

  
  void writeTriangleMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
  }

  void parsePLY(const std::string &fileName,
                std::vector<vec3f> &v,
                std::vector<vec3f> &n,
                std::vector<vec3i> &idx);

  void writePlyMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
  }

  void writeObject(std::shared_ptr<Object> object, 
                   const affine3f &instanceXfm)
  {
    for (int shapeID=0;shapeID<object->shapes.size();shapeID++) {
      std::shared_ptr<Shape> shape = object->shapes[shapeID];
      if (shape->type == "trianglemesh") {
        writeTriangleMesh(shape,instanceXfm);
      } else if (shape->type == "plymesh") {
        writePlyMesh(shape,instanceXfm);
      } else {
      }
    }
    for (int instID=0;instID<object->objectInstances.size();instID++) {
      writeObject(object->objectInstances[instID]->object,
                  instanceXfm*object->objectInstances[instID]->xfm.atStart);
    }
  }


  void pbrtInfo(int ac, char **av)
  {
    std::vector<std::string> fileName;
    bool dbg = false;
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg[0] == '-') {
        if (arg == "-dbg" || arg == "--dbg")
          dbg = true;
        else if (arg == "--path" || arg == "-path")
          basePath = av[++i];
        else
          THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
      } else {
        fileName.push_back(arg);
      }          
    }
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "pbrtlint - checking valid parsing(-only) on a pbrt file ..." << std::endl;
    for (int i=0;i<fileName.size();i++)
      std::cout << " - " << fileName[i];
    std::cout << std::endl;

    if (basePath.str() == "")
      basePath = FileName(fileName[0]).path();
  
    pbrt_parser::Parser *parser = new pbrt_parser::Parser(dbg,basePath);
    try {
      for (int i=0;i<fileName.size();i++)
        parser->parse(fileName[i]);
    
      std::cout << " => yay! parsing successful..." << std::endl;
    
      std::shared_ptr<Scene> scene = parser->getScene();
      writeObject(scene->world,ospcommon::one);
    } catch (std::runtime_error e) {
      std::cerr << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      std::cerr << "(this means that either there's something wrong with that PBRT file, or that the parser can't handle it)" << std::endl;
      exit(1);
    }
  }

} // ::pbrt_parser

int main(int ac, char **av)
{
  pbrt_parser::pbrtInfo(ac,av);
  return 0;
}
