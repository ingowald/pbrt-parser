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

namespace pbrt_parser {


  extern "C" void pbrtParser_saveToBinary(const std::string &fileName)
  { pbrt_parser::saveToBinary(fileName); }
  
extern "C" void pbrtParser_readFromToBinary(const std::string &fileName);
