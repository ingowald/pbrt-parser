// ======================================================================== //
// Copyright 2016-2017 Ingo Wald                                            //
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

// pbrt
#include "pbrt/Parser.h"
// biff
#include "biff.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>

namespace pbrt_parser {

  using std::cout;
  using std::endl;

  FileName basePath = "";

  biff::Writer *writer = NULL;

  std::map<std::shared_ptr<Shape>,biff::Instance> exportedShape;
  std::map<std::tuple<std::shared_ptr<Material>,std::string,std::string>,int> exportedMaterial;

  //! transform used when original instance was emitted

  // std::map<int,affine3f> transformOfFirstInstance;

  std::vector<int> rootObjects;

  inline std::string prettyNumber(const size_t s) {
    double val = s;
    char result[100];
    if (val >= 1e12f) {
      sprintf(result,"%.1fT",val/1e12f);
    } else if (val >= 1e9f) {
      sprintf(result,"%.1fG",val/1e9f);
    } else if (val >= 1e6f) {
      sprintf(result,"%.1fM",val/1e6f);
    } else if (val >= 1e3f) {
      sprintf(result,"%.1fK",val/1e3f);
    } else {
      sprintf(result,"%lu",s);
    }
    return result;
  }



  // template<typename T>
  // std::string genMaterialParam(std::shared_ptr<Material> material, const char *name);
  
  // template<>
  // std::string genMaterialParam<vec3f>(std::shared_ptr<Material> material, const char *name)
  // {
  //   std::stringstream ss;
  //   vec3f v = material->getParam3f(name,vec3f(0.0f));
  //   ss << "  <param name=\"" << name << "\" type=\"float3\">" << v.x << " " << v.y << " " << v.z << "</param>" << endl;
  //   return ss.str();
  // }

  // template<>
  // std::string genMaterialParam<bool>(std::shared_ptr<Material> material, const char *name)
  // {
  //   std::stringstream ss;
  //   bool v = material->getParamBool(name,false);
  //   ss << "  <param name=\"" << name << "\" type=\"int\">" << (int)v << "</param>" << endl;
  //   return ss.str();
  // }

  // template<>
  // std::string genMaterialParam<float>(std::shared_ptr<Material> material, const char *name)
  // {
  //   std::stringstream ss;
  //   float v = material->getParam1f(name,false);
  //   ss << "  <param name=\"" << name << "\" type=\"float\">" << (int)v << "</param>" << endl;
  //   return ss.str();
  // }

    
  int exportTexture(const std::string &name)
  {
    PING; PRINT(name); return -1;
  }


  int exportMaterial(std::shared_ptr<Material> material,
                     const std::string colorTextureName,
                     const std::string bumpMapTextureName)
  {
    if (!material) 
      // default material
      return 0;

    std::tuple<std::shared_ptr<Material>,std::string,std::string> texturedMaterial
      = std::make_tuple(material,colorTextureName,bumpMapTextureName);
    if (exportedMaterial.find(texturedMaterial) != exportedMaterial.end())
      return exportedMaterial[texturedMaterial];

    biff::Material biffMat;
    const std::string type = material->type;
    if (type == "disney") {
      biffMat.param_vec3f["color"] = material->getParam3f("color");
      biffMat.param_float["spectrans"] = material->getParam1f("spectrans");
      biffMat.param_float["clearcoatgloss"] = material->getParam1f("clearcoatgloss");
      biffMat.param_float["speculartint"] = material->getParam1f("speculartint");
      biffMat.param_float["eta"] = material->getParam1f("eta");
      biffMat.param_float["sheentint"] = material->getParam1f("sheentint");
      biffMat.param_float["metallic"] = material->getParam1f("metallic");
      biffMat.param_float["anisotropic"] = material->getParam1f("anisotropic");
      biffMat.param_float["clearcoat"] = material->getParam1f("clearcoat");
      biffMat.param_float["clearcoat"] = material->getParam1f("clearcoat");
      biffMat.param_float["roughness"] = material->getParam1f("roughness");
      biffMat.param_float["sheen"] = material->getParam1f("sheen");
      biffMat.param_float["difftrans"] = material->getParam1f("difftrans");
      biffMat.param_float["flatness"] = material->getParam1f("flatness");
      biffMat.param_int["thin"] = material->getParamBool("flatness");
      if (colorTextureName != "")
        biffMat.param_texture["map_color"] = exportTexture(colorTextureName);
      if (bumpMapTextureName != "")
        biffMat.param_texture["map_bump"] = exportTexture(bumpMapTextureName);
      int thisID = exportedMaterial[texturedMaterial] = writer->push(biffMat);
      return thisID;
    } else {
      int thisID = -1;
      exportedMaterial[texturedMaterial] = thisID;
      return thisID;
    }
  }

  int writeTriangleMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
    std::shared_ptr<Material> mat = shape->material;
    cout << "writing shape " << shape->toString() << " w/ material " << (mat?mat->toString():"<null>") << endl;

    std::string texture_color   = shape->getParamString("color");
    std::string texture_bumpMap = shape->getParamString("bumpMap");
    int materialID = exportMaterial(shape->material,texture_color,texture_bumpMap);

    const affine3f xfm = instanceXfm*shape->transform;

    biff::TriMesh biffMesh;
    
    if (texture_bumpMap != "")
      biffMesh.texture.color = exportTexture(texture_bumpMap);
    if (texture_color != "")
      biffMesh.texture.displacement = exportTexture(texture_color);
    
