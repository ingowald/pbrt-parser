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

#include <cmath>

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
    // const std::string mapName = in->getParamString("mapname");
    // if (mapName == "")
    //   std::cerr << "warning: no 'mapname' in infinite lightsource!?" << std::endl;
    
    light->transform = in->transform.atStart;
    // light->mapName = mapName;

    for (auto it : in->param) {
      std::string name = it.first;
      if (name == "mapname") {
        light->mapName = in->getParamString(name);
        continue;
      }
      if (name == "L") {
        in->getParam3f(&light->L.x,name);
        continue;
      }
      if (name == "scale") {
        in->getParam3f(&light->scale.x,name);
        continue;
      }
      if (name == "nsamples") {
        light->nSamples = in->getParam1i(name,light->nSamples);
        continue;
      }
      throw std::runtime_error("unknown 'infinite' light source param '"+name+"'");
    }
    
    return light;
  }
  
  LightSource::SP SemanticParser::createLightSource_distant
  (pbrt::syntactic::LightSource::SP in)
  {
    DistantLightSource::SP light = std::make_shared<DistantLightSource>();
    for (auto it : in->param) {
      std::string name = it.first;
      if (name == "from") {
        in->getParam3f(&light->from.x,name);
        continue;
      }
      if (name == "to") {
        in->getParam3f(&light->to.x,name);
        continue;
      }
      if (name == "L") {
        in->getParam3f(&light->L.x,name);
        continue;
      }
      if (name == "scale") {
        in->getParam3f(&light->scale.x,name);
        continue;
      }
      throw std::runtime_error("unknown 'distant' light source param '"+name+"'");
    }
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
    if (type == "distant") 
      return createLightSource_distant(in);

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


  // ==================================================================
  // CIE 1931 color matching functions
  //
  // From:
  // https://research.nvidia.com/publication/simple-analytic-approximations-cie-xyz-color-matching-functions
  // ==================================================================

  inline float cie_x(float lambda)
  {
      float t1 = (lambda - 442.0f) * ((lambda < 442.0f) ? 0.0624f : 0.0374f);
      float t2 = (lambda - 599.8f) * ((lambda < 599.8f) ? 0.0264f : 0.0323f);
      float t3 = (lambda - 501.1f) * ((lambda < 501.1f) ? 0.0490f : 0.0382f);
  
      return 0.362f * std::exp(-0.5f * t1 * t1) + 1.056f * std::exp(-0.5f * t2 * t2) - 0.065f * std::exp(-0.5f * t3 * t3);
  }
  
  inline float cie_y(float lambda)
  {
      float t1 = (lambda - 568.8f) * ((lambda < 568.8f) ? 0.0213f : 0.0247f);
      float t2 = (lambda - 530.9f) * ((lambda < 530.9f) ? 0.0613f : 0.0322f);
  
      return 0.821f * std::exp(-0.5f * t1 * t1) + 0.286f * std::exp(-0.5f * t2 * t2);
  }
  
  inline float cie_z(float lambda)
  {
      float t1 = (lambda - 437.0f) * ((lambda < 437.0f) ? 0.0845f : 0.0278f);
      float t2 = (lambda - 459.0f) * ((lambda < 459.0f) ? 0.0385f : 0.0725f);
  
      return 1.217f * std::exp(-0.5f * t1 * t1) + 0.681f * std::exp(-0.5f * t2 * t2);
  }

  // ==================================================================
  // see: http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
  // ==================================================================

  inline vec3f xyz_to_rgb(const vec3f& xyz)
  {
      // Assume sRGB working space and D65 reference white
      return mat3f(
          {  3.2404542, -0.9692660,  0.0556434 },
          { -1.5371385,  1.8760108, -0.2040259 },
          { -0.4985314,  0.0415560,  1.0572252 }
          ) * xyz;
  }

  // ==================================================================
  // DiffuseAreaLightBB
  // ==================================================================

  /*! convert blackbody temperature to linear rgb */
  vec3f DiffuseAreaLightBB::LinRGB() const
  {
    // Doubles: c (speed of light) is huge, h (Planck's constant) is small..
    static double const k = 1.3806488E-23;
    static double const h = 6.62606957E-34;
    static double const c = 2.99792458E8;

    static double const lmin = 400.0;
    static double const lmax = 700.0;
    static double const step = 1.0;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float n = 0.0f;

    // Evaluate blackbody spd at lambda nm
    auto bb = [this](float lambda) {
      lambda *= 1E-3; // nm to microns

      return ( ( 2.0 * 1E24 * h * c * c ) / std::pow(lambda, 5.0) )
           * ( 1.0 / (std::exp((1E6 * h * c) / (lambda * k * temperature)) - 1.0) );
    };

    // lambda where radiance is max
    float lambda_max_radiance = 2.8977721e-3 / temperature * 1e9 /* m2nm */;

    float max_radiance = bb(lambda_max_radiance);

    // Evaluate blackbody spd and convert to xyz
    for (double lambda = lmin; lambda <= lmax; lambda += step)
    {
        float p = bb(lambda) / max_radiance;

        x += p * cie_x(lambda);
        y += p * cie_y(lambda);
        z += p * cie_z(lambda);
        n +=     cie_y(lambda);
    }

    return xyz_to_rgb(vec3f(x / n, y / n, z / n));
  }
} // ::pbrt
