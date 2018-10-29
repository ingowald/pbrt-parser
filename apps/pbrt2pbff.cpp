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

  /*! return 'pretty' string for given size_t (e,g "1.1M" for "1100000") */
  inline std::string prettyNumber(const size_t s)
  {
    char buf[100];
    if (s >= (1024LL*1024LL*1024LL*1024LL)) {
      sprintf(buf,"%.2fT",s/(1024.f*1024.f*1024.f*1024.f));
    } else if (s >= (1024LL*1024LL*1024LL)) {
      sprintf(buf,"%.2fG",s/(1024.f*1024.f*1024.f));
    } else if (s >= (1024LL*1024LL)) {
      sprintf(buf,"%.2fM",s/(1024.f*1024.f));
    } else if (s >= (1024LL)) {
      sprintf(buf,"%.2fK",s/(1024.f));
    } else {
      sprintf(buf,"%li",s);
    }
    return buf;
  }
  

  /*! tests if this is a st array suitable for a fully implicit quad
      mesh. If it is, the st array _must_ be a repeating sequence of
      "(0,0),(1,0),(1,1),(0,1)" ... If the st's are anything other
      than that, they carry meaning, and can't be axed */
  bool theseAreFullyImplicitQuadMeshSTs(ParamArray<float>::SP stArray)
  {
    if (!stArray) return false;
    if (stArray->size() % 8) return false;
    const float required_pattern[8] = { 0,0, 1,0, 1,1, 0,1 };
    for (int quadID=0;quadID<stArray->size()/8;quadID++) {
      for (int i=0;i<8;i++)
        if ((*stArray)[8*quadID+i] != required_pattern[i]) return false;
    }
    return true;
  }
  
  /*! tests if this is a 'indices' array suitable for a fully implicit
      quad mesh. If it is, the array _must_ be a repeating sequence of
      "(4*i,4*i+1,4*i+2,(first tri) 4*i,4*2+1,4*i+3(second tri)" */
  bool theseAreFullyImplicitQuadMeshIndices(ParamArray<int>::SP indices)
  {
    if (!indices) return false;
    if (indices->size() % 6) return false;
    for (int quadID=0;quadID<indices->size()/6;quadID++) {
      if ((*indices)[6*quadID+0] != (4*quadID+0)) return false;
      if ((*indices)[6*quadID+1] != (4*quadID+1)) return false;
      if ((*indices)[6*quadID+2] != (4*quadID+2)) return false;
      if ((*indices)[6*quadID+3] != (4*quadID+0)) return false;
      if ((*indices)[6*quadID+4] != (4*quadID+2)) return false;
      if ((*indices)[6*quadID+5] != (4*quadID+3)) return false;
    }
    return true;
  }

  /*! tests if this is a 'faceIndices' array suitable for a fully implicit
      quad mesh. If it is, the array _must_ be a repeating sequence of
      "(idx/2,idx/2)" */
  bool theseAreFullyImplicitQuadMeshFaceIndices(ParamArray<int>::SP indices)
  {
    if (!indices) return false;
    if (indices->size() % 2) return false;
    for (int quadID=0;quadID<indices->size()/2;quadID++) {
      if ((*indices)[2*quadID+0] != quadID) return false;
      if ((*indices)[2*quadID+1] != quadID) return false;
    }
    return true;
  }

  void tryConvertFatTriMeshToImplicitQuadMesh(Shape::SP shape, size_t &savedBytes)
  {
    std::cout << "moana-pass: passing over " << shape->type << std::endl;
    try {
      
      // check: is this even a triangle mesh!?
      if (shape->type != "trianglemesh")
        throw std::string("not a triangle mesh");
      
      // check: does it have a index field?
      ParamArray<int>::SP indices = shape->findParam<int>("index");
      if (!indices)
        throw std::string("no 'indices' array found!?");
      
      // check: are all those indices fully implicit?
      if (!theseAreFullyImplicitQuadMeshIndices(indices))
        throw std::string("'indices' of triangle mesh are not fully implicit quad indices");
      
      ParamArray<int>::SP faceIndices = shape->findParam<int>("faceIndices");
      if (faceIndices && !theseAreFullyImplicitQuadMeshFaceIndices(faceIndices))
        throw std::string("has 'faceIndex' field that does not match our expectations!?");
      
      ParamArray<float>::SP stArray = shape->findParam<float>("st");
      if (stArray && !theseAreFullyImplicitQuadMeshSTs(stArray))
        throw std::string("has 'st' field that does not match our expectations!?");
      
      std::cout << "-> yay! looks like a implicit quad cage; killing redundant fields!" << std::endl;
      shape->type = "implicitQuads";
      
      if (faceIndices) {
        shape->removeParam("indices");
        size_t numBytes = indices->getSize()*sizeof(int);
        std::cout << " ... axing implicit 'indices' array (" << prettyNumber(numBytes) << "B)" << std::endl;
        savedBytes += numBytes;
      }
      if (stArray) {
        shape->removeParam("st");
        size_t numBytes = stArray->getSize()*sizeof(float);
        std::cout << " ... axing implicit 'st' array (" << prettyNumber(numBytes) << "B)" << std::endl;
        savedBytes += numBytes;
      }
      if (faceIndices) {
        shape->removeParam("faceIndices");
        size_t numBytes = faceIndices->getSize()*sizeof(int);
        std::cout << " ... axing implicit 'faceIndices' array (" << prettyNumber(numBytes) << "B)" << std::endl;
        savedBytes += numBytes;
      }

      
    } catch (std::string reason) {
      std::cout << "-> did not convert: " << reason << std::endl;
    }
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
      size_t totalBytesSaved = 0;
      if (moana)
        for_each_unique_shape(scene,[&](Shape::SP &shape)
                              {tryConvertFatTriMeshToImplicitQuadMesh(shape,totalBytesSaved);});
      std::cout << "*** summary of moana pass: saved " << prettyNumber(totalBytesSaved) << "Bs" << std::endl;
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
