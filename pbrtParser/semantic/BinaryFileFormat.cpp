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

#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
  
#define PBRT_PARSER_VECTYPE_NAMESPACE  ospcommon

#include "pbrtParser/semantic/Scene.h"
// std
#include <iostream>
#include <sstream>
#include <fstream>
#include <stack>
#include <string.h>

namespace pbrt {
  namespace semantic {

#define    FORMAT_MAJOR 0
#define    FORMAT_MINOR 3
  
    const uint32_t ourFormatID = (FORMAT_MAJOR << 16) + FORMAT_MINOR;

    enum {
      TYPE_ERROR,
      TYPE_SCENE,
      TYPE_OBJECT,
      TYPE_GEOMETRY,
      TYPE_INSTANCE,
      TYPE_CAMERA,
      TYPE_MATERIAL,
      TYPE_DISNEY_MATERIAL,
      TYPE_UBER_MATERIAL,
      TYPE_MIX_MATERIAL,
      TYPE_GLASS_MATERIAL,
      TYPE_MIRROR_MATERIAL,
      TYPE_MATTE_MATERIAL,
      TYPE_PLASTIC_MATERIAL,
      TYPE_TRANSLUCENT_MATERIAL,
      TYPE_TEXTURE,
      TYPE_IMAGE_TEXTURE,
      TYPE_PTEX_FILE_TEXTURE,
      TYPE_CONSTANT_TEXTURE,
      TYPE_WINDY_TEXTURE,
      TYPE_FRAME_BUFFER,
      TYPE_TRIANGLE_MESH,
    };
    
    /*! a simple buffer for binary data */
    struct SerializedEntity : public std::vector<uint8_t>
    {
      typedef std::shared_ptr<SerializedEntity> SP;
    };

    struct BinaryReader {

