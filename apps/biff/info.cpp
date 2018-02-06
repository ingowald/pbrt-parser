// ======================================================================== //
// Copyright 2016-2017 Ingo Wald                                            //
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
// biff
#include "biff.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>

namespace biff {


  inline std::string prettyNumber(const size_t s) {
    double val = s;
    char result[100];
    if (val >= 1e12f) {
      sprintf(result,"%.1fT",val/1e12f);
    } else if (val >= 1e9f) {
      sprintf(result,"%.1fB",val/1e9f);
    } else if (val >= 1e6f) {
      sprintf(result,"%.1fM",val/1e6f);
    } else if (val >= 1e3f) {
      sprintf(result,"%.1fK",val/1e3f);
    } else {
      sprintf(result,"%lu",s);
    }
    return result;
  }


  extern "C" int main(int ac, char **av)
  {
    if (ac != 2)
      throw std::runtime_error("usage: biffInfo <biffSceneDirectory>");

    std::shared_ptr<Scene> scene = Scene::read(av[1]);
    size_t numInstances = 0;
    size_t numUniqueTriangles = 0;
    size_t numInstancedTriangles = 0;

    for (auto mesh : scene->triMeshes) {
      numUniqueTriangles += mesh->vtx.size();
    }

    for (auto inst : scene->instances) {
      numInstances++;
      switch(inst.geomType) {
      case Instance::TRI_MESH:
        numInstancedTriangles += scene->triMeshes[inst.geomID]->vtx.size();
        break;
      default:
        throw std::runtime_error("un-handled instance type "+std::to_string((int)inst.geomType));
      }
    }

    std::cout << " num instances          : " << prettyNumber(numInstances) << std::endl;
    std::cout << " num *unique* triangles : " << prettyNumber(numUniqueTriangles) << std::endl;
    std::cout << " num *instanced* tris   : " << prettyNumber(numInstancedTriangles) << std::endl;
  }
}
