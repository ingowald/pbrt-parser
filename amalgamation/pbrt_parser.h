// ======================================================================== //
// Copyright 2015-2019 Ingo Wald & Fabio Pellacini                          //
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

//
// This library code from code from the happly library.
//
// A header-only implementation of the .ply file format.
// https://github.com/nmwsharp/happly
// By Nicholas Sharp - nsharp@cs.cmu.edu
//
// MIT License
//
// Copyright (c) 2018 Nick Sharp
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#pragma once
#pragma once

#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#if defined(_WIN32) && defined(PBRT_PARSER_DLL_INTERFACE)
#  ifdef pbrtParser_semantic_EXPORTS
#    define PBRT_PARSER_INTERFACE __declspec(dllexport)
#  else
#    define PBRT_PARSER_INTERFACE __declspec(dllimport)
#  endif
#else
#  define PBRT_PARSER_INTERFACE /* ignore on linux/osx */
#endif

#include <iostream>
#include <math.h> // using cmath causes issues under Windows
#include <cfloat>
#include <limits>

/*! \file pbrt/Parser.h *Internal* parser class used by \see
  pbrt_parser::Scene::parseFromFile() - as end user, you should
  never have to look into this file directly, and only use \see
  pbrt_parser::Scene::parseFromFile() */

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntax-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
    /*! namespace for all math routines */
    namespace math {
        struct vec2f {
          float x, y;
          vec2f() = default;
          explicit vec2f(float v) : x{v}, y{v} { }
          vec2f(float x, float y) : x{x}, y{y} { }
          typedef float scalar_t;
        };
        struct vec3f {
          float x, y, z;
          vec3f() = default;
          explicit vec3f(float v) : x{v}, y{v}, z{v} { }
          vec3f(float x, float y, float z) : x{x}, y{y}, z{z} { }
          typedef float scalar_t;
        };
        struct vec4f {
          float x, y, z, w;
          vec4f() = default;
          explicit vec4f(float v) : x{v}, y{v}, z{v}, w{v} { }
          vec4f(float x, float y, float z, float w) : x{x}, y{y}, z{z}, w{w} { }
          typedef float scalar_t;
        };
        struct vec2i {
          int x, y;
          vec2i() = default;
          explicit vec2i(int v) : x{v}, y{v} { }
          vec2i(int x, int y) : x{x}, y{y} { }
          typedef int scalar_t;
        };
        struct vec3i {
          int x, y, z;
          vec3i() = default;
          explicit vec3i(int v) : x{v}, y{v}, z{v} { }
          vec3i(int x, int y, int z) : x{x}, y{y}, z{z} { }
          typedef int scalar_t;
        };
        struct vec4i {
          int x, y, z, w;
          vec4i() = default;
          explicit vec4i(int v) : x{v}, y{v}, z{v}, w{v} { }
          vec4i(int x, int y, int z, int w) : x{x}, y{y}, z{z}, w{w} { }
          typedef int scalar_t;
        };
        struct mat3f {
          vec3f vx, vy, vz;
          mat3f() = default;
          explicit mat3f(const vec3f& v) : vx{v.x,0,0}, vy{0,v.y,0}, vz{0,0,v.z} { }
          mat3f(const vec3f& x, const vec3f& y, const vec3f& z) : vx{x}, vy{y}, vz{z} { }
        };
        struct affine3f {
          mat3f l;
          vec3f p;
          affine3f() = default;
          affine3f(const mat3f& l, const vec3f& p) :  l{l}, p{p} { }
          static affine3f identity() { return affine3f(mat3f(vec3f(1)), vec3f(0)); }
          static affine3f scale(const vec3f& u) { return affine3f(mat3f(u),vec3f(0)); }
          static affine3f translate(const vec3f& u) { return affine3f(mat3f(vec3f(1)), u); }
          static affine3f rotate(const vec3f& _u, float r);
        };
        struct box3f {
          vec3f lower, upper;
          box3f() = default;
          box3f(const vec3f& lower, const vec3f& upper) : lower{lower}, upper{upper} { }
          inline bool  empty()  const { return upper.x < lower.x || upper.y < lower.y || upper.z < lower.z; }
          static box3f empty_box() { return box3f(vec3f(FLT_MAX), vec3f(FLT_MIN)); }
          void extend(const vec3f& a);
          void extend(const box3f& a);
        };

        inline vec3f operator-(const vec3f& a) { return vec3f(-a.x, -a.y, -a.z); }
        inline vec3f operator-(const vec3f& a, const vec3f& b) { return vec3f(a.x - b.x, a.y - b.y, a.z - b.z); }
        inline vec3f operator+(const vec3f& a, const vec3f& b) { return vec3f(a.x + b.x, a.y + b.y, a.z + b.z); }
        inline vec3f operator*(const vec3f& a, float b) { return vec3f(a.x * b, a.y * b, a.z * b); }
        inline vec3f operator*(float a, const vec3f& b) { return vec3f(a * b.x, a * b.y, a * b.z); }
        inline vec3f operator*(const vec3f& a, const vec3f& b) { return vec3f(a.x * b.x, a.y * b.y, a.z * b.z); }
        inline mat3f operator*(const mat3f& a, float b) { return mat3f(a.vx * b, a.vy * b, a.vz * b); }
        inline vec3f operator*(const mat3f& a, const vec3f& b) { return a.vx * b.x + a.vy * b.y + a.vz * b.z; }
        inline mat3f operator*(const mat3f& a, const mat3f& b) { return mat3f(a * b.vx, a * b.vy, a * b.vz); }
        inline vec3f operator*(const affine3f& a, const vec3f& b) { return a.l * b + a.p; }
        inline affine3f operator*(const affine3f& a, const affine3f& b) { return affine3f(a.l * b.l, a.l * b.p + a.p); }

        inline float dot(const vec3f& a, const vec3f& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
        inline vec3f cross(const vec3f& a, const vec3f& b) { return vec3f(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x); }
        inline vec3f normalize(const vec3f& a) { return a * (1 / sqrtf(dot(a,a))); }

        inline mat3f transpose(const mat3f& a) { return mat3f(vec3f(a.vx.x, a.vy.x, a.vz.x), vec3f(a.vx.y, a.vy.y, a.vz.y), vec3f(a.vx.z, a.vy.z, a.vz.z)); }
        inline float determinant(const mat3f& a) { return dot(a.vx,cross(a.vy,a.vz)); }
        inline mat3f adjoint_transpose(const mat3f& a) { return mat3f(cross(a.vy,a.vz),cross(a.vz,a.vx),cross(a.vx,a.vy)); }
        inline mat3f inverse_transpose(const mat3f& a) { return adjoint_transpose(a) * (1 / determinant(a)); }
        inline mat3f inverse(const mat3f& a) {return transpose(inverse_transpose(a)); }
        inline affine3f inverse(const affine3f& a) { mat3f il = inverse(a.l); return affine3f(il,-(il*a.p)); }

        inline vec3f xfmPoint(const affine3f& m, const vec3f& p) { return m * p; }
        inline vec3f xfmVector(const affine3f& m, const vec3f& v) { return m.l * v; }
        inline vec3f xfmNormal(const affine3f& m, const vec3f& n) { return inverse_transpose(m.l) * n; }

        inline affine3f affine3f::rotate(const vec3f& _u, float r) {
          vec3f u = normalize(_u);
          float s = sinf(r), c = cosf(r);
          mat3f l = mat3f(vec3f(u.x*u.x+(1-u.x*u.x)*c, u.x*u.y*(1-c)+u.z*s, u.x*u.z*(1-c)-u.y*s),
                          vec3f(u.x*u.y*(1-c)-u.z*s, u.y*u.y+(1-u.y*u.y)*c, u.y*u.z*(1-c)+u.x*s),   
                          vec3f(u.x*u.z*(1-c)+u.y*s, u.y*u.z*(1-c)-u.x*s, u.z*u.z+(1-u.z*u.z)*c));
          return affine3f(l, vec3f(0));
        }

        inline float max(float a, float b) { return a<b ? b:a; }
        inline float min(float a, float b) { return a>b ? b:a; }
        inline vec3f max(const vec3f& a, const vec3f& b) { return vec3f(max(a.x,b.x),max(a.y,b.y),max(a.z,b.z)); }
        inline vec3f min(const vec3f& a, const vec3f& b) { return vec3f(min(a.x,b.x),min(a.y,b.y),min(a.z,b.z)); }
        inline void box3f::extend(const vec3f& p) { lower=min(lower,p); upper=max(upper,p); }
        inline void box3f::extend(const box3f& b) { lower=min(lower,b.lower); upper=max(upper,b.upper); }

        inline std::ostream& operator<<(std::ostream& o, const vec2f& v) { return o << "(" << v.x << "," << v.y << ")"; }
        inline std::ostream& operator<<(std::ostream& o, const vec3f& v) { return o << "(" << v.x << "," << v.y << "," << v.z << ")"; }
        inline std::ostream& operator<<(std::ostream& o, const vec4f& v) { return o << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")"; }
        inline std::ostream& operator<<(std::ostream& o, const vec2i& v) { return o << "(" << v.x << "," << v.y << ")"; }
        inline std::ostream& operator<<(std::ostream& o, const vec3i& v) { return o << "(" << v.x << "," << v.y << "," << v.z << ")"; }
        inline std::ostream& operator<<(std::ostream& o, const vec4i& v) { return o << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")"; }
        inline std::ostream& operator<<(std::ostream& o, const mat3f& m) { return o << "{ vx = " << m.vx << ", vy = " << m.vy << ", vz = " << m.vz << "}"; }
        inline std::ostream& operator<<(std::ostream& o, const affine3f& m) { return o << "{ l = " << m.l << ", p = " << m.p << " }"; }
        inline std::ostream& operator<<(std::ostream &o, const box3f& b) { return o << "[" << b.lower <<":"<<b.upper<<"]"; }
    }
  } // ::pbrt::syntactic
} // ::pbrt




