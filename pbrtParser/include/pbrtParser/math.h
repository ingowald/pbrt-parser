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

#pragma once

#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#if defined(_MSC_VER)
#  define PBRT_PARSER_DLL_EXPORT __declspec(dllexport)
#  define PBRT_PARSER_DLL_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
#  define PBRT_PARSER_DLL_EXPORT __attribute__((visibility("default")))
#  define PBRT_PARSER_DLL_IMPORT __attribute__((visibility("default")))
#else
#  define PBRT_PARSER_DLL_EXPORT
#  define PBRT_PARSER_DLL_IMPORT
#endif

#if defined(PBRT_PARSER_DLL_INTERFACE)
#  ifdef pbrtParser_EXPORTS
#    define PBRT_PARSER_INTERFACE PBRT_PARSER_DLL_EXPORT
#  else
#    define PBRT_PARSER_INTERFACE PBRT_PARSER_DLL_IMPORT
#  endif
#else
#  define PBRT_PARSER_INTERFACE /*static lib*/
#endif

#include <iostream>
#include <math.h> // using cmath causes issues under Windows
#include <cfloat>
#include <limits>
#include <utility>
#include <vector>

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
      affine3f() = default;
      affine3f(const mat3f& l, const vec3f& p) :  l{l}, p{p} { }
      static affine3f identity() { return affine3f(mat3f(vec3f(1)), vec3f(0)); }
      static affine3f scale(const vec3f& u) { return affine3f(mat3f(u),vec3f(0)); }
      static affine3f translate(const vec3f& u) { return affine3f(mat3f(vec3f(1)), u); }
      static affine3f rotate(const vec3f& _u, float r);
      mat3f l;
      vec3f p;
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
    typedef std::vector<std::pair<float, float>> pairNf;

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


#ifdef __WIN32__
#  define osp_snprintf sprintf_s
#else
#  define osp_snprintf snprintf
#endif
  
  /*! added pretty-print function for large numbers, printing 10000000 as "10M" instead */
  inline std::string prettyDouble(const double val) {
    const double absVal = abs(val);
    char result[1000];

    if      (absVal >= 1e+15f) osp_snprintf(result,1000,"%.1f%c",val/1e18f,'E');
    else if (absVal >= 1e+15f) osp_snprintf(result,1000,"%.1f%c",val/1e15f,'P');
    else if (absVal >= 1e+12f) osp_snprintf(result,1000,"%.1f%c",val/1e12f,'T');
    else if (absVal >= 1e+09f) osp_snprintf(result,1000,"%.1f%c",val/1e09f,'G');
    else if (absVal >= 1e+06f) osp_snprintf(result,1000,"%.1f%c",val/1e06f,'M');
    else if (absVal >= 1e+03f) osp_snprintf(result,1000,"%.1f%c",val/1e03f,'k');
    else if (absVal <= 1e-12f) osp_snprintf(result,1000,"%.1f%c",val*1e15f,'f');
    else if (absVal <= 1e-09f) osp_snprintf(result,1000,"%.1f%c",val*1e12f,'p');
    else if (absVal <= 1e-06f) osp_snprintf(result,1000,"%.1f%c",val*1e09f,'n');
    else if (absVal <= 1e-03f) osp_snprintf(result,1000,"%.1f%c",val*1e06f,'u');
    else if (absVal <= 1e-00f) osp_snprintf(result,1000,"%.1f%c",val*1e03f,'m');
    else osp_snprintf(result,1000,"%f",(float)val);
    return result;
  }

  /*! added pretty-print function for large numbers, printing 10000000 as "10M" instead */
  inline std::string prettyNumber(const size_t s) {
    const double val = s;
    char result[1000];

    if      (val >= 1e+15f) osp_snprintf(result,1000,"%.1f%c",val/1e18f,'E');
    else if (val >= 1e+15f) osp_snprintf(result,1000,"%.1f%c",val/1e15f,'P');
    else if (val >= 1e+12f) osp_snprintf(result,1000,"%.1f%c",val/1e12f,'T');
    else if (val >= 1e+09f) osp_snprintf(result,1000,"%.1f%c",val/1e09f,'G');
    else if (val >= 1e+06f) osp_snprintf(result,1000,"%.1f%c",val/1e06f,'M');
    else if (val >= 1e+03f) osp_snprintf(result,1000,"%.1f%c",val/1e03f,'k');
    else osp_snprintf(result,1000,"%zu",s);
    return result;
  }
#undef osp_snprintf

  }
} // ::pbrt
