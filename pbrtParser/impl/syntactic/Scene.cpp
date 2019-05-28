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

#include "Parser.h"
// std
#include <iostream>
#include <sstream>
#include <utility>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    /*! parse the given file name, return parsed scene */
    std::shared_ptr<Scene> Scene::parse(const std::string &fileName)
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
      ss << "Inst: " << object->toString() << " xfm " << (math::affine3f&)xfm.atStart; 
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
      for (size_t i=0;i<this->size();i++)
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
      for (size_t i=0;i<this->size();i++)
        ss << get(i) << " ";
      ss << "]";
      return ss.str();
    }

    template<> std::string ParamArray<int>::toString() const
    { 
      std::stringstream ss;
      ss << getType() << " ";
      ss << "[ ";
      for (size_t i=0;i<this->size();i++)
        ss << get(i) << " ";
      ss << "]";
      return ss.str();
    }

    template<> std::string ParamArray<bool>::toString() const
    { 
      std::stringstream ss;
      ss << getType() << " ";
      ss << "[ ";
      for (size_t i=0;i<this->size();i++)
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
    bool ParamSet::getParamPairNf(pairNf::value_type *result, std::size_t* N, const std::string &name) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return 0;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() % 2 != 0)
        throw std::runtime_error("found param of given name and type, but components aren't pairs! (PairNf, name='"+name+"'");
      *N = p->getSize()/2;
      if (result != nullptr)
      {
        for (std::size_t i=0; i<p->getSize(); i+=2)
        {
          result[i/2] = std::make_pair(p->get(i), p->get(i+1));
        }
      }
      return true;
    }

    pairNf ParamSet::getParamPairNf(const std::string &name, const pairNf &fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("3f: found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() % 2 != 0)
        throw std::runtime_error("found param of given name and type, but components aren't pairs! (PairNf, name='"+name+"'");
      std::size_t N = p->getSize()/2;
      pairNf res(N);
      for (std::size_t i=0; i<p->getSize(); i+=2)
      {
        res[i/2] = std::make_pair(p->get(i), p->get(i+1));
      }
      return res;
    }

    bool ParamSet::getParam3f(float *result, const std::string &name) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return 0;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() != 3)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (3f, name='"+name+"'");
      result[0] = p->get(0);
      result[1] = p->get(1);
      result[2] = p->get(2);
      return true;
    }

    vec3f ParamSet::getParam3f(const std::string &name, const vec3f &fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("3f: found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() != 3)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (3f, name='"+name+"'");
      return vec3f(p->get(0),p->get(1),p->get(2));
    }

    bool ParamSet::getParam2f(float *result, const std::string &name) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return 0;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() != 2)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (2f, name='"+name+"'");
      result[0] = p->get(0);
      result[1] = p->get(1);
      return true;
    }

    vec2f ParamSet::getParam2f(const std::string &name, const vec2f &fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("2f: found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() != 2)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (2f, name='"+name+"'");
      return vec2f(p->get(0),p->get(1));
    }


    float ParamSet::getParam1f(const std::string &name, const float fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("1f: found param of given name, but of wrong type! (name was '"+name+"'");
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
        throw std::runtime_error("1i: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1) {
        throw std::runtime_error("found param of given name and type, but wrong number of components! (1i, name='"+name+"'");
      }
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
        throw std::runtime_error("str: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (str, name='"+name+"'");
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
        throw std::runtime_error("tex: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (tex, name='"+name+"'");
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
        throw std::runtime_error("bool: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (bool, name='"+name+"'");
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
    {}

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

  } // ::pbrt::syntactic
} // ::pbrt
