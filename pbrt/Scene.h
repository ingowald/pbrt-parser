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

    struct Attributes: public RefCounted {
      Attributes();
      Attributes(const Attributes &other);

      virtual Attributes *clone() const { return new Attributes(*this); }
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
      virtual std::string toString() const { return type; }

      const std::string type;
      std::map<std::string,Ref<Param> > param;
    };

    struct Camera : public Node {
      Camera(const std::string &type) : Node(type) {};
    };

    struct Sampler : public Node {
      Sampler(const std::string &type) : Node(type) {};
    };
    struct SurfaceIntegrator : public Node {
      SurfaceIntegrator(const std::string &type) : Node(type) {};
    };
    struct VolumeIntegrator : public Node {
      VolumeIntegrator(const std::string &type) : Node(type) {};
    };
    struct PixelFilter : public Node {
      PixelFilter(const std::string &type) : Node(type) {};
    };

    struct Accelerator : public Node {
      Accelerator(const std::string &type) : Node(type) {};
    };

    struct Shape : public Node {
      Shape(const std::string &type,
            Ref<Attributes> attributes,
            affine3f &transform) 
        : Node(type), 
          attributes(attributes),
          transform(transform)
      {};

      Ref<Attributes> attributes;
      affine3f        transform;
    };
    struct Volume : public Node {
      Volume(const std::string &type) : Node(type) {};
    };

    struct LightSource : public Node {
      LightSource(const std::string &type) : Node(type) {};
    };
    struct AreaLightSource : public Node {
      AreaLightSource(const std::string &type) : Node(type) {};
    };

    struct Film : public Node {
      Film(const std::string &type) : Node(type) {};
    };

    struct Renderer : public Node {
      Renderer(const std::string &type) : Node(type) {};
    };

    // a "LookAt" in the pbrt file has three vec3fs, no idea what for
    // right now - need to rename once we figure that out
    struct LookAt : public RefCounted {
      LookAt(const vec3f &v0, 
             const vec3f &v1, 
             const vec3f &v2)
        : v0(v0),v1(v1),v2(v2)
      {}

      vec3f v0, v1, v2;
    };

    //! what's in a objectbegin/objectned, as well as the root object
    struct Object : public RefCounted {
      struct Instance : public RefCounted {
        Instance(Ref<Object> object,
                 affine3f    xfm)
          : object(object), xfm(xfm)
        {}

        Ref<Object> object;
        affine3f    xfm;
      };

      std::string name;

      //! list of cameras defined in the scene
      std::vector<Ref<Camera> > cameras;
      //! list of all shapes defined in the scene
      std::vector<Ref<Shape> > shapes;
      //! list of all shapes defined in the scene
      std::vector<Ref<Object::Instance> > objectInstances;
    };


    struct Scene : public Object {

      //! last lookat specified in the scene, or NULL if none.
      Ref<LookAt> lookAt;

      //! last sampler specified in the scene, or NULL if none.
      Ref<Sampler> sampler;

      //! last volume integrator specified in the scene, or NULL if none.
      Ref<VolumeIntegrator> volumeIntegrator;

      //! last surface integrator specified in the scene, or NULL if none.
      Ref<SurfaceIntegrator> surfaceIntegrator;

      //! last pixel filter specified in the scene, or NULL if none.
      Ref<PixelFilter> pixelFilter;
    };

  }
}
