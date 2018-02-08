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

    size_t numUniqueFlatCurves = 0;
    size_t numUniqueFlatCurveSegments = 0;
    size_t numInstancedFlatCurves = 0;
    size_t numInstancedFlatCurveSegments = 0;

    size_t numUniqueRoundCurves = 0;
    size_t numUniqueRoundCurveSegments = 0;
    size_t numInstancedRoundCurves = 0;
    size_t numInstancedRoundCurveSegments = 0;

    for (auto mesh : scene->triMeshes) {
      numUniqueTriangles += mesh->vtx.size();
    }
    for (auto curve : scene->flatCurves) {
      numUniqueFlatCurves ++;
      numUniqueFlatCurveSegments += (curve->vtx.size()-1);
    }
    for (auto curve : scene->roundCurves) {
      numUniqueRoundCurves ++;
      numUniqueRoundCurveSegments += (curve->vtx.size()-1);
    }

    for (auto inst : scene->instances) {
      numInstances++;
      switch(inst.geomType) {
      case Instance::TRI_MESH:
        numInstancedTriangles += scene->triMeshes[inst.geomID]->vtx.size();
        break;
      case Instance::FLAT_CURVE:
        numInstancedFlatCurves ++;
        numInstancedFlatCurveSegments += (scene->flatCurves[inst.geomID]->vtx.size()-1);
        break;
      case Instance::ROUND_CURVE:
        numInstancedRoundCurves ++;
        numInstancedRoundCurveSegments += (scene->roundCurves[inst.geomID]->vtx.size()-1);
        break;
      default:
        throw std::runtime_error("un-handled instance type "+std::to_string((int)inst.geomType));
      }
    }

    std::cout << " num instances          : " << prettyNumber(numInstances) << std::endl;
    std::cout << " *** triangles *** " << std::endl;
    std::cout << " - num unique      : " << prettyNumber(numUniqueTriangles) << std::endl;
    std::cout << " - num instanced   : " << prettyNumber(numInstancedTriangles) << std::endl;
    std::cout << " *** flat curves *** " << std::endl;
    std::cout << " - unique curves        : " << prettyNumber(numUniqueFlatCurves) << std::endl;
    std::cout << " - unique segments      : " << prettyNumber(numUniqueFlatCurveSegments) << std::endl;
    std::cout << " - instanced curves     : " << prettyNumber(numInstancedFlatCurves) << std::endl;
    std::cout << " - instanced segments   : " << prettyNumber(numInstancedFlatCurveSegments) << std::endl;
    std::cout << " *** round curves *** " << std::endl;
    std::cout << " - unique curves        : " << prettyNumber(numUniqueRoundCurves) << std::endl;
    std::cout << " - unique segments      : " << prettyNumber(numUniqueRoundCurveSegments) << std::endl;
    std::cout << " - instanced curves     : " << prettyNumber(numInstancedRoundCurves) << std::endl;
    std::cout << " - instanced segments   : " << prettyNumber(numInstancedRoundCurveSegments) << std::endl;
  }
}
