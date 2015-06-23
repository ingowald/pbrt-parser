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

#pragma once

#include "pbrt/common.h"
// stl
#include <map>
#include <vector>

namespace plib {
  namespace pbrt {
    
    struct Param : public RefCounted {
      virtual std::string getType() const = 0;
      virtual size_t getSize() const = 0;

      /*! used during parsing, to add a newly parsed parameter value
          to the list */
      virtual void add(const std::string &text) = 0;
    };

    template<typename T>
    struct ParamT : public Param {
      ParamT(const std::string &type) : type(type) {};
      virtual std::string getType() const { return type; };
      virtual size_t getSize() const { return paramVec.size(); }
      /*! used during parsing, to add a newly parsed parameter value
          to the list */
      virtual void add(const std::string &text);
    private:
      std::string type;
      std::vector<T> paramVec;
    };

    struct Material : public RefCounted {
      std::string name;
      Material(const std::string &name) : name(name) {};
      std::map<std::string,Ref<Param> > param;
    };

    struct Texture : public RefCounted {
      std::string name;
      std::string texelType;
      std::string mapType;

      Texture(const std::string &name,
              const std::string &texelType,
              const std::string &mapType) 
        : name(name), texelType(texelType), mapType(mapType)
      {};
      std::map<std::string,Ref<Param> > param;
    };

    struct Node : public RefCounted {
      Node(const std::string &type) : type(type) {};

      const std::string type;
      std::map<std::string,Ref<Param> > param;
    };

    struct Camera : public Node {
      Camera(const std::string &type) : Node(type) {};
    };

    struct LightSource : public Node {
      LightSource(const std::string &type) : Node(type) {};
    };

    struct Film : public Node {
      Film(const std::string &type) : Node(type) {};
    };

    struct Renderer : public Node {
      Renderer(const std::string &type) : Node(type) {};
    };

    struct Scene : public RefCounted {
    };

  }
}
