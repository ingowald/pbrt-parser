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

#include "SemanticParser.h"

#ifndef PRINT
# define PRINT(var) std::cout << #var << "=" << var << std::endl;
# define PING std::cout << __FILE__ << "::" << __LINE__ << ": " << __PRETTY_FUNCTION__ << std::endl;
#endif

namespace pbrt {

  Material::SP SemanticParser::createMaterial_uber(pbrt::syntactic::Material::SP in)
  {
    UberMaterial::SP mat = std::make_shared<UberMaterial>(in->name);
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
        if (in->hasParamTexture(name))
          mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
        else if (in->hasParam1f(name))
          mat->roughness = in->getParam1f(name);
        else
          throw std::runtime_error("uber::roughness in un-recognized format...");
        // else
        //   in->getParam3f(&mat->roughness.x,name);
      }
      else if (name == "uroughness") {
        // if (in->hasParamTexture(name))
        //   mat->map_uRoughness = findOrCreateTexture(in->getParamTexture(name));
        // else
          mat->uRoughness = in->getParam1f(name);
      }
      else if (name == "vroughness") {
        // if (in->hasParamTexture(name))
        //   mat->map_vRoughness = findOrCreateTexture(in->getParamTexture(name));
        // else
          mat->vRoughness = in->getParam1f(name);
      }
      // else if (name == "remaproughness") {
      //   mat->remapRoughness = in->getParamBool(name);
      // }
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

  Material::SP SemanticParser::createMaterial_metal(pbrt::syntactic::Material::SP in)
  {
    MetalMaterial::SP mat = std::make_shared<MetalMaterial>(in->name);
    for (auto it : in->param) {
      std::string name = it.first;
      if (name == "roughness") {
        if (in->hasParamTexture(name))
          mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
        else
          mat->roughness = in->getParam1f(name);
      }
      else if (name == "uroughness") {
        if (in->hasParamTexture(name))
          mat->map_uRoughness = findOrCreateTexture(in->getParamTexture(name));
        else
          mat->uRoughness = in->getParam1f(name);
      }
      else if (name == "vroughness") {
        if (in->hasParamTexture(name))
          mat->map_vRoughness = findOrCreateTexture(in->getParamTexture(name));
        else
          mat->vRoughness = in->getParam1f(name);
      }
      else if (name == "remaproughness") {
        mat->remapRoughness = in->getParamBool(name);
      }
      else if (name == "eta") {
        if (in->hasParam3f(name))
          in->getParam3f(&mat->eta.x,name);
        else {
          std::size_t N=0;
          in->getParamPairNf(nullptr,&N,name);
          mat->spectrum_eta.spd.resize(N);
          in->getParamPairNf(mat->spectrum_eta.spd.data(),&N,name);
        }
      }
      else if (name == "k") {
        if (in->hasParam3f(name))
          in->getParam3f(&mat->k.x,name);
        else {
          std::size_t N=0;
          in->getParamPairNf(nullptr,&N,name);
          mat->spectrum_k.spd.resize(N);
          in->getParamPairNf(mat->spectrum_k.spd.data(),&N,name);
        }
      }
      else if (name == "bumpmap") {
        mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
      }
      else if (name == "type") {
        /* ignore */
      } else
        throw std::runtime_error("un-handled metal-material parameter '"+it.first+"'");
    };
    return mat;
  }

