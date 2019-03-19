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

// ospcommon, which we use for a vector class
#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
// pbrt-parser
#define PBRT_PARSER_VECTYPE_NAMESPACE    ospcommon
#include "pbrtParser/semantic/Scene.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>
#include <set>

namespace pbrt {
  namespace semantic {
    
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
        numQuads.print("quads");
        numDisks.print("disks");
        numSpheres.print("spheres");
        numCurves.print("curves");
        numCurveSegments.print("curve segments");
        numLights.print("lights");
        std::cout << "total num materials " << usedMaterials.size() << std::endl;
        std::cout << "scene bounds " << scene->getBounds() << std::endl;
      }

      void traverse(Object::SP object)
      {
        const bool firstTime = (alreadyTraversed.find(object) == alreadyTraversed.end());
        alreadyTraversed.insert(object);

        numObjects.add(firstTime,1);
        // numLights.add(firstTime,object->lightSources.size());
        // numVolumes.add(firstTime,object->volumes.size());
        numShapes.add(firstTime,object->shapes.size());
      
        for (auto geom : object->shapes) {
          usedMaterials.insert(geom->material);
          if (TriangleMesh::SP mesh=std::dynamic_pointer_cast<TriangleMesh>(geom)){
            numTriangles.add(firstTime,mesh->index.size());
          } else if (QuadMesh::SP mesh=std::dynamic_pointer_cast<QuadMesh>(geom)){
            numQuads.add(firstTime,mesh->index.size());
          } else if (Sphere::SP sphere=std::dynamic_pointer_cast<Sphere>(geom)){
            numSpheres.add(firstTime,1);
          } else if (Disk::SP disk=std::dynamic_pointer_cast<Disk>(geom)){
            numDisks.add(firstTime,1);
          } else if (Curve::SP curves=std::dynamic_pointer_cast<Curve>(geom)){
            numCurves.add(firstTime,1);
          } else
            std::cout << "un-handled geometry type : " << geom->toString() << std::endl;
        }

        numInstances.add(firstTime,object->instances.size());
        for (auto inst : object->instances) {
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

      Counter
      numInstances,
                                            numTriangles,
                                            numQuads,
                                            numSpheres,
                                            numDisks,
                                            numObjects,
                                            numVertices,
                                            numCurves,
                                            numCurveSegments,
                                            numShapes,
                                            numLights,
                                            numVolumes;
      std::set<Material::SP> usedMaterials;
    };
  
    void pbfInfo(int ac, char **av)
    {
      std::string fileName;
      bool parseOnly = false;
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg == "--lint" || arg == "-lint") {
          parseOnly = true;
        } else if (arg[0] == '-') {
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
        if (endsWith(fileName,".pbsf") || endsWith(fileName,".pbrt"))
          scene = importPBRT(fileName);
        else if (endsWith(fileName,".pbf"))
          // scene = importPBRT(fileName);
          scene = Scene::loadFrom(fileName);
        else
          throw std::runtime_error("un-recognized input file extension");
        
        std::cout << " => yay! parsing successful..." << std::endl;
        if (parseOnly) exit(0);
        PBRTInfo info(scene);
      } catch (std::runtime_error e) {
        std::cerr << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
        std::cerr << "(this means that either there's something wrong with that PBRT file, or that the parser can't handle it)" << std::endl;
        exit(1);
      }
    }
  
    extern "C" int main(int ac, char **av)
    {
      pbfInfo(ac,av);
      return 0;
    }
    
  } // ::pbrt::semantic
} // ::pbrt
