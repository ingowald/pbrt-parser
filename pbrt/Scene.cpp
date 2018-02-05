// ======================================================================== //
// Copyright 2015-2017 Ingo Wald                                            //
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

namespace pbrt_parser {

  using std::endl;

  std::string Object::toString() const 
  { 
    std::stringstream ss;
    ss << "Object '"<<name<<"' : "<<shapes.size()<<"shps, " << objectInstances.size() << "insts";
    return ss.str();
  }

  std::string Object::Instance::toString() const
  { 
    std::stringstream ss;
    ss << "Inst: " << object->toString() << " xfm " << xfm; 
    return ss.str();
  }

  // ==================================================================
  // Param
  // ==================================================================
  template<> void ParamT<float>::add(const std::string &text)
  { paramVec.push_back(atof(text.c_str())); }

  template<> void ParamT<int>::add(const std::string &text)
  { paramVec.push_back(atoi(text.c_str())); }

  template<> void ParamT<std::string>::add(const std::string &text)
  { paramVec.push_back(text); }

  template<> void ParamT<bool>::add(const std::string &text)
  { 
    if (text == "true")
      paramVec.push_back(true); 
    else if (text == "false")
      paramVec.push_back(false); 
    else
      throw std::runtime_error("invalid value '"+text+"' for bool parameter");
  }


  template<> std::string ParamT<float>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<paramVec.size();i++)
      ss << paramVec[i] << " ";
    ss << "]";
    return ss.str();
  }

  template<> std::string ParamT<std::string>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<paramVec.size();i++)
      ss << paramVec[i] << " ";
    ss << "]";
    return ss.str();
  }

  template<> std::string ParamT<int>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<paramVec.size();i++)
      ss << paramVec[i] << " ";
    ss << "]";
    return ss.str();
  }

  template<> std::string ParamT<bool>::toString() const
  { 
    std::stringstream ss;
    ss << getType() << " ";
    ss << "[ ";
    for (int i=0;i<paramVec.size();i++)
      ss << paramVec[i] << " ";
    ss << "]";
    return ss.str();
  }

  template struct ParamT<float>;
  template struct ParamT<int>;
  template struct ParamT<bool>;
  template struct ParamT<std::string>;

  // ==================================================================
  // Parameterized
  // ==================================================================
  vec3f Parameterized::getParam3f(const std::string &name, const vec3f &fallBack) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return fallBack;
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamT<float>> p = std::dynamic_pointer_cast<ParamT<float>>(pr);
    if (!p)
      throw std::runtime_error("Parameterized::getParam3f: found param of given name, but of wrong type!");
    if (p->getSize() != 3)
      throw std::runtime_error("Parameterized::getParam3f: found param of given name and type, but wrong number of components!");
    return vec3f(p->paramVec[0],p->paramVec[1],p->paramVec[2]);
  }

  float Parameterized::getParam1f(const std::string &name, const float fallBack) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return fallBack;
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamT<float>> p = std::dynamic_pointer_cast<ParamT<float>>(pr);
    if (!p)
      throw std::runtime_error("Parameterized::getParam1f: found param of given name, but of wrong type!");
    if (p->getSize() != 1)
      throw std::runtime_error("Parameterized::getParam1f: found param of given name and type, but wrong number of components!");
    return p->paramVec[0];
  }

  bool Parameterized::getParamBool(const std::string &name, const bool fallBack) const
  {
    std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
    if (it == param.end())
      return fallBack;
    std::shared_ptr<Param> pr = it->second;
    const std::shared_ptr<ParamT<bool>> p = std::dynamic_pointer_cast<ParamT<bool>>(pr);
    if (!p)
      throw std::runtime_error("Parameterized::getParam1f: found param of given name, but of wrong type!");
    if (p->getSize() != 1)
      throw std::runtime_error("Parameterized::getParam1f: found param of given name and type, but wrong number of components!");
    return p->paramVec[0];
  }

  // ==================================================================
  // Shape
  // ==================================================================

  /*! constructor */
  Shape::Shape(const std::string &type,
               std::shared_ptr<Material>   material,
               std::shared_ptr<Attributes> attributes,
               affine3f &transform) 
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
    ss << "Material type='"<< type << "' {" << endl;
    for (std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.begin(); 
         it != param.end(); it++) {
      ss << " - " << it->first << " : " << it->second->toString() << endl;
    }
    ss << "}" << endl;
    return ss.str();
  }

} // ::pbrt_parser
