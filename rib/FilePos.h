// ======================================================================== //
// Copyright 2019 Ingo Wald                                                 //
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

#pragma once

#include "../pbrtParser/semantic/Scene.h"
#include <sstream>

namespace rib {
  using namespace pbrt;
  
  struct FilePos {
    inline std::string toString() const {
      std::stringstream ss;
      ss << "@line " << line;
      return ss.str();
    }
    
    int line { 1 };
    
    static FilePos current;
  };
  
}


