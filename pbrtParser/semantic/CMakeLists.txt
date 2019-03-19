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
  Scene.h
  Scene.cpp
  BinaryFileFormat.cpp
  importPBRT.cpp
  )

# ------------------------------------------------------------------
add_library(pbrtParser_semantic SHARED
  ${SOURCES}
  )
target_link_libraries(pbrtParser_semantic
  pbrtParser_syntactic
  )
set_target_properties(pbrtParser_semantic PROPERTIES VERSION ${pbrtParser_VERSION})

# ------------------------------------------------------------------
add_library(pbrtParser_semantic_static
  ${SOURCES}
  )
target_link_libraries(pbrtParser_semantic_static
  pbrtParser_syntactic_static
  )

# ------------------------------------------------------------------
install(TARGETS pbrtParser_semantic DESTINATION lib)
install(TARGETS pbrtParser_semantic_static DESTINATION lib)
install(FILES Scene.h DESTINATION include/pbrtParser/semantic)

