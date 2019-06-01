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

#include "pbrtParser/Scene.h"
#include "../syntactic/Scene.h"
// std
#include <map>
#include <sstream>

namespace pbrt {

  using PBRTScene = pbrt::syntactic::Scene;
  using pbrt::syntactic::ParamArray;

  /*! helper function - return whether file ends with given extension */
  inline bool endsWith(const std::string &s, const std::string &suffix)
  {
    return s.substr(s.size()-suffix.size(),suffix.size()) == suffix;
  }

  /*! The class that "semantically" parses a syntactic::Scene into a
      semantic::Scene. In a syntactic scene we know only the
      high-level types of objects (e.g., that something is a array of
      floats, or a "parameter" of type "float" with name "radius" and
      value "1.0", or a "shape" with a given "type" string and some
      set of parameters), but do not know what those data actually
      mean (e.g., that that array of floats is an array or vertices in
      a triangle mesh, that this "float radius 1.0" is a
      SphereShape::radius, or that the given shape is a
      TriangleMeshShape etc. The semantic parser si the class that
      translates from this syntactic thingy to actual C++ classes */
  struct SemanticParser
  {
    /*! the resulting _semantic_ scene we're building */
    Scene::SP result;
    
    /*! the _syntatic_ scnee we're parsing (our _input_) */
    PBRTScene::SP pbrtScene;

    /*! constructor that also perfoms all the work - converts the
      input 'pbrtScene' to a naivescenelayout, and assings that to
      'result' */
    SemanticParser(PBRTScene::SP pbrtScene)
      : pbrtScene(pbrtScene)
    {
      result        = std::make_shared<Scene>();
      result->world  = findOrEmitObject(pbrtScene->world);

      if (!unhandledShapeTypeCounter.empty()) {
        std::cerr << "WARNING: scene contained some un-handled shapes!" << std::endl;
        for (auto type : unhandledShapeTypeCounter)
          std::cerr << " - " << type.first << " : " << type.second << " occurrances" << std::endl;
      }
    }

  private:
    // ==================================================================
    // Textures
    // ==================================================================
    std::map<pbrt::syntactic::Texture::SP,Texture::SP> textureMapping;

    /*! do create a track representation of given texture, _without_
      checking whether that was already created */
    Texture::SP createTextureFrom(pbrt::syntactic::Texture::SP in);
    /*! @{ specializing parsing function for given texture type */
    Texture::SP createTexture_mix(pbrt::syntactic::Texture::SP in);
    Texture::SP createTexture_scale(pbrt::syntactic::Texture::SP in);
    Texture::SP createTexture_ptex(pbrt::syntactic::Texture::SP in);
    Texture::SP createTexture_constant(pbrt::syntactic::Texture::SP in);
    Texture::SP createTexture_checker(pbrt::syntactic::Texture::SP in);
    /*! @} */
    Texture::SP findOrCreateTexture(pbrt::syntactic::Texture::SP in);
    
    // ==================================================================
    // LigthSources
    // ==================================================================
    std::map<pbrt::syntactic::LightSource::SP,LightSource::SP> lightSourceMapping;
    
    /*! do create a track representation of given light, _without_
      checking whether that was already created */
    LightSource::SP createLightSourceFrom(pbrt::syntactic::LightSource::SP in);

    /*! check if this light has already been imported, and if so,
      find what we imported, and reutrn this. otherwise import and
      store for later use.
      
      important: it is perfectly OK for this light to be a null
      object - the area ligths in moana have this features, for
      example */
    LightSource::SP findOrCreateLightSource(pbrt::syntactic::LightSource::SP in);
    LightSource::SP createLightSource_infinite(pbrt::syntactic::LightSource::SP in);
    LightSource::SP createLightSource_distant(pbrt::syntactic::LightSource::SP in);

    // ==================================================================
    // Materials
    // ==================================================================
    std::map<pbrt::syntactic::Material::SP,Material::SP> materialMapping;