/*! \file pbrt/sytax/Scene.h Defines the root pbrt scene - at least
  the *syntactci* part of it - to be created/parsed by this parser */


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
  
    struct Object;
    struct Material;
    struct Medium;
    struct Texture;

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
#if defined(PBRT_PARSER_VECTYPE_NAMESPACE)
    using vec2f    = PBRT_PARSER_VECTYPE_NAMESPACE::vec2f;
    using vec3f    = PBRT_PARSER_VECTYPE_NAMESPACE::vec3f;
    using vec4f    = PBRT_PARSER_VECTYPE_NAMESPACE::vec4f;
    using vec2i    = PBRT_PARSER_VECTYPE_NAMESPACE::vec2i;
    using vec3i    = PBRT_PARSER_VECTYPE_NAMESPACE::vec3i;
    using vec4i    = PBRT_PARSER_VECTYPE_NAMESPACE::vec4i;
    using affine3f = PBRT_PARSER_VECTYPE_NAMESPACE::affine3f;
    using box3f    = PBRT_PARSER_VECTYPE_NAMESPACE::box3f;
#else
    using vec2f    = math::vec2f;
    using vec3f    = math::vec3f;
    using vec4f    = math::vec4f;
    using vec2i    = math::vec2i;
    using vec3i    = math::vec3i;
    using vec4i    = math::vec4i;
    using affine3f = math::affine3f;
    using box3f    = math::box3f;
