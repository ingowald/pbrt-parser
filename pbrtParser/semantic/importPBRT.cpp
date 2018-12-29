// ======================================================================== //
// Copyright 2018 Ingo Wald                                                 //
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

#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
  
#define PBRT_PARSER_VECTYPE_NAMESPACE  ospcommon

#include "pbrtParser/semantic/Scene.h"
#include "pbrtParser/syntactic/Scene.h"
// std
#include <map>
#include <sstream>
// #define _USE_MATH_DEFINES 1
// #include <math.h>
// #include <cmath>

namespace pbrt {
  namespace semantic {

    using PBRTScene = pbrt::syntactic::Scene;
    using pbrt::syntactic::ParamArray;

    inline bool endsWith(const std::string &s, const std::string &suffix)
    {
      return s.substr(s.size()-suffix.size(),suffix.size()) == suffix;
    }

    struct SemanticParser
    {
      Scene::SP result;

      // ==================================================================
      // MATERIALS
      // ==================================================================
      std::map<pbrt::syntactic::Texture::SP,semantic::Texture::SP> textureMapping;

      /*! do create a track representation of given texture, _without_
        checking whether that was already created */
      semantic::Texture::SP createTextureFrom(pbrt::syntactic::Texture::SP in)
      {
        if (in->mapType == "imagemap") {
          const std::string fileName = in->getParamString("filename");
          if (fileName == "")
            std::cerr << "warning: pbrt image texture, but no filename!?" << std::endl;
          return std::make_shared<ImageTexture>(fileName);
        }
        if (in->mapType == "constant") {
          ConstantTexture::SP tex = std::make_shared<ConstantTexture>();
          if (in->hasParam1f("value"))
            tex->value = vec3f(in->getParam1f("value"));
          else
            in->getParam3f(&tex->value.x,"value");
          return tex;
        }
        if (in->mapType == "windy") {
          WindyTexture::SP tex = std::make_shared<WindyTexture>();
          return tex;
        }
        if (in->mapType == "scale") {
          ScaleTexture::SP tex = std::make_shared<ScaleTexture>();
#if 1
          tex->tex1 = findOrCreateTexture(in->getParamTexture("tex1"));
          tex->tex2 = findOrCreateTexture(in->getParamTexture("tex2"));
#else
          std::string tex1_name = in->getParamString("tex1");
          std::string tex2_name = in->getParamString("tex2");
          tex->tex1 = findOrCreateTexture(in->attributes->namedTexture[tex1_name]);
          tex->tex2 = findOrCreateTexture(in->attributes->namedTexture[tex2_name]);
#endif
          return tex;
        }
        if (in->mapType == "ptex") {
          const std::string fileName = in->getParamString("filename");
          if (fileName == "")
            std::cerr << "warning: pbrt image texture, but no filename!?" << std::endl;
          return std::make_shared<PtexFileTexture>(fileName);
        }
        throw std::runtime_error("un-handled pbrt texture type '"+in->toString()+"'");
        return std::make_shared<Texture>();
      }
      
      semantic::Texture::SP findOrCreateTexture(pbrt::syntactic::Texture::SP in)
      {
        if (!textureMapping[in]) {
          textureMapping[in] = createTextureFrom(in);
        }
        return textureMapping[in];
      }
    

      // ==================================================================
      // MATERIALS
      // ==================================================================
      std::map<pbrt::syntactic::Material::SP,semantic::Material::SP> materialMapping;