  Material::SP SemanticParser::createMaterial_matte(pbrt::syntactic::Material::SP in)
  {
    MatteMaterial::SP mat = std::make_shared<MatteMaterial>(in->name);
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
        if (in->hasParam1f(name))
          mat->sigma = in->getParam1f(name);
        else 
          mat->map_sigma = findOrCreateTexture(in->getParamTexture(name));
      }
      else if (name == "type") {
        /* ignore */
      }
      else if (name == "bumpmap") {
        mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
      } else
        throw std::runtime_error("un-handled matte-material parameter '"+it.first+"'");
    };
    return mat;
  }

  Material::SP SemanticParser::createMaterial_fourier(pbrt::syntactic::Material::SP in)
  {
    FourierMaterial::SP mat = std::make_shared<FourierMaterial>(in->name);
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

  Material::SP SemanticParser::createMaterial_mirror(pbrt::syntactic::Material::SP in)
  {
    MirrorMaterial::SP mat = std::make_shared<MirrorMaterial>(in->name);
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

  Material::SP SemanticParser::createMaterial_substrate(pbrt::syntactic::Material::SP in)
  {
    SubstrateMaterial::SP mat = std::make_shared<SubstrateMaterial>(in->name);
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
      else if (name == "uroughness") {
        if (in->hasParamTexture(name)) {
          mat->uRoughness = 1.f;
          mat->map_uRoughness = findOrCreateTexture(in->getParamTexture(name));
        } else
          mat->uRoughness = in->getParam1f(name);
      }
      else if (name == "vroughness") {
        if (in->hasParamTexture(name)) {
          mat->vRoughness = 1.f;
          mat->map_vRoughness = findOrCreateTexture(in->getParamTexture(name));
        } else
          mat->vRoughness = in->getParam1f(name);
      }
      else if (name == "remaproughness") {
        mat->remapRoughness = in->getParamBool(name);
      }
      else if (name == "bumpmap") {
        mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
      }
      else if (name == "type") {
        /* ignore */
      } else
        throw std::runtime_error("un-handled substrate-material parameter '"+it.first+"'");
    };
    return mat;
  }

  Material::SP SemanticParser::createMaterial_disney(pbrt::syntactic::Material::SP in)
  {
    DisneyMaterial::SP mat = std::make_shared<DisneyMaterial>(in->name);

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

  Material::SP SemanticParser::createMaterial_mix(pbrt::syntactic::Material::SP in)
  {
    MixMaterial::SP mat = std::make_shared<MixMaterial>(in->name);
          
    if (in->hasParamTexture("amount"))
      mat->map_amount = findOrCreateTexture(in->getParamTexture("amount"));
    else
      in->getParam3f(&mat->amount.x,"amount");
          
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

  Material::SP SemanticParser::createMaterial_plastic(pbrt::syntactic::Material::SP in)
  {
    PlasticMaterial::SP mat = std::make_shared<PlasticMaterial>(in->name);
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
        if (in->hasParamTexture(name))
          mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
        else
          mat->roughness = in->getParam1f(name);
      }
      else if (name == "remaproughness") {
        mat->remapRoughness = in->getParamBool(name);
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
  
  Material::SP SemanticParser::createMaterial_translucent(pbrt::syntactic::Material::SP in)
  {
    TranslucentMaterial::SP mat = std::make_shared<TranslucentMaterial>(in->name);

    in->getParam3f(&mat->transmit.x,"transmit");
    in->getParam3f(&mat->reflect.x,"reflect");
    if (in->hasParamTexture("Kd"))
      mat->map_kd = findOrCreateTexture(in->getParamTexture("Kd"));
    else
      in->getParam3f(&mat->kd.x,"Kd");
          
    return mat;
  }

  Material::SP SemanticParser::createMaterial_glass(pbrt::syntactic::Material::SP in)
  {
    GlassMaterial::SP mat = std::make_shared<GlassMaterial>(in->name);

    in->getParam3f(&mat->kr.x,"Kr");
    in->getParam3f(&mat->kt.x,"Kt");
    mat->index = in->getParam1f("index");
        
    return mat;
  }

      
  /*! do create a track representation of given material, _without_
    checking whether that was already created */
  Material::SP SemanticParser::createMaterialFrom(pbrt::syntactic::Material::SP in)
  {
    if (!in) {
      std::cerr << "warning: empty material!" << std::endl;
      return Material::SP();
    }
      
    const std::string type = in->type=="" ? in->getParamString("type") : in->type;

    // ==================================================================
    if (type == "") 
      return std::make_shared<Material>();
        
    // ==================================================================
    if (type == "plastic") 
      return createMaterial_plastic(in);
      
    // ==================================================================
    if (type == "matte")
      return createMaterial_matte(in);
      
    // ==================================================================
    if (type == "metal") 
      return createMaterial_metal(in);
      
    // ==================================================================
    if (type == "fourier") 
      return createMaterial_fourier(in);
      
    // ==================================================================
    if (type == "mirror") 
      return createMaterial_mirror(in);
      
    // ==================================================================
    if (type == "uber") 
      return createMaterial_uber(in);
      
    // ==================================================================
    if (type == "substrate") 
      return createMaterial_substrate(in);
      
    // ==================================================================
    if (type == "disney") 
      return createMaterial_disney(in);

    // ==================================================================
    if (type == "mix")
      return createMaterial_mix(in);

    // ==================================================================
    if (type == "translucent")
      return createMaterial_translucent(in);

    // ==================================================================
    if (type == "glass") 
      return createMaterial_glass(in);

    // ==================================================================
#ifndef NDEBUG
    std::cout << "Warning: un-recognizd material type '"+type+"'" << std::endl;
#endif
    return std::make_shared<Material>();
  }

  /*! check if this material has already been imported, and if so,
    find what we imported, and reutrn this. otherwise import and
    store for later use.
      
    important: it is perfectly OK for this material to be a null
    object - the area ligths in moana have this features, for
    example */
  Material::SP SemanticParser::findOrCreateMaterial(pbrt::syntactic::Material::SP in)
  {
    // null materials get passed through ...
    if (!in)
      return Material::SP();

    if (!materialMapping[in]) {
      materialMapping[in] = createMaterialFrom(in);
    }
    return materialMapping[in];
  }


} // ::pbrt
