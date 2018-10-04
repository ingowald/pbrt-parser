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

  struct Object;
  struct Material;
  struct Medium;
  struct Texture;

  /*! start-time and end-time transforms - PBRT allows for specifying
      transforms at both 'start' and 'end' time, to allow for linear
      motion */
  struct Transforms {
    affine3f atStart;
    affine3f atEnd;
  };
  
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

  struct Texture;

  template<>
  struct PBRT_PARSER_INTERFACE ParamT<Texture> : public Param {
    ParamT(const std::string &type) : type(type) {};
    virtual std::string getType() const { return type; };
    virtual size_t getSize() const { return 1; }
    virtual std::string toString() const;
    
    /*! used during parsing, to add a newly parsed parameter value
      to the list */
    virtual void add(const std::string &text) { throw std::runtime_error("should never get called.."); }
    //    private:
    std::string type;
    std::shared_ptr<Texture> texture;
  };

  /*! any class that can store (and query) parameters */
  struct PBRT_PARSER_INTERFACE Parameterized {

    Parameterized() = default;
    Parameterized(Parameterized &&) = default;
    Parameterized(const Parameterized &) = default;
    
    vec3f getParam3f(const std::string &name, const vec3f &fallBack=vec3f(0)) const;
    float getParam1f(const std::string &name, const float fallBack=0) const;
    int getParam1i(const std::string &name, const int fallBack=0) const;
    bool getParamBool(const std::string &name, const bool fallBack=false) const;
    std::string getParamString(const std::string &name) const;
    std::shared_ptr<Texture> getParamTexture(const std::string &name) const;

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
    Attributes(Attributes &&other) = default;
    Attributes(const Attributes &other) = default;

    virtual std::shared_ptr<Attributes> clone() const { return std::make_shared<Attributes>(*this); }

    std::pair<std::string,std::string>               mediumInterface;
    std::map<std::string,std::shared_ptr<Material> > namedMaterial;
    std::map<std::string,std::shared_ptr<Medium> >   namedMedium;
    std::map<std::string,std::shared_ptr<Texture> >  namedTexture;
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

  struct PBRT_PARSER_INTERFACE Medium : public Parameterized {
    Medium(const std::string &type) : type(type) {};

    /*! pretty-print this medium (for debugging) */
    std::string toString() const;

    /*! the 'type' of the medium, such as 'uber'medium, 'matte',
      'mix' etc. Note that the PBRT format has two inconsistent
      ways of specifying that type: for the 'Medium' command it
      specifies the type explicitly right after the 'matierla'
      command; for the 'makenamedmedium' it uses an implicit
      'type' parameter */
    std::string type;
  };

  struct PBRT_PARSER_INTERFACE Texture : public Parameterized {
    std::string name;
    std::string texelType;
    std::string mapType;

    Texture(const std::string &name,
            const std::string &texelType,
            const std::string &mapType) 
      : name(name), texelType(texelType), mapType(mapType)
    {};
    // std::map<std::string,std::shared_ptr<Param> > param;
  };

  struct PBRT_PARSER_INTERFACE Node : public Parameterized {
    Node(const std::string &type)
      : type(type)
      {}

    /*! pretty-printing, for debugging */
    virtual std::string toString() const = 0;

    const std::string type;
  };

  /*! a PBRT "Camera" object - does not actually specify any
      particular members by itself, all semantics is included in the
      'type' field and its (type-specfic) parameters */
  struct PBRT_PARSER_INTERFACE Camera : public Node {
    /*! constructor */
    Camera(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Camera<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE Sampler : public Node {
    /*! constructor */
    Sampler(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Sampler<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE Integrator : public Node {
    /*! constructor */
    Integrator(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Integrator<"+type+">"; }
  };
  struct PBRT_PARSER_INTERFACE SurfaceIntegrator : public Node {
    /*! constructor */
    SurfaceIntegrator(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "SurfaceIntegrator<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE VolumeIntegrator : public Node {
    /*! constructor */
    VolumeIntegrator(const std::string &type) : Node(type) {};
    
    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "VolumeIntegrator<"+type+">"; }
  };
  
  struct PBRT_PARSER_INTERFACE PixelFilter : public Node {
    /*! constructor */
    PixelFilter(const std::string &type) : Node(type) {};
    
    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "PixelFilter<"+type+">"; }
  };

  /*! a PBRT 'geometric shape' (a geometry in ospray terms) - ie,
    something that has primitives that together form some sort of
    surface(s) that a ray can intersect*/
  struct PBRT_PARSER_INTERFACE Shape : public Node {
    /*! constructor */
    Shape(const std::string &type,
          std::shared_ptr<Material>   material,
          std::shared_ptr<Attributes> attributes,
          const Transforms &transform);
    // Shape(const Shape &shape) = default;
    // Shape(Shape &&shape) = default;

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Shape<"+type+">"; }

    /*! material active when the shape was generated - PBRT has only
      one material per shape */
    std::shared_ptr<Material>   material;
    std::shared_ptr<Attributes> attributes;
    Transforms                  transform;
  };

  struct PBRT_PARSER_INTERFACE Volume : public Node {
    Volume(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Volume<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE LightSource : public Node {
    LightSource(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "LightSource<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE AreaLightSource : public Node {
    AreaLightSource(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "AreaLightSource<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE Film : public Node {
    Film(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Film<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE Accelerator : public Node {
    Accelerator(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Accelerator<"+type+">"; }
  };

  struct PBRT_PARSER_INTERFACE Renderer : public Node {
    Renderer(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Renderer<"+type+">"; }
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
               const Transforms  &xfm)
        : object(object), xfm(xfm)
      {}
      
      std::string toString() const;

      std::shared_ptr<Object> object;
      Transforms    xfm;
    };

    //! pretty-print scene info into a std::string 
    virtual std::string toString(const int depth = 0) const;

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

    //! pretty-print scene info into a std::string 
    std::string toString(const int depth = 0) { return world->toString(depth); }

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