      /*! do create a track representation of given material, _without_
        checking whether that was already created */
      semantic::Material::SP createMaterialFrom(pbrt::syntactic::Material::SP in)
      {
        if (!in) {
          std::cerr << "warning: empty material!" << std::endl;
          return semantic::Material::SP();
        }
      
        const std::string type = in->type=="" ? in->getParamString("type") : in->type;

        // ==================================================================
        if (type == "plastic") {
          PlasticMaterial::SP mat = std::make_shared<PlasticMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kd") {
              if (in->hasParamTexture(name)) {
                mat->kd = vec3f(1.f);
                mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kd.x,name);
            }
            else if (name == "Ks") {
              if (in->hasParamTexture(name)) {
                mat->ks = vec3f(1.f);
                mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->ks.x,name);
            }
            else if (name == "roughness") {
              mat->roughness = in->getParam1f(name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled plastic-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "matte") {
          MatteMaterial::SP mat = std::make_shared<MatteMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kd") {
              if (in->hasParamTexture(name)) {
                mat->kd = vec3f(1.f);
                mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kd.x,name);
            }
            else if (name == "sigma") {
              mat->sigma = in->getParam1f(name);
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled matte-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "metal") {
          MetalMaterial::SP mat = std::make_shared<MetalMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "roughness") {
              mat->roughness = in->getParam1f(name);
            }
            else if (name == "eta") {
              mat->spectrum_eta = in->getParamString(name);
            }
            else if (name == "k") {
              mat->spectrum_k = in->getParamString(name);
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled metal-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "fourier") {
          FourierMaterial::SP mat = std::make_shared<FourierMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "bsdffile") {
              mat->fileName = in->getParamString(name);
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled fourier-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "mirror") {
          MirrorMaterial::SP mat = std::make_shared<MirrorMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kr") {
              if (in->hasParamTexture(name)) {
                throw std::runtime_error("mapping Kr for mirror materials not implemented");
              } else
                in->getParam3f(&mat->kr.x,name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled mirror-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "uber") {
          UberMaterial::SP mat = std::make_shared<UberMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kd") {
              if (in->hasParamTexture(name)) {
                mat->kd = vec3f(1.f);
                mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kd.x,name);
            }
            else if (name == "Kr") {
              if (in->hasParamTexture(name)) {
                mat->kr = vec3f(1.f);
                mat->map_kr = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kr.x,name);
            }
            else if (name == "Kt") {
              if (in->hasParamTexture(name)) {
                mat->kt = vec3f(1.f);
                mat->map_kt = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kt.x,name);
            }
            else if (name == "Ks") {
              if (in->hasParamTexture(name)) {
                mat->ks = vec3f(1.f);
                mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->ks.x,name);
            }
            else if (name == "alpha") {
              if (in->hasParamTexture(name)) {
                mat->alpha = 1.f;
                mat->map_alpha = findOrCreateTexture(in->getParamTexture(name));
              } else
                mat->alpha = in->getParam1f(name);
            }
            else if (name == "opacity") {
              if (in->hasParamTexture(name)) {
                mat->opacity = vec3f(1.f);
                mat->map_opacity = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->opacity.x,name);
            }
            else if (name == "index") {
              mat->index = in->getParam1f(name);
            }
            else if (name == "roughness") {
              mat->roughness = in->getParam1f(name);
            }
            else if (name == "shadowalpha") {
              if (in->hasParamTexture(name)) {
                mat->shadowAlpha = 1.f;
                mat->map_shadowAlpha = findOrCreateTexture(in->getParamTexture(name));
              } else
                mat->shadowAlpha = in->getParam1f(name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled uber-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "disney") {
          DisneyMaterial::SP mat = std::make_shared<DisneyMaterial>();

          in->getParam3f(&mat->color.x,"color");
          mat->anisotropic    = in->getParam1f("anisotropic",    0.f );
          mat->clearCoat      = in->getParam1f("clearcoat",      0.f );
          mat->clearCoatGloss = in->getParam1f("clearcoatgloss", 1.f );
          mat->diffTrans      = in->getParam1f("difftrans",      1.35f );
          mat->eta            = in->getParam1f("eta",            1.2f );
          mat->flatness       = in->getParam1f("flatness",       0.2f );
          mat->metallic       = in->getParam1f("metallic",       0.f );
          mat->roughness      = in->getParam1f("roughness",      0.9f );
          mat->sheen          = in->getParam1f("sheen",          0.3f );
          mat->sheenTint      = in->getParam1f("sheentint",      0.68f );
          mat->specTrans      = in->getParam1f("spectrans",      0.f );
          mat->specularTint   = in->getParam1f("speculartint",   0.f );
          mat->thin           = in->getParamBool("thin",           true);
          return mat;
        }

        // ==================================================================
        if (type == "mix") {
          MixMaterial::SP mat = std::make_shared<MixMaterial>();

          in->getParam3f(&mat->mix.x,"amount");

          const std::string name0 = in->getParamString("namedmaterial1");
          if (name0 == "")
            throw std::runtime_error("mix material w/o 'namedmaterial1' parameter");
          const std::string name1 = in->getParamString("namedmaterial2");
          if (name1 == "")
            throw std::runtime_error("mix material w/o 'namedmaterial2' parameter");

          assert(in->attributes);
          pbrt::syntactic::Material::SP mat0 = in->attributes->namedMaterial[name0];
          assert(mat0);
          pbrt::syntactic::Material::SP mat1 = in->attributes->namedMaterial[name1];
          assert(mat1);

          mat->material0    = findOrCreateMaterial(mat0);
          mat->material1    = findOrCreateMaterial(mat1);
        
          return mat;
        }

        // ==================================================================
        if (type == "translucent") {
          TranslucentMaterial::SP mat = std::make_shared<TranslucentMaterial>();

          in->getParam3f(&mat->transmit.x,"transmit");
          in->getParam3f(&mat->reflect.x,"reflect");
          mat->map_kd = findOrCreateTexture(in->getParamTexture("Kd"));
          assert(mat->map_kd);
        
          return mat;
        }

        // ==================================================================
        if (type == "glass") {
          GlassMaterial::SP mat = std::make_shared<GlassMaterial>();

          in->getParam3f(&mat->kr.x,"Kr");
          in->getParam3f(&mat->kt.x,"Kt");
          mat->index = in->getParam1f("index");
        
          return mat;
        }

        // ==================================================================
#ifndef NDEBUG
        std::cout << "Warning: un-recognizd material type '"+type+"'" << std::endl;
#endif
        return std::make_shared<semantic::Material>();
      }

      /*! check if this material has already been imported, and if so,
        find what we imported, and reutrn this. otherwise import and
        store for later use.
      
        important: it is perfectly OK for this material to be a null
        object - the area ligths in moana have this features, for
        example */
      semantic::Material::SP findOrCreateMaterial(pbrt::syntactic::Material::SP in)
      {
        // null materials get passed through ...
        if (!in)
          return semantic::Material::SP();
      
        if (!materialMapping[in]) {
          materialMapping[in] = createMaterialFrom(in);
        }
        return materialMapping[in];
      }
    
      /*! counter that tracks, for each un-handled shape type, how many
        "instances" of that shape type it could not create (note those
        are not _real_ instances in the ray tracing sense, just
        "occurrances" of that geometry type in the scene graph,
        _before_ object instantiation */
      std::map<std::string,size_t> unhandledShapeTypeCounter;
    
      /*! constructor that also perfoms all the work - converts the
        input 'pbrtScene' to a naivescenelayout, and assings that to
        'result' */
      SemanticParser(PBRTScene::SP pbrtScene)
        : pbrtScene(pbrtScene)
      {
        result        = std::make_shared<Scene>();
        result->root  = findOrEmitObject(pbrtScene->world);

        if (!unhandledShapeTypeCounter.empty()) {
          std::cerr << "WARNING: scene contained some un-handled shapes!" << std::endl;
          for (auto type : unhandledShapeTypeCounter)
            std::cerr << " - " << type.first << " : " << type.second << " occurrances" << std::endl;
        }
      }

    private:
      Instance::SP emitInstance(pbrt::syntactic::Object::Instance::SP pbrtInstance)
      {
        Instance::SP ourInstance
          = std::make_shared<Instance>();
        ourInstance->xfm    = (const affine3f&)pbrtInstance->xfm.atStart;
        ourInstance->object = findOrEmitObject(pbrtInstance->object);
        return ourInstance;
      }

      /*! extract 'texture' parameters from shape, and assign to geometry */
      void extractTextures(Geometry::SP geom, pbrt::syntactic::Shape::SP shape)
      {
        for (auto param : shape->param) {
          if (param.second->getType() != "texture")
            continue;

          geom->textures[param.first] = findOrCreateTexture(shape->getParamTexture(param.first));//param.second);
        }
      }

      Geometry::SP emitPlyMesh(pbrt::syntactic::Shape::SP shape)
      {
        // const affine3f xfm = instanceXfm*(ospcommon::affine3f&)shape->transform.atStart;
        const std::string fileName
          = pbrtScene->makeGlobalFileName(shape->getParamString("filename"));
        TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));
        pbrt_helper_loadPlyTriangles(fileName,ours->vertex,ours->normal,ours->index);

        affine3f xfm = shape->transform.atStart;
        for (vec3f &v : ours->vertex)
          v = xfmPoint(xfm,v);
        for (vec3f &v : ours->normal)
          v = xfmVector(xfm,v);

        extractTextures(ours,shape);
      
        return ours;
      }

      template<typename T>
      std::vector<T> extractVector(pbrt::syntactic::Shape::SP shape, const std::string &name)
      {
        std::vector<T> result;
        typename ParamArray<typename T::scalar_t>::SP param = shape->findParam<typename T::scalar_t>(name);
        if (param) {

          int dims = sizeof(T)/sizeof(typename T::scalar_t);
          size_t num = param->size();// / T::dims;
          const T *const data = (const T*)param->data();
        
          result.resize(num);
          std::copy(data,data+num,result.begin());
        }
        return result;
      }
    
      Geometry::SP emitTriangleMesh(pbrt::syntactic::Shape::SP shape)
      {
        TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));

