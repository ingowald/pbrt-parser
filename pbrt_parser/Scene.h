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

#pragma once

/*! \file pbrt/Scene.h Defines the root pbrt scene to be
    created/parsed by this parser */

// stl
#include <map>
#include <vector>
#include <memory>

#ifdef _WIN32
#  ifdef pbrt_parser_EXPORTS
#    define PBRT_PARSER_INTERFACE __declspec(dllexport)
#  else
#    define PBRT_PARSER_INTERFACE __declspec(dllimport)
#  endif
#else
#  define PBRT_PARSER_INTERFACE /* ignore on linux */
#endif

namespace pbrt_parser {
  
  struct Object;
  struct Material;
  struct Medium;
  struct Texture;

  /*! the PBRT_PARSER_VECTYPE_NAMESPACE #define allows the
      application/user to override the 'internal' vec3f, vec3i, and
      affine3f classes with whatever the application wants us to
      use. 

    - if PBRT_PARSER_VECTYPE_NAMESPACE is _not defined_, we will
      define our own pbrt_parser::vec3f, pbrt_parser::vec3i, and
      pbrt_parser::affine3f types as plain structs. Vec3i and vec3f
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
#if defined(PBRT_PARSER_VECTYPE_NAMESPACE)
  using vec3f = PBRT_PARSER_VECTYPE_NAMESPACE::vec3f;
  using vec3i = PBRT_PARSER_VECTYPE_NAMESPACE::vec3i;
  using affine3f = PBRT_PARSER_VECTYPE_NAMESPACE::affine3f;
#else
  struct vec3f {
    float x, y, z;
  };
  struct vec3i {
    int x, y, z;
  };
  struct affine3f {
    /*! x-basis vector of affine transform matrix */
    vec3f vx;
    /*! y-basis vector of affine transform matrix */
    vec3f vy;
    /*! z-basis vector of affine transform matrix */
    vec3f vz;
    /*! translational part */
    vec3f p;
  };
