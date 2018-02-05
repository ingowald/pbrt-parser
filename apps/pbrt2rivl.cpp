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
// stl
#include <iostream>
#include <vector>
#include <sstream>

namespace pbrt_parser {

  using std::cout;
  using std::endl;

  FileName basePath = "";

  // the output file we're writing.
  FILE *out = NULL;
  FILE *bin = NULL;

  size_t numUniqueTriangles = 0;
  size_t numInstancedTriangles = 0;
  size_t numUniqueObjects = 0;
  size_t numInstances = 0;

  std::map<std::shared_ptr<Shape>,int> alreadyExported;
  //! transform used when original instance was emitted
  std::map<int,affine3f> transformOfFirstInstance;
  std::map<int,int> numTrisOfInstance;

  size_t nextTransformID = 0;

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


  int nextNodeID = 0;

  template<typename T>
  std::string genMaterialParam(std::shared_ptr<Material> material, const char *name);
  
  template<>
  std::string genMaterialParam<vec3f>(std::shared_ptr<Material> material, const char *name)
  {
    std::stringstream ss;
    vec3f v = material->getParam3f(name,vec3f(0.0f));
    ss << "  <param name=\"" << name << "\" type=\"float3\">" << v.x << " " << v.y << " " << v.z << "</param>" << endl;
    return ss.str();
  }

  template<>
  std::string genMaterialParam<bool>(std::shared_ptr<Material> material, const char *name)
  {
    std::stringstream ss;
    bool v = material->getParamBool(name,false);
    ss << "  <param name=\"" << name << "\" type=\"int\">" << (int)v << "</param>" << endl;
    return ss.str();
  }

  template<>
  std::string genMaterialParam<float>(std::shared_ptr<Material> material, const char *name)
  {
    std::stringstream ss;
    float v = material->getParam1f(name,false);
    ss << "  <param name=\"" << name << "\" type=\"float\">" << (int)v << "</param>" << endl;
    return ss.str();
  }

    
  int exportMaterial(std::shared_ptr<Material> material,
                     const std::string colorTextureName,
                     const std::string bumpmapTextureName)
  {
    if (!material) 
      // default material
      return 0;

    std::tuple<std::shared_ptr<Material>,std::string,std::string> texturedMaterial
      = std::make_tuple(material,colorTextureName,bumpmapTextureName);
    static std::map<std::tuple<std::shared_ptr<Material>,std::string,std::string>,int> 
      alreadyExported;
    if (alreadyExported.find(texturedMaterial) != alreadyExported.end())
      return alreadyExported[texturedMaterial];

    const std::string type = material->type;

    if (type == "disney") {
      std::stringstream ss;
      ss << "<Material name=\"name\" type=\"DisneyMaterial\">" << endl;
      ss << genMaterialParam<vec3f>(material,"color");
      ss << genMaterialParam<float>(material,"spectrans");
      ss << genMaterialParam<float>(material,"clearcoatgloss");
      ss << genMaterialParam<float>(material,"speculartint");
      ss << genMaterialParam<float>(material,"eta");
      ss << genMaterialParam<float>(material,"sheentint");
      ss << genMaterialParam<float>(material,"metallic");
      ss << genMaterialParam<float>(material,"anisotropic");
      ss << genMaterialParam<float>(material,"clearcoat");
      ss << genMaterialParam<float>(material,"roughness");
      ss << genMaterialParam<float>(material,"sheen");
      ss << genMaterialParam<float>(material,"difftrans");
      ss << genMaterialParam<float>(material,"flatness");
      ss << genMaterialParam<bool>(material,"thin");
      if (colorTextureName != "")
        ss << "  <param name=\"color_texture\" type=\"string\">" << colorTextureName << "</param>" << endl;
      if (bumpmapTextureName != "")
        ss << "  <param name=\"bumpmap_texture\" type=\"string\">" << bumpmapTextureName << "</param>" << endl;

      ss << "</Material>" << endl;
      fprintf(out,"%s\n",ss.str().c_str());
      int thisID = nextNodeID++;
      alreadyExported[texturedMaterial] = thisID;
      return thisID;
    } else if (type == "uber") {
      std::stringstream ss;
      ss << "<Material name=\"doesntMatter\" type=\"OBJMaterial\">" << endl;
      {
        vec3f v;
        try {
          v = material->getParam3f("Kd",vec3f(0.0f));
        } catch (std::runtime_error e) {
          v = vec3f(.6f);
        };
        ss << "  <param name=\"kd\" type=\"float3\">" << v.x << " " << v.y << " " << v.z << "</param>" << endl;
      }
      ss << "</Material>" << endl;
      fprintf(out,"%s\n",ss.str().c_str());
      int thisID = nextNodeID++;
      alreadyExported[texturedMaterial] = thisID;
      return thisID;
    } else {
      std::stringstream ss;
      vec3f v = vec3f(.3f);
      ss << "<Material name=\"doesntMatter\" type=\"OBJMaterial\">" << endl;
      ss << "  <param name=\"kd\" type=\"float3\">" << v.x << " " << v.y << " " << v.z << "</param>" << endl;
      printf("WARNING: UNHANDLED MATERIAL TYPE '%s'!!!\n",type.c_str());
      ss << "</Material>" << endl;

      fprintf(out,"%s\n",ss.str().c_str());
      int thisID = nextNodeID++;
      alreadyExported[texturedMaterial] = thisID;
      return thisID;
    }
  }

