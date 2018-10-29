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
#include "pbrt_parser/Scene.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>
#include <assert.h>
// std
#include <set>
#include <stack>

namespace pbrt_parser {

  using std::cout;
  using std::endl;


  void runMoanaOptimizationsOn(Shape::SP shape)
  {
    std::cout << "moana-pass: passing over " << shape->type << std::endl;
  }

  /*! executes given lambda for every (unique) shape in the scene.
    Note that the shape parameter passed to the lambda is a reference
    to the variable in Object::shapes vector that contained that
    shape, so in theory it can be overwritten with a different
    shape. */
  template<typename Lambda>
  void for_each_unique_shape(Scene::SP scene,
                             const Lambda &lambda)
  {
    assert(scene);
    /*! list of all objects already traversed, to make sure we're not
      doing any object twice */
    std::set<Object::SP>   alreadyDone;
    std::stack<Object::SP> workStack;
    workStack.push(scene->world);
    while (!workStack.empty()) {
      Object::SP object = workStack.top();
      workStack.pop();
      if (!object)
        continue;
      if (alreadyDone.find(object) != alreadyDone.end())
        continue;

      alreadyDone.insert(object);
      for (auto inst : object->objectInstances)
        workStack.push(inst->object);
      for (auto &shape : object->shapes)
        lambda(shape);
    }
  }

  void usage(const std::string &msg)
  {
    if (msg != "")
      std::cerr << "Error: " << msg << std::endl << std::endl;
    std::cout << "./pbrt2pbff inFile.pbrt <args>" << std::endl;
    std::cout << std::endl;
    std::cout << "  -o <out.pbff>  : where to write the output to" << std::endl;
    std::cout << "  --moana        : perform some moana-scene specific optimizations" << std::endl;
    std::cout << "                   (tris to quads, removing reundant fields, etc)" << std::endl;
  }
  
  void pbrt2pbff(int ac, char **av)
  {
    std::string inFileName;
    std::string outFileName;
    bool moana = false;
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg == "-o") {
        assert(i+1 < ac);
        outFileName = av[++i];
      } else if (arg == "-moana" || arg == "--moana" || arg == "--optimize-moana" || arg == "-om") {
        moana = true;
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

    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "parsing pbrt file " << inFileName << std::endl;
    std::shared_ptr<Scene> scene;
    try {
      scene = pbrt_parser::Scene::parseFromFile(inFileName);
      std::cout << " => yay! parsing successful..." << std::endl;
    } catch (std::runtime_error e) {
      std::cerr << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      std::cerr << "(this means that either there's something wrong with that PBRT file, or that the parser can't handle it)" << std::endl;
      exit(1);
    }
    try {
      if (moana)
        for_each_unique_shape(scene,[](Shape::SP &shape)
                              {runMoanaOptimizationsOn(shape);});
    } catch (std::runtime_error e) {
      std::cerr << "**** ERROR IN RUNNING MOANA OPTIMIZATIONS ****" << std::endl << e.what() << std::endl;
      exit(1);
    }
    std::cout << "writing to binary file " << outFileName << std::endl;
    pbrtParser_saveToBinary(scene,outFileName);
    std::cout << " => yay! writing successful..." << std::endl;
  }

} // ::pbrt_parser

int main(int ac, char **av)
{
  pbrt_parser::pbrt2pbff(ac,av);
  return 0;
}
