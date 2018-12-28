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
#include "pbrtParser/syntax/Scene.h"
// ospcommon
#include "ospcommon/common.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>
#include <set>

namespace pbrt {
  namespace syntax {
    
    using std::cout;
    using std::endl;

    bool endsWith(const std::string &s, const std::string &suffix)
    {
      return s.substr(s.size()-suffix.size(),suffix.size()) == suffix;
    }

    struct PBRTInfo {
      PBRTInfo(Scene::SP scene)
      {
        traverse(scene->world);
        numObjects.print("objects");
        numShapes.print("shapes");
        numTriangles.print("triangles");
        numCurves.print("curves");
        numCurveSegments.print("curve segments");
        numLights.print("lights");
        std::cout << "total num materials " << usedMaterials.size() << std::endl;
      }

      void traverseTriangleMesh(Shape::SP mesh, bool firstTime)
      {
        if (mesh->findParam<float>("P") && mesh->findParam<int>("indices")) {
          numTriangles.add(firstTime,mesh->findParam<int>("indices")->getSize()/3);
        } else {
          throw std::runtime_error("triangle mesh, but not loaded ....");
        }
      }

      void traverseCurve(Shape::SP curve, bool firstTime)
      {
        if (curve->findParam<float>("P")) {
          numCurves.add(firstTime,1);
          numCurveSegments.add(firstTime,curve->findParam<float>("P")->getSize()-1);
        } else {
          throw std::runtime_error("curve, but no 'P' field!?");
        }
      }

      void traverse(Object::SP object)
      {
        const bool firstTime = (alreadyTraversed.find(object) == alreadyTraversed.end());
        alreadyTraversed.insert(object);

        numObjects.add(firstTime,1);
        numLights.add(firstTime,object->lightSources.size());
        numVolumes.add(firstTime,object->volumes.size());
        numShapes.add(firstTime,object->shapes.size());
        numInstances.add(firstTime,object->objectInstances.size());
      
        for (auto shape : object->shapes) {
          usedMaterials.insert(shape->material);
          const std::string type = shape->type;
          if (type == "trianglemesh")
            traverseTriangleMesh(shape,firstTime);
          else if (type == "curve")
            traverseCurve(shape,firstTime);
          else
            std::cout << "un-handled shape " << shape->type << std::endl;
        }

        for (auto inst : object->objectInstances) {
          traverse(inst->object);
        }
      }

      std::set<Object::SP> alreadyTraversed;
    
      struct Counter {
        void print(const std::string &name)
        {
          std::cout << "number of " << name << std::endl;
          std::cout << " - unique    : " << ospcommon::prettyNumber(unique) << std::endl;
          std::cout << " - instanced : " << ospcommon::prettyNumber(instanced) << std::endl;
        }
        void add(bool firstTime, size_t N) { instanced += N; if (firstTime) unique += N; }
      
        size_t unique = 0;
        size_t instanced = 0;
      };

      Counter numInstances, numTriangles, numObjects, numVertices, numCurves, numCurveSegments, numShapes, numLights, numVolumes;
      std::set<Material::SP> usedMaterials;
    };
  
    void pbrtInfo(int ac, char **av)
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
      std::cout << "pbrtinfo - printing info on pbrt file ..." << std::endl;
      std::cout << "-------------------------------------------------------" << std::endl;
    
      std::shared_ptr<Scene> scene;
      try {
        if (endsWith(fileName,".pbsf"))
          scene = pbrt::syntax::readBinary(fileName);
        else if (endsWith(fileName,".pbrt"))
          scene = pbrt::syntax::parse(fileName);
        else
          throw std::runtime_error("un-recognized input file extension");
        
        std::cout << " => yay! parsing successful..." << std::endl;
        PBRTInfo info(scene);
      } catch (std::runtime_error e) {
        std::cerr << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
        std::cerr << "(this means that either there's something wrong with that PBRT file, or that the parser can't handle it)" << std::endl;
        exit(1);
      }
    }
  
    extern "C" int main(int ac, char **av)
    {
      pbrtInfo(ac,av);
      return 0;
    }
    
  } // ::pbrt::syntax
} // ::pbrt
