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

namespace pbrt {

#ifndef PRINT
# define PRINT(var) std::cout << #var << "=" << var << std::endl;
# define PING std::cout << __FILE__ << "::" << __LINE__ << ": " << __PRETTY_FUNCTION__ << std::endl;
#endif

  /*! extract 'texture' parameters from shape, and assign to shape */
  void SemanticParser::extractTextures(Shape::SP geom, pbrt::syntactic::Shape::SP shape)
  {
    for (auto param : shape->param) {
      if (param.second->getType() != "texture")
        continue;

      geom->textures[param.first] = findOrCreateTexture(shape->getParamTexture(param.first));//param.second);
    }
  }
  
  Texture::SP SemanticParser::createTexture_mix(pbrt::syntactic::Texture::SP in)
  {
    MixTexture::SP tex = std::make_shared<MixTexture>();

    if (in->hasParam3f("amount"))
      in->getParam3f(&tex->amount.x,"amount");
    else if (in->hasParam1f("amount"))
      tex->amount = vec3f(in->getParam1f("amount"));
    else 
      tex->map_amount = findOrCreateTexture(in->getParamTexture("amount"));
          
    if (in->hasParamTexture("tex1"))
      tex->tex1 = findOrCreateTexture(in->getParamTexture("tex1"));
    else if (in->hasParam3f("tex1"))
      in->getParam3f(&tex->scale1.x,"tex1");
    else
      tex->scale1 = vec3f(in->getParam1f("tex1"));
          
    if (in->hasParamTexture("tex2"))
      tex->tex2 = findOrCreateTexture(in->getParamTexture("tex2"));
    else if (in->hasParam3f("tex2"))
      in->getParam3f(&tex->scale2.x,"tex2");
    else
      tex->scale2 = vec3f(in->getParam1f("tex2"));
    return tex;
  }
  
  Texture::SP SemanticParser::createTexture_scale(pbrt::syntactic::Texture::SP in)
  {
    ScaleTexture::SP tex = std::make_shared<ScaleTexture>();
    if (in->hasParamTexture("tex1"))
      tex->tex1 = findOrCreateTexture(in->getParamTexture("tex1"));
    else if (in->hasParam3f("tex1"))
      in->getParam3f(&tex->scale1.x,"tex1");
    else
      tex->scale1 = vec3f(in->getParam1f("tex1"));
          
    if (in->hasParamTexture("tex2"))
      tex->tex2 = findOrCreateTexture(in->getParamTexture("tex2"));
    else if (in->hasParam3f("tex2"))
      in->getParam3f(&tex->scale2.x,"tex2");
    else
      tex->scale2 = vec3f(in->getParam1f("tex2"));
    return tex;
  }
  
  Texture::SP SemanticParser::createTexture_ptex(pbrt::syntactic::Texture::SP in)
  {
    const std::string fileName = in->getParamString("filename");
    if (fileName == "")
      std::cerr << "warning: pbrt image texture, but no filename!?" << std::endl;
    return std::make_shared<PtexFileTexture>(fileName);
  }


  Texture::SP SemanticParser::createTexture_constant(pbrt::syntactic::Texture::SP in)
  {
    ConstantTexture::SP tex = std::make_shared<ConstantTexture>();
    if (in->hasParam1f("value"))
      tex->value = vec3f(in->getParam1f("value"));
    else
      in->getParam3f(&tex->value.x,"value");
    return tex;
  }
  
  Texture::SP SemanticParser::createTexture_checker(pbrt::syntactic::Texture::SP in)
  {
    CheckerTexture::SP tex = std::make_shared<CheckerTexture>();
    for (auto it : in->param) {
      const std::string name = it.first;
      if (name == "uscale") {
        tex->uScale = in->getParam1f(name);
        continue;
      }
      if (name == "vscale") {
        tex->vScale = in->getParam1f(name);
        continue;
      }
      if (name == "tex1") {
        in->getParam3f(&tex->tex1.x,name);
        continue;
      }
      if (name == "tex2") {
        in->getParam3f(&tex->tex2.x,name);
        continue;
      }
      throw std::runtime_error("unknown checker texture param '"+name+"'");
    }
    return tex;
  }
  
  /*! do create a track representation of given texture, _without_
    checking whether that was already created */
  Texture::SP SemanticParser::createTextureFrom(pbrt::syntactic::Texture::SP in)
  {
    if (!in) return Texture::SP();
    
    // ------------------------------------------------------------------
    // switch to type-specialized parsing functions ...
    // ------------------------------------------------------------------
    if (in->mapType == "scale") 
      return createTexture_scale(in);
    if (in->mapType == "mix") 
      return createTexture_mix(in);
    if (in->mapType == "ptex") 
      return createTexture_ptex(in);
    if (in->mapType == "constant") 
      return createTexture_constant(in);
    if (in->mapType == "checkerboard") 
      return createTexture_checker(in);
      
    // ------------------------------------------------------------------
    // do small ones right here (todo: move those to separate
    // functions for cleanliness' sake)
    // ------------------------------------------------------------------
    if (in->mapType == "imagemap") {
      const std::string fileName = in->getParamString("filename");
      if (fileName == "")
        std::cerr << "warning: pbrt image texture, but no filename!?" << std::endl;
      return std::make_shared<ImageTexture>(fileName);
    }
    if (in->mapType == "fbm") {
      FbmTexture::SP tex = std::make_shared<FbmTexture>();
      return tex;
    }
    if (in->mapType == "windy") {
      WindyTexture::SP tex = std::make_shared<WindyTexture>();
      return tex;
    }
    if (in->mapType == "marble") {
      MarbleTexture::SP tex = std::make_shared<MarbleTexture>();
      if (in->hasParam1f("scale"))
        tex->scale = in->getParam1f("scale");
      return tex;
    }
    if (in->mapType == "wrinkled") {
      WrinkledTexture::SP tex = std::make_shared<WrinkledTexture>();
      return tex;
    }
    throw std::runtime_error("un-handled pbrt texture type '"+in->toString()+"'");
    return std::make_shared<Texture>();
  }

  Texture::SP SemanticParser::findOrCreateTexture(pbrt::syntactic::Texture::SP in)
  {
    if (!textureMapping[in]) {
      textureMapping[in] = createTextureFrom(in);
    }
    return textureMapping[in];
  }
    

} // ::pbrt
