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

#pragma once

#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
#include "ospcommon/FileName.h"

namespace pbrt_parser {

  using namespace ospcommon;
  
#ifdef _WIN32
#  ifdef pbrt_parser_EXPORTS
#    define PBRT_PARSER_INTERFACE __declspec(dllexport)
#  else
#    define PBRT_PARSER_INTERFACE __declspec(dllimport)
#  endif
#else
#  define PBRT_PARSER_INTERFACE /* ignore on linux */
#endif
  
} // ::pbrt_parser

