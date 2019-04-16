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

  /*! parse syntactic camera's 'film' value */
  void createFilm(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt)
  {
    if (pbrt->film &&
        pbrt->film->findParam<int>("xresolution") &&
        pbrt->film->findParam<int>("yresolution")) {
      vec2i resolution;
      std::string fileName = "";
      if (pbrt->film->hasParamString("filename"))
        fileName = pbrt->film->getParamString("filename");
        
      resolution.x = pbrt->film->findParam<int>("xresolution")->get(0);
      resolution.y = pbrt->film->findParam<int>("yresolution")->get(0);
      ours->film = std::make_shared<Film>(resolution,fileName);
    } else {
      std::cout << "warning: could not determine film resolution from pbrt scene" << std::endl;
    }
  }

  /*! iterate through syntactic parameters and find value fov field */
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

    for (auto camera : pbrt->cameras) {
      if (!camera)
        throw std::runtime_error("invalid pbrt camera");

      const float fov = findCameraFov(camera);

      // TODO: lensradius and focaldistance:

      //     float 	lensradius 	0 	The radius of the lens. Used to render scenes with depth of field and focus effects. The default value yields a pinhole camera.
      const float lensRadius = 0.f;

      // float 	focaldistance 	10^30 	The focal distance of the lens. If "lensradius" is zero, this has no effect. Otherwise, it specifies the distance from the camera origin to the focal plane.
      const float focalDistance = 1.f;
    
      const affine3f frame = inverse(camera->transform.atStart);
    
      ours->simplified.lens_center = frame.p;
      ours->simplified.lens_du = lensRadius * frame.l.vx;
      ours->simplified.lens_dv = lensRadius * frame.l.vy;

      const float fovDistanceToUnitPlane = 0.5f / tanf(fov/2 * M_PI/180.f);
      ours->simplified.screen_center = frame.p + focalDistance * frame.l.vz;
      ours->simplified.screen_du = - focalDistance/fovDistanceToUnitPlane * frame.l.vx;
      ours->simplified.screen_dv = focalDistance/fovDistanceToUnitPlane * frame.l.vy;

      scene->cameras.push_back(ours);
    }
  }

  /*! create semantic camera from syntactic one */
  Camera::SP createCamera(pbrt::syntactic::Camera::SP camera)
  {
    if (!camera) return Camera::SP();

    Camera::SP ours = std::make_shared<Camera>();
    if (camera->hasParam1f("fov"))
      ours->fov = camera->getParam1f("fov");
    if (camera->hasParam1f("lensradius"))
      ours->lensRadius = camera->getParam1f("lensradius");
    if (camera->hasParam1f("focaldistance"))
      ours->focalDistance = camera->getParam1f("focaldistance");

    ours->frame = inverse(camera->transform.atStart);
      
    ours->simplified.lens_center
      = ours->frame.p;
    ours->simplified.lens_du
      = ours->lensRadius * ours->frame.l.vx;
    ours->simplified.lens_dv
      = ours->lensRadius * ours->frame.l.vy;
      
    const float fovDistanceToUnitPlane = 0.5f / tanf(ours->fov/2 * M_PI/180.f);
    ours->simplified.screen_center
      = ours->frame.p + ours->focalDistance * ours->frame.l.vz;
    ours->simplified.screen_du
      = - ours->focalDistance/fovDistanceToUnitPlane * ours->frame.l.vx;
    ours->simplified.screen_dv
      = ours->focalDistance/fovDistanceToUnitPlane * ours->frame.l.vy;

    return ours;
  }

} // ::pbrt
