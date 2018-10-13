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

  typedef enum : uint8_t {
    Type_eof=0,
      
    // param types
    Type_float=10,
      Type_int,
      Type_bool,
      Type_string,
      Type_texture,
      Type_rgb,
      Type_spectrum,
      Type_point,
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
    if (type == "rgb") return Type_rgb;
    if (type == "float") return Type_float;
    if (type == "point") return Type_point;
    if (type == "color") return Type_color;

    if (type == "integer") return Type_int;
    if (type == "string") return Type_string;
    if (type == "spectrum") return Type_spectrum;

    if (type == "texture") return Type_texture;

    throw std::runtime_error("un-recognized type '"+type+"'");
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

    /*! start a new write buffer on the stack - all future writes will go into this buffer */
    void startNewWrite()
    { outBuffer.push(std::make_shared<OutBuffer>()); }
    
    /*! write the topmost write buffer to disk, and free its memory */
    void executeWrite()
    {
      size_t size = outBuffer.top()->size();
      if (size) {
        binFile.write((const char *)&size,sizeof(size));
        binFile.write((const char *)outBuffer.top()->data(),size);
      }
      outBuffer.pop();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Texture> texture)
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
        write(camera->transforms);
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
      } executeWrite();
      return alreadyEmitted[integrator] = alreadyEmitted.size();
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
      } executeWrite();
      return alreadyEmitted[sampler] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Film> film)
    {
      if (!film) return -1;
      
      static std::map<std::shared_ptr<Film>,int> alreadyEmitted;
      if (alreadyEmitted.find(film) != alreadyEmitted.end())
        return alreadyEmitted[film];
      
      startNewWrite(); {
        write(Type_film);
        write(film->type);
      } executeWrite();
      return alreadyEmitted[film] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<PixelFilter> pixelFilter)
    {
      if (!pixelFilter) return -1;
      
      static std::map<std::shared_ptr<PixelFilter>,int> alreadyEmitted;
      if (alreadyEmitted.find(pixelFilter) != alreadyEmitted.end())
        return alreadyEmitted[pixelFilter];
      
      startNewWrite(); {
        write(Type_pixelFilter);
        write(pixelFilter->type);
      } executeWrite();
      return alreadyEmitted[pixelFilter] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Medium> medium)
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
    
    /*! if this material has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Material> material)
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
    
    /*! if this instance has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Object::Instance> instance)
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
    
    /*! if this shape has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Shape> shape)
    {
      if (!shape) return -1;

      static std::map<std::shared_ptr<Shape>,int> alreadyEmitted;
      if (alreadyEmitted.find(shape) != alreadyEmitted.end())
        return alreadyEmitted[shape];
      
      startNewWrite(); {
        write(Type_shape);
        write(shape->type);
        write((int)emitOnce(shape->material));
        write(shape->transform);
        writeParams(*shape);
      } executeWrite();
      return alreadyEmitted[shape] = alreadyEmitted.size();
    }

    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Object> object)
    {
      if (!object) return -1;
      
      static std::map<std::shared_ptr<Object>,int> alreadyEmitted;
      if (alreadyEmitted.find(object) != alreadyEmitted.end())
        return alreadyEmitted[object];
      
      startNewWrite(); {
        write(Type_object);
        
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
      for (auto it : ps.param) {
        const std::string name = it.first;
        Param::SP param = it.second;
        
        TypeTag typeTag = typeOf(param->getType());
        write(typeTag);
        write(name);

        switch(typeTag) {
        case Type_float: 
        case Type_rgb: 
        case Type_color:
        case Type_point:
          writeVec(*param->as<float>());
          break;
        case Type_int: {
          writeVec(*param->as<int>());
        } break;
        case Type_bool: {
          writeVec(*param->as<bool>());
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
#define    FORMAT_MAJOR 0
#define    FORMAT_MINOR 1
    
      uint32_t formatID = (FORMAT_MAJOR << 16) + FORMAT_MINOR;
      binFile.write((const char *)&formatID,sizeof(formatID));
      
      for (auto cam : scene->cameras) {
        startNewWrite(); {
          write(emitOnce(cam));
        }
      }
      
      startNewWrite(); {
        write(emitOnce(scene->film));
      } executeWrite();

      startNewWrite(); {
        write(emitOnce(scene->sampler));
      } executeWrite();

      startNewWrite(); {
        write(emitOnce(scene->integrator));
      } executeWrite();

      startNewWrite(); {
        write(emitOnce(scene->volumeIntegrator));
      } executeWrite();

      startNewWrite(); {
        write(emitOnce(scene->surfaceIntegrator));
      } executeWrite();

      startNewWrite(); {
        write(emitOnce(scene->pixelFilter));
      } executeWrite();

      startNewWrite(); {
        emitOnce(scene->world);
      } executeWrite();

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
        PRINT(pos);
        PRINT(size);
        PRINT(data.size());
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
      PING;
      int formatID;
      assert(binFile.good());
      binFile.read((char*)&formatID,sizeof(formatID));
      assert(binFile.good());
      PING;
      PRINT((int*)(size_t)formatID);
      return formatID;
    }

    Block readBlock()
    {
      Block block;
      size_t size;
      assert(binFile.good());
      binFile.read((char*)&size,sizeof(size));
      assert(binFile.good());
      PRINT(size);

      assert(size > 0);
      block.data.resize(size);
      assert(binFile.good());
      binFile.read((char*)block.data.data(),size);
      assert(binFile.good());

      return block;
    }

    void readParams(ParamSet &paramSet, Block &block)
    {
      uint16_t numParams = block.read<uint16_t>();
      PRINT(numParams);
    }
    
    Camera::SP readCamera(Block &block)
    {
      // write(Type_camera);
      // write(camera->type);
      // write(camera->transforms);

      std::string type = block.readString();
      Transforms transforms = block.read<Transforms>();
      
      Camera::SP camera = std::make_shared<Camera>(type,transforms);
      
      readParams(*camera,block);
      return camera;
    }
    
    Scene::SP readScene()
    {
      Scene::SP scene = std::make_shared<Scene>();
      PING;
      readHeader();
      PING;

      std::vector<Object::SP> objects;
    
      while (1) {
        BinaryReader::Block block = readBlock();
        TypeTag typeTag = block.read<TypeTag>();
        PRINT((int)typeTag);

        switch(typeTag) {
        case Type_camera:
          scene->cameras.push_back(readCamera(block));
          break;
        case Type_eof:
          std::cout << "Done parsing!" << std::endl;
          assert(!objects.empty());
          scene->world = objects.back();
          break;
        default:
          throw std::runtime_error("unsupported type tag '"+std::to_string((int)typeTag)+"'");
        };
      }
      return scene;
    }
  };
    
  
  pbrt_parser::Scene::SP readFromBinary(const std::string &fileName)
  {
    BinaryReader reader(fileName);
    return reader.readScene();    
  }
  
}

/*! given an already created scene, read given binary file, and populate this scene */
extern "C" pbrt_parser::Scene::SP pbrtParser_readFromBinary(const std::string &fileName)
{ return pbrt_parser::readFromBinary(fileName); }

extern "C" void pbrtParser_saveToBinary(pbrt_parser::Scene::SP scene, const std::string &fileName)
{ pbrt_parser::saveToBinary(scene,fileName); }
  