        // vertices - param "P", 3x float each
        ours->vertex = extractVector<vec3f>(shape,"P");
        // vertex normals - param "N", 3x float each
        ours->normal = extractVector<vec3f>(shape,"N");
        // triangle vertex indices - param "P", 3x int each
        ours->index = extractVector<vec3i>(shape,"indices");

        affine3f xfm = shape->transform.atStart;
        for (vec3f &v : ours->vertex)
          v = xfmPoint(xfm,v);
        for (vec3f &v : ours->normal)
          v = xfmNormal(xfm,v);

        extractTextures(ours,shape);
      
        return ours;
      }

      Geometry::SP emitGeometry(pbrt::syntactic::Shape::SP shape)
      {
        if (shape->type == "plymesh") 
          return emitPlyMesh(shape);
        if (shape->type == "trianglemesh")
          return emitTriangleMesh(shape);

        // throw std::runtime_error("un-handled shape "+shape->type);
        unhandledShapeTypeCounter[shape->type]++;
        // std::cout << "WARNING: un-handled shape " << shape->type << std::endl;
        return Geometry::SP();
      }

      Geometry::SP findOrCreateGeometry(pbrt::syntactic::Shape::SP pbrtShape)
      {
        if (emittedGeometries.find(pbrtShape) != emittedGeometries.end())
          return emittedGeometries[pbrtShape];

        emittedGeometries[pbrtShape] = emitGeometry(pbrtShape);
        return emittedGeometries[pbrtShape];
      }
    
