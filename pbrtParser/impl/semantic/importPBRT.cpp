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

#include "pbrtParser/Scene.h"
#include "../syntactic/Scene.h"
#include "SemanticParser.h"
// std
#include <map>
#include <sstream>

namespace pbrt {

// #if 0
//   namespace ply {
//     void parse(const std::string &fileName,
//                std::vector<vec3f> &pos,
//                std::vector<vec3f> &nor,
//                std::vector<vec3i> &idx)
//     {
//       happly::PLYData ply(fileName);
        
//       if(ply.hasElement("vertex")) {
//         happly::Element& elem = ply.getElement("vertex");
//         if(elem.hasProperty("x") && elem.hasProperty("y") && elem.hasProperty("z")) {
//           std::vector<float> x = elem.getProperty<float>("x");
//           std::vector<float> y = elem.getProperty<float>("y");
//           std::vector<float> z = elem.getProperty<float>("z");
//           pos.resize(x.size());
//           for(int i = 0; i < x.size(); i ++) {
//             pos[i] = vec3f(x[i], y[i], z[i]);
//           }
//         } else {
//           throw std::runtime_error("missing positions in ply");
//         }
//         if(elem.hasProperty("nx") && elem.hasProperty("ny") && elem.hasProperty("nz")) {
//           std::vector<float> x = elem.getProperty<float>("nx");
//           std::vector<float> y = elem.getProperty<float>("ny");
//           std::vector<float> z = elem.getProperty<float>("nz");
//           nor.resize(x.size());
//           for(int i = 0; i < x.size(); i ++) {
//             nor[i] = vec3f(x[i], y[i], z[i]);
//           }
//         }
//       } else {
//         throw std::runtime_error("missing positions in ply");
//       }
//       if (ply.hasElement("face")) {
//         happly::Element& elem = ply.getElement("face");
//         if(elem.hasProperty("vertex_indices")) {
//           std::vector<std::vector<int>> fasces = elem.getListProperty<int>("vertex_indices");
//           for(int j = 0; j < fasces.size(); j ++) {
//             std::vector<int>& face = fasces[j];
//             for (int i=2;i<face.size();i++) {
//               idx.push_back(vec3i(face[0], face[i-1], face[i]));
//             }
//           }
//         } else {
//           throw std::runtime_error("missing faces in ply");
//         }          
//       } else {
//         throw std::runtime_error("missing faces in ply");
//       }                        
//     }

//   } // ::pbrt::ply

//   using PBRTScene = pbrt::syntactic::Scene;
//   using pbrt::syntactic::ParamArray;

//   struct SemanticParser
//   {
//     Scene::SP result;

//     // ==================================================================
//     // MATERIALS
//     // ==================================================================
//     std::map<pbrt::syntactic::Texture::SP,Texture::SP> textureMapping;

//     // ==================================================================
//     // MATERIALS
//     // ==================================================================
//     std::map<pbrt::syntactic::Material::SP,Material::SP> materialMapping;

//     /*! do create a track representation of given material, _without_
//       checking whether that was already created */
//     Material::SP createMaterialFrom(pbrt::syntactic::Material::SP in)
//     {
//       PING; PRINT(in);
//       if (!in) {
//         std::cerr << "warning: empty material!" << std::endl;
//         return Material::SP();
//       }
      
//       const std::string type = in->type=="" ? in->getParamString("type") : in->type;
//       const std::string name = in->name;
      
//       // ==================================================================
//       if (type == "") {
//         return std::make_shared<Material>();
//       }
        
//       // ==================================================================
//       if (type == "plastic") {
//         PlasticMaterial::SP mat = std::make_shared<PlasticMaterial>(name);
//         for (auto it : in->param) {
//           std::string name = it.first;
//           if (name == "Kd") {
//             if (in->hasParamTexture(name)) {
//               mat->kd = vec3f(1.f);
//               mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->kd.x,name);
//           }
//           else if (name == "Ks") {
//             if (in->hasParamTexture(name)) {
//               mat->ks = vec3f(1.f);
//               mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->ks.x,name);
//           }
//           else if (name == "roughness") {
//             if (in->hasParamTexture(name))
//               mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
//             else
//               mat->roughness = in->getParam1f(name);
//           }
//           else if (name == "remaproughness") {
//             mat->remapRoughness = in->getParamBool(name);
//           }
//           else if (name == "bumpmap") {
//             mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
//           }
//           else if (name == "type") {
//             /* ignore */
//           } else
//             throw std::runtime_error("un-handled plastic-material parameter '"+it.first+"'");
//         };
//         return mat;
//       }
      
//       // ==================================================================
//       if (type == "matte") {
//         MatteMaterial::SP mat = std::make_shared<MatteMaterial>(name);
//         for (auto it : in->param) {
//           std::string name = it.first;
//           if (name == "Kd") {
//             if (in->hasParamTexture(name)) {
//               mat->kd = vec3f(1.f);
//               mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->kd.x,name);
//           }
//           else if (name == "sigma") {
//             if (in->hasParam1f(name))
//               mat->sigma = in->getParam1f(name);
//             else 
//               mat->map_sigma = findOrCreateTexture(in->getParamTexture(name));
//           }
//           else if (name == "type") {
//             /* ignore */
//           }
//           else if (name == "bumpmap") {
//             mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
//           } else
//             throw std::runtime_error("un-handled matte-material parameter '"+it.first+"'");
//         };
//         return mat;
//       }
      
//       // ==================================================================
//       if (type == "metal") {
//         MetalMaterial::SP mat = std::make_shared<MetalMaterial>(name);
//         for (auto it : in->param) {
//           std::string name = it.first;
//           if (name == "roughness") {
//             if (in->hasParamTexture(name))
//               mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
//             else
//               mat->roughness = in->getParam1f(name);
//           }
//           else if (name == "uroughness") {
//             if (in->hasParamTexture(name))
//               mat->map_uRoughness = findOrCreateTexture(in->getParamTexture(name));
//             else
//               mat->uRoughness = in->getParam1f(name);
//           }
//           else if (name == "vroughness") {
//             if (in->hasParamTexture(name))
//               mat->map_vRoughness = findOrCreateTexture(in->getParamTexture(name));
//             else
//               mat->vRoughness = in->getParam1f(name);
//           }
//           else if (name == "remaproughness") {
//             mat->remapRoughness = in->getParamBool(name);
//           }
//           else if (name == "eta") {
//             if (in->hasParam3f(name))
//               in->getParam3f(&mat->eta.x,name);
//             else
//               mat->spectrum_eta = in->getParamString(name);
//           }
//           else if (name == "k") {
//             if (in->hasParam3f(name))
//               in->getParam3f(&mat->k.x,name);
//             else
//               mat->spectrum_k = in->getParamString(name);
//           }
//           else if (name == "bumpmap") {
//             mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
//           }
//           else if (name == "type") {
//             /* ignore */
//           } else
//             throw std::runtime_error("un-handled metal-material parameter '"+it.first+"'");
//         };
//         return mat;
//       }
      
//       // ==================================================================
//       if (type == "fourier") {
//         FourierMaterial::SP mat = std::make_shared<FourierMaterial>(name);
//         for (auto it : in->param) {
//           std::string name = it.first;
//           if (name == "bsdffile") {
//             mat->fileName = in->getParamString(name);
//           }
//           else if (name == "type") {
//             /* ignore */
//           } else
//             throw std::runtime_error("un-handled fourier-material parameter '"+it.first+"'");
//         };
//         return mat;
//       }
      
//       // ==================================================================
//       if (type == "mirror") {
//         MirrorMaterial::SP mat = std::make_shared<MirrorMaterial>(name);
//         for (auto it : in->param) {
//           std::string name = it.first;
//           if (name == "Kr") {
//             if (in->hasParamTexture(name)) {
//               throw std::runtime_error("mapping Kr for mirror materials not implemented");
//             } else
//               in->getParam3f(&mat->kr.x,name);
//           }
//           else if (name == "bumpmap") {
//             mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
//           }
//           else if (name == "type") {
//             /* ignore */
//           } else
//             throw std::runtime_error("un-handled mirror-material parameter '"+it.first+"'");
//         };
//         return mat;
//       }
      
//       // ==================================================================
//       if (type == "uber") {
//         UberMaterial::SP mat = std::make_shared<UberMaterial>(name);
//         for (auto it : in->param) {
//           std::string name = it.first;
//           if (name == "Kd") {
//             if (in->hasParamTexture(name)) {
//               mat->kd = vec3f(1.f);
//               mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->kd.x,name);
//           }
//           else if (name == "Kr") {
//             if (in->hasParamTexture(name)) {
//               mat->kr = vec3f(1.f);
//               mat->map_kr = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->kr.x,name);
//           }
//           else if (name == "Kt") {
//             if (in->hasParamTexture(name)) {
//               mat->kt = vec3f(1.f);
//               mat->map_kt = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->kt.x,name);
//           }
//           else if (name == "Ks") {
//             if (in->hasParamTexture(name)) {
//               mat->ks = vec3f(1.f);
//               mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->ks.x,name);
//           }
//           else if (name == "alpha") {
//             if (in->hasParamTexture(name)) {
//               mat->alpha = 1.f;
//               mat->map_alpha = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               mat->alpha = in->getParam1f(name);
//           }
//           else if (name == "opacity") {
//             if (in->hasParamTexture(name)) {
//               mat->opacity = vec3f(1.f);
//               mat->map_opacity = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->opacity.x,name);
//           }
//           else if (name == "index") {
//             mat->index = in->getParam1f(name);
//           }
//           else if (name == "roughness") {
//             if (in->hasParamTexture(name))
//               mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
//             else if (in->hasParam1f(name))
//               mat->roughness = in->getParam1f(name);
//             else
//               throw std::runtime_error("uber::roughness in un-recognized format...");
//             // else
//             //   in->getParam3f(&mat->roughness.x,name);
//           }
//           else if (name == "shadowalpha") {
//             if (in->hasParamTexture(name)) {
//               mat->shadowAlpha = 1.f;
//               mat->map_shadowAlpha = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               mat->shadowAlpha = in->getParam1f(name);
//           }
//           else if (name == "bumpmap") {
//             mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
//           }
//           else if (name == "type") {
//             /* ignore */
//           } else
//             throw std::runtime_error("un-handled uber-material parameter '"+it.first+"'");
//         };
//         return mat;
//       }
      
//       // ==================================================================
//       if (type == "substrate") {
//         SubstrateMaterial::SP mat = std::make_shared<SubstrateMaterial>(name);
//         for (auto it : in->param) {
//           std::string name = it.first;
//           if (name == "Kd") {
//             if (in->hasParamTexture(name)) {
//               mat->kd = vec3f(1.f);
//               mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->kd.x,name);
//           }
//           else if (name == "Ks") {
//             if (in->hasParamTexture(name)) {
//               mat->ks = vec3f(1.f);
//               mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               in->getParam3f(&mat->ks.x,name);
//           }
//           else if (name == "uroughness") {
//             if (in->hasParamTexture(name)) {
//               mat->uRoughness = 1.f;
//               mat->map_uRoughness = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               mat->uRoughness = in->getParam1f(name);
//           }
//           else if (name == "vroughness") {
//             if (in->hasParamTexture(name)) {
//               mat->vRoughness = 1.f;
//               mat->map_vRoughness = findOrCreateTexture(in->getParamTexture(name));
//             } else
//               mat->vRoughness = in->getParam1f(name);
//           }
//           else if (name == "remaproughness") {
//             mat->remapRoughness = in->getParamBool(name);
//           }
//           else if (name == "bumpmap") {
//             mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
//           }
//           else if (name == "type") {
//             /* ignore */
//           } else
//             throw std::runtime_error("un-handled substrate-material parameter '"+it.first+"'");
//         };
//         return mat;
//       }
      
//       // ==================================================================
//       if (type == "disney") {
//         DisneyMaterial::SP mat = std::make_shared<DisneyMaterial>(name);

//         in->getParam3f(&mat->color.x,"color");
//         mat->anisotropic    = in->getParam1f("anisotropic",    0.f );
//         mat->clearCoat      = in->getParam1f("clearcoat",      0.f );
//         mat->clearCoatGloss = in->getParam1f("clearcoatgloss", 1.f );
//         mat->diffTrans      = in->getParam1f("difftrans",      1.35f );
//         mat->eta            = in->getParam1f("eta",            1.2f );
//         mat->flatness       = in->getParam1f("flatness",       0.2f );
//         mat->metallic       = in->getParam1f("metallic",       0.f );
//         mat->roughness      = in->getParam1f("roughness",      0.9f );
//         mat->sheen          = in->getParam1f("sheen",          0.3f );
//         mat->sheenTint      = in->getParam1f("sheentint",      0.68f );
//         mat->specTrans      = in->getParam1f("spectrans",      0.f );
//         mat->specularTint   = in->getParam1f("speculartint",   0.f );
//         mat->thin           = in->getParamBool("thin",           true);
//         return mat;
//       }

//       // ==================================================================
//       if (type == "mix") {
//         MixMaterial::SP mat = std::make_shared<MixMaterial>(name);
          
//         if (in->hasParamTexture("amount"))
//           mat->map_amount = findOrCreateTexture(in->getParamTexture("amount"));
//         else
//           in->getParam3f(&mat->amount.x,"amount");
          
//         const std::string name0 = in->getParamString("namedmaterial1");
//         PRINT(name0);
//         if (name0 == "")
//           throw std::runtime_error("mix material w/o 'namedmaterial1' parameter");
//         const std::string name1 = in->getParamString("namedmaterial2");
//         PRINT(name1);
//         if (name1 == "")
//           throw std::runtime_error("mix material w/o 'namedmaterial2' parameter");

//         assert(in->attributes);
//         pbrt::syntactic::Material::SP mat0 = in->attributes->namedMaterial[name0];
//         assert(mat0);
//         pbrt::syntactic::Material::SP mat1 = in->attributes->namedMaterial[name1];
//         assert(mat1);

//         mat->material0    = findOrCreateMaterial(mat0);
//         mat->material1    = findOrCreateMaterial(mat1);

//         PING;
//         PRINT(mat->material0);
//         PRINT(mat->material1);
        
//         return mat;
//       }

//       // ==================================================================
//       if (type == "translucent") {
//         TranslucentMaterial::SP mat = std::make_shared<TranslucentMaterial>(name);

//         in->getParam3f(&mat->transmit.x,"transmit");
//         in->getParam3f(&mat->reflect.x,"reflect");
//         if (in->hasParamTexture("Kd"))
//           mat->map_kd = findOrCreateTexture(in->getParamTexture("Kd"));
//         else
//           in->getParam3f(&mat->kd.x,"Kd");
          
//         return mat;
//       }

//       // ==================================================================
//       if (type == "glass") {
//         GlassMaterial::SP mat = std::make_shared<GlassMaterial>(name);

//         in->getParam3f(&mat->kr.x,"Kr");
//         in->getParam3f(&mat->kt.x,"Kt");
//         mat->index = in->getParam1f("index");
        
//         return mat;
//       }

//       // ==================================================================
// #ifndef NDEBUG
//       std::cout << "Warning: un-recognizd material type '"+type+"'" << std::endl;
// #endif
//       return std::make_shared<Material>();
//     }

//     /*! check if this material has already been imported, and if so,
//       find what we imported, and reutrn this. otherwise import and
//       store for later use.
      
//       important: it is perfectly OK for this material to be a null
//       object - the area ligths in moana have this features, for
//       example */
//     Material::SP findOrCreateMaterial(pbrt::syntactic::Material::SP in)
//     {
//       // null materials get passed through ...
//       if (!in)
//         return Material::SP();
      
//       if (!materialMapping[in]) {
//         materialMapping[in] = createMaterialFrom(in);
//       }
//       return materialMapping[in];
//     }
    
//     /*! counter that tracks, for each un-handled shape type, how many
//       "instances" of that shape type it could not create (note those
//       are not _real_ instances in the ray tracing sense, just
//       "occurrances" of that shape type in the scene graph,
//       _before_ object instantiation */
//     std::map<std::string,size_t> unhandledShapeTypeCounter;
    
//     /*! constructor that also perfoms all the work - converts the
//       input 'pbrtScene' to a naivescenelayout, and assings that to
//       'result' */
//     SemanticParser(PBRTScene::SP pbrtScene)
//       : pbrtScene(pbrtScene)
//     {
//       result        = std::make_shared<Scene>();
//       result->world  = findOrEmitObject(pbrtScene->world);

//       std::cout << "semantic - done with findoremit" << std::endl;
//       if (!unhandledShapeTypeCounter.empty()) {
//         std::cerr << "WARNING: scene contained some un-handled shapes!" << std::endl;
//         for (auto type : unhandledShapeTypeCounter)
//           std::cerr << " - " << type.first << " : " << type.second << " occurrances" << std::endl;
//       }
//     }

//   private:
//     Instance::SP emitInstance(pbrt::syntactic::Object::Instance::SP pbrtInstance)
//     {
//       Instance::SP ourInstance
//         = std::make_shared<Instance>();
//       ourInstance->xfm    = (const affine3f&)pbrtInstance->xfm.atStart;
//       ourInstance->object = findOrEmitObject(pbrtInstance->object);
//       return ourInstance;
//     }

//     /*! extract 'texture' parameters from shape, and assign to shape */
//     void extractTextures(Shape::SP geom, pbrt::syntactic::Shape::SP shape)
//     {
//       for (auto param : shape->param) {
//         if (param.second->getType() != "texture")
//           continue;

//         geom->textures[param.first] = findOrCreateTexture(shape->getParamTexture(param.first));//param.second);
//       }
//     }

//     Shape::SP emitPlyMesh(pbrt::syntactic::Shape::SP shape)
//     {
//       const std::string fileName
//         = pbrtScene->makeGlobalFileName(shape->getParamString("filename"));
//       TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));
//       ply::parse(fileName,ours->vertex,ours->normal,ours->index);

//       affine3f xfm = shape->transform.atStart;
//       for (vec3f &v : ours->vertex)
//         v = xfmPoint(xfm,v);
//       for (vec3f &v : ours->normal)
//         v = xfmNormal(xfm,v);

//       extractTextures(ours,shape);
//       return ours;
//     }

//     template<typename T>
//     std::vector<T> extractVector(pbrt::syntactic::Shape::SP shape, const std::string &name)
//     {
//       std::vector<T> result;
//       typename ParamArray<typename T::scalar_t>::SP param = shape->findParam<typename T::scalar_t>(name);
//       if (param) {

//         int dims = sizeof(T)/sizeof(typename T::scalar_t);
//         size_t num = param->size() / dims;// / T::dims;
//         const T *const data = (const T*)param->data();
        
//         result.resize(num);
//         std::copy(data,data+num,result.begin());
//       }
//       return result;
//     }
    
//     Shape::SP emitTriangleMesh(pbrt::syntactic::Shape::SP shape)
//     {
//       TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));

//       // vertices - param "P", 3x float each
//       ours->vertex = extractVector<vec3f>(shape,"P");
//       // vertex normals - param "N", 3x float each
//       ours->normal = extractVector<vec3f>(shape,"N");
//       // triangle vertex indices - param "P", 3x int each
//       ours->index = extractVector<vec3i>(shape,"indices");

//       affine3f xfm = shape->transform.atStart;
//       for (vec3f &v : ours->vertex)
//         v = xfmPoint(xfm,v);
//       for (vec3f &v : ours->normal)
//         v = xfmNormal(xfm,v);
//       extractTextures(ours,shape);
      
//       return ours;
//     }



//     Shape::SP emitCurve(pbrt::syntactic::Shape::SP shape)
//     {
//       Curve::SP ours = std::make_shared<Curve>(findOrCreateMaterial(shape->material));

//       // -------------------------------------------------------
//       // check 'type'
//       const std::string type
//         = shape->hasParamString("type")
//         ? shape->getParamString("type")
//         : std::string("");
//       if (type == "cylinder")
//         ours->type = Curve::CurveType_Cylinder;
//       else if (type == "ribbon")
//         ours->type = Curve::CurveType_Ribbon;
//       else if (type == "flat")
//         ours->type = Curve::CurveType_Flat;
//       else 
//         ours->type = Curve::CurveType_Unknown;
        
//       // -------------------------------------------------------
//       // check 'basis'
//       const std::string basis
//         = shape->hasParamString("basis")
//         ? shape->getParamString("basis")
//         : std::string("");
//       if (basis == "bezier")
//         ours->basis = Curve::CurveBasis_Bezier;
//       else if (basis == "bspline")
//         ours->basis = Curve::CurveBasis_BSpline;
//       else 
//         ours->basis = Curve::CurveBasis_Unknown;
        
//       // -------------------------------------------------------
//       // check 'width', 'width0', 'width1'
//       if (shape->hasParam1f("width")) 
//         ours->width0 = ours->width1 = shape->getParam1f("width");
        
//       if (shape->hasParam1f("width0")) 
//         ours->width0 = shape->getParam1f("width0");
//       if (shape->hasParam1f("width1")) 
//         ours->width1 = shape->getParam1f("width1");

//       if (shape->hasParam1i("degree")) 
//         ours->degree = shape->getParam1i("degree");

//       // vertices - param "P", 3x float each
//       ours->P = extractVector<vec3f>(shape,"P");
//       return ours;
//     }



      
//     Shape::SP emitSphere(pbrt::syntactic::Shape::SP shape)
//     {
//       Sphere::SP ours = std::make_shared<Sphere>(findOrCreateMaterial(shape->material));

//       ours->transform = shape->transform.atStart;
//       ours->radius    = shape->getParam1f("radius");
//       extractTextures(ours,shape);
      
//       return ours;
//     }

//     Shape::SP emitDisk(pbrt::syntactic::Shape::SP shape)
//     {
//       Disk::SP ours = std::make_shared<Disk>(findOrCreateMaterial(shape->material));

//       ours->transform = shape->transform.atStart;
//       ours->radius    = shape->getParam1f("radius");
        
//       if (shape->hasParam1f("height"))
//         ours->height    = shape->getParam1f("height");
        
//       extractTextures(ours,shape);
      
//       return ours;
//     }

//     Shape::SP emitShape(pbrt::syntactic::Shape::SP shape)
//     {
//       if (shape->type == "plymesh") 
//         return emitPlyMesh(shape);
//       if (shape->type == "trianglemesh")
//         return emitTriangleMesh(shape);
//       if (shape->type == "curve")
//         return emitCurve(shape);
//       if (shape->type == "sphere")
//         return emitSphere(shape);
//       if (shape->type == "disk")
//         return emitDisk(shape);

//       // throw std::runtime_error("un-handled shape "+shape->type);
//       unhandledShapeTypeCounter[shape->type]++;
//       // std::cout << "WARNING: un-handled shape " << shape->type << std::endl;
//       return Shape::SP();
//     }

//     AreaLightSource::SP getAreaLight(pbrt::syntactic::Attributes::SP attributes)
//     {
//       return 0;  kljlakdf;
//     }
    
//     Shape::SP findOrCreateShape(pbrt::syntactic::Shape::SP pbrtShape)
//     {
//       if (emittedShapes.find(pbrtShape) != emittedShapes.end())
//         return emittedShapes[pbrtShape];

//       emittedShapes[pbrtShape] = emitShape(pbrtShape);
//       emittedShapes[pbrtShape]->areaLight = getAreaLight(pbrtShape->attributes);
//       return emittedShapes[pbrtShape];
//     }
    
//     Object::SP findOrEmitObject(pbrt::syntactic::Object::SP pbrtObject)
//     {
//       if (emittedObjects.find(pbrtObject) != emittedObjects.end()) {
//         return emittedObjects[pbrtObject];
//       }
      
//       Object::SP ourObject = std::make_shared<Object>();
//       ourObject->name = pbrtObject->name;
//       for (auto shape : pbrtObject->shapes) {
//         Shape::SP ourShape = findOrCreateShape(shape);
//         if (ourShape)
//           ourObject->shapes.push_back(ourShape);
//       }
//       for (auto instance : pbrtObject->objectInstances)
//         ourObject->instances.push_back(emitInstance(instance));

//       emittedObjects[pbrtObject] = ourObject;
//       return ourObject;
//     }

//     std::map<pbrt::syntactic::Object::SP,Object::SP> emittedObjects;
//     std::map<pbrt::syntactic::Shape::SP,Shape::SP>   emittedShapes;
    
//     PBRTScene::SP pbrtScene;
//   };

//   void createFilm(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt)
//   {
//     if (pbrt->film &&
//         pbrt->film->findParam<int>("xresolution") &&
//         pbrt->film->findParam<int>("yresolution")) {
//       vec2i resolution;
//       std::string fileName = "";
//       if (pbrt->film->hasParamString("filename"))
//         fileName = pbrt->film->getParamString("filename");
        
//       resolution.x = pbrt->film->findParam<int>("xresolution")->get(0);
//       resolution.y = pbrt->film->findParam<int>("yresolution")->get(0);
//       ours->film = std::make_shared<Film>(resolution,fileName);
//     } else {
//       std::cout << "warning: could not determine film resolution from pbrt scene" << std::endl;
//     }
//   }

//   float findCameraFov(pbrt::syntactic::Camera::SP camera)
//   {
//     if (!camera->findParam<float>("fov")) {
//       std::cerr << "warning - pbrt file has camera, but camera has no 'fov' field; replacing with constant 30 degrees" << std::endl;
//       return 30;
//     }
//     return camera->findParam<float>("fov")->get(0);
//   }

//   /*! create a scene->camera from the pbrt model, if specified, or
//     leave unchanged if not */
//   void createCamera(Scene::SP scene, pbrt::syntactic::Scene::SP pbrt)
//   {
//     Camera::SP ours = std::make_shared<Camera>();
//     if (pbrt->cameras.empty()) {
//       std::cout << "warning: no 'camera'(s) in pbrt file" << std::endl;
//       return;
//     }

//     for (auto camera : pbrt->cameras) {
//       if (!camera)
//         throw std::runtime_error("invalid pbrt camera");

//       const float fov = findCameraFov(camera);

//       // TODO: lensradius and focaldistance:

//       //     float 	lensradius 	0 	The radius of the lens. Used to render scenes with depth of field and focus effects. The default value yields a pinhole camera.
//       const float lensRadius = 0.f;

//       // float 	focaldistance 	10^30 	The focal distance of the lens. If "lensradius" is zero, this has no effect. Otherwise, it specifies the distance from the camera origin to the focal plane.
//       const float focalDistance = 1.f;
    
//       const affine3f frame = inverse(camera->transform.atStart);
    
//       ours->simplified.lens_center = frame.p;
//       ours->simplified.lens_du = lensRadius * frame.l.vx;
//       ours->simplified.lens_dv = lensRadius * frame.l.vy;

//       const float fovDistanceToUnitPlane = 0.5f / tanf(fov/2 * M_PI/180.f);
//       ours->simplified.screen_center = frame.p + focalDistance * frame.l.vz;
//       ours->simplified.screen_du = - focalDistance/fovDistanceToUnitPlane * frame.l.vx;
//       ours->simplified.screen_dv = focalDistance/fovDistanceToUnitPlane * frame.l.vy;

//       scene->cameras.push_back(ours);
//     }
//   }

//   Camera::SP createCamera(pbrt::syntactic::Camera::SP camera)
//   {
//     if (!camera) return Camera::SP();

//     Camera::SP ours = std::make_shared<Camera>();
//     if (camera->hasParam1f("fov"))
//       ours->lensRadius = camera->getParam1f("fov");
//     if (camera->hasParam1f("lensradius"))
//       ours->lensRadius = camera->getParam1f("lensradius");
//     if (camera->hasParam1f("focaldistance"))
//       ours->focalDistance = camera->getParam1f("focaldistance");

//     ours->frame = inverse(camera->transform.atStart);
      
//     ours->simplified.lens_center
//       = ours->frame.p;
//     ours->simplified.lens_du
//       = ours->lensRadius * ours->frame.l.vx;
//     ours->simplified.lens_dv
//       = ours->lensRadius * ours->frame.l.vy;
      
//     const float fovDistanceToUnitPlane = 0.5f / tanf(ours->fov/2 * M_PI/180.f);
//     ours->simplified.screen_center
//       = ours->frame.p + ours->focalDistance * ours->frame.l.vz;
//     ours->simplified.screen_du
//       = - ours->focalDistance/fovDistanceToUnitPlane * ours->frame.l.vx;
//     ours->simplified.screen_dv
//       = ours->focalDistance/fovDistanceToUnitPlane * ours->frame.l.vy;

//     return ours;
//   }
// #endif
  Scene::SP importPBRT(const std::string &fileName, const std::string &basePath)
  {
    pbrt::syntactic::Scene::SP pbrt;
    if (endsWith(fileName,".pbrt"))
      pbrt = pbrt::syntactic::Scene::parse(fileName, basePath);
    else
      throw std::runtime_error("could not detect input file format!? (unknown extension in '"+fileName+"')");
      
    Scene::SP scene = SemanticParser(pbrt).result;
    createFilm(scene,pbrt);
    for (auto cam : pbrt->cameras)
      scene->cameras.push_back(createCamera(cam));
    return scene;
  }

} // ::pbrt