#endif

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

      virtual std::shared_ptr<Attributes> clone() const { return std::make_shared<Attributes>(*this); }

      std::pair<std::string,std::string>               mediumInterface;
      std::map<std::string,std::shared_ptr<Material> > namedMaterial;
      std::map<std::string,std::shared_ptr<Medium> >   namedMedium;
      std::map<std::string,std::shared_ptr<Texture> >  namedTexture;
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

      /*! query parameter of 3f type, and if found, store in result and
        return true; else return false */
      bool getParam3f(float *result, const std::string &name) const;
      math::vec3f getParam3f(const std::string &name, const math::vec3f &fallBack) const;
#if defined(PBRT_PARSER_VECTYPE_NAMESPACE)
      vec3f getParam3f(const std::string &name, const vec3f &fallBack) const {
        math::vec3f res = getParam3f(name, (const math::vec3f&)fallBack);
        return *(vec3f*)&res;
      }
#endif
      float getParam1f(const std::string &name, const float fallBack=0) const;
      int getParam1i(const std::string &name, const int fallBack=0) const;
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
    
      LightSource(const std::string &type) : Node(type) {};

      /*! pretty-printing, for debugging */
      virtual std::string toString() const override { return "LightSource<"+type+">"; }
    };

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
  
  } // ::pbrt::syntx
} // ::pbrt

