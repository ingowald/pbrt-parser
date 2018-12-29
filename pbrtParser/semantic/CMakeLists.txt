## ======================================================================== ##
## Copyright 2015-2018 Ingo Wald                                            ##
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

ADD_LIBRARY(pbrtParser_semantic SHARED
  Scene.cpp
  BinaryFileFormat.cpp
  importPBRT.cpp
  )
#TARGET_COMPILE_DEFINITIONS(pbrtParser_semantic PUBLIC pbrtParser_semantic_EXPORTS=1)
TARGET_LINK_LIBRARIES(pbrtParser_semantic
  pbrtParser_syntactic
  )
 

install(TARGETS pbrtParser_semantic DESTINATION lib)
install(FILES Scene.h DESTINATION include/pbrtParser/semantic)

