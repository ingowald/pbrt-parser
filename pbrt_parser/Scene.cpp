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
// std
#include <iostream>
#include <sstream>

namespace pbrt_parser {
 
  /*! parse the given file name, return parsed scene */
  std::shared_ptr<Scene> Scene::parseFromFile(const std::string &fileName)
  {
    std::shared_ptr<Parser> parser = std::make_shared<Parser>();
    parser->parse(fileName);
    return parser->getScene();
  }
    

  std::string Object::toString(int depth) const 
  { 
    std::stringstream ss;
    ss << "Object '"<<name<<"' : "<<shapes.size()<<"shps, " << objectInstances.size() << "insts";
    if (depth > 0) {
      ss << " shapes:" << std::endl;
      for (auto &shape : shapes)
        ss << " - " << shape->type << std::endl;
      ss << " insts:" << std::endl;
      for (auto &inst : objectInstances)
        ss << " - " << inst->object->toString(depth-1) << std::endl;
    }
    return ss.str();
  }

  std::string Object::Instance::toString() const
  { 
    std::stringstream ss;
    ss << "Inst: " << object->toString() << " xfm " << (ospcommon::affine3f&)xfm.atStart; 
    return ss.str();
  }

  // ==================================================================
  // Param
  // ==================================================================
  template<> void ParamArray<float>::add(const std::string &text)
  { this->push_back(atof(text.c_str())); }

  template<> void ParamArray<int>::add(const std::string &text)
  { this->push_back(atoi(text.c_str())); }

  template<> void ParamArray<std::string>::add(const std::string &text)
  { this->push_back(text); }

  template<> void ParamArray<bool>::add(const std::string &text)
  { 
    if (text == "true")
      this->push_back(true); 
    else if (text == "false")
      this->push_back(false); 
    else
      throw std::runtime_error("invalid value '"+text+"' for bool parameter");
  }


  template<> std::string ParamArray<float>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<this->size();i++)
      ss << get(i) << " ";
    ss << "]";
    return ss.str();
  }

  std::string ParamArray<Texture>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    ss << "<TODO>";
    ss << "]";
    return ss.str();
  }

  template<> std::string ParamArray<std::string>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<this->size();i++)
      ss << get(i) << " ";
    ss << "]";
    return ss.str();
  }

  template<> std::string ParamArray<int>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<this->size();i++)
      ss << get(i) << " ";
    ss << "]";
    return ss.str();
  }

  template<> std::string ParamArray<bool>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<this->size();i++)
      ss << get(i) << " ";
    ss << "]";
    return ss.str();
  }

  template struct ParamArray<float>;
  template struct ParamArray<int>;
  template struct ParamArray<bool>;
  template struct ParamArray<std::string>;

  // ==================================================================
  // ParamSet
  // ==================================================================
  vec3f ParamSet::getParam3f(const std::string &name, const vec3f &fallBack) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return fallBack;
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
    if (!p)
      throw std::runtime_error("found param of given name, but of wrong type!");
    if (p->getSize() != 3)
      throw std::runtime_error("found param of given name and type, but wrong number of components!");
    return vec3f(p->get(0),p->get(1),p->get(2));
  }

  float ParamSet::getParam1f(const std::string &name, const float fallBack) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return fallBack;
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
    if (!p)
      throw std::runtime_error("found param of given name, but of wrong type!");
    if (p->getSize() != 1)
      throw std::runtime_error("found param of given name and type, but wrong number of components!");
    return p->get(0);
  }

  int ParamSet::getParam1i(const std::string &name, const int fallBack) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return fallBack;
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamArray<int>> p = std::dynamic_pointer_cast<ParamArray<int>>(pr);
    if (!p)
      throw std::runtime_error("found param of given name, but of wrong type!");
    if (p->getSize() != 1)
      throw std::runtime_error("found param of given name and type, but wrong number of components!");
    return p->get(0);
  }

  std::string ParamSet::getParamString(const std::string &name) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return "";
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamArray<std::string>> p = std::dynamic_pointer_cast<ParamArray<std::string>>(pr);
    if (!p)
      throw std::runtime_error("found param of given name, but of wrong type!");
    if (p->getSize() != 1)
      throw std::runtime_error("found param of given name and type, but wrong number of components!");
    return p->get(0);
  }

  std::shared_ptr<Texture> ParamSet::getParamTexture(const std::string &name) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return std::shared_ptr<Texture>();
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamArray<Texture>> p = std::dynamic_pointer_cast<ParamArray<Texture>>(pr);
    if (!p)
      throw std::runtime_error("found param of given name, but of wrong type!");
    if (p->getSize() != 1)
      throw std::runtime_error("found param of given name and type, but wrong number of components!");
    return p->texture;
  }

  bool ParamSet::getParamBool(const std::string &name, const bool fallBack) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return fallBack;
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamArray<bool>> p = std::dynamic_pointer_cast<ParamArray<bool>>(pr);
    if (!p)
      throw std::runtime_error("found param of given name, but of wrong type!");
    if (p->getSize() != 1)
      throw std::runtime_error("found param of given name and type, but wrong number of components!");
    return p->get(0);
  }

  // ==================================================================
  // Shape
  // ==================================================================

  /*! constructor */
  Shape::Shape(const std::string &type,
               std::shared_ptr<Material>   material,
               std::shared_ptr<Attributes> attributes,
               const Transform &transform) 
    : Node(type), 
      material(material),
      attributes(attributes),
      transform(transform)
  {};

  // ==================================================================
  // Material
  // ==================================================================
  std::string Material::toString() const
  {
    std::stringstream ss;
    ss << "Material type='"<< type << "' {" << std::endl;
    for (std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.begin(); 
         it != param.end(); it++) {
      ss << " - " << it->first << " : " << it->second->toString() << std::endl;
    }
    ss << "}" << std::endl;
    return ss.str();
  }

} // ::pbrt_parser

extern "C" pbrt_parser::Scene::SP pbrtParser_loadScene(const std::string &fileName)
{
  return pbrt_parser::Scene::parseFromFile(fileName);
}
  
  
