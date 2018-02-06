/* mit license here ... */

// ospcommon
#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
// std
#include <map>

#pragma once

namespace biff {
  using namespace ospcommon;

  struct Params {
    std::map<std::string,std::string> param_string;
    std::map<std::string,int>         param_int;
    std::map<std::string,float>       param_float;
    std::map<std::string,vec3f>       param_vec3f;
    std::map<std::string,int>         param_texture;
  };

  /*! those go into materials.xml */
  struct Material : public Params {
    std::string type;
  };

  struct Texture : public Params {
    std::string name;
    std::string texelType;
    std::string mapType;

    std::vector<uint8_t> rawData;
  };


  /*! those go into triangle_meshes.bin */
  struct TriMesh {
    struct {
      int color { -1 };
      int displacement { -1 };
    } texture;
    int materialID;
    std::vector<vec3i> idx;
    std::vector<vec3f> vtx;
    std::vector<vec3f> nor;
    std::vector<vec2f> txt;
  };

  struct FlatCurve {
    std::vector<vec3f> vtx;
    float rad0;
    float rad1;
  };

  struct RoundCurve {
    void serialize(FILE *file);
    void deserialize(FILE *file);

    std::vector<vec3f> vtx;
    float rad0;
    float rad1;
  };

  /*! these go into instances.bin */
  struct Instance {
    typedef enum { TRIANGLE_MESH, FLAT_CURVE, ROUND_CURVE } GeomType;

    Instance() = default;
    Instance(GeomType geomType, int geomID, const affine3f &xfm)
      : geomType(geomType), geomID(geomID), xfm(xfm)
    {}

    GeomType geomType;
    int geomID; /*! id _inside_ the respective geometry */
    affine3f xfm;
  };
  
  /*! this one goes to scene.xml */
  struct Scene {
    std::vector<size_t> triMeshOffsets;    
    std::vector<size_t> textureOffsets;
    std::vector<size_t> roundCurveOffsets;
    std::vector<size_t> flatCurveOffsets;
    std::vector<size_t> instanceOffsets;
    std::vector<std::shared_ptr<Material>> materials;
  };
  
  struct Writer {
    Writer(const std::string &outputPath);
    ~Writer();

    int push(const TriMesh &mesh);
    int push(const FlatCurve &curve);
    int push(const Instance &instance);
    int push(const Material &material);
    int push(const Texture &texture);

    int numTriMeshes { 0 };
    int numInstances { 0 };
    int numMaterials { 0 };
    int numTextures  { 0 };
  private:
    template<typename T>
    void write(std::ofstream &o, const T &t)
    { o.write((const char *)&t,sizeof(t)); }
    template<typename T>
    void write(std::ofstream &o, const T *t, size_t N)
    { o.write((const char *)t,N*sizeof(T)); }

    std::ofstream triMeshFile;
    std::ofstream flatCurveFile;
    std::ofstream roundCurveFile;
    std::ofstream instanceFile;
    std::ofstream materialFile;
    std::ofstream textureFile;
    std::ofstream texDataFile;
    // std::ofstream sceneFile;
  };

}