    { // parse "point P"
      std::shared_ptr<ParamT<float> > param_P = shape->findParam<float>("P");
      if (param_P) {
        const size_t numPoints = param_P->paramVec.size() / 3;
        for (int i=0;i<numPoints;i++) {
          vec3f v(param_P->paramVec[3*i+0],
                  param_P->paramVec[3*i+1],
                  param_P->paramVec[3*i+2]);
          biffMesh.vtx.push_back(xfmPoint(xfm,v));
        }
      }
    }
      
    { // parse "int indices"
      std::shared_ptr<ParamT<int> > param_indices = shape->findParam<int>("indices");
      if (param_indices) {
        const size_t numIndices = param_indices->paramVec.size() / 3;
        for (int i=0;i<numIndices;i++) {
          vec3i v(param_indices->paramVec[3*i+0],
                  param_indices->paramVec[3*i+1],
                  param_indices->paramVec[3*i+2]);
          biffMesh.idx.push_back(v);
        }
      }
    }        
    return writer->push(biffMesh);
  }

  void parsePLY(const std::string &fileName,
                std::vector<vec3f> &v,
                std::vector<vec3f> &n,
                std::vector<vec3i> &idx);

#if 0
  int writePlyMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
    std::vector<vec3f> p, n;
    std::vector<vec3i> idx;
      
    std::shared_ptr<Material> mat = shape->material;
    cout << "writing shape " << shape->toString() << " w/ material " << (mat?mat->toString():"<null>") << endl;

    std::shared_ptr<ParamT<std::string> > param_fileName = shape->findParam<std::string>("filename");
    FileName fn = FileName(basePath) + param_fileName->paramVec[0];
    parsePLY(fn.str(),p,n,idx);

    int thisID = nextNodeID++;
    const affine3f xfm = instanceXfm*shape->transform;
    transformOfFirstInstance[thisID] = xfm;
      
    // -------------------------------------------------------
    fprintf(out,"<Mesh id=\"%i\">\n",thisID);
    fprintf(out,"  <materiallist>0</materiallist>\n");

    // -------------------------------------------------------
    fprintf(out,"  <vertex num=\"%li\" ofs=\"%li\"/>\n",
            p.size(),ftell(bin));
    for (int i=0;i<p.size();i++) {
      vec3f v = xfmPoint(xfm,p[i]);
      fwrite(&v,sizeof(v),1,bin);
    }

    // -------------------------------------------------------
    fprintf(out,"  <prim num=\"%li\" ofs=\"%li\"/>\n",
            idx.size(),ftell(bin));
    for (int i=0;i<idx.size();i++) {
      vec3i v = idx[i];
      fwrite(&v,sizeof(v),1,bin);
      int z = 0.f;
      fwrite(&z,sizeof(z),1,bin);
    }
    // -------------------------------------------------------
    fprintf(out,"</Mesh>\n");
    // -------------------------------------------------------
    return thisID;
  }
#endif


  biff::Instance firstTimeExportShape(const std::shared_ptr<Shape> shape,
                                      const affine3f &xfm)
  {
    if (shape->type == "trianglemesh") {
      return biff::Instance(biff::Instance::TRIANGLE_MESH,
                            writeTriangleMesh(shape,xfm),
                            xfm);
    }
    throw std::runtime_error("can't export shape " + shape->type);

      // if (shape->type == "plymesh") {
      //   int thisID = writePlyMesh(shape,instanceXfm);
      //   rootObjects.push_back(thisID);
      //   continue;
      // }
  }

  void writeObject(const std::shared_ptr<Object> &object, 
                   const affine3f &instanceXfm)
  {
    // cout << "writing " << object->toString() << endl;

    // std::vector<int> child;

    for (int shapeID=0;shapeID<object->shapes.size();shapeID++) {
      std::shared_ptr<Shape> shape = object->shapes[shapeID];

      if (exportedShape.find(shape) != exportedShape.end()) {
        biff::Instance existing = exportedShape[shape];
        
        affine3f xfm = instanceXfm * rcp(existing.xfm);
        writer->push(biff::Instance(existing.geomType,existing.geomID,xfm));
      } else {
        writer->push(firstTimeExportShape(shape,instanceXfm));
      }
    }
    for (int instID=0;instID<object->objectInstances.size();instID++) {
      writeObject(object->objectInstances[instID]->object,
                  instanceXfm*object->objectInstances[instID]->xfm);
    }      
  }

  void pbrt2biff(int ac, char **av)
  {
    std::vector<std::string> fileName;
    bool dbg = false;
    std::string outFileName = "";
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg[0] == '-') {
        if (arg == "-dbg" || arg == "--dbg")
          dbg = true;
        else if (arg == "--path" || arg == "-path")
          basePath = av[++i];
        else if (arg == "-o")
          outFileName = av[++i];
        else
          THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
      } else {
        fileName.push_back(arg);
      }          
    }
    if (outFileName == "")
      throw std::runtime_error("no output file name specified");
    biff::Writer = new biff::Writer(outFileName);

    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "parsing:";
    for (int i=0;i<fileName.size();i++)
      std::cout << " " << fileName[i];
    std::cout << std::endl;

    if (basePath.str() == "")
      basePath = FileName(fileName[0]).path();
  
    std::shared_ptr<pbrt_parser::Parser> parser = std::make_shared<pbrt_parser::Parser>(dbg,basePath);
    try {
      for (int i=0;i<fileName.size();i++)
        parser->parse(fileName[i]);
    
      std::cout << "==> parsing successful (grammar only for now)" << std::endl;
    
      std::shared_ptr<Scene> scene = parser->getScene();
      writeObject(scene->world,ospcommon::one);

      cout << "Done exporting to BIFF file" << endl;
    } catch (std::runtime_error e) {
      std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      exit(1);
    }
  }

} // ::pbrt_parser

int main(int ac, char **av)
{
  pbrt_parser::pbrt2biff(ac,av);
  return 0;
}

