// ======================================================================== //
// Copyright 2015-2019 Ingo Wald                                            //
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

#include "pbrtParser/Scene.h"
// std
#include <iostream>
#include <sstream>
#include <fstream>
#include <stack>
#include <string.h>

#ifndef PRINT
# define PRINT(var) std::cout << #var << "=" << var << std::endl;
# define PING std::cout << __FILE__ << "::" << __LINE__ << ": " << __PRETTY_FUNCTION__ << std::endl;
#endif

namespace pbrt {

#define    PBRT_PARSER_SEMANTIC_FORMAT_ID 5

  /* 
     4: InfiniteLight::L,nsamples,scale
     3: added Shape::reverseOrientatetion
     2: added light sources to objects
     V0.6: added diffuse area lights
   */
  
  const uint32_t ourFormatTag = (PBRT_PARSER_SEMANTIC_FORMAT_ID);

  enum {
    TYPE_ERROR=0,
    TYPE_SCENE,
    TYPE_OBJECT,
    TYPE_SHAPE,
    TYPE_INSTANCE,
    TYPE_CAMERA,
    TYPE_FILM,
    TYPE_SPECTRUM,
    
    TYPE_MATERIAL=10,
    TYPE_DISNEY_MATERIAL,
    TYPE_UBER_MATERIAL,
    TYPE_MIX_MATERIAL,
    TYPE_GLASS_MATERIAL,
    TYPE_MIRROR_MATERIAL,
    TYPE_MATTE_MATERIAL,
    TYPE_SUBSTRATE_MATERIAL,
    TYPE_SUBSURFACE_MATERIAL,
    TYPE_FOURIER_MATERIAL,
    TYPE_METAL_MATERIAL,
    TYPE_PLASTIC_MATERIAL,
    TYPE_TRANSLUCENT_MATERIAL,
    
    TYPE_TEXTURE=30,
    TYPE_IMAGE_TEXTURE,
    TYPE_SCALE_TEXTURE,
    TYPE_PTEX_FILE_TEXTURE,
    TYPE_CONSTANT_TEXTURE,
    TYPE_CHECKER_TEXTURE,
    TYPE_WINDY_TEXTURE,
    TYPE_FBM_TEXTURE,
    TYPE_MARBLE_TEXTURE,
    TYPE_MIX_TEXTURE,
    TYPE_WRINKLED_TEXTURE,
    
    TYPE_TRIANGLE_MESH=50,
    TYPE_QUAD_MESH,
    TYPE_SPHERE,
    TYPE_DISK,
    TYPE_CURVE,

    TYPE_DIFFUSE_AREALIGHT_BB=60,
    TYPE_DIFFUSE_AREALIGHT_RGB,

    TYPE_INFINITE_LIGHT_SOURCE=70,
    TYPE_DISTANT_LIGHT_SOURCE,
  };
    
  /*! a simple buffer for binary data */
  struct SerializedEntity : public std::vector<uint8_t>
  {
    typedef std::shared_ptr<SerializedEntity> SP;
  };

  struct BinaryReader {

    BinaryReader(const std::string &fileName)
      : binFile(fileName, std::ios_base::binary)
    {
      int32_t formatTag;
      
      binFile.read((char*)&formatTag,sizeof(formatTag));
      if (formatTag != ourFormatTag) {
        std::cout << "Warning: pbf file uses a different format tag ("
                  << ((int*)(size_t)formatTag) << ") than what this library is expeting ("
                  << ((int *)(size_t)ourFormatTag) << ")" << std::endl;
        int ourMajor = ourFormatTag >> 16;
        int fileMajor = formatTag >> 16;
        if (ourMajor != fileMajor)
          std::cout << "**** WARNING ***** : Even the *major* file format version is different - "
                    << "this means the file _should_ be incompatible with this library. "
                    << "Please regenerate the pbf file." << std::endl;
      }
      while (1) {
        uint64_t size;
        binFile.read((char*)&size,sizeof(size));
        if (!binFile.good())
          break;
        int32_t tag;
        binFile.read((char*)&tag,sizeof(tag));
        currentEntityData.resize(size);
        binFile.read((char *)currentEntityData.data(),size);
        currentEntityOffset = 0;

        Entity::SP newEntity = createEntity(tag);
        readEntities.push_back(newEntity);
        if (newEntity) newEntity->readFrom(*this);
        currentEntityData.clear();
      }
      // exit(0);
    }

