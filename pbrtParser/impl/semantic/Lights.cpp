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

  LightSource::SP SemanticParser::createLightSource_infinite
  (pbrt::syntactic::LightSource::SP in)
  {
    InfiniteLightSource::SP light = std::make_shared<InfiniteLightSource>();
    const std::string mapName = in->getParamString("mapname");
    if (mapName == "")
      std::cerr << "warning: no 'mapname' in infinite lightsource!?" << std::endl;
    
    light->transform = in->transform.atStart;
    light->mapName = mapName;
    
    return light;
  }
  
  /*! do create a track representation of given light, _without_
    checking whether that was already created */
  LightSource::SP SemanticParser::createLightSourceFrom(pbrt::syntactic::LightSource::SP in)
  {
    if (!in) {
      std::cerr << "warning: empty light!" << std::endl;
      return LightSource::SP();
    }
      
    const std::string type = in->type=="" ? in->getParamString("type") : in->type;

    // ==================================================================
    if (type == "infinite") 
      return createLightSource_infinite(in);

    // ==================================================================
#ifndef NDEBUG
    std::cout << "Warning: un-recognized light type '"+type+"'" << std::endl;
#endif
    throw std::runtime_error("un-recognized light type '"+type+"'");
    // return std::make_shared<LightSource>();
  }

  /*! check if this material has already been imported, and if so,
    find what we imported, and reutrn this. otherwise import and
    store for later use.
      
    important: it is perfectly OK for this material to be a null
    object - the area ligths in moana have this features, for
    example */
  LightSource::SP SemanticParser::findOrCreateLightSource(pbrt::syntactic::LightSource::SP in)
  {
    if (!in)
      return LightSource::SP();

    if (!lightSourceMapping[in]) {
      lightSourceMapping[in] = createLightSourceFrom(in);
    }
    return lightSourceMapping[in];
  }


} // ::pbrt
