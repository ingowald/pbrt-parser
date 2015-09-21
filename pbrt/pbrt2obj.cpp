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
#include "pbrt/Parser.h"
// stl
#include <iostream>
#include <vector>

using namespace plib::pbrt;

namespace plib {
  namespace pbrt {

    using std::cout;
    using std::endl;

    void writeTriangleMesh(Ref<Shape> shape, const affine3f &instanceXfm)
    {
      const affine3f xfm = instanceXfm*shape->transform;

      static size_t numVerticesWritten = 0;
      size_t firstVertexID = numVerticesWritten;
      
//       "indices"
// "point P"
// "float uv"
// "normal N"
    }

    void writeObject(Ref<Object> object, const affine3f &instanceXfm)
    {
      for (int shapeID=0;shapeID<object->shapes.size();shapeID++) {
        Ref<Shape> shape = object->shapes[shapeID];
        if (shape->type == "trianglemesh") {
          writeTriangleMesh(shape,instanceXfm);
        } else 
          cout << "invalid shape #" << shapeID << " : " << shape->type << endl;
      }
      for (int instID=0;instID<object->objectInstances.size();instID++) {
        writeObject(object->objectInstances[instID]->object,
                    instanceXfm*object->objectInstances[instID]->xfm);
      }
    }


    void pbrt2obj(int ac, char **av)
    {
      std::vector<std::string> fileName;
      bool dbg = false;
      std::string outFileName = "";
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg[0] == '-') {
          if (arg == "-dbg" || arg == "--dbg")
            dbg = true;
          else if (arg == "-o")
            outFileName = av[++i];
          else
            THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
        } else {
          fileName.push_back(arg);
        }          
      }
  
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
        writeObject(scene.cast<Object>(),embree::one);
      } catch (std::runtime_error e) {
        std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
        exit(1);
      }
    }

  }
}

int main(int ac, char **av)
{
  plib::pbrt::pbrt2obj(ac,av);
  return 0;
}
