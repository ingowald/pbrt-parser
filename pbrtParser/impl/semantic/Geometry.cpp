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
// ply parser:
#include "../3rdParty/happly.h"

namespace pbrt {
  namespace ply {
    
    void parse(const std::string &fileName,
               std::vector<vec3f> &pos,
               std::vector<vec3f> &nor,
               std::vector<vec2f> &tex,
               std::vector<vec3i> &idx)
    {
      happly::PLYData ply(fileName);
        
      if(ply.hasElement("vertex")) {
        happly::Element& elem = ply.getElement("vertex");
        if(elem.hasProperty("x") && elem.hasProperty("y") && elem.hasProperty("z")) {
          std::vector<float> x = elem.getProperty<float>("x");
          std::vector<float> y = elem.getProperty<float>("y");
          std::vector<float> z = elem.getProperty<float>("z");
          pos.resize(x.size());
          for(int i = 0; i < (int)x.size(); i ++) {
            pos[i] = vec3f(x[i], y[i], z[i]);
          }
        } else {
          throw std::runtime_error("missing positions in ply");
        }
        if(elem.hasProperty("nx") && elem.hasProperty("ny") && elem.hasProperty("nz")) {
          std::vector<float> x = elem.getProperty<float>("nx");
          std::vector<float> y = elem.getProperty<float>("ny");
          std::vector<float> z = elem.getProperty<float>("nz");
          nor.resize(x.size());
          for(int i = 0; i < (int)x.size(); i ++) {
            nor[i] = vec3f(x[i], y[i], z[i]);
          }
        }
        if(elem.hasProperty("u") && elem.hasProperty("v")) {
          std::vector<float> u = elem.getProperty<float>("u");
          std::vector<float> v = elem.getProperty<float>("v");
          tex.resize(u.size());
          for(int i = 0; i < (int)u.size(); i ++) {
            tex[i] = vec2f(u[i], v[i]);
          }
        }
      } else {
        throw std::runtime_error("missing positions in ply");
      }
      if (ply.hasElement("face")) {
        happly::Element& elem = ply.getElement("face");
        if(elem.hasProperty("vertex_indices")) {
          std::vector<std::vector<int>> faces = elem.getListPropertyAnySign<int>("vertex_indices");
          for(int j = 0; j < (int)faces.size(); j ++) {
            std::vector<int>& face = faces[j];
            for (int i=2;i<(int)face.size();i++) {
              idx.push_back(vec3i(face[0], face[i-1], face[i]));
            }
          }
        } else {
          throw std::runtime_error("missing faces in ply");
        }          
      } else {
        throw std::runtime_error("missing faces in ply");
      }                        
    }
  } // ::pbrt::ply


  
  Instance::SP SemanticParser::emitInstance(pbrt::syntactic::Object::Instance::SP pbrtInstance)
  {
    Instance::SP ourInstance
      = std::make_shared<Instance>();
    ourInstance->xfm    = (const affine3f&)pbrtInstance->xfm.atStart;
    ourInstance->object = findOrEmitObject(pbrtInstance->object);
    return ourInstance;
  }