      BinaryReader(const std::string &fileName)
        : binFile(fileName)
      {
        int formatTag;

        binFile.sync_with_stdio(false);
      
        binFile.read((char*)&formatTag,sizeof(formatTag));

        while (1) {
          size_t size;
          binFile.read((char*)&size,sizeof(size));
          if (!binFile.good())
            break;
          int tag;
          binFile.read((char*)&tag,sizeof(tag));
          currentEntityData.resize(size);
          binFile.read((char *)currentEntityData.data(),size);
          currentEntityOffset = 0;
        
          Entity::SP newEntity = createEntity(tag);
          // std::cout << "entity #" << readEntities.size() << " is " << (newEntity?newEntity->toString():std::string("<null>")) << std::endl;
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
      template<typename T> void read(std::vector<T> &vt) { vt = readVector<T>(); }
      template<typename T> void read(T &t) { t = read<T>(); }
    
      template<typename T> inline void read(std::shared_ptr<T> &t)
      {
        int ID = read<int>();
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
        case TYPE_PTEX_FILE_TEXTURE:
          return std::make_shared<PtexFileTexture>();
        case TYPE_CONSTANT_TEXTURE:
          return std::make_shared<ConstantTexture>();
        case TYPE_WINDY_TEXTURE:
          return std::make_shared<WindyTexture>();
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
        case TYPE_MATTE_MATERIAL:
          return std::make_shared<MatteMaterial>();
        case TYPE_FRAME_BUFFER:
          return std::make_shared<FrameBuffer>(vec2i(0));
        case TYPE_CAMERA:
          return std::make_shared<Camera>();
        case TYPE_TRIANGLE_MESH:
          return std::make_shared<TriangleMesh>();
        case TYPE_INSTANCE:
          return std::make_shared<Instance>();
        case TYPE_OBJECT:
          return std::make_shared<Object>();
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
        assert(ID < readEntities.size() && ID >= 0);

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
      int length = read<int>();
      std::vector<char> cv(length);
      copyBytes(&cv[0],length);
      std::string s = std::string(cv.begin(),cv.end());
      return s;
    }

    template<typename T>
    std::vector<T> BinaryReader::readVector()
    {
      size_t length = read<size_t>();
      std::vector<T> vt(length);
      for (int i=0;i<length;i++)
        vt[i] = read<T>();
      return vt;
    }
  
    

    /*! helper class that writes out a PBRT scene graph in a binary form
      that is much faster to parse */
    struct BinaryWriter {

      BinaryWriter(const std::string &fileName)
        : binFile(fileName)
      {
        int formatTag = ourFormatID;
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

      void write(const std::string &t)
      {
        write((int32_t)t.size());
        writeRaw(&t[0],t.size());
      }
    
      template<typename T>
      void write(const std::vector<T> &t)
      {
        const void *ptr = (const void *)t.data();
        size_t size = t.size();
        write(size);
        if (!t.empty())
          writeRaw(ptr,t.size()*sizeof(T));
      }

      void write(const std::vector<bool> &t)
      {
        std::vector<unsigned char> asChar(t.size());
        for (int i=0;i<t.size();i++)
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
      void executeWrite(int tag)
      {
        size_t size = serializedEntity.top()->size();
        // std::cout << "writing block of size " << size << std::endl;
        binFile.write((const char *)&size,sizeof(size));
        binFile.write((const char *)&tag,sizeof(tag));
        binFile.write((const char *)serializedEntity.top()->data(),size);
        serializedEntity.pop();
      
      }

      std::map<Entity::SP,int> emittedEntity;

      int serialize(Entity::SP entity)
      {
        if (!entity) {
          // std::cout << "warning: null entity" << std::endl;
          return -1;
        }
      
        if (emittedEntity.find(entity) != emittedEntity.end())
          return emittedEntity[entity];

        startNewEntity();
        int tag = entity->writeTo(*this);
        executeWrite(tag);
        return (emittedEntity[entity] = emittedEntity.size());
      }
    };

    /*! serialize out to given binary writer */
    int Scene::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(frameBuffer));
      binary.write(binary.serialize(camera));
      binary.write(binary.serialize(root));
      return TYPE_SCENE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Scene::readFrom(BinaryReader &binary) 
    {
      frameBuffer = binary.getEntity<FrameBuffer>(binary.read<int>());
      camera      = binary.getEntity<Camera>(binary.read<int>());
      root        = binary.getEntity<Object>(binary.read<int>());
    }



    /*! serialize out to given binary writer */
    int Camera::writeTo(BinaryWriter &binary) 
    {
      binary.write(screen_center);
      binary.write(screen_du);
      binary.write(screen_dv);
      binary.write(lens_center);
      binary.write(lens_du);
      binary.write(lens_dv);
      return TYPE_CAMERA;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Camera::readFrom(BinaryReader &binary) 
    {
      binary.read(screen_center);
      binary.read(screen_du);
      binary.read(screen_dv);
      binary.read(lens_center);
      binary.read(lens_du);
      binary.read(lens_dv);
    }



    /*! serialize out to given binary writer */
    int FrameBuffer::writeTo(BinaryWriter &binary) 
    {
      binary.write(resolution);
      return TYPE_FRAME_BUFFER;
    }
  
    /*! serialize _in_ from given binary file reader */
    void FrameBuffer::readFrom(BinaryReader &binary) 
    {
      binary.read(resolution);
      // pixels.resize(area(resolution));
    }



    /*! serialize out to given binary writer */
    int Geometry::writeTo(BinaryWriter &binary) 
    {
      if (!material)
        binary.write((int)-1);
      else
        binary.write(binary.serialize(material));
      return TYPE_GEOMETRY;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Geometry::readFrom(BinaryReader &binary) 
    {
      int matID = binary.read<int>();
      material = binary.getEntity<Material>(matID);
    }




    /*! serialize out to given binary writer */
    int TriangleMesh::writeTo(BinaryWriter &binary) 
    {
      Geometry::writeTo(binary);
      binary.write(vertex);
      binary.write(normal);
      binary.write(index);
      return TYPE_TRIANGLE_MESH;
    }
  
    /*! serialize _in_ from given binary file reader */
    void TriangleMesh::readFrom(BinaryReader &binary) 
    {
      Geometry::readFrom(binary);
      binary.read(vertex);
      binary.read(normal);
      binary.read(index);
    }






  
    /*! serialize out to given binary writer */
    int Texture::writeTo(BinaryWriter &binary) 
    {
      return TYPE_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Texture::readFrom(BinaryReader &binary) 
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
    int Material::writeTo(BinaryWriter &binary) 
    {
      /*! \todo serialize materials (can only do once mateirals are fleshed out) */
      return TYPE_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Material::readFrom(BinaryReader &binary) 
    {
      /*! \todo serialize materials (can only do once mateirals are fleshed out) */
    }





    /*! serialize out to given binary writer */
    int DisneyMaterial::writeTo(BinaryWriter &binary) 
    {
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
      binary.write(kd);
      binary.write(ks);
      binary.write(kr);
      binary.write(alpha);
      binary.write(index);
      binary.write(roughness);
      binary.write(shadowAlpha);
      binary.write(binary.serialize(map_kd));
      binary.write(binary.serialize(map_kr));
      binary.write(binary.serialize(map_ks));
      binary.write(binary.serialize(map_alpha));
      binary.write(binary.serialize(map_shadowAlpha));
      binary.write(binary.serialize(map_bump));
      return TYPE_UBER_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void UberMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(kd);
      binary.read(ks);
      binary.read(kr);
      binary.read(alpha);
      binary.read(index);
      binary.read(roughness);
      binary.read(shadowAlpha);
      binary.read(map_kd);
      binary.read(map_kr);
      binary.read(map_ks);
      binary.read(map_alpha);
      binary.read(map_shadowAlpha);
      binary.read(map_bump);
    }




    /*! serialize out to given binary writer */
    int MixMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(material0));
      binary.write(binary.serialize(material1));
      binary.write(mix);
      return TYPE_MIX_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MixMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(material0);
      binary.read(material1);
      binary.read(mix);
    }





    /*! serialize out to given binary writer */
    int TranslucentMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(map_kd));
      binary.write(reflect);
      binary.write(transmit);
      return TYPE_TRANSLUCENT_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void TranslucentMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(map_kd);
      binary.read(reflect);
      binary.read(transmit);
    }





    /*! serialize out to given binary writer */
    int GlassMaterial::writeTo(BinaryWriter &binary) 
    {
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
      binary.write(binary.serialize(map_kd));
      binary.write(kd);
      return TYPE_MATTE_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MatteMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(map_kd);
      binary.read(kd);
    }




    /*! serialize out to given binary writer */
    int MirrorMaterial::writeTo(BinaryWriter &binary) 
    {
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
      binary.write(binary.serialize(map_kd));
      binary.write(binary.serialize(map_ks));
      binary.write(kd);
      binary.write(ks);
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
    }





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



  
    /*! serialize out to given binary writer */
    int Object::writeTo(BinaryWriter &binary) 
    {
      binary.write(name);
      binary.write((int)geometries.size());
      for (auto geom : geometries) {
        binary.write(binary.serialize(geom));
      }
      binary.write((int)instances.size());
      for (auto inst : instances) {
        binary.write(binary.serialize(inst));
      }
      return TYPE_OBJECT;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Object::readFrom(BinaryReader &binary) 
    {
      name = binary.read<std::string>();
      // read geometries
      int numGeometries = binary.read<int>();
      assert(geometries.empty());
      for (int i=0;i<numGeometries;i++)
        geometries.push_back(binary.getEntity<Geometry>(binary.read<int>()));
      // read instances
      int numInstances = binary.read<int>();
      assert(instances.empty());
      for (int i=0;i<numInstances;i++)
        instances.push_back(binary.getEntity<Instance>(binary.read<int>()));
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
  
  } // ::pbrt::scene
} // ::pbrt
