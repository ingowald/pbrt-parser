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
      // object types
      Type_object=10,
      Type_instance,
      Type_shape,
      Type_material,
      /* add more here */
      } TypeTag;

  TypeTag typeOf(const std::string &s)
  {
    throw std::runtime_error("un-recognized type '"+s+"'");
  }
  
  struct OutBuffer : public std::vector<uint8_t>
  {
    typedef std::shared_ptr<OutBuffer> SP;
  };
  
  struct BinaryWriter {

    BinaryWriter(const std::string &fileName) : binFile(fileName) {}
    
    std::stack<OutBuffer::SP> outBuffer;
    std::ofstream binFile;

    void writeRaw(const void *ptr, size_t size)
    {
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
      writeRaw(ptr,t.size()*sizeof(T));
    }

    void writeVec(const std::vector<bool> &t)
    {
      std::vector<unsigned char> asChar(t.size());
      for (int i=0;i<t.size();i++)
        asChar[i] = t[i]?1:0;
      writeVec(asChar);
    }
    
    void startNewWrite()
    { outBuffer.push(std::make_shared<OutBuffer>()); }
    
    void executeWrite()
    {
      binFile.write((const char *)outBuffer.top()->data(),outBuffer.top()->size());
      outBuffer.pop();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int emitOnce(std::shared_ptr<Texture> texture)
    {
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
        case Type_float: {
          writeVec(*param->as<float>());
        } break;
        case Type_int: {
          writeVec(*param->as<int>());
        } break;
        case Type_bool: {
          writeVec(*param->as<bool>());
        } break;
        case Type_string: {
          writeVec(*param->as<std::string>());
        } break;
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
  };
  
  pbrt_parser::Scene::SP readFromBinary(const std::string &fileName)
  {
    pbrt_parser::Scene::SP scene = std::make_shared<pbrt_parser::Scene>();
    return scene;
  }
  
  void saveToBinary(pbrt_parser::Scene::SP scene, const std::string &fileName)
  {
    BinaryWriter writer(fileName);
    writer.emitOnce(scene->world);
  }

}

/*! given an already created scene, read given binary file, and populate this scene */
extern "C" pbrt_parser::Scene::SP pbrtParser_readFromBinary(const std::string &fileName)
{ return pbrt_parser::readFromBinary(fileName); }

extern "C" void pbrtParser_saveToBinary(pbrt_parser::Scene::SP scene, const std::string &fileName)
{ pbrt_parser::saveToBinary(scene,fileName); }
  
