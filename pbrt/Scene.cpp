// ======================================================================== //
// Copyright 2015 Ingo Wald
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

namespace plib {
  namespace pbrt {

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


    template struct ParamT<float>;
    template struct ParamT<int>;
    template struct ParamT<bool>;
    // template struct ParamT<Color>;

  }
}
