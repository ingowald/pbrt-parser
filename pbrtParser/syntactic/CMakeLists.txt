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
  Lexer.h
  Lexer.cpp
  Parser.h
  Parser.cpp
  Scene.h
  Scene.cpp
  BinaryIO.h
  BinaryIO.cpp
  parsePLY.cpp
  )

# ------------------------------------------------------------------
add_library(pbrtParser_syntactic SHARED
  ${SOURCES}
  )
set_target_properties(pbrtParser_syntactic PROPERTIES VERSION ${pbrtParser_VERSION})

# ------------------------------------------------------------------
add_library(pbrtParser_syntactic_static
  ${SOURCES}
  )
 
# ------------------------------------------------------------------
install(TARGETS pbrtParser_syntactic DESTINATION lib)
install(TARGETS pbrtParser_syntactic_static DESTINATION lib)
install(FILES Scene.h DESTINATION include/pbrtParser/syntactic)