    template<typename T>
    inline void copyBytes(T *t, size_t numBytes)
    {
      if ((currentEntityOffset + numBytes) > currentEntityData.size())
        throw std::runtime_error("invalid read attempt by entity - not enough data in data block!");
      memcpy((void *)t,(void *)((char*)currentEntityData.data()+currentEntityOffset),numBytes);
      currentEntityOffset += numBytes;
    }
                                        
    template<typename T> T read();
    template<typename T> std::vector<T> readVector();
    template<typename T> void read(std::vector<T> &vt)
    {
      vt = readVector<T>();
    }
    template<typename T> void read(std::vector<std::shared_ptr<T>> &vt)
    {
      vt = readVector<std::shared_ptr<T>>(); 
    }
    template<typename T> void read(T &t) { t = read<T>(); }

    void read(std::string &t)
    {
      int32_t size;
      read(size);
      t = std::string(size,' ');
      copyBytes(t.data(),size);
    }

    void read(Spectrum &t)
    {
      ((Entity*)&t)->readFrom(*this);
    }

    template<typename T1, typename T2>
    void read(std::map<T1,T2> &result)
    {
      int32_t size = read<int32_t>();
      result.clear();
      for (int i=0;i<size;i++) {
        T1 t1; T2 t2;
        read(t1);
        read(t2);
        result[t1] = t2;
      }
    }
      
    template<typename T> inline void read(std::shared_ptr<T> &t)
    {
      int32_t ID = read<int32_t>();
      t = getEntity<T>(ID);
    }

    Entity::SP createEntity(int typeTag)
    {
      switch (typeTag) {
      case TYPE_SCENE:
        return std::make_shared<Scene>();
      case TYPE_TEXTURE:
        return std::make_shared<Texture>();
      case TYPE_IMAGE_TEXTURE:
        return std::make_shared<ImageTexture>();
      case TYPE_SCALE_TEXTURE:
        return std::make_shared<ScaleTexture>();
      case TYPE_PTEX_FILE_TEXTURE:
        return std::make_shared<PtexFileTexture>();
      case TYPE_CONSTANT_TEXTURE:
        return std::make_shared<ConstantTexture>();
      case TYPE_CHECKER_TEXTURE:
        return std::make_shared<CheckerTexture>();
      case TYPE_WINDY_TEXTURE:
        return std::make_shared<WindyTexture>();
      case TYPE_FBM_TEXTURE:
        return std::make_shared<FbmTexture>();
      case TYPE_MARBLE_TEXTURE:
        return std::make_shared<MarbleTexture>();
      case TYPE_MIX_TEXTURE:
        return std::make_shared<MixTexture>();
      case TYPE_WRINKLED_TEXTURE:
        return std::make_shared<WrinkledTexture>();
      case TYPE_MATERIAL:
        return std::make_shared<Material>();
      case TYPE_DISNEY_MATERIAL:
        return std::make_shared<DisneyMaterial>();
      case TYPE_UBER_MATERIAL:
        return std::make_shared<UberMaterial>();
      case TYPE_MIX_MATERIAL:
        return std::make_shared<MixMaterial>();
      case TYPE_TRANSLUCENT_MATERIAL:
        return std::make_shared<TranslucentMaterial>();
      case TYPE_GLASS_MATERIAL:
        return std::make_shared<GlassMaterial>();
      case TYPE_PLASTIC_MATERIAL:
        return std::make_shared<PlasticMaterial>();
      case TYPE_MIRROR_MATERIAL:
        return std::make_shared<MirrorMaterial>();
      case TYPE_SUBSTRATE_MATERIAL:
        return std::make_shared<SubstrateMaterial>();
      case TYPE_SUBSURFACE_MATERIAL:
        return std::make_shared<SubSurfaceMaterial>();
      case TYPE_MATTE_MATERIAL:
        return std::make_shared<MatteMaterial>();
      case TYPE_FOURIER_MATERIAL:
        return std::make_shared<FourierMaterial>();
      case TYPE_METAL_MATERIAL:
        return std::make_shared<MetalMaterial>();
      case TYPE_FILM:
        return std::make_shared<Film>(vec2i(0));
      case TYPE_CAMERA:
        return std::make_shared<Camera>();
      case TYPE_TRIANGLE_MESH:
        return std::make_shared<TriangleMesh>();
      case TYPE_QUAD_MESH:
        return std::make_shared<QuadMesh>();
      case TYPE_SPHERE:
        return std::make_shared<Sphere>();
      case TYPE_DISK:
        return std::make_shared<Disk>();
      case TYPE_CURVE:
        return std::make_shared<Curve>();
      case TYPE_INSTANCE:
        return std::make_shared<Instance>();
      case TYPE_OBJECT:
        return std::make_shared<Object>();
      case TYPE_DIFFUSE_AREALIGHT_BB:
        return std::make_shared<DiffuseAreaLightBB>();
      case TYPE_DIFFUSE_AREALIGHT_RGB:
        return std::make_shared<DiffuseAreaLightRGB>();
      case TYPE_INFINITE_LIGHT_SOURCE:
        return std::make_shared<InfiniteLightSource>();
      case TYPE_DISTANT_LIGHT_SOURCE:
        return std::make_shared<DistantLightSource>();
      case TYPE_SPECTRUM:
        return std::make_shared<Spectrum>();
      default:
        std::cerr << "unknown entity type tag " << typeTag << " in binary file" << std::endl;
        return Entity::SP();
      };
    }
    
