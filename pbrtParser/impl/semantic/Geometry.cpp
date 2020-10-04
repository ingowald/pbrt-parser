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
#include "../3rdParty/rply.h"

static int rply_vertex_callback_vec3(p_ply_argument argument) {
  float* buffer;
  ply_get_argument_user_data(argument, (void**)&buffer, nullptr);

  long index;
  ply_get_argument_element(argument, nullptr, &index);

  float value = (float)ply_get_argument_value(argument);
  buffer[index * 3] = value;
  return 1;
}

static int rply_vertex_callback_vec2(p_ply_argument argument) {
    float* buffer;
    ply_get_argument_user_data(argument, (void**)&buffer, nullptr);

    long index;
    ply_get_argument_element(argument, nullptr, &index);

    float value = (float)ply_get_argument_value(argument);
    buffer[index * 2] = value;
    return 1;
}

static int rply_face_callback(p_ply_argument argument) {
  int* buffer;
  ply_get_argument_user_data(argument, (void**)&buffer, nullptr);

  long length, value_index;
  ply_get_argument_property(argument, nullptr, &length, &value_index);

  // the first value of a list property, the one that gives the number of entries
  if (value_index == -1) {
    if (length != 3) {
      // NOTE: we can also handles quads if necessary (length == 4)
      throw std::runtime_error("Found face with vertex count different from 3, only triangles are supported");
    }
    return 1; // continue;
  }

  long index;
  ply_get_argument_element(argument, nullptr, &index);
  buffer[index * 3 + value_index] = (int)ply_get_argument_value(argument);
  return 1;
}

