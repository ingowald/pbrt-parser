## ======================================================================== ##
## Copyright 2015-2019 Ingo Wald                                            ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

set(SOURCES
  # the 'public' header files
  include/pbrtParser/math.h
  include/pbrtParser/Scene.h

  # our actual implemenatation files
  impl/syntactic/Lexer.h
  impl/syntactic/Lexer.cpp
  impl/syntactic/Parser.h
  impl/syntactic/Parser.cpp
  impl/syntactic/Scene.h
  impl/syntactic/Scene.cpp

  # code for extracting the given entirires from syntactic to semantic
  impl/semantic/SemanticParser.h
  impl/semantic/Geometry.cpp
  impl/semantic/Camera.cpp
  impl/semantic/Textures.cpp
  impl/semantic/Materials.cpp
  impl/semantic/Lights.cpp
  impl/semantic/Scene.cpp
  impl/semantic/BinaryFileFormat.cpp
  impl/semantic/importPBRT.cpp
  )

# ------------------------------------------------------------------
add_library(pbrtParser SHARED
  ${SOURCES}
  )
set_target_properties(pbrtParser PROPERTIES VERSION ${pbrtParser_VERSION})
set_target_properties(pbrtParser PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(pbrtParser PUBLIC PBRT_PARSER_DLL_INTERFACE)

# ------------------------------------------------------------------
add_library(pbrtParser_static STATIC
  ${SOURCES}
  )
set_target_properties(pbrtParser_static PROPERTIES VERSION ${pbrtParser_VERSION})

# ------------------------------------------------------------------
install(TARGETS pbrtParser DESTINATION lib)
install(TARGETS pbrtParser_static DESTINATION lib)

install(FILES include/pbrtParser/math.h DESTINATION include/pbrtParser)
install(FILES include/pbrtParser/Scene.h DESTINATION include/pbrtParser)