    /*! return entity with given ID, dynamic-casted to given type. if
      entity wasn't recognized during parsing, a null pointer will
      be returned. if entity _was_ recognized in parser, but is not
      of this type, an exception will be thrown; if ID is -1, a null
      pointer will be returned */
    template<typename T>
    std::shared_ptr<T> getEntity(int ID)
    {
      // rule 1: ID -1 is a valid identifier for 'null object'
      if (ID == -1)
        return std::shared_ptr<T>();
      // assertion: only values with ID 0 ... N-1 are allowed
      assert(ID < (int)readEntities.size() && ID >= 0);

      // rule 2: if object _was_ a null pointer, return it (that was
      // an error during object creation)
      if (!readEntities[ID])
        return std::shared_ptr<T>();

      std::shared_ptr<T> t = std::dynamic_pointer_cast<T>(readEntities[ID]);
      
      // rule 3: if object was of different type, throw an exception
      if (!t)
        throw std::runtime_error("error in reading binary file - given entity is not of expected type!");

      // rule 4: otherwise, return object cast to that type
      return t;
    }

    std::vector<uint8_t> currentEntityData;
    size_t currentEntityOffset;
    std::vector<Entity::SP> readEntities;
    std::ifstream binFile;
  };

  template<typename T>
  T BinaryReader::read()
  {
    T t;
    copyBytes(&t,sizeof(t));
    return t;
  }

  template<>
  std::string BinaryReader::read()
  {
    std::string s;
    int32_t length = read<int32_t>();
    if (length) {
      std::vector<int8_t> cv(length);
      copyBytes(&cv[0],length);
      s = std::string(cv.begin(),cv.end());
    }
    return s;
  }

  template<typename T>
  std::vector<T> BinaryReader::readVector()
  {
    uint64_t length = read<uint64_t>();
    std::vector<T> vt(length);
    for (size_t i=0;i<length;i++) {
      read(vt[i]);
    }
    return vt;
  }
  
    

  /*! helper class that writes out a PBRT scene graph in a binary form
    that is much faster to parse */
  struct BinaryWriter {

    BinaryWriter(const std::string &fileName)
      : binFile(fileName, std::ios_base::binary)
    {
      int32_t formatTag = ourFormatTag;
      binFile.write((char*)&formatTag,sizeof(formatTag));
    }

    /*! our stack of output buffers - each object we're writing might
      depend on other objects that it references in its paramset, so
      we use a stack of such buffers - every object writes to its
      own buffer, and if it needs to write another object before
      itself that other object can simply push a new buffer on the
      stack, and first write that one before the child completes its
      own writes. */
    std::stack<SerializedEntity::SP> serializedEntity;

    /*! the file we'll be writing the buffers to */
    std::ofstream binFile;

    void writeRaw(const void *ptr, size_t size)
    {
      assert(ptr);
      serializedEntity.top()->insert(serializedEntity.top()->end(),(uint8_t*)ptr,(uint8_t*)ptr + size);
    }
    
    template<typename T>
    void write(const T &t)
    {
      writeRaw(&t,sizeof(t));
    }

    template<typename T>
    void write(std::shared_ptr<T> t)
    {
      write(serialize(t));
    }

    void write(const std::string &t)
    {
      write((int32_t)t.size());
      writeRaw(&t[0],t.size());
    }

    void write(Spectrum &t)
    {
      ((Entity*)&t)->writeTo(*this);
    }

    
    template<typename T>
    void write(const std::vector<T> &t)
    {
      const void *ptr = (const void *)t.data();
      size_t size = t.size();
      write((uint64_t)size);
      if (!t.empty())
        writeRaw(ptr,t.size()*sizeof(T));
    }