namespace pbrt {
  namespace ply {
    void parse(const std::string &fileName,
      std::vector<vec3f> &pos,
      std::vector<vec3f> &nor,
      std::vector<vec2f> &tex,
      std::vector<vec3i> &idx)
    {
      p_ply ply = ply_open(fileName.c_str(), nullptr, 0, nullptr);
      if (!ply)
        throw std::runtime_error(std::string("Couldn't open PLY file " + fileName).c_str());

      if (!ply_read_header(ply))
        throw std::runtime_error(std::string("Unable to read the header of PLY file " + fileName).c_str());

      long vertex_count = 0;
      long face_count = 0;
      bool has_normals = false;
      bool has_uvs = false;
      bool has_indices = false;
      const char* tex_coord_u_name = nullptr;
      const char* tex_coord_v_name = nullptr;
      const char* vertex_indices_name = nullptr;

      // Inspect structure of the PLY file.
      p_ply_element element = nullptr;
      while ((element = ply_get_next_element(ply, element)) != nullptr) {
        const char* name;
        long instance_count;
        ply_get_element_info(element, &name, &instance_count);

        // Check for vertex element.
        if (strcmp(name, "vertex") == 0) {
          vertex_count = instance_count;

          bool has_position_components[3] = {false};
          bool has_normal_components[3] = {false};
          bool has_uv_components[2] = {false};
         
          // Inspect vertex properties.
          p_ply_property property = nullptr;
          while ((property = ply_get_next_property(element, property)) != nullptr) {
            const char* pname;
            ply_get_property_info(property, &pname, nullptr, nullptr, nullptr);

            if (!strcmp(pname, "x"))
              has_position_components[0] = true;
            else if (!strcmp(pname, "y"))
              has_position_components[1] = true;
            else if (!strcmp(pname, "z"))
              has_position_components[2] = true;
            else if (!strcmp(pname, "nx"))
              has_normal_components[0] = true;
            else if (!strcmp(pname, "ny"))
              has_normal_components[1] = true;
            else if (!strcmp(pname, "nz"))
              has_normal_components[2] = true;
            else if (!strcmp(pname, "u") || !strcmp(pname, "s") || !strcmp(pname, "texture_u") || !strcmp(pname, "texture_s")) {
              has_uv_components[0] = true;
              tex_coord_u_name = pname;
            }
            else if (!strcmp(pname, "v") || !strcmp(pname, "t") || !strcmp(pname, "texture_v") || !strcmp(pname, "texture_t")) {
              has_uv_components[1] = true;
              tex_coord_v_name = pname;
            }
          }

          if (!(has_position_components[0] && has_position_components[1] && has_position_components[2]))
            throw std::runtime_error(fileName + ": Vertex coordinate property not found!");

          has_normals = has_normal_components[0] && has_normal_components[1] && has_normal_components[2];
          has_uvs = has_uv_components[0] && has_uv_components[1];
        }

        // Check for face element.
        else if (strcmp(name, "face") == 0) {
          face_count = instance_count;

          // Inspect face properties.
          p_ply_property property = nullptr;
          while ((property = ply_get_next_property(element, property)) != nullptr) {
            const char* pname;
            ply_get_property_info(property, &pname, nullptr, nullptr, nullptr);
            if (!strcmp(pname, "vertex_index") || !strcmp(pname, "vertex_indices")) {
              has_indices = true;
              vertex_indices_name = pname;
            }
          }
        }
      }

      if (vertex_count == 0 || face_count == 0)
        throw std::runtime_error(fileName + ": PLY file is invalid! No face/vertex elements found!");

      pos.resize(vertex_count);
      if (has_normals)
        nor.resize(vertex_count);
      if (has_uvs)
        tex.resize(vertex_count);
      if (has_indices)
        idx.resize(face_count);

      // Set callbacks to process the PLY properties.
      ply_set_read_cb(ply, "vertex", "x", rply_vertex_callback_vec3, &pos[0].x, 0);
      ply_set_read_cb(ply, "vertex", "y", rply_vertex_callback_vec3, &pos[0].y, 0);
      ply_set_read_cb(ply, "vertex", "z", rply_vertex_callback_vec3, &pos[0].z, 0);

      if (has_normals) {
        ply_set_read_cb(ply, "vertex", "nx", rply_vertex_callback_vec3, &nor[0].x, 0);
        ply_set_read_cb(ply, "vertex", "ny", rply_vertex_callback_vec3, &nor[0].y, 0);
        ply_set_read_cb(ply, "vertex", "nz", rply_vertex_callback_vec3, &nor[0].z, 0);
      }

      if (has_uvs) {
        ply_set_read_cb(ply, "vertex", tex_coord_u_name, rply_vertex_callback_vec2, &tex[0].x, 0);
        ply_set_read_cb(ply, "vertex", tex_coord_v_name, rply_vertex_callback_vec2, &tex[0].y, 0);
      }

      if (has_indices) {
        ply_set_read_cb(ply, "face", vertex_indices_name, rply_face_callback, &idx[0].x, 0);
      }

      // Read ply file.
      if (!ply_read(ply)) {
        ply_close(ply);
        throw std::runtime_error(fileName + ": unable to read the contents of PLY file");
      }
      ply_close(ply);
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

    Shape::SP newShape = emitShape(pbrtShape);
    emittedShapes[pbrtShape] = newShape;
    if (pbrtShape->attributes) {
      newShape->reverseOrientation
        = pbrtShape->attributes->reverseOrientation;
      /* now, add area light sources */
      
      if (!pbrtShape->attributes->areaLightSources.empty()) {
        std::cout << "Shape has " << pbrtShape->attributes->areaLightSources.size()
                  << " area light sources..." << std::endl;
        auto &areaLights = pbrtShape->attributes->areaLightSources;
        if (!areaLights.empty()) {
          if (areaLights.size() > 1)
            std::cout << "Warning: Shape has more than one area light!?" << std::endl;
          newShape->areaLight = parseAreaLight(areaLights[0]);
        }
      }
    }

    return newShape;
  }

  
  /*! check if object has already been emitted, and return reference
    is so; else emit new and return reference */
  Object::SP SemanticParser::findOrEmitObject(pbrt::syntactic::Object::SP pbrtObject)
  {
    if (emittedObjects.find(pbrtObject) != emittedObjects.end()) {
      return emittedObjects[pbrtObject];
    }
      
    Object::SP ourObject = std::make_shared<Object>();
    emittedObjects[pbrtObject] = ourObject;
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

    return ourObject;
  }

} // ::pbrt
