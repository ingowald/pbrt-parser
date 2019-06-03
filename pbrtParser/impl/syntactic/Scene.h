// ======================================================================== //
// Copyright 2015-2019 Ingo Wald                                            //
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

/*! \file pbrt/sytax/Scene.h Defines the root pbrt scene - at least
  the *syntactci* part of it - to be created/parsed by this parser */

#include "pbrtParser/math.h"

// stl
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <assert.h>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntax-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {

    /*! @{ forward definitions so we can use those in shared_ptrs in the attribute class */
    struct AreaLightSource;
    struct LightSource;
    struct Object;
    struct Material;
    struct Medium;
    struct Texture;
    /* @} */

    /*! the PBRT_PARSER_VECTYPE_NAMESPACE #define allows the
      application/user to override the 'internal' vec3f, vec3i, and
      affine3f classes with whatever the application wants us to
      use. 

      - if PBRT_PARSER_VECTYPE_NAMESPACE is _not defined_, we will
      define our own pbrt::syntactic::vec3f, pbrt::syntactic::vec3i, and
      pbrt::syntactic::affine3f types as plain structs. Vec3i and vec3f
      are three 32-bit ints and floats, respectively, and affine3f is
      a 4xvec3f matrix as defined below.

      - if PBRT_PARSER_VECTYPE_NAMESPACE _is_ defined, we will instead
      use the types vec3f,vec3i,affine3f in the namespace as supplied
      through this macro. Eg, if PBRT_PARSER_VECTYPE_NAMESPACE is set
      to "myveclib" we will be usign the types myveclib::vec3f,
      myveclib::vec3i, etc. For this to work, the following conditions
      ahve to be fulfilled:

      1) the given naemsapce has to contain all three type names:
      vec3f, vec3i, and affine3f.  (though note that with the C++-11
      "using" statement you can alias these to whatevr you like (e.g.,
      if your vector class is called Vec3f or float3, you can simply
      add a "using vec3f = Vec3f;" resp "using vec3f =
      anyOtherNameSpace::float3", etc; and since these are aliased to
      the same type you can continue to use them in your own
      aplication as you see fit).

      2) these types _have_ to be binary compatible with the fallback
      types we wold be using otherwise (ie, 3 floats for vec3f,
      4xvec3f for an affine3f, etc. The reason behind this is that the
      library itself internally uses this binary layout, so making the
      app use a different data layout will create trouble
    */
    using vec2f    = math::vec2f;
    using vec3f    = math::vec3f;
    using vec4f    = math::vec4f;
    using vec2i    = math::vec2i;
    using vec3i    = math::vec3i;
    using vec4i    = math::vec4i;
    using affine3f = math::affine3f;
    using box3f    = math::box3f;
    using pairNf   = math::pairNf;

    /*! start-time and end-time transform - PBRT allows for specifying
      transform at both 'start' and 'end' time, to allow for linear
      motion */
    struct Transform {
      affine3f atStart;
      affine3f atEnd;
    };

    struct PBRT_PARSER_INTERFACE Attributes {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Attributes> SP;
    
      Attributes();
      Attributes(Attributes &&other) = default;
      Attributes(const Attributes &other) = default;

      virtual std::shared_ptr<Attributes> clone() const
      { return std::make_shared<Attributes>(*this); }

      std::vector<std::shared_ptr<AreaLightSource>>    areaLightSources;
      std::pair<std::string,std::string>               mediumInterface;
      std::map<std::string,std::shared_ptr<Material> > namedMaterial;
      std::map<std::string,std::shared_ptr<Medium> >   namedMedium;
      std::map<std::string,std::shared_ptr<Texture> >  namedTexture;
      bool reverseOrientation { false };
    };

    /*! forward definition of a typed parameter */
    template<typename T>
    struct ParamArray;

#ifdef WIN32
# pragma warning (disable : 4251)
#endif

    /*! base abstraction for any type of parameter - typically,
      parameters in pbrt can be either single values or
      arbitrarily-sized arrays of values, so we will eventually
      implement them as std::vectors, with the special case of a
      single-value parameter being a std::vector with one element */
    struct PBRT_PARSER_INTERFACE Param : public std::enable_shared_from_this<Param> {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Param> SP;
    
      virtual std::string getType() const = 0;
      virtual size_t getSize() const = 0;
      virtual std::string toString() const = 0;

      /*! used during parsing, to add a newly parsed parameter value
        to the list */
      virtual void add(const std::string &text) = 0;

      template<typename T>
        std::shared_ptr<ParamArray<T>> as();
    };

    template<typename T>
    struct PBRT_PARSER_INTERFACE ParamArray : public Param, std::vector<T> {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<ParamArray<T>> SP;
    
      ParamArray(const std::string &type) : type(type) {};
    
      virtual std::string getType() const { return type; };
      virtual size_t getSize() const { return this->size(); }
      virtual std::string toString() const;
      T get(const size_t idx) const { return (*this)[idx]; }

      /*! used during parsing, to add a newly parsed parameter value
        to the list */
      virtual void add(const std::string &text);

      /*! type */
      std::string type;
    };

    struct Texture;

    template<>
    struct PBRT_PARSER_INTERFACE ParamArray<Texture> : public Param {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<ParamArray<Texture>> SP;
    
      ParamArray(const std::string &type) : type(type) {};
      virtual std::string getType() const { return type; };
      virtual size_t getSize() const { return 1; }
      virtual std::string toString() const;
    
      /*! used during parsing, to add a newly parsed parameter value
        to the list */
      virtual void add(const std::string &) { throw std::runtime_error("should never get called.."); }
      //    private:
      std::string type;
      std::shared_ptr<Texture> texture;
    };

    /*! any class that can store (and query) parameters */
    struct PBRT_PARSER_INTERFACE ParamSet {

      ParamSet() = default;
      ParamSet(ParamSet &&) = default;
      ParamSet(const ParamSet &) = default;

      /*! query number of (float,float) pairs N. Store in result if the former != NULL */
      bool getParamPairNf(pairNf::value_type *result, std::size_t* N, const std::string &name) const;
      /*! query parameter of 3f type, and if found, store in result and
        return true; else return false */
      bool getParam3f(float *result, const std::string &name) const;
      bool getParam2f(float *result, const std::string &name) const;
      math::pairNf getParamPairNf(const std::string &name, const math::pairNf &fallBack) const;
      math::vec3f getParam3f(const std::string &name, const math::vec3f &fallBack) const;
      math::vec2f getParam2f(const std::string &name, const math::vec2f &fallBack) const;
#if defined(PBRT_PARSER_VECTYPE_NAMESPACE)
      vec2f getParam2f(const std::string &name, const vec2f &fallBack) const {
        math::vec2f res = getParam2f(name, (const math::vec2f&)fallBack);
        return *(vec2f*)&res;
      }
      vec3f getParam3f(const std::string &name, const vec3f &fallBack) const {
        math::vec3f res = getParam3f(name, (const math::vec3f&)fallBack);
        return *(vec3f*)&res;
      }
#endif
      float getParam1f(const std::string &name, const float fallBack=0) const;
      int   getParam1i(const std::string &name, const int fallBack=0) const;
      bool getParamBool(const std::string &name, const bool fallBack=false) const;
      std::string getParamString(const std::string &name) const;
      std::shared_ptr<Texture> getParamTexture(const std::string &name) const;
      bool hasParamTexture(const std::string &name) const {
        return (bool)findParam<Texture>(name);
      }
      bool hasParamString(const std::string &name) const {
        return (bool)findParam<std::string>(name);
      }
      bool hasParam1i(const std::string &name) const {
        return  (bool)findParam<int>(name) && findParam<int>(name)->size() == 1;
      }
      bool hasParam1f(const std::string &name) const {
        return (bool)findParam<float>(name) && findParam<float>(name)->size() == 1;
      }
      bool hasParam2f(const std::string &name) const {
        return (bool)findParam<float>(name) && findParam<float>(name)->size() == 2;
      }
      bool hasParam3f(const std::string &name) const {
        return (bool)findParam<float>(name) && findParam<float>(name)->size() == 3;
      }
    
      template<typename T>
      std::shared_ptr<ParamArray<T> > findParam(const std::string &name) const {
        auto it = param.find(name);
        if (it == param.end()) return typename ParamArray<T>::SP();
        return it->second->as<T>(); //std::dynamic_pointer_cast<ParamArray<T> >(it->second);
      }

      std::map<std::string,std::shared_ptr<Param> > param;
    };

    struct PBRT_PARSER_INTERFACE Material : public ParamSet {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Material> SP;

      /*! constructor */
      Material(const std::string &type) : type(type) {};

      /*! pretty-print this material (for debugging) */
      std::string toString() const;

      Attributes::SP attributes;
    
      /*! the 'type' of the material, such as 'uber'material, 'matte',
        'mix' etc. Note that the PBRT format has two inconsistent
        ways of specifying that type: for the 'Material' command it
        specifies the type explicitly right after the 'matierla'
        command; for the 'makenamedmaterial' it uses an implicit
        'type' parameter */
      std::string type;

    /*! the logical name that this was defined under, such as
        "BackWall". Note this may be an empty string for some scenes
        (it is only defined for 'NamedMaterial's) */
      std::string name;
    };

    struct PBRT_PARSER_INTERFACE Medium : public ParamSet {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Medium> SP;
    
      Medium(const std::string &type) : type(type) {};

      /*! pretty-print this medium (for debugging) */
      std::string toString() const;

      /*! the 'type' of the medium */
      std::string type;
    };

    struct PBRT_PARSER_INTERFACE Texture : public ParamSet {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Texture> SP;
    
      Texture(const std::string &name,
              const std::string &texelType,
              const std::string &mapType) 
        : name(name), texelType(texelType), mapType(mapType)
      {};

      /*! pretty-print this medium (for debugging) */
      virtual std::string toString() const { return "Texture(name="+name+",texelType="+texelType+",mapType="+mapType+")"; }
    
      std::string name;
      std::string texelType;
      std::string mapType;
      /*! the attributes active when this was created - some texture
          use implicitly named textures, so have to keep this
          around */
      Attributes::SP attributes;
    };

    /*! base abstraction for any PBRT scene graph node - all shapes,
      volumes, etc are eventually derived from this comon base
      class */
    struct PBRT_PARSER_INTERFACE Node : public ParamSet {
      Node(const std::string &type)
        : type(type)
        {}

      /*! pretty-printing, for debugging */
      virtual std::string toString() const = 0;

      /*! the 'type' as specified in the PBRT field, such as
        'trianglemesh' for a tri mesh shape, etc */
      std::string type;
    };

    /*! a PBRT "Camera" object - does not actually specify any
      particular members by itself, all semantics is included in the
      'type' field and its (type-specfic) parameters */
    struct PBRT_PARSER_INTERFACE Camera : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Camera> SP;

      /*! constructor */
      Camera(const std::string &type,
             const Transform &transform)
        : Node(type),
        transform(transform)
        {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Camera<"+type+">"; }

      const Transform transform;
    };

    struct PBRT_PARSER_INTERFACE Sampler : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Sampler> SP;
    
      /*! constructor */
      Sampler(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Sampler<"+type+">"; }
    };

    struct PBRT_PARSER_INTERFACE Integrator : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Integrator> SP;
    
      /*! constructor */
      Integrator(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Integrator<"+type+">"; }
    };

    struct PBRT_PARSER_INTERFACE SurfaceIntegrator : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<SurfaceIntegrator> SP;
    
      /*! constructor */
      SurfaceIntegrator(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "SurfaceIntegrator<"+type+">"; }
    };

    struct PBRT_PARSER_INTERFACE VolumeIntegrator : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<VolumeIntegrator> SP;
    
      /*! constructor */
      VolumeIntegrator(const std::string &type) : Node(type) {};
    
      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "VolumeIntegrator<"+type+">"; }
    };
  
    struct PBRT_PARSER_INTERFACE PixelFilter : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<PixelFilter> SP;
    
      /*! constructor */
      PixelFilter(const std::string &type) : Node(type) {};
    
      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "PixelFilter<"+type+">"; }
    };

    /*! a PBRT 'geometric shape' (a shape in ospray terms) - ie,
      something that has primitives that together form some sort of
      surface(s) that a ray can intersect*/
    struct PBRT_PARSER_INTERFACE Shape : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Shape> SP;
    
      /*! constructor */
      Shape(const std::string &type,
            std::shared_ptr<Material>   material,
            std::shared_ptr<Attributes> attributes,
            const Transform &transform);

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Shape<"+type+">"; }

      /*! material active when the shape was generated - PBRT has only
        one material per shape */
      std::shared_ptr<Material>   material;
      std::shared_ptr<Attributes> attributes;
      Transform                   transform;
    };

    struct PBRT_PARSER_INTERFACE Volume : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Volume> SP;
    
      Volume(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Volume<"+type+">"; }
    };

    struct PBRT_PARSER_INTERFACE LightSource : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<LightSource> SP;
    
      LightSource(const std::string &type, 
                  const Transform &transform)
        : Node(type),
        transform(transform)
      {}

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "LightSource<"+type+">"; }

      const Transform transform;
    };

    /*! area light sources are different from regular light sources in
        that they get attached to getometry (shapes), whereas regular
        light sources are not. Thus, from a type perspective a
        AreaLightSource is _not_ "related" to a LightSource, which is
        why we don't have one derived from the other. */
    struct PBRT_PARSER_INTERFACE AreaLightSource : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<AreaLightSource> SP;
    
      AreaLightSource(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "AreaLightSource<"+type+">"; }
    };

    struct PBRT_PARSER_INTERFACE Film : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Film> SP;
    
      Film(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Film<"+type+">"; }
    };

    struct PBRT_PARSER_INTERFACE Accelerator : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Accelerator> SP;
    
      Accelerator(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Accelerator<"+type+">"; }
    };

    /*! the type of renderer to be used for rendering the given scene */
    struct PBRT_PARSER_INTERFACE Renderer : public Node {

      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Renderer> SP;
    
      Renderer(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "Renderer<"+type+">"; }
    };

    //! what's in a objectbegin/objectned, as well as the root object
    struct PBRT_PARSER_INTERFACE Object {
    
      /*! allows for writing pbrt::syntactic::Scene::SP, which is somewhat
        more concise than std::shared_ptr<pbrt::syntactic::Scene> */
      typedef std::shared_ptr<Object> SP;
    
      Object(const std::string &name) : name(name) {}
    
      struct PBRT_PARSER_INTERFACE Instance {

        /*! allows for writing pbrt::syntactic::Scene::SP, which is somewhat
          more concise than std::shared_ptr<pbrt::syntactic::Scene> */
        typedef std::shared_ptr<Instance> SP;
      
        Instance(const std::shared_ptr<Object> &object,
                 const Transform  &xfm)
          : object(object), xfm(xfm)
        {}
      
        std::string toString() const;

        std::shared_ptr<Object> object;
        Transform              xfm;
      };

      //! pretty-print scene info into a std::string 
      virtual std::string toString(const int depth = 0) const;

      /*! logical name of this object */
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

    /*! The main object defined by each pbrt (root) file is a scene - a
      scene contains all kind of global settings (such as integrator
      to use, cameras defined in this scene, which pixel filter to
      use, etc, plus some shape. */
    struct PBRT_PARSER_INTERFACE Scene {

      /*! allows for writing pbrt::syntactic::Scene::SP, which is somewhat more concise
        than std::shared_ptr<pbrt::syntactic::Scene> */
      typedef std::shared_ptr<Scene> SP;
    
      /*! default constructor - creates a new (and at first, empty) scene */
      Scene()
        : world(std::make_shared<Object>("<root>"))
        {}

      /*! parse the given file name, return parsed scene */
      static std::shared_ptr<Scene> parse(const std::string &fileName);
    
      //! pretty-print scene info into a std::string 
      std::string toString(const int depth = 0) { return world->toString(depth); }

      //! list of cameras defined in this object
      std::vector<Camera::SP> cameras;

      std::shared_ptr<Film> film;
    
      // //! last lookat specified in the scene, or nullptr if none.
      // std::shared_ptr<LookAt> lookAt;

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

      /*! the root scene shape, defined in the
        'WorldBegin'/'WorldEnd' statements */
      std::shared_ptr<Object> world;
    
      std::string makeGlobalFileName(const std::string &relativePath)
      { return basePath + relativePath; }
    

    /*! the base path for all filenames defined in this scene. In
        pbrt, lots of objects (a ply mesh shape, a texture, etc)
        will require additional files in other formats (e.g., the
        actual .ply file that the ply shape refers to; and the file
        names will typically be relative to the file that referenced
        them. To help computing a global file name for this we compute
        - and store - the name of the directory that contained the
        first file we parsed (ie, the "entry point" into the entire
        scene */
      std::string basePath;
    };

    template<typename T> std::shared_ptr<ParamArray<T>> Param::as()
    { return std::dynamic_pointer_cast<ParamArray<T>>(shared_from_this()); }

    extern "C" 
    void pbrt_helper_loadPlyTriangles(const std::string &fileName,
                                      std::vector<pbrt::syntactic::vec3f> &v,
                                      std::vector<pbrt::syntactic::vec3f> &n,
                                      std::vector<pbrt::syntactic::vec3i> &idx);

  } // ::pbrt::syntactic
} // ::pbrt
