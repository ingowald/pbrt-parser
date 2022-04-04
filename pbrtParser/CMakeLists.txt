## ======================================================================== ##
## Copyright 2015-2020 Ingo Wald                                            ##
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
  impl/3rdParty/rplyfile.h
  impl/3rdParty/rply.h
  impl/3rdParty/rply.c
  impl/syntactic/Buffer.h
  impl/syntactic/Buffer.inl
  impl/syntactic/FileMapping.h
  impl/syntactic/FileMapping.cpp
  impl/syntactic/Lexer.h
  impl/syntactic/Lexer.inl
  impl/syntactic/Parser.h
  impl/syntactic/Parser.inl
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
  impl/semantic/Integrator.cpp
  impl/semantic/Sampler.cpp
  impl/semantic/PixelFilter.cpp
  )

# ------------------------------------------------------------------
if (NOT WIN32)
  # iw, 1/1/2020: On windows, passing any std::string, std::vector,
  # std::shared_ptr etc through dll boundaries is just a plain bad
  # idea, so we'll build the shared library only on linux
  add_library(pbrtParser_shared SHARED
    ${SOURCES}
  )

  set_target_properties(pbrtParser_shared PROPERTIES
    VERSION ${pbrtParser_VERSION}
    CXX_VISIBILITY_PRESET hidden
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
  )

  target_compile_definitions(pbrtParser_shared PUBLIC PBRT_PARSER_DLL_INTERFACE)
  target_include_directories(pbrtParser_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )

  install(TARGETS pbrtParser_shared EXPORT pbrtParserConfig
    DESTINATION lib)
endif()

# ------------------------------------------------------------------
# iw, 1/1/2020: TODO - on windows, probably want to build separate
# debug and release versions of the library, and use some cmake magic
# to set some PBRT_PARSER_LIBARAIES etc variables that automatically
# select the right ones.
add_library(pbrtParser STATIC
  ${SOURCES}
)

#set_target_properties(pbrtParser PROPERTIES VERSION ${pbrtParser_VERSION})
#set_target_properties(pbrtParser PROPERTIES
#  VERSION ${pbrtParser_VERSION}
#  CXX_STANDARD 11
#  CXX_STANDARD_REQUIRED ON
#)

target_include_directories(pbrtParser PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )

# ------------------------------------------------------------------
install(TARGETS pbrtParser EXPORT pbrtParserConfig
  DESTINATION lib)

install(FILES include/pbrtParser/math.h include/pbrtParser/Scene.h
  DESTINATION include/pbrtParser)

install(EXPORT pbrtParserConfig
  DESTINATION lib/cmake/pbrtParser)

