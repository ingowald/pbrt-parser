// ======================================================================== //
// Copyright 2015-2018 Ingo Wald                                            //
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

#include "biff.h"
// for mkdir
#include <sys/stat.h>
#include <sys/types.h>
// XML
#include "XML.h"
// std
#include <sstream>

namespace biff {
  using namespace ospray;

  bool skip_texture_data = false;

  /*! convert the list of parameters to an XML string that can be
      inserted into an XML node */
  std::string Params::toXML() const
  {
    std::stringstream ss;
    for (auto p : this->param_int) 
      ss << "   <i name=\"" << p.first << "\">" << p.second << "</i>" << std::endl;
    for (auto p : this->param_float) 
      ss << "   <f name=\"" << p.first << "\">" << p.second << "</f>" << std::endl;
    for (auto p : this->param_vec3f) 
      ss << "   <f3 name=\"" << p.first << "\">" << p.second.x << " " << p.second.y << " " << p.second.z << "</f3>" << std::endl;
    for (auto p : this->param_string) 
      ss << "   <s name=\"" << p.first << "\">" << p.second << "</s>" << std::endl;
    for (auto p : this->param_texture) 
      ss << "   <t name=\"" << p.first << "\">" << p.second << "</t>" << std::endl;
    return ss.str();
  }

  vec3f stovec3f(const std::string &s)
  {
    vec3f v;
    int numRead = sscanf(s.c_str(),"%f %f %f",&v.x,&v.y,&v.z);
    if (numRead != 3)
      throw std::runtime_error("could not parse vec3f from string");
    return v;
  }

  /*! given an XML node, set these parameters from the node's children */
  void parseParamsFromXML(Params &p, const ospray::xml::Node &node)
  {
    xml::for_each_child_of(node,[&](const xml::Node &child){
        const std::string  type  = child.name;
        const std::string  name  = child.getProp("name");
        const std::string &value = child.content;
        if (type == "i") 
          p.param_int[name] = std::stoll(value);
        else if (type == "t")
          p.param_texture[name] = std::stoll(value);
        else if (type == "f")
          p.param_float[name] = atof(value.c_str());
        else if (type == "f3")
          p.param_vec3f[name] = stovec3f(value);
        else if (type == "s")
          p.param_string[name] = value;
        else 
          throw std::runtime_error("un-regocnized parameter type "+type);
          
      });
  }



  Writer::Writer(const std::string &outputPath)
  {
    mkdir(outputPath.c_str(),S_IRWXU);
    triMeshFile.open(outputPath+"/tri_meshes.bin");
    instanceFile.open(outputPath+"/instances.bin");
    // textureFile.open(outputPath+"/textures.xml");
    texDataFile.open(outputPath+"/tex_data.bin");
    flatCurveFile.open(outputPath+"/flat_curves.bin");
    roundCurveFile.open(outputPath+"/round_curves.bin");
    sceneFile.open(outputPath+"/scene.xml");

    sceneFile << "<xml version=\"1.0\">" << std::endl;;
    sceneFile << " <biff version=\"0.1\">" << std::endl;
  }

  Writer::~Writer()
  {
    sceneFile << "  <scene" << std::endl
              << "   numTriMeshes=\"" << numTriMeshes << "\"" << std::endl
              << "   numFlatCurves=\"" << numFlatCurves << "\"" << std::endl
              << "   numRoundCurves=\"" << numRoundCurves << "\"" << std::endl
              << "   numTextures=\"" << numTextures << "\"" << std::endl
              << "   numInstances=\"" << numInstances << "\"" << std::endl
              << "   />" << std::endl;
    sceneFile << " </biff>" << std::endl;
    sceneFile << "</xml>" << std::endl;
    sceneFile.close();
  }

  int Writer::push(const Material &m)
  {
    sceneFile << "  <material type=\"" << m.type << "\">" << std::endl;
    sceneFile << m.toXML();
    sceneFile << "  </material>" << std::endl;
    return numMaterials++;
  }

