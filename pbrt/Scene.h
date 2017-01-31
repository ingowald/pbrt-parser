// ======================================================================== //
// Copyright 2015-2017 Ingo Wald                                            //
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

#include "pbrt/pbrt.h"
// stl
#include <map>
#include <vector>

namespace pbrt_parser {
    
  struct PBRT_PARSER_INTERFACE Param {
    virtual std::string getType() const = 0;
    virtual size_t getSize() const = 0;
    virtual std::string toString() const = 0;

    /*! used during parsing, to add a newly parsed parameter value
      to the list */
    virtual void add(const std::string &text) = 0;
  };

  template<typename T>
  struct PBRT_PARSER_INTERFACE ParamT : public Param {
    ParamT(const std::string &type) : type(type) {};
    virtual std::string getType() const { return type; };
    virtual size_t getSize() const { return paramVec.size(); }
    virtual std::string toString() const;

    /*! used during parsing, to add a newly parsed parameter value
      to the list */
    virtual void add(const std::string &text);
    //    private:
    std::string type;
    std::vector<T> paramVec;
  };

  /*! any class that can store (and query) parameters */
  struct PBRT_PARSER_INTERFACE Parameterized {

    vec3f getParam3f(const std::string &name, const vec3f &fallBack) const;

    template<typename T>
      std::shared_ptr<ParamT<T> > findParam(const std::string &name) const {
      auto it = param.find(name);
      if (it == param.end()) return std::shared_ptr<ParamT<T>>();
      return std::dynamic_pointer_cast<ParamT<T> >(it->second);
    }

    std::map<std::string,std::shared_ptr<Param> > param;
  };

  struct PBRT_PARSER_INTERFACE Attributes {
    Attributes();
    Attributes(const Attributes &other);

    virtual std::shared_ptr<Attributes> clone() const { return std::make_shared<Attributes>(*this); }
  };

  struct PBRT_PARSER_INTERFACE Material : public Parameterized {
    Material(const std::string &type) : type(type) {};

    /*! pretty-print this material (for debugging) */
    std::string toString() const;

    /*! the 'type' of the material, such as 'uber'material, 'matte',
      'mix' etc. Note that the PBRT format has two inconsistent
      ways of specifying that type: for the 'Material' command it
      specifies the type explicitly right after the 'matierla'
      command; for the 'makenamedmaterial' it uses an implicit
      'type' parameter */
    std::string type;
  };

  struct PBRT_PARSER_INTERFACE Texture {
    std::string name;
    std::string texelType;
    std::string mapType;

    Texture(const std::string &name,
            const std::string &texelType,
            const std::string &mapType) 
      : name(name), texelType(texelType), mapType(mapType)
    {};
    std::map<std::string,std::shared_ptr<Param> > param;
  };

  struct PBRT_PARSER_INTERFACE Node : public Parameterized {
    Node(const std::string &type) : type(type) {};
    virtual std::string toString() const { return type; }

    const std::string type;
    //      std::map<std::string,std::shared_ptr<Param> > param;
  };

  struct PBRT_PARSER_INTERFACE Camera : public Node {
    Camera(const std::string &type) : Node(type) {};
  };

  struct PBRT_PARSER_INTERFACE Sampler : public Node {
    Sampler(const std::string &type) : Node(type) {};
  };
  struct PBRT_PARSER_INTERFACE Integrator : public Node {
    Integrator(const std::string &type) : Node(type) {};
  };
  struct PBRT_PARSER_INTERFACE SurfaceIntegrator : public Node {
    SurfaceIntegrator(const std::string &type) : Node(type) {};
  };
  struct PBRT_PARSER_INTERFACE VolumeIntegrator : public Node {
    VolumeIntegrator(const std::string &type) : Node(type) {};
  };
  struct PBRT_PARSER_INTERFACE PixelFilter : public Node {
    PixelFilter(const std::string &type) : Node(type) {};
  };