    template<typename T>
    void write(const std::vector<std::shared_ptr<T>> &t)
    {
      size_t size = t.size();
      write((uint64_t)size);
      for (size_t i=0;i<size;i++)
        write(t[i]);
    }


      
    template<typename T1, typename T2>
    void write(const std::map<T1,std::shared_ptr<T2>> &values)
    {
      int32_t size = values.size();
      write(size);
      for (auto it : values) {
        write(it.first);
        write(it.second);
        // write(serialize(it.second));
      }
    }


    void write(const std::vector<bool> &t)
    {
      std::vector<unsigned char> asChar(t.size());
      for (size_t i=0;i<t.size();i++)
        asChar[i] = t[i]?1:0;
      write(asChar);
    }

    void write(const std::vector<std::string> &t)
    {
      write(t.size());
      for (auto &s : t) {
        write(s);
      }
    }
    
    /*! start a new write buffer on the stack - all future writes will go into this buffer */
    void startNewEntity()
    { serializedEntity.push(std::make_shared<SerializedEntity>()); }
    
    /*! write the topmost write buffer to disk, and free its memory */
    void executeWrite(int32_t tag)
    {
      uint64_t size = (uint64_t)serializedEntity.top()->size();
      // std::cout << "writing block of size " << size << std::endl;
      binFile.write((const char *)&size,sizeof(size));
      binFile.write((const char *)&tag,sizeof(tag));
      binFile.write((const char *)serializedEntity.top()->data(),size);
      serializedEntity.pop();
      
    }

    std::map<Entity::SP,int32_t> emittedEntity;

    int32_t serialize(Entity::SP entity)
    {
      if (!entity) {
        // std::cout << "warning: null entity" << std::endl;
        return -1;
      }
      
      if (emittedEntity.find(entity) != emittedEntity.end())
        return emittedEntity[entity];

      startNewEntity();
      int32_t tag = (int32_t)entity->writeTo(*this);
      executeWrite(tag);
      int32_t num = (int32_t)emittedEntity.size();
      return emittedEntity[entity] = num;
    }
  };