extern "C" PBRT_PARSER_INTERFACE
void pbrt_syntactic_writeBinary(pbrt::syntactic::Scene::SP scene,
                             const std::string &fileName);

/*! given an already created scene, read given binary file, and populate this scene */
extern "C" PBRT_PARSER_INTERFACE
void pbrt_syntactic_readBinary(pbrt::syntactic::Scene::SP &scene,
                            const std::string &fileName);

/*! load pbrt scene from a pbrt file */
extern "C" PBRT_PARSER_INTERFACE
void pbrt_syntactic_parse(pbrt::syntactic::Scene::SP &scene,
                       const std::string &fileName);

/*! a helper function to load a ply file */
extern "C" PBRT_PARSER_INTERFACE
void pbrt_helper_loadPlyTriangles(const std::string &fileName,
                                  std::vector<pbrt::syntactic::vec3f> &v,
                                  std::vector<pbrt::syntactic::vec3f> &n,
                                  std::vector<pbrt::syntactic::vec3i> &idx);

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    /*! C++ function that uses the C entry point. We have to use this
        trick to make visual studio happy: in visual studio, the
        extern C declared C entry point cannot return a C++
        std::shared_ptr; so that C entry point has to have it as a
        reference parameter - which requires an additional (inline'ed,
        and thus not linked) wrapper that reutrns this as a
        parameter */
    inline Scene::SP readBinary(const std::string &fn)
    {
      Scene::SP scene;
      pbrt_syntactic_readBinary(scene, fn);
      return scene; 
    }

    /*! C++ function that uses the C entry point. We have to use this
        trick to make visual studio happy: in visual studio, the
        extern C declared C entry point cannot return a C++
        std::shared_ptr; so that C entry point has to have it as a
        reference parameter - which requires an additional (inline'ed,
        and thus not linked) wrapper that reutrns this as a
        parameter */
    inline Scene::SP parse(const std::string &fn)
    {
      Scene::SP scene;
      pbrt_syntactic_parse(scene, fn);
      return scene;
    }
  
  } // ::pbrt::syntactic
} // ::pbrt




/*! \file pbrt/semantic/Scene.h Defines the final semantically parsed
    scene */