      Object::SP findOrEmitObject(pbrt::syntactic::Object::SP pbrtObject)
      {
        // PING;
        if (emittedObjects.find(pbrtObject) != emittedObjects.end()) {
          // std::cout << "FOUND OBJECT INSTANCE " << pbrtObject->name << std::endl;
          return emittedObjects[pbrtObject];
        }
      
        Object::SP ourObject = std::make_shared<Object>();
        ourObject->name = pbrtObject->name;
        for (auto shape : pbrtObject->shapes) {
          Geometry::SP ourGeometry = findOrCreateGeometry(shape);
          if (ourGeometry)
            ourObject->geometries.push_back(ourGeometry);
        }
        for (auto instance : pbrtObject->objectInstances)
          ourObject->instances.push_back(emitInstance(instance));

        emittedObjects[pbrtObject] = ourObject;
        return ourObject;
      }

      std::map<pbrt::syntactic::Object::SP,Object::SP> emittedObjects;
      std::map<pbrt::syntactic::Shape::SP,Geometry::SP> emittedGeometries;
    
      PBRTScene::SP pbrtScene;
      // Scene::SP ours;
    };

    vec2i createFrameBuffer(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt)
    {
      if (pbrt->film &&
          pbrt->film->findParam<int>("xresolution") &&
          pbrt->film->findParam<int>("yresolution")) {
        vec2i resolution;
        resolution.x = pbrt->film->findParam<int>("xresolution")->get(0);
        resolution.y = pbrt->film->findParam<int>("yresolution")->get(0);
        ours->frameBuffer = std::make_shared<FrameBuffer>(resolution);
      } else {
        std::cout << "warning: could not determine film resolution from pbrt scene" << std::endl;
      }
    }

