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

#include "Parser.h"
#include "Scene.h"
// std
#include <iostream>
#include <sstream>
#include <fstream>
#include <stack>

namespace pbrt_parser {

#define    FORMAT_MAJOR 0
#define    FORMAT_MINOR 7
  
  const uint32_t ourFormatID = (FORMAT_MAJOR << 16) + FORMAT_MINOR;

  typedef enum : uint8_t {
    Type_eof=0,
      
      // param types
      Type_float=10,
      Type_integer,
      Type_bool,
      Type_string,
      Type_texture,
      Type_rgb,
      Type_spectrum,
      Type_point,
      Type_point2,
      Type_point3,
      Type_normal,
      Type_color,
      // object types
      Type_object=100,
      Type_instance,
      Type_shape,
      Type_material,
      Type_camera,
      Type_film,
      Type_medium,
      Type_sampler,
      Type_pixelFilter,
      Type_integrator,
      Type_volumeIntegrator,
      Type_surfaceIntegrator,
      /* add more here */
      } TypeTag;

  TypeTag typeOf(const std::string &type)
  {
    if (type == "bool")     return Type_bool;
    if (type == "integer")  return Type_integer;

    if (type == "float")    return Type_float;
    if (type == "point")    return Type_point;
    if (type == "point2")   return Type_point2;
    if (type == "point3")   return Type_point3;
    if (type == "normal")   return Type_normal;

    if (type == "rgb")      return Type_rgb;
    if (type == "color")    return Type_color;
    if (type == "spectrum") return Type_spectrum;

    if (type == "string")   return Type_string;

    if (type == "texture")  return Type_texture;

    throw std::runtime_error("un-recognized type '"+type+"'");
  }

  std::string typeToString(TypeTag tag)
  {
    switch(tag) {
    default:
    case Type_bool     : return "bool";
    case Type_integer  : return "integer";
    case Type_rgb      : return "rgb";
    case Type_float    : return "float";
    case Type_point    : return "point";
    case Type_point2   : return "point2";
    case Type_point3   : return "point3";
    case Type_normal   : return "normal";
    case Type_string   : return "string";
    case Type_spectrum : return "spectrum";
    case Type_texture  : return "texture";
      throw std::runtime_error("typeToString() : type tag '"+std::to_string((int)tag)+"' not implmemented");
    }
  }
  
  /*! a simple buffer for binary data */
  struct OutBuffer : public std::vector<uint8_t>
  {
    typedef std::shared_ptr<OutBuffer> SP;
  };
  
  /*! helper class that writes out a PBRT scene graph in a binary form
    that is much faster to parse */
  struct BinaryWriter {

    BinaryWriter(const std::string &fileName) : binFile(fileName) {}

    /*! our stack of output buffers - each object we're writing might
      depend on other objects that it references in its paramset, so
      we use a stack of such buffers - every object writes to its
      own buffer, and if it needs to write another object before
      itself that other object can simply push a new buffer on the
      stack, and first write that one before the child completes its
      own writes. */
    std::stack<OutBuffer::SP> outBuffer;

    /*! the file we'll be writing the buffers to */
    std::ofstream binFile;