  /*! serialize out to given binary writer */
  int Scene::writeTo(BinaryWriter &binary) 
  {
    binary.write(binary.serialize(film));
    binary.write(cameras);
    binary.write(binary.serialize(world));
    return TYPE_SCENE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Scene::readFrom(BinaryReader &binary) 
  {
    binary.read(film);
    binary.read(cameras);
    binary.read(world);
  }

  /*! serialize out to given binary writer */
  int Camera::writeTo(BinaryWriter &binary) 
  {
    binary.write(fov);
    binary.write(focalDistance);
    binary.write(lensRadius);
    binary.write(frame);
    binary.write(simplified);
    return TYPE_CAMERA;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Camera::readFrom(BinaryReader &binary) 
  {
    binary.read(fov);
    binary.read(focalDistance);
    binary.read(lensRadius);
    binary.read(frame);
    binary.read(simplified);
  }



  /*! serialize out to given binary writer */
  int Film::writeTo(BinaryWriter &binary) 
  {
    binary.write(resolution);
    binary.write(fileName);
    return TYPE_FILM;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Film::readFrom(BinaryReader &binary) 
  {
    binary.read(resolution);
    binary.read(fileName);
  }



  /*! serialize out to given binary writer */
  int Shape::writeTo(BinaryWriter &binary) 
  {
    binary.write(binary.serialize(material));
    binary.write(textures);
    binary.write(areaLight);
    binary.write((int8_t)reverseOrientation);
    return TYPE_SHAPE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Shape::readFrom(BinaryReader &binary) 
  {
    binary.read(material);
    binary.read(textures);
    binary.read(areaLight);
    reverseOrientation = binary.read<int8_t>();
  }


  // ==================================================================
  // TriangleMesh
  // ==================================================================

  /*! serialize out to given binary writer */
  int TriangleMesh::writeTo(BinaryWriter &binary) 
  {
    Shape::writeTo(binary);
    binary.write(vertex);
    binary.write(normal);
    binary.write(index);
    return TYPE_TRIANGLE_MESH;
  }
  
  /*! serialize _in_ from given binary file reader */
  void TriangleMesh::readFrom(BinaryReader &binary) 
  {
    Shape::readFrom(binary);
    binary.read(vertex);
    binary.read(normal);
    binary.read(index);
  }


  // ==================================================================
  // QuadMesh
  // ==================================================================

  /*! serialize out to given binary writer */
  int QuadMesh::writeTo(BinaryWriter &binary) 
  {
    Shape::writeTo(binary);
    binary.write(vertex);
    binary.write(normal);
    binary.write(index);
    return TYPE_QUAD_MESH;
  }
  
  /*! serialize _in_ from given binary file reader */
  void QuadMesh::readFrom(BinaryReader &binary) 
  {
    Shape::readFrom(binary);
    binary.read(vertex);
    binary.read(normal);
    binary.read(index);
  }


  // ==================================================================
  // Curve
  // ==================================================================

  /*! serialize out to given binary writer */
  int Curve::writeTo(BinaryWriter &binary) 
  {
    Shape::writeTo(binary);
    binary.write(width0);
    binary.write(width1);
    binary.write(basis);
    binary.write(type);
    binary.write(degree);
    binary.write(P);
    binary.write(transform);
    return TYPE_CURVE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Curve::readFrom(BinaryReader &binary) 
  {
    Shape::readFrom(binary);
    binary.read(width0);
    binary.read(width1);
    binary.read(basis);
    binary.read(type);
    binary.read(degree);
    binary.read(P);
    binary.read(transform);
  }


  // ==================================================================
  // Disk
  // ==================================================================

  /*! serialize out to given binary writer */
  int Disk::writeTo(BinaryWriter &binary) 
  {
    Shape::writeTo(binary);
    binary.write(radius);
    binary.write(height);
    binary.write(transform);
    return TYPE_DISK;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Disk::readFrom(BinaryReader &binary) 
  {
    Shape::readFrom(binary);
    binary.read(radius);
    binary.read(height);
    binary.read(transform);
  }




  // ==================================================================
  // Sphere
  // ==================================================================

  /*! serialize out to given binary writer */
  int Sphere::writeTo(BinaryWriter &binary) 
  {
    Shape::writeTo(binary);
    binary.write(radius);
    binary.write(transform);
    return TYPE_SPHERE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Sphere::readFrom(BinaryReader &binary) 
  {
    Shape::readFrom(binary);
    binary.read(radius);
    binary.read(transform);
  }


  // ==================================================================
  // LightSources
  // ==================================================================

  /*! serialize out to given binary writer */
  int InfiniteLightSource::writeTo(BinaryWriter &binary) 
  {
    binary.write(mapName);
    binary.write(transform);
    binary.write(L);
    binary.write(scale);
    binary.write(nSamples);
    return TYPE_INFINITE_LIGHT_SOURCE;
  }

  /*! serialize out to given binary reader */
  void InfiniteLightSource::readFrom(BinaryReader &binary) 
  {
    binary.read(mapName);
    binary.read(transform);
    binary.read(L);
    binary.read(scale);
    binary.read(nSamples);
  }
  
  /*! serialize out to given binary writer */
  int DistantLightSource::writeTo(BinaryWriter &binary) 
  {
    binary.write(from);
    binary.write(to);
    binary.write(L);
    binary.write(scale);
    return TYPE_DISTANT_LIGHT_SOURCE;
  }

  /*! serialize out to given binary reader */
  void DistantLightSource::readFrom(BinaryReader &binary) 
  {
    binary.read(from);
    binary.read(to);
    binary.read(L);
    binary.read(scale);
  }
  
  
  


  // ==================================================================
  // AreaLights
  // ==================================================================

  /*! serialize out to given binary writer */
  int AreaLight::writeTo(BinaryWriter &/*unused: binary*/) 
  {
    /* should never be stored as a final entity */ return -1;
  }
  
  /*! serialize out to given binary writer */
  int DiffuseAreaLightRGB::writeTo(BinaryWriter &binary) 
  {
    AreaLight::writeTo(binary);
    binary.write(L);
    return TYPE_DIFFUSE_AREALIGHT_RGB;
  }
  
  /*! serialize out to given binary writer */
  int DiffuseAreaLightBB::writeTo(BinaryWriter &binary) 
  {
    AreaLight::writeTo(binary);
    binary.write(temperature);
    binary.write(scale);
    return TYPE_DIFFUSE_AREALIGHT_BB;
  }
  

  /*! serialize out to given binary reader */
  void AreaLight::readFrom(BinaryReader &/*unused: binary*/) 
  {
    /* no fields */
  }
  
  /*! serialize out to given binary reader */
  void DiffuseAreaLightRGB::readFrom(BinaryReader &binary) 
  {
    AreaLight::readFrom(binary);
    binary.read(L);
  }
  
  /*! serialize out to given binary reader */
  void DiffuseAreaLightBB::readFrom(BinaryReader &binary) 
  {
    AreaLight::readFrom(binary);
    binary.read(temperature);
    binary.read(scale);
  }
  
  
  // ==================================================================
  // Spectrum
  // ==================================================================

  /*! serialize out to given binary writer */
  int Spectrum::writeTo(BinaryWriter &binary) 
  {
    binary.write(spd);
    return TYPE_SPECTRUM;
  }


  /*! serialize out to given binary reader */
  void Spectrum::readFrom(BinaryReader &binary) 
  {
    binary.read(spd);
  }

  // ==================================================================
  // Texture
  // ==================================================================
  
  /*! serialize out to given binary writer */
  int Texture::writeTo(BinaryWriter &/*unused: binary*/) 
  {
    return TYPE_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Texture::readFrom(BinaryReader &/*unused: binary*/) 
  {
  }






  /*! serialize out to given binary writer */
  int ConstantTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    binary.write(value);
    return TYPE_CONSTANT_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void ConstantTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
    binary.read(value);
  }



  /*! serialize out to given binary writer */
  int CheckerTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    binary.write(uScale);
    binary.write(vScale);
    binary.write(tex1);
    binary.write(tex2);
    return TYPE_CHECKER_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void CheckerTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
    binary.read(uScale);
    binary.read(vScale);
    binary.read(tex1);
    binary.read(tex2);
  }




  /*! serialize out to given binary writer */
  int ImageTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    binary.write(fileName);
    return TYPE_IMAGE_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void ImageTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
    binary.read(fileName);
  }




  /*! serialize out to given binary writer */
  int ScaleTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    binary.write(binary.serialize(tex1));
    binary.write(binary.serialize(tex2));
    binary.write(scale1);
    binary.write(scale2);
    return TYPE_SCALE_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void ScaleTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
    binary.read(tex1);
    binary.read(tex2);
    binary.read(scale1);
    binary.read(scale2);
  }


  /*! serialize out to given binary writer */
  int MixTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    binary.write(binary.serialize(map_amount));
    binary.write(binary.serialize(tex1));
    binary.write(binary.serialize(tex2));
    binary.write(scale1);
    binary.write(scale2);
    binary.write(amount);
    return TYPE_MIX_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void MixTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
    binary.read(map_amount);
    binary.read(tex1);
    binary.read(tex2);
    binary.read(scale1);
    binary.read(scale2);
    binary.read(amount);
  }




  /*! serialize out to given binary writer */
  int PtexFileTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    binary.write(fileName);
    return TYPE_PTEX_FILE_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void PtexFileTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
    binary.read(fileName);
  }




  /*! serialize out to given binary writer */
  int WindyTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    return TYPE_WINDY_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void WindyTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
  }




  /*! serialize out to given binary writer */
  int FbmTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    return TYPE_FBM_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void FbmTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
  }