    float findCameraFov(pbrt::syntactic::Camera::SP camera)
    {
      if (!camera->findParam<float>("fov")) {
        std::cerr << "warning - pbrt file has camera, but camera has no 'fov' field; replacing with constant 30 degrees" << std::endl;
        return 30;
      }
      return camera->findParam<float>("fov")->get(0);
    }

    /*! create a scene->camera from the pbrt model, if specified, or
      leave unchanged if not */
    void createCamera(Scene::SP scene, pbrt::syntactic::Scene::SP pbrt)
    {
      Camera::SP ours = std::make_shared<Camera>();
      if (pbrt->cameras.empty()) {
        std::cout << "warning: no 'camera'(s) in pbrt file" << std::endl;
        return;
      }

      pbrt::syntactic::Camera::SP camera = pbrt->cameras[0];
      if (!camera)
        throw std::runtime_error("invalid pbrt camera");

      const float fov = findCameraFov(camera);

      // TODO: lensradius and focaldistance:

      //     float 	lensradius 	0 	The radius of the lens. Used to render scenes with depth of field and focus effects. The default value yields a pinhole camera.
      const float lensRadius = 0.f;

      // float 	focaldistance 	10^30 	The focal distance of the lens. If "lensradius" is zero, this has no effect. Otherwise, it specifies the distance from the camera origin to the focal plane.
      const float focalDistance = 1.f;
    

    
      const affine3f frame = rcp(camera->transform.atStart);
    
      ours->lens_center = frame.p;
      ours->lens_du = lensRadius * frame.l.vx;
      ours->lens_dv = lensRadius * frame.l.vy;

      // assuming screen height of 1
      // tan(fov/2) = (1/2)/z = 1/(2z)
      // 2z = 1/tan(fov/2)
      // z = 0.5 / tan(fov/2)
      const float fovDistanceToUnitPlane = 0.5f / tanf(fov/2 * M_PI/180.f);
      ours->screen_center = frame.p + focalDistance * frame.l.vz;
      ours->screen_du = - focalDistance/fovDistanceToUnitPlane * frame.l.vx;
      ours->screen_dv = focalDistance/fovDistanceToUnitPlane * frame.l.vy;

      scene->camera = ours;
    }

    semantic::Scene::SP importPBRT(const std::string &fileName)
    {
      pbrt::syntactic::Scene::SP pbrt;
      if (endsWith(fileName,".pbsf"))
        pbrt = pbrt::syntactic::readBinary(fileName);
      else if (endsWith(fileName,".pbrt"))
        pbrt = pbrt::syntactic::parse(fileName);
      else
        throw std::runtime_error("could not detect input file format!? (unknown extension in '"+fileName+"')");
      
      semantic::Scene::SP scene = SemanticParser(pbrt).result;
      createFrameBuffer(scene,pbrt);
      createCamera(scene,pbrt);
      return scene;
    }

  } // ::pbrt::semantic
} // ::pbrt
