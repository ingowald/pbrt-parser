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

#pragma once

#include "FilePos.h"
#include "ospcommon/AffineSpace.h"

#include <stack>

namespace rib {
  using namespace pbrt::semantic;
  using namespace ospcommon;
  using ospcommon::affine3f;
  
  typedef enum { FLOAT, INT, STRING } ParamType;

  template<typename T> struct getParamType;
  template<> struct getParamType<float> { static const ParamType value = FLOAT; };
  template<> struct getParamType<int> { static const ParamType value = INT; };
  template<> struct getParamType<std::string> { static const ParamType value = STRING; };
    
  struct Param {
    typedef std::shared_ptr<Param> SP;
    Param(ParamType type, const std::string &name)
      : type(type),
        name(name)
    {}
    
    const ParamType type;
    const std::string name;
  };
  
  template<typename T>
  struct ParamT : public Param {
    ParamT(const std::string &name, const std::vector<T> &values)
      : Param(getParamType<T>::value,name),
        values(values)
    {}
        
    std::vector<T> values;
  };

  struct Params {
    typedef std::shared_ptr<Params> SP;
    
    Param::SP operator[](const std::string &name)
    {
      if (values.find(name) == values.end())
        return Param::SP();
      else
        return values[name];
    }
    void add(Param::SP p) {
      values[p->name] = p;
    }
    
    std::map<std::string,Param::SP> values;
  };
  
  struct RIBParser {
    RIBParser(const std::string &fileName);

    affine3f currentXfm() { return xfmStack.top(); }

    void ignore(const std::string &what) { ignored[what]++; }
    
    void applyXfm(const affine3f &xfm)
    {
      affine3f newXfm = xfmStack.top() * xfm;
      // std::cout << "xfm: " << xfmStack.top() << "+" << xfm << " -> " << newXfm << std::endl;
      xfmStack.top() = newXfm;
    }
    void pushXfm() { xfmStack.push(xfmStack.top()); }
    void popXfm() { xfmStack.pop(); }

    void add(pbrt::semantic::QuadMesh *qm);
    
    /*! note: this creates a POINTER, not a shared-ptr, else the yacc
        stack gets confused! */
    pbrt::semantic::QuadMesh *
    makeSubdivMesh(const std::string &type,
                   const std::vector<int> &faceVertexCount,
                   const std::vector<int> &vertexIndices,
                   const std::vector<int> &ignore0,
                   const std::vector<int> &ignore1,
                   const std::vector<float> &ignore2,
                   Params &params);

    /*! note: this creates a POINTER, not a shared-ptr, else the yacc
        stack gets confused! */
    pbrt::semantic::QuadMesh *
    makePolygonMesh(const std::vector<int> &faceVertexCount,
                    const std::vector<int> &vertexIndices,
                    Params &params);

    void instantiate(const std::string &name)
    {
      Object::SP obj = namedObjects[name];
      if (!obj)
        std::cerr << "warning: cannot find object '" << name << "'" << std::endl;
      else {
        affine3f xfm = xfmStack.top();
        Instance::SP inst = std::make_shared<Instance>(obj,xfm);
        currentObject->instances.push_back(inst);
      }
    }
    void beginObject(const std::string &name)
    {
      objectStack.push(currentObject);
      xfmStack.push(affine3f(one));
      currentObject = std::make_shared<Object>();
      std::cout << "#rib: starting on new object '" << name << "'" << std::endl;
      namedObjects[name] = currentObject;
    }
    void endObject()
    {
      currentObject = objectStack.top();
      objectStack.pop();
      xfmStack.pop();
    }
    
    Scene::SP  scene;
    Object::SP currentObject;

    std::stack<affine3f> xfmStack;
    std::stack<Object::SP> objectStack;
    std::map<std::string,Object::SP> namedObjects;
    /*! occurrances of ignored statements in rib file ... only used
        for proper warning of missing/ignored objects */
    std::map<std::string,int> ignored;
  };
  
} // ::rib