  int writeTriangleMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
    numUniqueObjects++;
    std::shared_ptr<Material> mat = shape->material;
    cout << "writing shape " << shape->toString() << " w/ material " << (mat?mat->toString():"<null>") << endl;

    std::string texture_color   = shape->getParamString("color");
    std::string texture_bumpmap = shape->getParamString("bumpmap");
    int materialID = exportMaterial(shape->material,texture_color,texture_bumpmap);

    int thisID = nextNodeID++;
    const affine3f xfm = instanceXfm*shape->transform;
    // PRINT(instanceXfm);
    // PRINT(shape->transform);
    // PRINT(xfm);

    alreadyExported[shape] = thisID;
    transformOfFirstInstance[thisID] = xfm;

    fprintf(out,"<Mesh id=\"%i\">\n",thisID);
    fprintf(out,"  <materiallist>%i</materiallist>\n",materialID);
    {
      if (texture_bumpmap != "")
        fprintf(out,"  <displacement name=\"%s\"/>\n",texture_bumpmap.c_str());
    }
    { // parse "point P"
      std::shared_ptr<ParamT<float> > param_P = shape->findParam<float>("P");
      if (param_P) {
        size_t ofs = ftell(bin);
        const size_t numPoints = param_P->paramVec.size() / 3;
        for (int i=0;i<numPoints;i++) {
          vec3f v(param_P->paramVec[3*i+0],
                  param_P->paramVec[3*i+1],
                  param_P->paramVec[3*i+2]);
          v = xfmPoint(xfm,v);
          fwrite(&v,sizeof(v),1,bin);
          // fprintf(out,"v %f %f %f\n",v.x,v.y,v.z);
          // numVerticesWritten++;
        }
          
        fprintf(out,"  <vertex num=\"%li\" ofs=\"%li\"/>\n",
                numPoints,ofs);
      }
        
    }
      
