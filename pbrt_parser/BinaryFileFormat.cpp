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
    // param types
    Type_float=0,
      Type_int,
      Type_bool,
      Type_string,
      Type_texture,
      Type_rgb,
      Type_spectrum,
      Type_point,
      Type_color,
      // object types
      Type_object=10,
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
    int emitOnce(std::shared_ptr<Camera> camera)
    {
      if (!camera) return -1;
      
      static std::map<std::shared_ptr<Camera>,int> alreadyEmitted;
      if (alreadyEmitted.find(camera) != alreadyEmitted.end())
        return alreadyEmitted[camera];
      
      startNewWrite(); {
        write(Type_camera);
        write(camera->type);
      } executeWrite();
      return alreadyEmitted[camera] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Integrator> integrator)
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
    int emitOnce(std::shared_ptr<VolumeIntegrator> volumeIntegrator)
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
    int emitOnce(std::shared_ptr<SurfaceIntegrator> surfaceIntegrator)
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
    int emitOnce(std::shared_ptr<Sampler> sampler)
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
      startNewWrite(); {
        write(formatID);
      } executeWrite();
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
        size_t eofIndicator = (size_t)-1;
        write(eofIndicator);
      } executeWrite();
    }
  };
  
  pbrt_parser::Scene::SP readFromBinary(const std::string &fileName)
  {
    pbrt_parser::Scene::SP scene = std::make_shared<pbrt_parser::Scene>();
    return scene;
  }
  
  void saveToBinary(pbrt_parser::Scene::SP scene, const std::string &fileName)
  {
    BinaryWriter writer(fileName);
    writer.emit(scene);
  }

}

/*! given an already created scene, read given binary file, and populate this scene */
extern "C" pbrt_parser::Scene::SP pbrtParser_readFromBinary(const std::string &fileName)
{ return pbrt_parser::readFromBinary(fileName); }

extern "C" void pbrtParser_saveToBinary(pbrt_parser::Scene::SP scene, const std::string &fileName)
{ pbrt_parser::saveToBinary(scene,fileName); }
  