#endif

  /*! start-time and end-time transforms - PBRT allows for specifying
      transforms at both 'start' and 'end' time, to allow for linear
      motion */
  struct Transforms {
    affine3f atStart;
    affine3f atEnd;
  };

  /*! forward definition of a typed parameter */
  template<typename T>
  struct ParamArray;

  /*! base abstraction for any type of parameter - typically,
      parameters in pbrt can be either single values or
      arbitrarily-sized arrays of values, so we will eventually
      implement them as std::vectors, with the special case of a
      single-value parameter being a std::vector with one element */
  struct PBRT_PARSER_INTERFACE Param : public std::enable_shared_from_this<Param> {
    /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
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
  struct PBRT_PARSER_INTERFACE ParamArray : public Param, public std::vector<T> {
    /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
    typedef std::shared_ptr<ParamArray<T>> SP;
    
    ParamArray(const std::string &type) : type(type) {};
    
    virtual std::string getType() const { return type; };
    virtual size_t getSize() const { return this->size(); }
    virtual std::string toString() const;
    T get(const size_t idx) const { return (*this)[idx]; }

    /*! used during parsing, to add a newly parsed parameter value
      to the list */
    virtual void add(const std::string &text);
    //    private:
    std::string type;

    // std::vector<T> paramVec;
  };

  struct Texture;

  template<>
  struct PBRT_PARSER_INTERFACE ParamArray<Texture> : public Param {
    ParamArray(const std::string &type) : type(type) {};
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
  struct PBRT_PARSER_INTERFACE ParamSet {

    ParamSet() = default;
    ParamSet(ParamSet &&) = default;
    ParamSet(const ParamSet &) = default;
    
    vec3f getParam3f(const std::string &name, const vec3f &fallBack) const;
    float getParam1f(const std::string &name, const float fallBack=0) const;
    int getParam1i(const std::string &name, const int fallBack=0) const;
    bool getParamBool(const std::string &name, const bool fallBack=false) const;
    std::string getParamString(const std::string &name) const;
    std::shared_ptr<Texture> getParamTexture(const std::string &name) const;

    template<typename T>
      std::shared_ptr<ParamArray<T> > findParam(const std::string &name) const {
      auto it = param.find(name);
      if (it == param.end()) return typename ParamArray<T>::SP();
      return it->second->as<T>(); //std::dynamic_pointer_cast<ParamArray<T> >(it->second);
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

  struct PBRT_PARSER_INTERFACE Material : public ParamSet {
    /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
    typedef std::shared_ptr<Material> SP;

    /*! constructor */
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

  struct PBRT_PARSER_INTERFACE Medium : public ParamSet {
    Medium(const std::string &type) : type(type) {};

    /*! pretty-print this medium (for debugging) */
    std::string toString() const;

    /*! the 'type' of the medium */
    std::string type;
  };

  struct PBRT_PARSER_INTERFACE Texture : public ParamSet {
    std::string name;
    std::string texelType;
    std::string mapType;

    Texture(const std::string &name,
            const std::string &texelType,
            const std::string &mapType) 
      : name(name), texelType(texelType), mapType(mapType)
    {};
  };

  struct PBRT_PARSER_INTERFACE Node : public ParamSet {
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

    /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
    typedef std::shared_ptr<Camera> SP;

    /*! constructor */
    Camera(const std::string &type,
           const Transforms &transforms)
      : Node(type),
      transforms(transforms)
    {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Camera<"+type+">"; }

    const Transforms transforms;
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

    /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
    typedef std::shared_ptr<Shape> SP;
    
    /*! constructor */
    Shape(const std::string &type,
          std::shared_ptr<Material>   material,
          std::shared_ptr<Attributes> attributes,
          const Transforms &transform);

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

  /*! the type of renderer to be used for rendering the given scene */
  struct PBRT_PARSER_INTERFACE Renderer : public Node {
    Renderer(const std::string &type) : Node(type) {};

    /*! pretty-printing, for debugging */
    virtual std::string toString() const override { return "Renderer<"+type+">"; }
  };

  // // a "LookAt" in the pbrt file has three vec3fs, no idea what for
  // // right now - need to rename once we figure that out
  // struct PBRT_PARSER_INTERFACE LookAt {

  //   /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
  //     more consise, and easier to read */
  //   typedef std::shared_ptr<LookAt> SP;
    
  //   LookAt(const vec3f &v0, 
  //          const vec3f &v1, 
  //          const vec3f &v2)
  //     : v0(v0),v1(v1),v2(v2)
  //   {}

  //   vec3f v0, v1, v2;
  // };

  //! what's in a objectbegin/objectned, as well as the root object
  struct PBRT_PARSER_INTERFACE Object {
    
    /*! allows for writing pbrt_parser::Scene::SP, which is somewhat more concise
        than std::shared_ptr<pbrt_parser::Scene> */
    typedef std::shared_ptr<Object> SP;
    
    Object(const std::string &name) : name(name) {}
    
    struct PBRT_PARSER_INTERFACE Instance {
      typedef std::shared_ptr<Instance> SP;
      
      Instance(const std::shared_ptr<Object> &object,
               const Transforms  &xfm)
        : object(object), xfm(xfm)
      {}
      
      std::string toString() const;

      std::shared_ptr<Object> object;
      Transforms              xfm;
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
      use, etc, plus some geometry. */
  struct PBRT_PARSER_INTERFACE Scene {

    /*! allows for writing pbrt_parser::Scene::SP, which is somewhat more concise
        than std::shared_ptr<pbrt_parser::Scene> */
    typedef std::shared_ptr<Scene> SP;
    
    /*! default constructor - creates a new (and at first, empty) scene */
    Scene()
      : world(std::make_shared<Object>("<root>"))
      {}

    /*! parse the given file name, return parsed scene */
    static std::shared_ptr<Scene> parseFromFile(const std::string &fileName);
    
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

    /*! the root scene geometry, defined in the
      'WorldBegin'/'WorldEnd' statements */
    std::shared_ptr<Object> world;

    std::string makeGlobalFileName(const std::string &relativePath)
    { return basePath + relativePath; }
    
    /*! the base path for all filenames defined in this scene. In
        pbrt, lots of objects (a ply mesh geometry, a texture, etc)
        will require additional files in other formats (e.g., the
        actual .ply file that the ply geometry refers to; and the file
        names will typically be relative to the file that referenced
        them. To help computing a global file name for this we compute
        - and store - the name of the directory that contained the
        first file we parsed (ie, the "entry point" into the entire
        scene */
    std::string basePath;
  };

  template<typename T> std::shared_ptr<ParamArray<T>> Param::as()
  { return std::dynamic_pointer_cast<ParamArray<T>>(shared_from_this()); }
  
} // ::pbrt_parser

extern "C" pbrt_parser::Scene::SP pbrtParser_loadScene(const std::string &fileName);

/*! a helper function to load a ply file */
extern "C" void pbrtParser_loadPlyTriangles(const std::string &fileName,
                                            std::vector<pbrt_parser::vec3f> &v,
                                            std::vector<pbrt_parser::vec3f> &n,
                                            std::vector<pbrt_parser::vec3i> &idx);