  /*! serialize out to given binary writer */
  int MarbleTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    binary.write(scale);
    return TYPE_MARBLE_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void MarbleTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
    binary.read(scale);
  }





  /*! serialize out to given binary writer */
  int WrinkledTexture::writeTo(BinaryWriter &binary) 
  {
    Texture::writeTo(binary);
    return TYPE_WRINKLED_TEXTURE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void WrinkledTexture::readFrom(BinaryReader &binary) 
  {
    Texture::readFrom(binary);
  }






  /*! serialize out to given binary writer */
  int Material::writeTo(BinaryWriter &binary) 
  {
    binary.write(name);
    /*! \todo serialize materials (can only do once mateirals are fleshed out) */
    return TYPE_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Material::readFrom(BinaryReader &binary) 
  {
    name = binary.read<std::string>();
    /*! \todo serialize materials (can only do once mateirals are fleshed out) */
  }





  /*! serialize out to given binary writer */
  int DisneyMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(anisotropic);
    binary.write(clearCoat);
    binary.write(clearCoatGloss);
    binary.write(color);
    binary.write(diffTrans);
    binary.write(eta);
    binary.write(flatness);
    binary.write(metallic);
    binary.write(roughness);
    binary.write(sheen);
    binary.write(sheenTint);
    binary.write(specTrans);
    binary.write(specularTint);
    binary.write(thin);
    return TYPE_DISNEY_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void DisneyMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(anisotropic);
    binary.read(clearCoat);
    binary.read(clearCoatGloss);
    binary.read(color);
    binary.read(diffTrans);
    binary.read(eta);
    binary.read(flatness);
    binary.read(metallic);
    binary.read(roughness);
    binary.read(sheen);
    binary.read(sheenTint);
    binary.read(specTrans);
    binary.read(specularTint);
    binary.read(thin);
  }






  /*! serialize out to given binary writer */
  int UberMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(kd);
    binary.write(binary.serialize(map_kd));
    binary.write(ks);
    binary.write(binary.serialize(map_ks));
    binary.write(kr);
    binary.write(binary.serialize(map_kr));
    binary.write(kt);
    binary.write(binary.serialize(map_kt));
    binary.write(opacity);
    binary.write(binary.serialize(map_opacity));
    binary.write(alpha);
    binary.write(binary.serialize(map_alpha));
    binary.write(shadowAlpha);
    binary.write(binary.serialize(map_shadowAlpha));
    binary.write(index);
    binary.write(roughness);
    binary.write(binary.serialize(map_roughness));
    binary.write(binary.serialize(map_bump));
    return TYPE_UBER_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void UberMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(kd);
    binary.read(map_kd);
    binary.read(ks);
    binary.read(map_ks);
    binary.read(kr);
    binary.read(map_kr);
    binary.read(kt);
    binary.read(map_kt);
    binary.read(opacity);
    binary.read(map_opacity);
    binary.read(alpha);
    binary.read(map_alpha);
    binary.read(shadowAlpha);
    binary.read(map_shadowAlpha);
    binary.read(index);
    binary.read(roughness);
    binary.read(map_roughness);
    binary.read(map_bump);
  }




  /*! serialize out to given binary writer */
  int SubstrateMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(kd);
    binary.write(binary.serialize(map_kd));
    binary.write(ks);
    binary.write(binary.serialize(map_ks));
    binary.write(binary.serialize(map_bump));
    binary.write(uRoughness);
    binary.write(binary.serialize(map_uRoughness));
    binary.write(vRoughness);
    binary.write(binary.serialize(map_vRoughness));
    binary.write(remapRoughness);
    return TYPE_SUBSTRATE_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void SubstrateMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(kd);
    binary.read(map_kd);
    binary.read(ks);
    binary.read(map_ks);
    binary.read(map_bump);
    binary.read(uRoughness);
    binary.read(map_uRoughness);
    binary.read(vRoughness);
    binary.read(map_vRoughness);
    binary.read(remapRoughness);
  }




  /*! serialize out to given binary writer */
  int SubSurfaceMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(uRoughness);
    binary.write(vRoughness);
    binary.write(remapRoughness);
    binary.write(name);
    return TYPE_SUBSURFACE_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void SubSurfaceMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(uRoughness);
    binary.read(vRoughness);
    binary.read(remapRoughness);
    binary.read(name);
  }




  /*! serialize out to given binary writer */
  int MixMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(binary.serialize(material0));
    binary.write(binary.serialize(material1));
    binary.write(binary.serialize(map_amount));
    binary.write(amount);
    return TYPE_MIX_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void MixMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(material0);
    binary.read(material1);
    binary.read(map_amount);
    binary.read(amount);
  }





  /*! serialize out to given binary writer */
  int TranslucentMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(binary.serialize(map_kd));
    binary.write(reflect);
    binary.write(transmit);
    binary.write(kd);
    return TYPE_TRANSLUCENT_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void TranslucentMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(map_kd);
    binary.read(reflect);
    binary.read(transmit);
    binary.read(kd);
  }





  /*! serialize out to given binary writer */
  int GlassMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(kr);
    binary.write(kt);
    binary.write(index);
    return TYPE_GLASS_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void GlassMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(kr);
    binary.read(kt);
    binary.read(index);
  }



  /*! serialize out to given binary writer */
  int MatteMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(binary.serialize(map_kd));
    binary.write(kd);
    binary.write(sigma);
    binary.write(binary.serialize(map_sigma));
    binary.write(binary.serialize(map_bump));
    return TYPE_MATTE_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void MatteMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(map_kd);
    binary.read(kd);
    binary.read(sigma);
    binary.read(map_sigma);
    binary.read(map_bump);
  }




  /*! serialize out to given binary writer */
  int FourierMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(fileName);
    return TYPE_FOURIER_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void FourierMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(fileName);
  }




  /*! serialize out to given binary writer */
  int MetalMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(roughness);
    binary.write(uRoughness);
    binary.write(vRoughness);
    binary.write(remapRoughness);
    binary.write(spectrum_eta);
    binary.write(spectrum_k);
    binary.write(eta);
    binary.write(k);
    binary.write(binary.serialize(map_bump));
    binary.write(binary.serialize(map_roughness));
    binary.write(binary.serialize(map_uRoughness));
    binary.write(binary.serialize(map_vRoughness));
    return TYPE_METAL_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void MetalMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(roughness);
    binary.read(uRoughness);
    binary.read(vRoughness);
    binary.read(remapRoughness);
    binary.read(spectrum_eta);
    binary.read(spectrum_k);
    binary.read(eta);
    binary.read(k);
    binary.read(map_bump);
    binary.read(map_roughness);
    binary.read(map_uRoughness);
    binary.read(map_vRoughness);
  }




  /*! serialize out to given binary writer */
  int MirrorMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(binary.serialize(map_bump));
    binary.write(kr);
    return TYPE_MIRROR_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void MirrorMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(map_bump);
    binary.read(kr);
  }




  
  /*! serialize out to given binary writer */
  int PlasticMaterial::writeTo(BinaryWriter &binary) 
  {
    Material::writeTo(binary);
    binary.write(binary.serialize(map_kd));
    binary.write(binary.serialize(map_ks));
    binary.write(kd);
    binary.write(ks);
    binary.write(roughness);
    binary.write(remapRoughness);
    binary.write(binary.serialize(map_roughness));
    binary.write(binary.serialize(map_bump));
    return TYPE_PLASTIC_MATERIAL;
  }
  
  /*! serialize _in_ from given binary file reader */
  void PlasticMaterial::readFrom(BinaryReader &binary) 
  {
    Material::readFrom(binary);
    binary.read(map_kd);
    binary.read(map_ks);
    binary.read(kd);
    binary.read(ks);
    binary.read(roughness);
    binary.read(remapRoughness);
    binary.read(map_roughness);
    binary.read(map_bump);
  }




  // ==================================================================
  // Instance
  // ==================================================================

  /*! serialize out to given binary writer */
  int Instance::writeTo(BinaryWriter &binary) 
  {
    binary.write(xfm);
    binary.write(binary.serialize(object));
    return TYPE_INSTANCE;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Instance::readFrom(BinaryReader &binary) 
  {
    binary.read(xfm);
    binary.read(object);
  }



  
  // ==================================================================
  // Object
  // ==================================================================

  /*! serialize out to given binary writer */
  int Object::writeTo(BinaryWriter &binary) 
  {
    binary.write(name);
    binary.write((int32_t)shapes.size());
    for (auto geom : shapes) {
      binary.write(binary.serialize(geom));
    }
    binary.write((int32_t)lightSources.size());
    for (auto ls : lightSources) {
      binary.write(binary.serialize(ls));
    }
    binary.write((int32_t)instances.size());
    for (auto inst : instances) {
      binary.write(binary.serialize(inst));
    }
    return TYPE_OBJECT;
  }
  
  /*! serialize _in_ from given binary file reader */
  void Object::readFrom(BinaryReader &binary) 
  {
    name = binary.read<std::string>();

    // read shapes
    int32_t numShapes = binary.read<int32_t>();
    assert(shapes.empty());
    for (int32_t i=0;i<numShapes;i++)
      shapes.push_back(binary.getEntity<Shape>(binary.read<int32_t>()));

    // read lightSources
    int numLightSources = binary.read<int32_t>();
    assert(lightSources.empty());
    for (int i=0;i<numLightSources;i++)
      lightSources.push_back(binary.getEntity<LightSource>(binary.read<int32_t>()));

    // read instances
    int32_t numInstances = binary.read<int32_t>();
    assert(instances.empty());
    for (int32_t i=0;i<numInstances;i++)
      instances.push_back(binary.getEntity<Instance>(binary.read<int32_t>()));
  }


  


  /*! save scene to given file name, and reutrn number of bytes written */
  size_t Scene::saveTo(const std::string &outFileName)
  {
    BinaryWriter binary(outFileName);
    Entity::SP sp = shared_from_this();
    binary.serialize(sp);
    return binary.binFile.tellp();
  }

  Scene::SP Scene::loadFrom(const std::string &inFileName)
  {
    BinaryReader binary(inFileName);
    if (binary.readEntities.empty())
      throw std::runtime_error("error in Scene::load - no entities");
    Scene::SP scene = std::dynamic_pointer_cast<Scene>(binary.readEntities.back());
    assert(scene);
    return scene;
  }
  
} // ::pbrt