  Shape::SP SemanticParser::emitPlyMesh(pbrt::syntactic::Shape::SP shape)
  {
    const std::string fileName
      = pbrtScene->makeGlobalFileName(shape->getParamString("filename"));
    TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));
    ply::parse(fileName,ours->vertex,ours->normal,ours->texcoord,ours->index);

    affine3f xfm = shape->transform.atStart;
    for (vec3f &v : ours->vertex)
      v = xfmPoint(xfm,v);
    for (vec3f &v : ours->normal)
      v = xfmNormal(xfm,v);

    extractTextures(ours,shape);
    return ours;
  }

  Shape::SP SemanticParser::emitTriangleMesh(pbrt::syntactic::Shape::SP shape)
  {
    TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));

    // vertices - param "P", 3x float each
    ours->vertex = extractVector<vec3f>(shape,"P");
    // vertex normals - param "N", 3x float each
    ours->normal = extractVector<vec3f>(shape,"N");
    // per-vertex texture coordinates - param "uv", 2x float each
    ours->texcoord = extractVector<vec2f>(shape,"uv");
    // triangle vertex indices - param "indices", 3x int each
    ours->index = extractVector<vec3i>(shape,"indices");

    affine3f xfm = shape->transform.atStart;
    for (vec3f &v : ours->vertex)
      v = xfmPoint(xfm,v);
    for (vec3f &v : ours->normal)
      v = xfmNormal(xfm,v);
    extractTextures(ours,shape);
      
    return ours;
  }

  Shape::SP SemanticParser::emitCurve(pbrt::syntactic::Shape::SP shape)
  {
    Curve::SP ours = std::make_shared<Curve>(findOrCreateMaterial(shape->material));
    ours->transform = shape->transform.atStart;

    // -------------------------------------------------------
    // check 'type'
    // -------------------------------------------------------
    const std::string type
      = shape->hasParamString("type")
      ? shape->getParamString("type")
      : std::string("");
    if (type == "cylinder")
      ours->type = Curve::CurveType_Cylinder;
    else if (type == "ribbon")
      ours->type = Curve::CurveType_Ribbon;
    else if (type == "flat")
      ours->type = Curve::CurveType_Flat;
    else 
      ours->type = Curve::CurveType_Unknown;
        
    // -------------------------------------------------------
    // check 'basis'
    // -------------------------------------------------------
    const std::string basis
      = shape->hasParamString("basis")
      ? shape->getParamString("basis")
      : std::string("");
    if (basis == "bezier")
      ours->basis = Curve::CurveBasis_Bezier;
    else if (basis == "bspline")
      ours->basis = Curve::CurveBasis_BSpline;
    else 
      ours->basis = Curve::CurveBasis_Unknown;
        
    // -------------------------------------------------------
    // check 'width', 'width0', 'width1'
    // -------------------------------------------------------
    if (shape->hasParam1f("width")) 
      ours->width0 = ours->width1 = shape->getParam1f("width");
        
    if (shape->hasParam1f("width0")) 
      ours->width0 = shape->getParam1f("width0");
    if (shape->hasParam1f("width1")) 
      ours->width1 = shape->getParam1f("width1");

    if (shape->hasParam1i("degree")) 
      ours->degree = shape->getParam1i("degree");

    // -------------------------------------------------------
    // vertices - param "P", 3x float each
    // -------------------------------------------------------
    ours->P = extractVector<vec3f>(shape,"P");
    return ours;
  }

      
  Shape::SP SemanticParser::emitSphere(pbrt::syntactic::Shape::SP shape)
  {
    Sphere::SP ours = std::make_shared<Sphere>(findOrCreateMaterial(shape->material));

    ours->transform = shape->transform.atStart;
    ours->radius    = shape->getParam1f("radius");
    extractTextures(ours,shape);
      
    return ours;
  }

  
  Shape::SP SemanticParser::emitDisk(pbrt::syntactic::Shape::SP shape)
  {
    Disk::SP ours = std::make_shared<Disk>(findOrCreateMaterial(shape->material));

    ours->transform = shape->transform.atStart;
    ours->radius    = shape->getParam1f("radius");
        
    if (shape->hasParam1f("height"))
      ours->height    = shape->getParam1f("height");
        
    extractTextures(ours,shape);
      
    return ours;
  }

  
  Shape::SP SemanticParser::emitShape(pbrt::syntactic::Shape::SP shape)
  {
    if (shape->type == "plymesh") 
      return emitPlyMesh(shape);
    if (shape->type == "trianglemesh")
      return emitTriangleMesh(shape);
    if (shape->type == "curve")
      return emitCurve(shape);
    if (shape->type == "sphere")
      return emitSphere(shape);
    if (shape->type == "disk")
      return emitDisk(shape);

    // throw std::runtime_error("un-handled shape "+shape->type);
    unhandledShapeTypeCounter[shape->type]++;
    // std::cout << "WARNING: un-handled shape " << shape->type << std::endl;
    return Shape::SP();
  }

  AreaLight::SP SemanticParser::parseAreaLight(pbrt::syntactic::AreaLightSource::SP in)
  {
    if (!in)
      return AreaLight::SP();

    if (in->type == "diffuse") {
      /*! parsing here is a bit more tricky, because the L parameter
          can be either an RGB, or a blackbody. Since the latter has
          two floats to the former's three floats we'll simply switch
          based on parameter count here .... */
      if (in->hasParam2f("L")) {
        DiffuseAreaLightBB::SP diffuse = std::make_shared<DiffuseAreaLightBB>();
        vec2f v;
        in->getParam2f(&v.x,"L");
        diffuse->temperature = v.x;
        diffuse->scale       = v.y;
        return diffuse;
      } else if (in->hasParam3f("L")) {
        DiffuseAreaLightRGB::SP diffuse = std::make_shared<DiffuseAreaLightRGB>();
        in->getParam3f(&diffuse->L.x,"L");
        return diffuse;
      } else {
        std::cout << "Warning: diffuse area light, but no 'L' parameter, or L is neither two (blackbody) nor three (rgba) floats?! Ignoring." << std::endl;
        return AreaLight::SP();
      }
      // auto L_it = in->param.find("L");
      // if (L_it == in->param.end()) {
      //   std::cout << "Warning: diffuse area light, but no 'L' parameter?! Ignoring." << std::endl;
      //   return AreaLight::SP();
      // }

      // syntactic::Param::SP param = L_it->second;
      // if (param->getType() == "blackbody") {
      //   // not blackbody - assume it's RGB
      //   DiffuseAreaLightBB::SP diffuse = std::make_shared<DiffuseAreaLightBB>();
        
      //   in->getParam3f(&diffuse->blackBody.x,"L");
      //   return diffuse;
      // } else {
      //   // not blackbody - assume it's RGB
      //   DiffuseAreaLightRGB::SP diffuse = std::make_shared<DiffuseAreaLightRGB>();
      //   in->getParam3f(&diffuse->blackBody.x,"L");
      //   return diffuse;
      // }
      
    }

    std::cout << "Warning: unknown area light type '" << in->type << "'." << std::endl;
    return AreaLight::SP();
  }
  
  /*! check if shapehas already been emitted, and return reference
    is so; else emit new and return reference */
  Shape::SP SemanticParser::findOrCreateShape(pbrt::syntactic::Shape::SP pbrtShape)
  {
    if (emittedShapes.find(pbrtShape) != emittedShapes.end())
      return emittedShapes[pbrtShape];

    emittedShapes[pbrtShape] = emitShape(pbrtShape);
    if (emittedShapes[pbrtShape])
      emittedShapes[pbrtShape]->reverseOrientation = pbrtShape->attributes->reverseOrientation;
    /* now, add area light sources */
    if (!pbrtShape->attributes->areaLightSources.empty()) {
      std::cout << "Shape has " << pbrtShape->attributes->areaLightSources.size()
                << " area light sources..." << std::endl;
      assert(pbrtShape->attributes);
      auto &areaLights = pbrtShape->attributes->areaLightSources;
      if (!areaLights.empty()) {
        if (areaLights.size() > 1)
          std::cout << "Warning: Shape has more than one area light!?" << std::endl;
        assert(emittedShapes[pbrtShape]);
        emittedShapes[pbrtShape]->areaLight = parseAreaLight(areaLights[0]);
      }
    }
    
    return emittedShapes[pbrtShape];
  }

  
  /*! check if object has already been emitted, and return reference
    is so; else emit new and return reference */
  Object::SP SemanticParser::findOrEmitObject(pbrt::syntactic::Object::SP pbrtObject)
  {
    if (emittedObjects.find(pbrtObject) != emittedObjects.end()) {
      return emittedObjects[pbrtObject];
    }
      
    Object::SP ourObject = std::make_shared<Object>();
    ourObject->name = pbrtObject->name;
    
    for (auto lightSource : pbrtObject->lightSources) {
      LightSource::SP ourLightSource = findOrCreateLightSource(lightSource);
      if (ourLightSource)
        ourObject->lightSources.push_back(ourLightSource);
    }
    
    for (auto shape : pbrtObject->shapes) {
      Shape::SP ourShape = findOrCreateShape(shape);
      if (ourShape)
        ourObject->shapes.push_back(ourShape);
    }
    
    for (auto instance : pbrtObject->objectInstances)
      ourObject->instances.push_back(emitInstance(instance));

    emittedObjects[pbrtObject] = ourObject;
    return ourObject;
  }

} // ::pbrt