    void writeRaw(const void *ptr, size_t size)
    {
      assert(ptr);
      outBuffer.top()->insert(outBuffer.top()->end(),(uint8_t*)ptr,(uint8_t*)ptr + size);
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
    void writeVec(const std::vector<T> &t)
    {
      const void *ptr = (const void *)t.data();
      write(t.size());
      if (!t.empty())
        writeRaw(ptr,t.size()*sizeof(T));
    }

    void writeVec(const std::vector<bool> &t)
    {
      std::vector<unsigned char> asChar(t.size());
      for (int i=0;i<t.size();i++)
        asChar[i] = t[i]?1:0;
      writeVec(asChar);
    }

    void writeVec(const std::vector<std::string> &t)
    {
      write(t.size());
      for (auto &s : t) {
        write(s);
      }
    }
    
    /*! start a new write buffer on the stack - all future writes will go into this buffer */
    void startNewWrite()
    { outBuffer.push(std::make_shared<OutBuffer>()); }
    
    /*! write the topmost write buffer to disk, and free its memory */
    void executeWrite()
    {
      size_t size = outBuffer.top()->size();
      if (size) {
        // std::cout << "writing block of size " << size << std::endl;
        binFile.write((const char *)&size,sizeof(size));
        binFile.write((const char *)outBuffer.top()->data(),size);
      }
      outBuffer.pop();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Texture::SP texture)
    {
      if (!texture) return -1;
      
      static std::map<std::shared_ptr<Texture>,int> alreadyEmitted;
      if (alreadyEmitted.find(texture) != alreadyEmitted.end())
        return alreadyEmitted[texture];
      
      startNewWrite(); {
        write(Type_texture);
        write(texture->name);
        write(texture->texelType);
        write(texture->mapType);
        writeParams(*texture);
      } executeWrite();
      return alreadyEmitted[texture] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Camera::SP camera)
    {
      if (!camera) return -1;
      
      static std::map<std::shared_ptr<Camera>,int> alreadyEmitted;
      if (alreadyEmitted.find(camera) != alreadyEmitted.end())
        return alreadyEmitted[camera];
      
      startNewWrite(); {
        write(Type_camera);
        write(camera->type);
        write(camera->transform);
        writeParams(*camera);
      } executeWrite();
      return alreadyEmitted[camera] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Integrator::SP integrator)
    {
      if (!integrator) return -1;

      static std::map<std::shared_ptr<Integrator>,int> alreadyEmitted;
      if (alreadyEmitted.find(integrator) != alreadyEmitted.end())
        return alreadyEmitted[integrator];
      
      startNewWrite(); {
        write(Type_integrator);
        write(integrator->type);
        writeParams(*integrator);
      } executeWrite();
      return alreadyEmitted[integrator] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Shape::SP shape)
    {
      if (!shape) return -1;

      static std::map<std::shared_ptr<Shape>,int> alreadyEmitted;
      if (alreadyEmitted.find(shape) != alreadyEmitted.end())
        return alreadyEmitted[shape];
      
      startNewWrite(); {
        write(Type_shape);
        write(shape->type);
        write(shape->transform);
        write((int)emitOnce(shape->material));
        writeParams(*shape);
      } executeWrite();
      return alreadyEmitted[shape] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Material::SP material)
    {
      if (!material) return -1;

      static std::map<std::shared_ptr<Material>,int> alreadyEmitted;
      if (alreadyEmitted.find(material) != alreadyEmitted.end())
        return alreadyEmitted[material];
      
      startNewWrite(); {
        write(Type_material);
        write(material->type);
        writeParams(*material);
      } executeWrite();
      return alreadyEmitted[material] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(VolumeIntegrator::SP volumeIntegrator)
    {
      if (!volumeIntegrator) return -1;

      static std::map<std::shared_ptr<VolumeIntegrator>,int> alreadyEmitted;
      if (alreadyEmitted.find(volumeIntegrator) != alreadyEmitted.end())
        return alreadyEmitted[volumeIntegrator];
      
      startNewWrite(); {
        write(Type_volumeIntegrator);
        write(volumeIntegrator->type);
        writeParams(*volumeIntegrator);
      } executeWrite();
      return alreadyEmitted[volumeIntegrator] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(SurfaceIntegrator::SP surfaceIntegrator)
    {
      if (!surfaceIntegrator) return -1;

      static std::map<std::shared_ptr<SurfaceIntegrator>,int> alreadyEmitted;
      if (alreadyEmitted.find(surfaceIntegrator) != alreadyEmitted.end())
        return alreadyEmitted[surfaceIntegrator];
      
      startNewWrite(); {
        write(Type_surfaceIntegrator);
        write(surfaceIntegrator->type);
        writeParams(*surfaceIntegrator);
      } executeWrite();
      return alreadyEmitted[surfaceIntegrator] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Sampler::SP sampler)
    {
      if (!sampler) return -1;
      
      static std::map<std::shared_ptr<Sampler>,int> alreadyEmitted;
      if (alreadyEmitted.find(sampler) != alreadyEmitted.end())
        return alreadyEmitted[sampler];
      
      startNewWrite(); {
        write(Type_sampler);
        write(sampler->type);
        writeParams(*sampler);
      } executeWrite();
      return alreadyEmitted[sampler] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Film::SP film)
    {
      if (!film) return -1;

      static std::map<std::shared_ptr<Film>,int> alreadyEmitted;
      if (alreadyEmitted.find(film) != alreadyEmitted.end())
        return alreadyEmitted[film];
      
      startNewWrite(); {
        write(Type_film);
        write(film->type);
        writeParams(*film);
      } executeWrite();
      return alreadyEmitted[film] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(PixelFilter::SP pixelFilter)
    {
      if (!pixelFilter) return -1;
      
      static std::map<std::shared_ptr<PixelFilter>,int> alreadyEmitted;
      if (alreadyEmitted.find(pixelFilter) != alreadyEmitted.end())
        return alreadyEmitted[pixelFilter];
      
      startNewWrite(); {
        write(Type_pixelFilter);
        write(pixelFilter->type);
        writeParams(*pixelFilter);
      } executeWrite();
      return alreadyEmitted[pixelFilter] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Medium::SP medium)
    {
      if (!medium) return -1;
      
      static std::map<std::shared_ptr<Medium>,int> alreadyEmitted;
      if (alreadyEmitted.find(medium) != alreadyEmitted.end())
        return alreadyEmitted[medium];
      
      startNewWrite(); {
        write(Type_medium);
        write(medium->type);
        writeParams(*medium);
      } executeWrite();
      return alreadyEmitted[medium] = alreadyEmitted.size();
    }
    
    /*! if this instance has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Object::Instance::SP instance)
    {
      if (!instance) return -1;

      static std::map<std::shared_ptr<Object::Instance>,int> alreadyEmitted;
      if (alreadyEmitted.find(instance) != alreadyEmitted.end())
        return alreadyEmitted[instance];
      
      startNewWrite(); {
        write(Type_instance);
        write(instance->xfm);
        write((int)emitOnce(instance->object));
      } executeWrite();
      
      return alreadyEmitted[instance] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(Object::SP object)
    {
      if (!object) return -1;
      
      static std::map<std::shared_ptr<Object>,int> alreadyEmitted;
      if (alreadyEmitted.find(object) != alreadyEmitted.end())
        return alreadyEmitted[object];
      
      startNewWrite(); {
        write(Type_object);
        write(object->name);
        
        std::vector<int> shapeIDs;
        for (auto shape : object->shapes) shapeIDs.push_back(emitOnce(shape));
        writeVec(shapeIDs);

        std::vector<int> instanceIDs;
        for (auto instance : object->objectInstances) instanceIDs.push_back(emitOnce(instance));
        writeVec(instanceIDs);

      } executeWrite();
      return alreadyEmitted[object] = alreadyEmitted.size();
    }
    
    void writeParams(const ParamSet &ps)
    {
      write((uint16_t)ps.param.size());
      // std::cout << " - writing " << ps.param.size() << " params" << std::endl;
      for (auto it : ps.param) {
        const std::string name = it.first;
        Param::SP param = it.second;
        
        TypeTag typeTag = typeOf(param->getType());
        // std::cout << "   - writing " << param->getType() << " " << name << std::endl;
        write(typeTag);
        write(name);

        switch(typeTag) {
        case Type_bool: {
          writeVec(*param->as<bool>());
        } break;
        case Type_float: 
        case Type_rgb: 
        case Type_color:
        case Type_point:
        case Type_point2:
        case Type_point3:
        case Type_normal:
          writeVec(*param->as<float>());
          break;
        case Type_integer: {
          writeVec(*param->as<int>());
        } break;
        case Type_spectrum:
        case Type_string: 
          writeVec(*param->as<std::string>());
          break;
        case Type_texture: {
          int32_t texID = emitOnce(param->as<Texture>()->texture);
          write(texID);
        } break;
        default:
          throw std::runtime_error("save-to-binary of parameter type '"
                                   +param->getType()+"' not implemented yet");
        }
      }
    }
    void emit(Scene::SP scene)
    {
      uint32_t formatID = ourFormatID;
      binFile.write((const char *)&formatID,sizeof(formatID));
      
      for (auto cam : scene->cameras) 
        emitOnce(cam);
      
      emitOnce(scene->film);
      emitOnce(scene->sampler);
      emitOnce(scene->integrator);
      emitOnce(scene->volumeIntegrator);
      emitOnce(scene->surfaceIntegrator);
      emitOnce(scene->pixelFilter);
      emitOnce(scene->world);

      startNewWrite(); {
        TypeTag eof = Type_eof;
        write(eof);
      } executeWrite();
    }
  };
  
  void saveToBinary(pbrt_parser::Scene::SP scene, const std::string &fileName)
  {
    BinaryWriter writer(fileName);
    writer.emit(scene);
  }













  struct BinaryReader {

    struct Block {
      Block() = default;
      Block(Block &&) = default;
      std::vector<unsigned char> data;
      size_t pos = 0;
      
      template<typename T>
      T read() {
        T t;
        readRaw(&t,sizeof(t));
        return t;
      }

      void readRaw(void *ptr, size_t size)
      {
        assert((pos + size) <= data.size());
        memcpy((char*)ptr,data.data()+pos,size);
        pos += size;
      };
      
      std::string readString()
      {
        // write((int32_t)t.size());
        // writeRaw(&t[0],t.size());
        int32_t size = read<int32_t>();
        std::string s(size,' ');
        readRaw(&s[0],size);
        return s;
      }

      template<typename T>
      void readVec(std::vector<T> &v)
      {
        size_t size = read<size_t>();
        v.resize(size);
        if (size) {
          readRaw((char*)v.data(),size*sizeof(T));
        }
      }
      void readVec(std::vector<std::string> &v)
      {
        size_t size = read<size_t>();
        for (int i=0;i<size;i++)
          v.push_back(readString());
      }
      void readVec(std::vector<bool> &v)
      {
        std::vector<unsigned char> asChar;
        readVec(asChar);
        v.resize(asChar.size());
        for (int i=0;i<asChar.size();i++)
          v[i] = asChar[i];
      }

      template<typename T>
      std::vector<T> readVec()
      {
        std::vector<T> t;
        readVec(t);
        return t;
      }
    };
    
    BinaryReader(const std::string &fileName) : binFile(fileName)
    {
      assert(binFile.good());
    }

    /*! our stack of output buffers - each object we're writing might
      depend on other objects that it references in its paramset, so
      we use a stack of such buffers - every object writes to its
      own buffer, and if it needs to write another object before
      itself that other object can simply push a new buffer on the
      stack, and first write that one before the child completes its
      own writes. */
    std::stack<OutBuffer::SP> outBuffer;

    /*! the file we'll be writing the buffers to */
    std::ifstream binFile;

    int readHeader()
    {
      int formatID;
      assert(binFile.good());
      binFile.read((char*)&formatID,sizeof(formatID));
      assert(binFile.good());
      if (formatID != ourFormatID)
        throw std::runtime_error("file formats/versions do not match!");
      return formatID;
    }

    Block readBlock()
    {
      Block block;
      size_t size;
      assert(binFile.good());
      binFile.read((char*)&size,sizeof(size));
      assert(binFile.good());

      assert(size > 0);
      block.data.resize(size);
      assert(binFile.good());
      binFile.read((char*)block.data.data(),size);
      assert(binFile.good());

      return block;
    }
    
    template<typename T>
    typename ParamArray<T>::SP readParam(const std::string &type, Block &block)
    {
      typename ParamArray<T>::SP param = std::make_shared<ParamArray<T>>(type);
      block.readVec(*param);
      return param;
    }
    
    void readParams(ParamSet &paramSet, Block &block)
    {
      // write((uint16_t)ps.param.size());
      // for (auto it : ps.param) {
      //   const std::string name = it.first;
      //   Param::SP param = it.second;
        
      //   TypeTag typeTag = typeOf(param->getType());
      //   write(typeTag);
      //   write(name);

      //   switch(typeTag) {
      //   case Type_float: 
      //   case Type_rgb: 
      //   case Type_color:
      //   case Type_point:
      //     writeVec(*param->as<float>());
      //     break;
      //   case Type_int: {
      //     writeVec(*param->as<int>());
      //   } break;
      //   case Type_bool: {
      //     writeVec(*param->as<bool>());
      //   } break;
      //   case Type_spectrum:
      //   case Type_string: 
      //     writeVec(*param->as<std::string>());
      //     break;
      //   case Type_texture: {
      //     int32_t texID = emitOnce(param->as<Texture>()->texture);
      //     write(texID);
      //   } break;
      //   default:
      //     throw std::runtime_error("save-to-binary of parameter type '"
      //                              +param->getType()+"' not implemented yet");
      //   }
      // }
      uint16_t numParams = block.read<uint16_t>();
      for (int paramID=0;paramID<numParams;paramID++) {
        TypeTag typeTag = block.read<TypeTag>();
        const std::string name = block.readString();
        switch(typeTag) {
        case Type_bool:
          paramSet.param[name] = readParam<bool>(typeToString(typeTag),block);
          break;
        case Type_integer:
          paramSet.param[name] = readParam<int>(typeToString(typeTag),block);
          break;
        case Type_float: 
        case Type_rgb: 
        case Type_color:
        case Type_point:
        case Type_point2:
        case Type_point3:
        case Type_normal:
          paramSet.param[name] = readParam<float>(typeToString(typeTag),block);
          break;
        case Type_spectrum: 
        case Type_string: 
          paramSet.param[name] = readParam<std::string>(typeToString(typeTag),block);
          break;
        case Type_texture: {
          int texID = block.read<int32_t>();
          typename ParamArray<Texture>::SP param = std::make_shared<ParamArray<Texture>>(typeToString(typeTag));
          param->texture = textures[texID];
          paramSet.param[name] = param;
        } break;
          //   int32_t texID = emitOnce(param->as<Texture>()->texture);
          //   write(texID);
        default:
          throw std::runtime_error("unknown parameter type "+std::to_string((int)typeTag));
        }
      }
    }
    
    Camera::SP readCamera(Block &block)
    {
      std::string type = block.readString();
      Transform transform = block.read<Transform>();
      
      Camera::SP camera = std::make_shared<Camera>(type,transform);
      
      readParams(*camera,block);
      return camera;
    }
    
    Integrator::SP readIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      Integrator::SP integrator = std::make_shared<Integrator>(type);
      
      readParams(*integrator,block);
      return integrator;
    }
    
    Shape::SP readShape(Block &block)
    {
      // write(Type_shape);
      // write(shape->type);
      // write(shape->transform);
      // write((int)emitOnce(shape->material));
      // writeParams(*shape);
      std::string type = block.readString();
      Attributes::SP attributes = std::make_shared<Attributes>(); // not stored or read right now!?
      Transform transform = block.read<Transform>();

      Material::SP material;
      int matID = block.read<int32_t>();
      if (matID >= 0 && matID < materials.size())
        material = materials[matID];
        
      Shape::SP shape = std::make_shared<Shape>(type,material,attributes,transform);
      
      readParams(*shape,block);
      return shape;
    }
    
    Material::SP readMaterial(Block &block)
    {
      std::string type = block.readString();
      
      Material::SP material = std::make_shared<Material>(type);
      
      readParams(*material,block);
      return material;
    }
    
    Texture::SP readTexture(Block &block)
    {
      // write(Type_texture);
      // write(texture->name);
      // write(texture->texelType);
      // write(texture->mapType);
      // writeParams(*texture);
      std::string name = block.readString();
      std::string texelType = block.readString();
      std::string mapType = block.readString();
      
      Texture::SP texture = std::make_shared<Texture>(name,texelType,mapType);
      
      readParams(*texture,block);
      return texture;
    }
    
    SurfaceIntegrator::SP readSurfaceIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      SurfaceIntegrator::SP surfaceIntegrator = std::make_shared<SurfaceIntegrator>(type);
      
      readParams(*surfaceIntegrator,block);
      return surfaceIntegrator;
    }
    
    PixelFilter::SP readPixelFilter(Block &block)
    {
      std::string type = block.readString();
      
      PixelFilter::SP pixelFilter = std::make_shared<PixelFilter>(type);
      
      readParams(*pixelFilter,block);
      return pixelFilter;
    }
    
    VolumeIntegrator::SP readVolumeIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      VolumeIntegrator::SP volumeIntegrator = std::make_shared<VolumeIntegrator>(type);
      
      readParams(*volumeIntegrator,block);
      return volumeIntegrator;
    }
    
    Sampler::SP readSampler(Block &block)
    {
      std::string type = block.readString();
      Sampler::SP sampler = std::make_shared<Sampler>(type);
      
      readParams(*sampler,block);
      return sampler;
    }
    
    Film::SP readFilm(Block &block)
    {
      // write(Type_film);
      // write(film->type);
      // writeParams(*film);
      std::string type = block.readString();
      Film::SP film = std::make_shared<Film>(type);

      readParams(*film,block);
      return film;
    }

    Object::SP readObject(Block &block)
    {
      // write(Type_object);
      
      // std::vector<int> shapeIDs;
      // for (auto shape : object->shapes) shapeIDs.push_back(emitOnce(shape));
      // writeVec(shapeIDs);
      
      // std::vector<int> instanceIDs;
      // for (auto instance : object->objectInstances) instanceIDs.push_back(emitOnce(instance));
      // writeVec(instanceIDs);

      const std::string name = block.readString();
      Object::SP object = std::make_shared<Object>(name);

      std::vector<int> shapeIDs = block.readVec<int>();
      for (int shapeID : shapeIDs)
        object->shapes.push_back(shapes[shapeID]);
      std::vector<int> instanceIDs = block.readVec<int>();
      for (int instanceID : instanceIDs)
        object->objectInstances.push_back(instances[instanceID]);
      
      // readParams(*object,block);
      return object;
    }



    Object::Instance::SP readInstance(Block &block)
    {
      // write(Type_instance);
      // write(instance->xfm);
      // write((int)emitOnce(instance->object));

      Transform transform = block.read<Transform>();
      int objectID = block.read<int>();
      Object::SP object = objects[objectID];
      return std::make_shared<Object::Instance>(object,transform);
    }

    


    
    std::vector<Object::SP> objects;
    std::vector<Material::SP> materials;
    std::vector<Shape::SP> shapes;
    std::vector<Object::Instance::SP> instances;
    std::vector<Texture::SP> textures;
    
    
    Scene::SP readScene()
    {
      Scene::SP scene = std::make_shared<Scene>();
      readHeader();

      while (1) {
        BinaryReader::Block block = readBlock();
        TypeTag typeTag = block.read<TypeTag>();

        
        switch(typeTag) {
        case Type_camera:
          scene->cameras.push_back(readCamera(block));
          break;
        case Type_pixelFilter:
          scene->pixelFilter = readPixelFilter(block);
          break;
        case Type_material:
          materials.push_back(readMaterial(block));
          break;
        case Type_shape:
          shapes.push_back(readShape(block));
          break;
        case Type_object:
          objects.push_back(readObject(block));
          break;
        case Type_instance:
          instances.push_back(readInstance(block));
          break;
        case Type_texture:
          textures.push_back(readTexture(block));
          break;
        case Type_integrator:
          scene->integrator = readIntegrator(block);
          break;
        case Type_surfaceIntegrator:
          scene->surfaceIntegrator = readSurfaceIntegrator(block);
          break;
        case Type_volumeIntegrator:
          scene->volumeIntegrator = readVolumeIntegrator(block);
          break;
        case Type_sampler:
          scene->sampler = readSampler(block);
          break;
        case Type_film:
          scene->film = readFilm(block);
          break;
        case Type_eof: {
          std::cout << "Done parsing!" << std::endl;
          assert(!objects.empty());
          scene->world = objects.back();
        } return scene;
        default:
          throw std::runtime_error("unsupported type tag '"+std::to_string((int)typeTag)+"'");
        };
      }      
    }
  };
  
  /*! given an already created scene, read given binary file, and populate this scene */
  extern "C" PBRT_PARSER_INTERFACE void pbrtParser_readFromBinary(pbrt_parser::Scene::SP &scene, const std::string &fileName)
  {
	  BinaryReader reader(fileName);
	  scene = reader.readScene();
  }

}

extern "C" PBRT_PARSER_INTERFACE void pbrtParser_saveToBinary(pbrt_parser::Scene::SP scene, const std::string &fileName)
{ pbrt_parser::saveToBinary(scene,fileName); }
  
