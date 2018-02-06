/* mit license here ... */

#include "biff.h"
// for mkdir
#include <sys/stat.h>
#include <sys/types.h>

namespace biff {
  
  Writer::Writer(const std::string &outputPath)
  {
    mkdir(outputPath.c_str(),S_IRWXU);
    triMeshFile.open(outputPath+"/tri_meshes.bin");
    instanceFile.open(outputPath+"/instances.bin");
    textureFile.open(outputPath+"/textures.xml");
    texDataFile.open(outputPath+"/tex_data.bin");
    flatCurveFile.open(outputPath+"/flat_curves.bin");
    roundCurveFile.open(outputPath+"/round_curves.bin");
    materialFile.open(outputPath+"/materials.xml");

    materialFile << "<xml>" << std::endl;;
    materialFile << "<biff version=\"0.1\"/>" << std::endl;;
    // sceneFile.open(outputPath+"/scene.xml");
  }

  Writer::~Writer()
  {
    materialFile << "</biff>" << std::endl;;
    materialFile << "</xml>" << std::endl;;
  }

  int Writer::push(const Material &m)
  {
    materialFile << "  <material type=\"" << m.type << "\">" << std::endl;
    for (auto p : m.param_int) 
      materialFile << "   <i name=\"" << p.first << "\">" << p.second << "</i>" << std::endl;
    for (auto p : m.param_float) 
      materialFile << "   <f name=\"" << p.first << "\">" << p.second << "</f>" << std::endl;
    for (auto p : m.param_vec3f) 
      materialFile << "   <f3 name=\"" << p.first << "\">" << p.second.x << " " << p.second.y << " " << p.second.z << "</f3>" << std::endl;
    for (auto p : m.param_string) 
      materialFile << "   <s name=\"" << p.first << "\">" << p.second << "</s>" << std::endl;
    for (auto p : m.param_texture) 
      materialFile << "   <t name=\"" << p.first << "\">" << p.second << "</t>" << std::endl;
    materialFile << "  </material>" << std::endl;
    return numMaterials++;
  }

  int Writer::push(const Texture &m)
  {
    size_t size = m.rawData.size();
    size_t ofs  = texDataFile.tellp();
    texDataFile.write((char*)&m.rawData[0],size);
    
    textureFile << "  <texture "
                << "name=\"" << m.name << "\" "
                << "texelType=\"" << m.texelType << "\" "
                << "mapType=\"" << m.mapType << "\" "
                << "ofs=\"" << ofs << "\" "
                << "size=\"" << size << "\" >" 
                << std::endl;
    for (auto p : m.param_int) 
      textureFile << "   <i name=\"" << p.first << "\">" << p.second << "</i>" << std::endl;
    for (auto p : m.param_float) 
      textureFile << "   <f name=\"" << p.first << "\">" << p.second << "</f>" << std::endl;
    for (auto p : m.param_vec3f) 
      textureFile << "   <f3 name=\"" << p.first << "\">" << p.second.x << " " << p.second.y << " " << p.second.z << "</f3>" << std::endl;
    for (auto p : m.param_string) 
      textureFile << "   <s name=\"" << p.first << "\">" << p.second << "</s>" << std::endl;
    for (auto p : m.param_texture) 
      textureFile << "   <t name=\"" << p.first << "\">" << p.second << "</t>" << std::endl;
    textureFile << "  </texture>" << std::endl;
    return numTextures++;
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

}