    /*! @{ type-specific extraction routines (ie, we already know the
        type, and only have to extract the potential/expected
        parameters) */
    Material::SP createMaterial_uber(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_metal(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_matte(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_fourier(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_mirror(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_substrate(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_disney(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_mix(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_plastic(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_translucent(pbrt::syntactic::Material::SP in);
    Material::SP createMaterial_glass(pbrt::syntactic::Material::SP in);
    /*! @} */
    
    /*! do create a track representation of given material, _without_
      checking whether that was already created */
    Material::SP createMaterialFrom(pbrt::syntactic::Material::SP in);

    /*! check if this material has already been imported, and if so,
      find what we imported, and reutrn this. otherwise import and
      store for later use.
      
      important: it is perfectly OK for this material to be a null
      object - the area ligths in moana have this features, for
      example */
    Material::SP findOrCreateMaterial(pbrt::syntactic::Material::SP in);

    /*! counter that tracks, for each un-handled shape type, how many
      "instances" of that shape type it could not create (note those
      are not _real_ instances in the ray tracing sense, just
      "occurrances" of that shape type in the scene graph,
      _before_ object instantiation */
    std::map<std::string,size_t> unhandledShapeTypeCounter;
    
    // ==================================================================
    // Geometry: shapes, instances, objects
    // ==================================================================

    std::map<pbrt::syntactic::Object::SP,Object::SP> emittedObjects;
    std::map<pbrt::syntactic::Shape::SP,Shape::SP>   emittedShapes;
    
    AreaLight::SP parseAreaLight(pbrt::syntactic::AreaLightSource::SP in);
    
    /*! check if object has already been emitted, and return reference
        is so; else emit new and return reference */
    Object::SP findOrEmitObject(pbrt::syntactic::Object::SP pbrtObject);
    
    /*! check if shapehas already been emitted, and return reference
        is so; else emit new and return reference */
    Shape::SP findOrCreateShape(pbrt::syntactic::Shape::SP pbrtShape);

    /*! emit given instance */
    Instance::SP emitInstance(pbrt::syntactic::Object::Instance::SP pbrtInstance);
    
    /*! emit given shape */
    Shape::SP emitShape(pbrt::syntactic::Shape::SP shape);

    /*! @{ type specific extraction routines for very specific shape type */
    Shape::SP emitPlyMesh(pbrt::syntactic::Shape::SP shape);
    Shape::SP emitDisk(pbrt::syntactic::Shape::SP shape);
    Shape::SP emitSphere(pbrt::syntactic::Shape::SP shape);
    Shape::SP emitCurve(pbrt::syntactic::Shape::SP shape);
    Shape::SP emitTriangleMesh(pbrt::syntactic::Shape::SP shape);
    /*! @} */


    
    // ==================================================================
    // General helper stuff
    // ==================================================================

    /*! extract 'texture' parameters from shape, and assign to shape */
    void extractTextures(Shape::SP geom, pbrt::syntactic::Shape::SP shape);

    template<typename T>
    std::vector<T> extractVector(pbrt::syntactic::Shape::SP shape, const std::string &name)
    {
      std::vector<T> result;
      typename ParamArray<typename T::scalar_t>::SP param = shape->findParam<typename T::scalar_t>(name);
      if (param) {

        int dims = sizeof(T)/sizeof(typename T::scalar_t);
        size_t num = param->size() / dims;// / T::dims;
        const T *const data = (const T*)param->data();
        
        result.resize(num);
        std::copy(data,data+num,result.begin());
      }
      return result;
    }
    
  };

  /*! iterate through syntactic parameters and find value fov field */
  float findCameraFov(pbrt::syntactic::Camera::SP camera);

  /*! parse syntactic camera's 'film' value */
  void createFilm(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt);
  
  /*! create a scene->camera from the pbrt model, if specified, or
    leave unchanged if not */
  void createCamera(Scene::SP scene, pbrt::syntactic::Scene::SP pbrt);

  /*! create semantic camera from syntactic one */
  Camera::SP createCamera(pbrt::syntactic::Camera::SP camera);

} // ::pbrt