    { // parse "int indices"
      std::shared_ptr<ParamT<int> > param_indices = shape->findParam<int>("indices");
      if (param_indices) {
        size_t ofs = ftell(bin);
        const size_t numIndices = param_indices->paramVec.size() / 3;
        numTrisOfInstance[thisID] = numIndices;
        numUniqueTriangles+=numIndices;
        for (int i=0;i<numIndices;i++) {
          vec4i v(param_indices->paramVec[3*i+0],
                  param_indices->paramVec[3*i+1],
                  param_indices->paramVec[3*i+2],
                  0);
          fwrite(&v,sizeof(v),1,bin);
        }
        fprintf(out,"  <prim num=\"%li\" ofs=\"%li\"/>\n",
                numIndices,ofs);
      }
    }        
    fprintf(out,"</Mesh>\n");
    return thisID;
  }

  void parsePLY(const std::string &fileName,
                std::vector<vec3f> &v,
                std::vector<vec3f> &n,
                std::vector<vec3i> &idx);

  int writePlyMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
    std::vector<vec3f> p, n;
    std::vector<vec3i> idx;
      
    numUniqueObjects++;
    std::shared_ptr<Material> mat = shape->material;
    cout << "writing shape " << shape->toString() << " w/ material " << (mat?mat->toString():"<null>") << endl;

    std::shared_ptr<ParamT<std::string> > param_fileName = shape->findParam<std::string>("filename");
    FileName fn = FileName(basePath) + param_fileName->paramVec[0];
    parsePLY(fn.str(),p,n,idx);

    int thisID = nextNodeID++;
    const affine3f xfm = instanceXfm*shape->transform;
    alreadyExported[shape] = thisID;
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
    numTrisOfInstance[thisID] = idx.size();
    numUniqueTriangles += idx.size();
    // -------------------------------------------------------
    fprintf(out,"</Mesh>\n");
    // -------------------------------------------------------
    return thisID;
  }

  void writeObject(const std::shared_ptr<Object> &object, 
                   const affine3f &instanceXfm)
  {
    // cout << "writing " << object->toString() << endl;

    // std::vector<int> child;

    for (int shapeID=0;shapeID<object->shapes.size();shapeID++) {
      std::shared_ptr<Shape> shape = object->shapes[shapeID];

      numInstances++;

      if (alreadyExported.find(shape) != alreadyExported.end()) {
          

        int childID = alreadyExported[shape];
        affine3f xfm = instanceXfm * //shape->transform * 
          rcp(transformOfFirstInstance[childID]);
        numInstancedTriangles += numTrisOfInstance[childID];

        int thisID = nextNodeID++;
        fprintf(out,"<Transform id=\"%i\" child=\"%i\">\n",
                thisID,
                childID);
        fprintf(out,"  %f %f %f\n",
                xfm.l.vx.x,
                xfm.l.vx.y,
                xfm.l.vx.z);
        fprintf(out,"  %f %f %f\n",
                xfm.l.vy.x,
                xfm.l.vy.y,
                xfm.l.vy.z);
        fprintf(out,"  %f %f %f\n",
                xfm.l.vz.x,
                xfm.l.vz.y,
                xfm.l.vz.z);
        fprintf(out,"  %f %f %f\n",
                xfm.p.x,
                xfm.p.y,
                xfm.p.z);
        fprintf(out,"</Transform>\n");
        rootObjects.push_back(thisID);
        continue;
      } 
      
      if (shape->type == "trianglemesh") {
        int thisID = writeTriangleMesh(shape,instanceXfm);
        rootObjects.push_back(thisID);
        continue;
      }

      if (shape->type == "plymesh") {
        int thisID = writePlyMesh(shape,instanceXfm);
        rootObjects.push_back(thisID);
        continue;
      }

      cout << "**** invalid shape #" << shapeID << " : " << shape->type << endl;
    }
    for (int instID=0;instID<object->objectInstances.size();instID++) {
      writeObject(object->objectInstances[instID]->object,
                  instanceXfm*object->objectInstances[instID]->xfm);
    }      
  }


  void pbrt2obj(int ac, char **av)
  {
    std::vector<std::string> fileName;
    bool dbg = false;
    std::string outFileName = "a.xml";
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
    out = fopen(outFileName.c_str(),"w");
    bin = fopen((outFileName+".bin").c_str(),"w");
    assert(out);
    assert(bin);

    fprintf(out,"<?xml version=\"1.0\"?>\n");
    fprintf(out,"<BGFscene>\n");

    int thisID = nextNodeID++;
    fprintf(out,"<Material name=\"default\" type=\"OBJMaterial\" id=\"%i\">\n",thisID);
    fprintf(out,"  <param name=\"kd\" type=\"float3\">0.7 0.7 0.7</param>\n");
    fprintf(out,"</Material>\n");
  
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

      {
        int thisID = nextNodeID++;
        fprintf(out,"<Group id=\"%i\" numChildren=\"%lu\">\n",thisID,rootObjects.size());
        for (int i=0;i<rootObjects.size();i++)
          fprintf(out,"%i ",rootObjects[i]);
        fprintf(out,"\n</Group>\n");
      }

      fprintf(out,"</BGFscene>");

      fclose(out);
      fclose(bin);
      cout << "Done exporting to OSP file" << endl;
      cout << " - unique objects/shapes    " << prettyNumber(numUniqueObjects) << endl;
      cout << " - num instances (inc.1sts) " << prettyNumber(numInstances) << endl;
      cout << " - unique triangles written " << prettyNumber(numUniqueTriangles) << endl;
      cout << " - instanced tris written   " << prettyNumber(numUniqueTriangles+numInstancedTriangles) << endl;
    } catch (std::runtime_error e) {
      std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      exit(1);
    }
  }

} // ::pbrt_parser

int main(int ac, char **av)
{
  pbrt_parser::pbrt2obj(ac,av);
  return 0;
}