  std::shared_ptr<Material> parseMaterial(const xml::Node &node)
  {
    std::shared_ptr<Material> mat = std::make_shared<Material>();
    mat->type = node.getProp("type");
    parseParamsFromXML(*mat,node);
    return mat;
  }

  int Writer::push(const Texture &m)
  {
    size_t size = m.rawData.size();
    size_t ofs  = texDataFile.tellp();
    texDataFile.write((char*)&m.rawData[0],size);
    
    sceneFile << "  <texture "
                << "name=\"" << m.name << "\" "
                << "texelType=\"" << m.texelType << "\" "
                << "mapType=\"" << m.mapType << "\" "
                << "ofs=\"" << ofs << "\" "
                << "size=\"" << size << "\" >" 
                << std::endl;
    sceneFile << m.toXML();
    sceneFile << "  </texture>" << std::endl;
    return numTextures++;
  }

  void readTextures(std::shared_ptr<Scene> scene, size_t numTextures)
  {
    assert(numTextures == scene->texture.size());
    if (skip_texture_data)
      std::cout << "#biff: requested to skip reading binary texture data, so skipping this ..." << std::endl;
    std::ifstream in(scene->baseName+"/tex_data.bin");
    for (auto tex : scene->textures) {
      if (tex->rawDataSize == 0) 
        continue;
      tex->rawData.resize(tex->rawDataSize);
      in.read((char*)&tex->rawData[0],tex->rawDataSize);
    }
  }

  std::shared_ptr<Texture> parseTexture(const xml::Node &node)
  {
    std::shared_ptr<Texture> tex = std::make_shared<Texture>();
    tex->name = node.getProp("name");
    tex->texelType = node.getProp("texelType");
    tex->mapType = node.getProp("mapType");
    tex->rawDataOffset = std::stoll(node.getProp("ofs"));
    tex->rawDataSize = std::stoll(node.getProp("size"));
    parseParamsFromXML(*tex,node);
    return tex;
  }







  int Writer::push(const Instance &inst)
  {
    write(instanceFile,(int)inst.geomType);
    write(instanceFile,inst.geomID);
    write(instanceFile,(const float*)&inst.xfm,12);
    return numInstances++;
  }

  int Writer::push(const TriMesh &mesh)
  {
    write(triMeshFile,(int)mesh.materialID);
    write(triMeshFile,(int)mesh.texture.color);
    write(triMeshFile,(int)mesh.texture.displacement);
    write(triMeshFile,(int)mesh.vtx.size());
    write(triMeshFile,&mesh.vtx[0],mesh.vtx.size());
    write(triMeshFile,(int)mesh.nor.size());
    write(triMeshFile,&mesh.nor[0],mesh.nor.size());
    write(triMeshFile,(int)mesh.idx.size());
    write(triMeshFile,&mesh.idx[0],mesh.idx.size());
    write(triMeshFile,(int)mesh.txt.size());
    write(triMeshFile,&mesh.txt[0],mesh.txt.size());
    return numTriMeshes++;
  }

  int Writer::push(const FlatCurve &curve)
  {
    write(flatCurveFile,(int)curve.materialID);
    write(flatCurveFile,(int)curve.degree);
    write(flatCurveFile,(int)curve.vtx.size());
    write(flatCurveFile,&curve.vtx[0],curve.vtx.size());
    write(flatCurveFile,(float)curve.rad0);
    write(flatCurveFile,(float)curve.rad1);
    return numFlatCurves++;
  }

  int Writer::push(const RoundCurve &curve)
  {
    write(roundCurveFile,(int)curve.materialID);
    write(roundCurveFile,(int)curve.degree);
    write(roundCurveFile,(int)curve.vtx.size());
    write(roundCurveFile,&curve.vtx[0],curve.vtx.size());
    write(roundCurveFile,(float)curve.rad0);
    write(roundCurveFile,(float)curve.rad1);
    return numRoundCurves++;
  }


  template<typename T>
  T read(std::ifstream &in)
  { T t; in.read((char*)&t,sizeof(T)); return t; }
  
