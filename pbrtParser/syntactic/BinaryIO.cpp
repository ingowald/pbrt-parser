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

#include "BinaryIO.h"
// std
#include <iostream>
#include <sstream>
#include <fstream>
#include <stack>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
#define    FORMAT_MAJOR 1
#define    FORMAT_MINOR 0
  
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
        END_OF_LIST
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
  
    void BinaryWriter::writeRaw(const void *ptr, size_t size)
    {
      assert(ptr);
      outBuffer.top()->insert(outBuffer.top()->end(),(uint8_t*)ptr,(uint8_t*)ptr + size);
    }
    
    
    template<typename T>
    void BinaryWriter::write(const T &t)
    {
      writeRaw(&t,sizeof(t));
    }
    
    void BinaryWriter::write(const std::string &t)
    {
      write((int32_t)t.size());
      writeRaw(&t[0],t.size());
    }

    template<typename T>
    void BinaryWriter::writeVec(const std::vector<T> &t)
    {
      const void *ptr = (const void *)t.data();
      write(t.size());
      if (!t.empty())
        writeRaw(ptr,t.size()*sizeof(T));
    }

    void BinaryWriter::writeVec(const std::vector<bool> &t)
    {
      std::vector<unsigned char> asChar(t.size());
      for (size_t i=0;i<t.size();i++)
        asChar[i] = t[i]?1:0;
      writeVec(asChar);
    }

    void BinaryWriter::writeVec(const std::vector<std::string> &t)
    {
      write(t.size());
      for (auto &s : t) {
        write(s);
      }
    }

    /*! start a new write buffer on the stack - all future writes will go into this buffer */
    void BinaryWriter::startNewWrite()
    { outBuffer.push(std::make_shared<OutBuffer>()); }

    /*! write the topmost write buffer to disk, and free its memory */
    void BinaryWriter::executeWrite()
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
    int BinaryWriter::emitOnce(Camera::SP camera)
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
    int BinaryWriter::emitOnce(Integrator::SP integrator)
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
    int BinaryWriter::emitOnce(Shape::SP shape)
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
    int BinaryWriter::emitOnce(Material::SP material)
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
    int BinaryWriter::emitOnce(VolumeIntegrator::SP volumeIntegrator)
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
    int BinaryWriter::emitOnce(SurfaceIntegrator::SP surfaceIntegrator)
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
    int BinaryWriter::emitOnce(Sampler::SP sampler)
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
    int BinaryWriter::emitOnce(Film::SP film)
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
    int BinaryWriter::emitOnce(PixelFilter::SP pixelFilter)
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
    int BinaryWriter::emitOnce(Medium::SP medium)
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
    int BinaryWriter::emitOnce(Object::Instance::SP instance)
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
    int BinaryWriter::emitOnce(Object::SP object)
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
    

    /*! write an enitity's set of parameters */
    void BinaryWriter::writeParams(const ParamSet &ps)
    {
      write((uint16_t)ps.param.size());
      for (auto it : ps.param) {
        const std::string name = it.first;
        Param::SP param = it.second;
        
        TypeTag typeTag = typeOf(param->getType());
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

    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Texture::SP texture)
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

    /*! write the root scene itself, by recusirvely writing all its entities */
    void BinaryWriter::emit(Scene::SP scene)
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
    
    void writeBinary(pbrt::syntactic::Scene::SP scene,
                     const std::string &fileName)
    {
      BinaryWriter writer(fileName);
      writer.emit(scene);
    }





    // ==================================================================
    // BinaryReader::Block
    // ==================================================================

    template<typename T>
    inline T BinaryReader::Block::read() {
      T t;
      readRaw(&t,sizeof(t));
      return t;
    }

    inline void BinaryReader::Block::readRaw(void *ptr, size_t size)
    {
      assert((pos + size) <= data.size());
      memcpy((char*)ptr,data.data()+pos,size);
      pos += size;
    };
      
    std::string BinaryReader::Block::readString()
    {
      int32_t size = read<int32_t>();
      std::string s(size,' ');
      readRaw(&s[0],size);
      return s;
    }

    template<typename T>
    inline void BinaryReader::Block::readVec(std::vector<T> &v)
    {
      size_t size = read<size_t>();
      v.resize(size);
      if (size) {
        readRaw((char*)v.data(),size*sizeof(T));
      }
    }
    inline void BinaryReader::Block::readVec(std::vector<std::string> &v)
    {
      size_t size = read<size_t>();
      v.resize(size);
      for (size_t i=0;i<size;i++)
        v[i] = readString();
      // v.push_back(readString());
    }
    inline void BinaryReader::Block::readVec(std::vector<bool> &v)
    {
      std::vector<unsigned char> asChar;
      readVec(asChar);
      v.resize(asChar.size());
      for (size_t i=0;i<asChar.size();i++)
        v[i] = asChar[i];
    }

    template<typename T>
    inline std::vector<T> BinaryReader::Block::readVec()
    {
      std::vector<T> t;
      readVec(t);
      return t;
    }
        


    // ==================================================================
    // BinaryReader
    // ==================================================================


    
    BinaryReader::BinaryReader(const std::string &fileName)
      : binFile(fileName)
    {
      assert(binFile.good());
      binFile.sync_with_stdio(false);
    }

    int BinaryReader::readHeader()
    {
      int formatID;
      assert(binFile.good());
      binFile.read((char*)&formatID,sizeof(formatID));
      assert(binFile.good());
      if (formatID != ourFormatID)
        throw std::runtime_error("file formats/versions do not match!");
      return formatID;
    }

    inline BinaryReader::Block BinaryReader::readBlock()
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
    typename ParamArray<T>::SP BinaryReader::readParam(const std::string &type, Block &block)
    {
      typename ParamArray<T>::SP param = std::make_shared<ParamArray<T>>(type);
      block.readVec(*param);
      return param;
    }
    
    void BinaryReader::readParams(ParamSet &paramSet, Block &block)
    {
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
    
    Camera::SP BinaryReader::readCamera(Block &block)
    {
      std::string type = block.readString();
      Transform transform = block.read<Transform>();
      
      Camera::SP camera = std::make_shared<Camera>(type,transform);
      
      readParams(*camera,block);
      return camera;
    }
    
    Integrator::SP BinaryReader::readIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      Integrator::SP integrator = std::make_shared<Integrator>(type);
      
      readParams(*integrator,block);
      return integrator;
    }
    
    Shape::SP BinaryReader::readShape(Block &block)
    {
      std::string type = block.readString();
      Attributes::SP attributes = std::make_shared<Attributes>(); // not stored or read right now!?
      Transform transform = block.read<Transform>();

      Material::SP material;
      int matID = block.read<int32_t>();
      if (matID >= 0 && matID < (int)materials.size())
        material = materials[matID];
        
      Shape::SP shape = std::make_shared<Shape>(type,material,attributes,transform);
      
      readParams(*shape,block);
      return shape;
    }
    
    Material::SP BinaryReader::readMaterial(Block &block)
    {
      std::string type = block.readString();
      
      Material::SP material = std::make_shared<Material>(type);
      
      readParams(*material,block);
      return material;
    }
    
    Texture::SP BinaryReader::readTexture(Block &block)
    {
      std::string name = block.readString();
      std::string texelType = block.readString();
      std::string mapType = block.readString();
      
      Texture::SP texture = std::make_shared<Texture>(name,texelType,mapType);
      
      readParams(*texture,block);
      return texture;
    }
    
    SurfaceIntegrator::SP BinaryReader::readSurfaceIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      SurfaceIntegrator::SP surfaceIntegrator = std::make_shared<SurfaceIntegrator>(type);
      
      readParams(*surfaceIntegrator,block);
      return surfaceIntegrator;
    }
    
    PixelFilter::SP BinaryReader::readPixelFilter(Block &block)
    {
      std::string type = block.readString();
      
      PixelFilter::SP pixelFilter = std::make_shared<PixelFilter>(type);
      
      readParams(*pixelFilter,block);
      return pixelFilter;
    }
    
    VolumeIntegrator::SP BinaryReader::readVolumeIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      VolumeIntegrator::SP volumeIntegrator = std::make_shared<VolumeIntegrator>(type);
      
      readParams(*volumeIntegrator,block);
      return volumeIntegrator;
    }
    
    Sampler::SP BinaryReader::readSampler(Block &block)
    {
      std::string type = block.readString();
      Sampler::SP sampler = std::make_shared<Sampler>(type);
      
      readParams(*sampler,block);
      return sampler;
    }
    
    Film::SP BinaryReader::readFilm(Block &block)
    {
      std::string type = block.readString();
      Film::SP film = std::make_shared<Film>(type);

      readParams(*film,block);
      return film;
    }

    Object::SP BinaryReader::readObject(Block &block)
    {
      const std::string name = block.readString();
      Object::SP object = std::make_shared<Object>(name);

      std::vector<int> shapeIDs = block.readVec<int>();
      for (int shapeID : shapeIDs)
        object->shapes.push_back(shapes[shapeID]);
      std::vector<int> instanceIDs = block.readVec<int>();
      for (int instanceID : instanceIDs)
        object->objectInstances.push_back(instances[instanceID]);
      
      return object;
    }



    Object::Instance::SP BinaryReader::readInstance(Block &block)
    {
      Transform transform = block.read<Transform>();
      int objectID = block.read<int>();
      Object::SP object = objects[objectID];
      return std::make_shared<Object::Instance>(object,transform);
    }

    


    Scene::SP BinaryReader::readScene()
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








    



    /*! given an already created scene, read given binary file, and populate this scene */
    extern "C" PBRT_PARSER_INTERFACE
    void pbrt_syntactic_readBinary(pbrt::syntactic::Scene::SP &scene,
                                const std::string &fileName)
    {
      BinaryReader reader(fileName);
      scene = reader.readScene();
    }


    extern "C" PBRT_PARSER_INTERFACE
    void pbrt_syntactic_writeBinary(pbrt::syntactic::Scene::SP scene,
                                 const std::string &fileName)
    { pbrt::syntactic::writeBinary(scene,fileName); }
  
    
  } // ::pbrt::syntactic
} // ::pbrt
