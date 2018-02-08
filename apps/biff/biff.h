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

// ospcommon
#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
// std
#include <map>

#pragma once

namespace biff {
  using namespace ospcommon;

  /*! if set, we will, for each texture, create the texture class
      itself, but will *not* read the actual texture binary data; this
      means there's still the 'fileName' parameter to the texture
      class itself, but the Texture::rawData will be left in the
      file */
  extern bool skip_texture_data;

  struct Params {
    /*! convert the list of parameters to an XML string that can be
      inserted into an XML node */
    std::string toXML() const;

    /*! name:value mapping of all parameters that have 'string' type */
    std::map<std::string,std::string> param_string;
    /*! name:value mapping of all parameters that have 'int' type */
    std::map<std::string,int>         param_int;
    /*! name:value mapping of all parameters that have 'float' type */
    std::map<std::string,float>       param_float;
    /*! name:value mapping of all parameters that have 'vec3f' type */
    std::map<std::string,vec3f>       param_vec3f;
    /*! name:value mapping of all parameters that have 'texture' type
        (the resulting value is a texture ID that poitns into the
        Scene::textures array */
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
    /*! the offset of that particular texture's pixel data in the
        binary file - will always be set by the reader, even if
        'biff::skip_texture_data' is set durign Scene::read */
    size_t rawDataOffset;
    /*! the size of that particular texture's pixel data in the binary
        file - will always be set by the reader, even if
        'biff::skip_texture_data' is set durign Scene::read. If that
        size is 0, the texture didn't _have_ binary data, as, for
        example, in the case of a scale texture */
    size_t rawDataSize;
    /*! binary data associated with this texture (could be in EXR or
        PTEX format, depending on what the input data that this was
        converted from has been using. If biff::skip_texture_data is
        set to true, the reader will _not_ read this data even if it
        is present in the file! */
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

  struct Curve {
    typedef enum { BSPLINE } BasisType;
    int materialID;
    int basis;
    int degree;
    std::vector<vec3f> vtx;
    float rad0;
    float rad1;
  };

  struct FlatCurve : public Curve {
  };

  struct RoundCurve : public Curve {
  };

  /*! these go into instances.bin */
  struct Instance {
    typedef enum { TRI_MESH, FLAT_CURVE, ROUND_CURVE } GeomType;

    Instance() = default;
    Instance(GeomType geomType, int geomID, const affine3f &xfm)
      : geomType(geomType), geomID(geomID), xfm(xfm)
    {}

    GeomType geomType;
    int geomID; /*! id _inside_ the respective geometry */
    affine3f xfm;
  };

  /*! a scene as it is read in by Scene::read() */
  struct Scene {
    Scene(const std::string &baseName) : baseName(baseName) {}

    std::vector<std::shared_ptr<TriMesh>>    triMeshes;
    std::vector<std::shared_ptr<Material>>   materials;
    std::vector<std::shared_ptr<Texture>>    textures;
    std::vector<std::shared_ptr<FlatCurve>>  flatCurves;
    std::vector<std::shared_ptr<RoundCurve>> roundCurves;
    std::vector<Instance> instances;
    std::string baseName;

    static std::shared_ptr<Scene> read(const std::string &fileName);
  };

  struct Writer {
    Writer(const std::string &outputPath);
    virtual ~Writer();

    int push(const TriMesh &mesh);
    int push(const FlatCurve &curve);
    int push(const RoundCurve &curve);
    int push(const Instance &instance);
    int push(const Material &material);
    int push(const Texture &texture);

    int numTriMeshes    { 0 };
    int numInstances    { 0 };
    int numMaterials    { 0 };
    int numTextures     { 0 };
    int numRoundCurves  { 0 };
    int numFlatCurves   { 0 };
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
    // std::ofstream textureFile;
    std::ofstream texDataFile;
    std::ofstream sceneFile;
  };

}
