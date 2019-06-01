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

#include "pbrtParser/Scene.h"
#include "../syntactic/Scene.h"
#include "SemanticParser.h"
// std
#include <map>
#include <sstream>

namespace pbrt {

  Scene::SP importPBRT(const std::string &fileName)
  {
    pbrt::syntactic::Scene::SP pbrt;
    if (endsWith(fileName,".pbrt"))
      pbrt = pbrt::syntactic::Scene::parse(fileName);
    else
      throw std::runtime_error("could not detect input file format!? (unknown extension in '"+fileName+"')");
      
    Scene::SP scene = SemanticParser(pbrt).result;
    createFilm(scene,pbrt);
    for (auto cam : pbrt->cameras)
      scene->cameras.push_back(createCamera(cam));
    return scene;
  }

} // ::pbrt