  template<typename T>
  void read(std::ifstream &in, T &t)
  { t = read<T>(in); }
  

  template<typename T>
  void read(std::ifstream &i, T *t, size_t N)
  { i.read((char *)t,N*sizeof(T)); }

  template<typename T>
  void read(std::ifstream &i, std::vector<T> &vt)
  {
    int num = read<int>(i);
    vt.resize(num);
    read(i,&vt[0],num);
  }
 
  void readInstances(std::shared_ptr<Scene> scene, size_t numInstances)
  {
    std::ifstream in(scene->baseName+"/instances.bin");
    for (size_t i=0;i<numInstances;i++) {
      biff::Instance inst;
      inst.geomType = (Instance::GeomType)read<int>(in);
      inst.geomID   = read<int>(in);
      read(in,&inst.xfm,1);
      scene->instances.push_back(inst);
    }
  }

  void readTriMeshes(std::shared_ptr<Scene> scene, size_t num)
  {
    std::ifstream in(scene->baseName+"/tri_meshes.bin");
    for (size_t i=0;i<num;i++) {
      std::shared_ptr<TriMesh> mesh = std::make_shared<TriMesh>();
      read(in,mesh->materialID);
      read(in,mesh->texture.color);
      read(in,mesh->texture.displacement);
      read(in,mesh->vtx);
      read(in,mesh->nor);
      read(in,mesh->idx);
      read(in,mesh->txt);
      scene->triMeshes.push_back(mesh);
    }
  }

  void readRoundCurves(std::shared_ptr<Scene> scene, size_t num)
  {
    std::ifstream in(scene->baseName+"/round_curves.bin");
    for (size_t i=0;i<num;i++) {
      std::shared_ptr<RoundCurve> curve = std::make_shared<RoundCurve>();
      read(in,curve->materialID);
      read(in,curve->degree);
      read(in,curve->vtx);
      read(in,curve->rad0);
      read(in,curve->rad1);
      scene->roundCurves.push_back(curve);
    }
  }

  void readFlatCurves(std::shared_ptr<Scene> scene, size_t num)
  {
    std::ifstream in(scene->baseName+"/flat_curves.bin");
    for (size_t i=0;i<num;i++) {
      std::shared_ptr<FlatCurve> curve = std::make_shared<FlatCurve>();
      read(in,curve->materialID);
      read(in,curve->degree);
      read(in,curve->vtx);
      read(in,curve->rad0);
      read(in,curve->rad1);
      scene->flatCurves.push_back(curve);
    }
  }


  std::shared_ptr<Scene> Scene::read(const std::string &fileName)
  {
    std::shared_ptr<Scene> scene = std::make_shared<Scene>(fileName);

    std::shared_ptr<xml::XMLDoc> doc = xml::readXML(fileName+"/scene.xml");
    std::shared_ptr<xml::Node> xmlNode = doc->child[0];
    std::shared_ptr<xml::Node> biffNode = xmlNode->child[0];
    xml::for_each_child_of(*biffNode,[&](const xml::Node &child){
        if (child.name == "material")
          scene->materials.push_back(parseMaterial(child));
        else if (child.name == "texture")
          scene->textures.push_back(parseTexture(child));
        else if (child.name == "scene") {
          readTextures(scene,stoll(child.getProp("numTextures")));
          readTriMeshes(scene,stoll(child.getProp("numTriMeshes")));
          readFlatCurves(scene,stoll(child.getProp("numFlatCurves")));
          readRoundCurves(scene,stoll(child.getProp("numRoundCurves")));
          readInstances(scene,stoll(child.getProp("numInstances")));
        }
        else throw std::runtime_error("unknown xml node "+child.name);
      });
    
    return scene;
  }

  /*! return material with given material ID, if valid, or NULL
    otherwise */
  std::shared_ptr<Material> Scene::getMaterial(int matID) const
  {
    if (matID < 0) return std::shared_ptr<Material> ();
    return materials[matID];
  }

}