// std
#include <mutex>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for final, *semantic* parser - unlike the syntactic
    parser, the semantic one will distinguish between different
    matieral types, differnet shape types, etc, and it will not only
    store 'name:value' pairs for parameters, but figure out which
    parameters which shape etc have, parse them from the
    parameters, etc */
  namespace semantic {

    using vec2f    = syntactic::vec2f;
    using vec3f    = syntactic::vec3f;
    using vec4f    = syntactic::vec4f;
    using vec2i    = syntactic::vec2i;
    using vec3i    = syntactic::vec3i;
    using vec4i    = syntactic::vec4i;
    using affine3f = syntactic::affine3f;
    using box3f    = syntactic::box3f;
    
    /*! internal class used for serializing a scene graph to/from disk */
    struct BinaryWriter;
    
    /*! internal class used for serializing a scene graph to/from disk */
    struct BinaryReader;

    struct Object;

    /*! base abstraction for any entity in the pbrt scene graph that's
        not a paramter type (eg, it's a shape/shape, a object, a
        instance, matierla, tetxture, etc */
    struct Entity : public std::enable_shared_from_this<Entity> {
      typedef std::shared_ptr<Entity> SP;

      template<typename T>
      std::shared_ptr<T> as() { return std::dynamic_pointer_cast<T>(shared_from_this()); }
      
      /*! pretty-printer, for debugging */
      virtual std::string toString() const = 0; // { return "Entity"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) = 0;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) {}
    };

    /*! base abstraction for any material type. Note this is
        intentionally *not* abstract, since we will use it for any
        syntactic material type that we couldn't parse/recognize. This
        allows the app to eventually find non-recognized mateirals and
        replace them with whatever that app wants to use as a default
        material. Note at least one pbrt-v3 model uses a un-named
        material type, which will return this (empty) base class */
    struct Material : public Entity {
      typedef std::shared_ptr<Material> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Material"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct Texture : public Entity {
      typedef std::shared_ptr<Texture> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "(Abstract)Texture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };

    /*! a texture defined by a 2D image. Note we hadnle ptex separately,
      as ptex is not a 2D image format */
    struct ImageTexture : public Texture {
      typedef std::shared_ptr<ImageTexture> SP;

      ImageTexture(const std::string &fileName="") : fileName(fileName) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "ImageTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      std::string fileName;
    };
  
    /*! a texture defined by a disney ptex file. these are kind-of like
      image textures, but also kind-of not, so we handle them
      separately */
    struct PtexFileTexture : public Texture {
      typedef std::shared_ptr<PtexFileTexture> SP;

      PtexFileTexture(const std::string &fileName="") : fileName(fileName) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "PtexFileTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      std::string fileName;
    };
  
    struct FbmTexture : public Texture {
      typedef std::shared_ptr<FbmTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "FbmTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct WindyTexture : public Texture {
      typedef std::shared_ptr<WindyTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "WindyTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct MarbleTexture : public Texture {
      typedef std::shared_ptr<MarbleTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MarbleTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float scale { 1.f };
    };
  
    struct WrinkledTexture : public Texture {
      typedef std::shared_ptr<WrinkledTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "WrinkledTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct ScaleTexture : public Texture {
      typedef std::shared_ptr<ScaleTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "ScaleTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      Texture::SP tex1, tex2;
      vec3f scale1 { 1.f };
      vec3f scale2 { 1.f };
    };
  
    struct MixTexture : public Texture {
      typedef std::shared_ptr<MixTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MixTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f amount { 1.f };
      Texture::SP map_amount;
      Texture::SP tex1, tex2;
      vec3f scale1 { 1.f };
      vec3f scale2 { 1.f };
    };
  
    struct ConstantTexture : public Texture {
      typedef std::shared_ptr<ConstantTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "ConstantTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f value;
    };
  
    /*! disney 'principled' material, as used in moana model */
    struct DisneyMaterial : public Material {
      typedef std::shared_ptr<DisneyMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "DisneyMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float anisotropic    { 0.f };
      float clearCoat      { 0.f };
      float clearCoatGloss { 1.f };
      vec3f color          { 1.f, 1.f, 1.f };
      float diffTrans      { 1.35f };
      float eta            { 1.2f };
      float flatness       { 0.2f };
      float metallic       { 0.f };
      float roughness      { .9f };
      float sheen          { .3f };
      float sheenTint      { 0.68f };
      float specTrans      { 0.f };
      float specularTint   { 0.f };
      bool  thin           { true };
    };

    struct MixMaterial : public Material
    {
      typedef std::shared_ptr<MixMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MixMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      Material::SP material0, material1;
      vec3f amount;
      Texture::SP map_amount;
    };
    
    struct MetalMaterial : public Material
    {
      typedef std::shared_ptr<MetalMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MetalMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float roughness  { 0.f };
      Texture::SP map_roughness;
      float uRoughness { 0.f };
      Texture::SP map_uRoughness;
      float vRoughness { 0.f };
      Texture::SP map_vRoughness;
      vec3f       eta  { 1.f, 1.f, 1.f };
      std::string spectrum_eta;
      vec3f       k    { 1.f, 1.f, 1.f };
      std::string spectrum_k;
      Texture::SP map_bump;
    };
    
    struct TranslucentMaterial : public Material
    {
      typedef std::shared_ptr<TranslucentMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "TranslucentMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f transmit { 1.f, 1.f, 1.f };
      vec3f reflect  { 0.f, 0.f, 0.f };
      vec3f kd;
      Texture::SP map_kd;
    };
    
    struct PlasticMaterial : public Material
    {
      typedef std::shared_ptr<PlasticMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "PlasticMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kd { .65f };
      Texture::SP map_kd;
      vec3f ks { .0f };
      Texture::SP map_ks;
      Texture::SP map_bump;
      float roughness { 0.00030000001f };
      Texture::SP map_roughness;
      bool remapRoughness { false };
   };
    
    struct SubstrateMaterial : public Material
    {
      typedef std::shared_ptr<SubstrateMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "SubstrateMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float uRoughness { .001f };
      Texture::SP map_uRoughness;
      float vRoughness { .001f };
      Texture::SP map_vRoughness;
      bool remapRoughness { false };
      
      vec3f kd { .65f };
      Texture::SP map_kd;
      vec3f ks { .0f };
      Texture::SP map_ks;
      Texture::SP map_bump;
    };
    
    struct SubSurfaceMaterial : public Material
    {
      typedef std::shared_ptr<SubSurfaceMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "SubSurfaceMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float uRoughness { .001f };
      float vRoughness { .001f };
      bool  remapRoughness { false };
      /*! in the one pbrt v3 model that uses that, these materials
          carry 'name' fields like "Apple" etc - not sure if that's
          supposed to specify some sub-surface medium properties!? */
      std::string name;
    };
    
    struct MirrorMaterial : public Material
    {
      typedef std::shared_ptr<MirrorMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MirrorMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kr { .7f };
      Texture::SP map_bump;
    };
    
    
    struct FourierMaterial : public Material
    {
      typedef std::shared_ptr<FourierMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "FourierMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      std::string fileName;
    };
    
    struct MatteMaterial : public Material
    {
      typedef std::shared_ptr<MatteMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MatteMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kd { .65f };
      Texture::SP map_kd;
      float sigma { 10.0f };
      Texture::SP map_sigma;
      Texture::SP map_bump;
    };
    
    struct GlassMaterial : public Material
    {
      typedef std::shared_ptr<GlassMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "GlassMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      vec3f kr { 2.5371551514f, 2.5371551514f, 2.5371551514f };
      vec3f kt { 0.8627451062f, 0.9411764741f, 1.f };
      float index { 1.2999999523f };
    };
    
    struct UberMaterial : public Material {
      typedef std::shared_ptr<UberMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "UberMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kd { .65f };
      Texture::SP map_kd;
    
      vec3f ks { 0.f };
      Texture::SP map_ks;
    
      vec3f kr { 0.f };
      Texture::SP map_kr;
      
      vec3f kt { 0.f };
      Texture::SP map_kt;
      
      vec3f opacity { 0.f };
      Texture::SP map_opacity;
      
      float alpha { 0.f };
      Texture::SP map_alpha;

      float shadowAlpha { 0.f };
      Texture::SP map_shadowAlpha;

      float index { 1.33333f };
      float roughness { 0.5f };
      Texture::SP map_roughness;
      Texture::SP map_bump;
    };

    /*! base abstraction for any geometric shape. for pbrt, this
        should actually be called a 'Shape'; we call it shape for
        historic reasons (it's the term that embree and ospray - as
        well as a good many others - use) */
    struct Shape : public Entity {
      typedef std::shared_ptr<Shape> SP;
    
      Shape(Material::SP material = Material::SP()) : material(material) {}
    
      /*! virtual destructor, to force this to be polymorphic */
      virtual ~Shape() {}
    
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      virtual size_t getNumPrims() const = 0;
      virtual box3f getBounds() = 0;
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) = 0;
      virtual box3f getPrimBounds(const size_t primID);
      
      /*! the pbrt material assigned to the underlying shape */
      Material::SP material;
      /*! textures directl assigned to the shape (ie, not to the
        material) */
      std::map<std::string,Texture::SP> textures;
    };

    /*! a plain triangle mesh, with vec3f vertex and normal arrays, and
      vec3i indices for triangle vertex indices. normal and texture
      arrays may be empty, but if they exist, will use the same vertex
      indices as vertex positions */
    struct TriangleMesh : public Shape {
      typedef std::shared_ptr<TriangleMesh> SP;

      /*! constructor */
      TriangleMesh(Material::SP material=Material::SP()) : Shape(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "TriangleMesh"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
    
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      virtual size_t getNumPrims() const override
      {
        return index.size();
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
    
      virtual box3f getBounds() override;

      std::vector<vec3f> vertex;
      std::vector<vec3f> normal;
      std::vector<vec3i> index;
      /*! mutex to lock anything within this object that might get
        changed by multiple threads (eg, computing bbox */
      std::mutex         mutex;
      bool  haveComputedBounds { false };
      box3f bounds;
    };

    /*! a plain qaud mesh, where every quad is a pair of two
      triangles. "degenerate" quads (in the sense of quads that are
      actually only triangles) are in fact allowed by having index.w
      be equal to index.z. 

      \note Note that PBRT itself doesn't actually _have_ quad meshes
      (only triangle meshes), but many objects do in fact contain
      either mostly or even all quads; so we added this type for
      convenience - that renderers that _can_ optimie for quads can
      easily convert from pbrt's trianglemehs to a quadmesh using \see
      QuadMesh::createFrom(TriangleMesh::SP) */
    struct QuadMesh : public Shape {
      typedef std::shared_ptr<QuadMesh> SP;

      /*! constructor */
      QuadMesh(Material::SP material=Material::SP()) : Shape(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "QuadMesh"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      /*! create a new quad mesh by merging triangle pairs in given
        triangle mesh. triangles that cannot be merged into quads will
        be stored as degenerate quads */
      static QuadMesh::SP makeFrom(TriangleMesh::SP triangleMesh);
    
      virtual size_t getNumPrims() const override
      {
        return index.size();
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
      virtual box3f getBounds() override;

      std::vector<vec3f> vertex;
      std::vector<vec3f> normal;
      // std::vector<vec2f> texcoord;
      std::vector<vec4i> index;
      /*! mutex to lock anything within this object that might get
        changed by multiple threads (eg, computing bbox */
      std::mutex         mutex;
      bool  haveComputedBounds { false };
      box3f bounds;
    };

    /*! what we create for 'Shape "curve" type "cylinder"' */
    struct Curve : public Shape {
      typedef std::shared_ptr<Curve> SP;

      typedef enum : uint8_t {
        CurveType_Cylinder=0, CurveType_Flat, CurveType_Ribbon, CurveType_Unknown
          } CurveType;
      typedef enum : uint8_t {
        CurveBasis_Bezier=0, CurveBasis_BSpline, CurveBasis_Unknown
          } BasisType;
      
      /*! constructor */
      Curve(Material::SP material=Material::SP()) : Shape(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Curve"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      /*! for now, return '1' prim for the entire curve. since in pbrt
          a curve can be an entire spline of many points, most
          renderers will likely want to subdivide that into multiple
          smaller cubis curve segements .... but since we can't know
          what those renderers will do let's just report a single prim
          here */
      virtual size_t getNumPrims() const override
      { return 1; }
      
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
    
      virtual box3f getBounds() override;

      affine3f  transform;
      /*! the array of control points */
      CurveType type;
      BasisType basis;
      uint8_t   degree { 3 };
      std::vector<vec3f> P;
      float width0 { 1.f };
      float width1 { 1.f };
    };
      
    /*! a single sphere, with a radius and a transform. note we do
        _not_ yet apply the transform to update ratius and center, and
        use the pbrt way of storing them individually (in thoery this
        allows ellipsoins, too!?) */
    struct Sphere : public Shape {
      typedef std::shared_ptr<Sphere> SP;

      /*! constructor */
      Sphere(Material::SP material=Material::SP()) : Shape(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Sphere"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
    
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      virtual size_t getNumPrims() const override
      {
        return 1;
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
    
      virtual box3f getBounds() override;

      affine3f transform;
      float radius { 1.f };
    };


    /*! a single unit disk, with a radius and a transform. note we do
        _not_ yet apply the transform to update ratius and center, and
        use the pbrt way of storing them individually (in thoery this
        allows ellipsoins, too!?) */
    struct Disk : public Shape {
      typedef std::shared_ptr<Disk> SP;

      /*! constructor */
      Disk(Material::SP material=Material::SP()) : Shape(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Disk"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
    
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      virtual size_t getNumPrims() const override
      {
        return 1;
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
    
      virtual box3f getBounds() override;

      affine3f transform;
      float radius { 1.f };
      float height { 0.f };
    };


    struct Instance : public Entity {
      Instance() {}
      Instance(const std::shared_ptr<Object> &object,
               const affine3f                &xfm)
        : object(object), xfm(xfm)
      {}
      
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Instance"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      virtual box3f getBounds() const;

      typedef std::shared_ptr<Instance> SP;
      
      std::shared_ptr<Object> object;
      affine3f                xfm; // { PBRT_PARSER_VECTYPE_NAMESPACE::one };
    };

    /*! a logical "NamedObject" that can be instanced */
    struct Object : public Entity {
      
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Object> SP;

      /*! constructor */
      Object(const std::string &name="") : name(name) {}

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Object"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      virtual box3f getBounds() const;
    
      std::vector<Shape::SP>     shapes;
      std::vector<Instance::SP>  instances;
      std::string name = "";
    };

    /*! a camera as specified in the root .pbrt file. Note that unlike
      other entities for the camera we go one step beyond just
      seminatically parsing the objects, and actually already
      transform to a easier camera model.... this should eventually be
      reverted, only storing the actual values in this place */
    struct Camera : public Entity {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
      typedef std::shared_ptr<Camera> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Camera"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float fov { 30.f };
      
      // PBRT-Doc: float lensradius 0 The radius of the lens. Used to
      // render scenes with depth of field and focus effects. The
      // default value yields a pinhole camera.
      float lensRadius { 0.f };
      
      // PBRT-Doc: float focaldistance 10^30 The focal distance of the lens. If
      // "lensradius" is zero, this has no effect. Otherwise, it
      // specifies the distance from the camera origin to the focal
      // plane
      float focalDistance { 1e3f }; // iw: real default is 1e30f, but
                                    // that messes up our 'simplified'
                                    // camera...

      // the camera transform active when the camera was created
      affine3f frame;
      
      struct {
        /*! @{ computed mostly from 'lookat' value - using a _unit_ screen
          (that spans [-1,-1]-[+1,+1) that does not involve either
          screen size, screen resolution, or aspect ratio */
        vec3f screen_center;
        vec3f screen_du;
        vec3f screen_dv;
        /*! @} */
        vec3f lens_center;
        vec3f lens_du;
        vec3f lens_dv;
      } simplified;
    };

    /*! specification of 'film' / frame buffer. note this will only
        store the size specification, not the actual pixels itself */
    struct Film : public Entity {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
      typedef std::shared_ptr<Film> SP;

      Film(const vec2i &resolution, const std::string &fileName="")
        : resolution(resolution),
          fileName(fileName)
      {}

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Film"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      vec2i              resolution;
      /*! some files do specify the desired filename to be written
          to. may be empty if not set */
      std::string        fileName;
    };

    /*! the complete scene - pretty much the 'root' object that
        contains the WorldBegin/WorldEnd entities, plus high-level
        stuff like camera, frame buffer specification, etc */
    struct Scene : public Entity //, public std::enable_shared_from_this<Scene>
    {
      typedef std::shared_ptr<Scene> SP;


      /*! save scene to given file name, and reutrn number of bytes written */
      size_t saveTo(const std::string &outFileName);
      static Scene::SP loadFrom(const std::string &inFileName);

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Scene"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      /*! checks if the scene contains more than one level of instancing */
      bool isMultiLevel() const;
      /*! checks if the scene contains more than one level of instancing */
      bool isSingleLevel() const;
      /*! checks if the scene contains more than one level of instancing */
      bool isSingleObject() const;
    
      /*! helper function that flattens a multi-level scene into a
        flattned scene */
      void makeSingleLevel();

      /*! return bounding box of scene */
      box3f getBounds() const
      { assert(world); return world->getBounds(); }

      /*! just in case there's multiple cameras defined, this is a
        vector */
      std::vector<Camera::SP> cameras;
      
      Film::SP                film;
    
      /*! the worldbegin/worldend content */
      Object::SP              world;
    };
    
    /* compute some _rough_ storage cost esimate for a scene. this will
       allow bricking builders to greedily split only the most egregious
       objects */
    double computeApproximateStorageWeight(Scene::SP scene);

    /*! parse a pbrt file (using the pbrt_parser project, and convert
      the result over to a naivescenelayout */
    Scene::SP importPBRT(const std::string &fileName);
  
  } // ::pbrt::semantic
} // ::pbrt