  /*! a PBRT 'geometric shape' (a geometry in ospray terms) - ie,
    something that has primitives that together form some sort of
    surface(s) that a ray can intersect*/
  struct PBRT_PARSER_INTERFACE Shape : public Node {
    /*! constructor */
    Shape(const std::string &type,
          std::shared_ptr<Material>   material,
          std::shared_ptr<Attributes> attributes,
          affine3f &transform);

    /*! material active when the shape was generated - PBRT has only
      one material per shape */
    std::shared_ptr<Material>   material;
    std::shared_ptr<Attributes> attributes;
    /*! the active transform stack that was active when then shape
      was defined */
    affine3f        transform;
  };

  struct PBRT_PARSER_INTERFACE Volume : public Node {
    Volume(const std::string &type) : Node(type) {};
  };

  struct PBRT_PARSER_INTERFACE LightSource : public Node {
    LightSource(const std::string &type) : Node(type) {};
  };

  struct PBRT_PARSER_INTERFACE AreaLightSource : public Node {
    AreaLightSource(const std::string &type) : Node(type) {};
  };

  struct PBRT_PARSER_INTERFACE Film : public Node {
    Film(const std::string &type) : Node(type) {};
  };

  struct PBRT_PARSER_INTERFACE Accelerator : public Node {
    Accelerator(const std::string &type) : Node(type) {};
  };

  struct PBRT_PARSER_INTERFACE Renderer : public Node {
    Renderer(const std::string &type) : Node(type) {};
  };

  // a "LookAt" in the pbrt file has three vec3fs, no idea what for
  // right now - need to rename once we figure that out
  struct PBRT_PARSER_INTERFACE LookAt {
    LookAt(const vec3f &v0, 
           const vec3f &v1, 
           const vec3f &v2)
      : v0(v0),v1(v1),v2(v2)
    {}

    vec3f v0, v1, v2;
  };

  //! what's in a objectbegin/objectned, as well as the root object
  struct PBRT_PARSER_INTERFACE Object {
    Object(const std::string &name) : name(name) {}
    
    struct PBRT_PARSER_INTERFACE Instance {
      Instance(const std::shared_ptr<Object> &object,
               const affine3f    &xfm)
        : object(object), xfm(xfm)
      {}
      
      std::string toString() const;

      std::shared_ptr<Object> object;
      affine3f    xfm;
    };

    virtual std::string toString() const;

    std::string name;

    //! list of all shapes defined in this object
    std::vector<std::shared_ptr<Shape> > shapes;
    //! list of all volumes defined in this object
    std::vector<std::shared_ptr<Volume> > volumes;
    //! list of all instances defined in this object
    std::vector<std::shared_ptr<Object::Instance> > objectInstances;
    //! list of all light sources defined in this object
    std::vector<std::shared_ptr<LightSource> > lightSources;
  };


  struct PBRT_PARSER_INTERFACE Scene {

    Scene() {
      world = std::make_shared<Object>("<root>");
    };

    //! list of cameras defined in this object
    std::vector<std::shared_ptr<Camera> > cameras;

    //! last lookat specified in the scene, or nullptr if none.
    std::shared_ptr<LookAt> lookAt;

    //! last sampler specified in the scene, or nullptr if none.
    std::shared_ptr<Sampler> sampler;

    //! last integrator specified in the scene, or nullptr if none.
    std::shared_ptr<Integrator> integrator;

    //! last volume integrator specified in the scene, or nullptr if none.
    std::shared_ptr<VolumeIntegrator> volumeIntegrator;

    //! last surface integrator specified in the scene, or nullptr if none.
    std::shared_ptr<SurfaceIntegrator> surfaceIntegrator;

    //! last pixel filter specified in the scene, or nullptr if none.
    std::shared_ptr<PixelFilter> pixelFilter;

    //! the 'world' scene geometry
    std::shared_ptr<Object> world;
  };

} // ::pbrt_parser
