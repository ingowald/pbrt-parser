## ======================================================================== ##
## Copyright 2009-2018 Ingo Wald                                            ##
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

ADD_EXECUTABLE(pbrtinfo pbrtinfo.cpp)
TARGET_LINK_LIBRARIES(pbrtinfo pbrtParser_syntactic)

ADD_EXECUTABLE(pbrt2obj pbrt2obj.cpp)
TARGET_LINK_LIBRARIES(pbrt2obj pbrtParser_syntactic)

# '.pbrt' to '.pbsf' (pbrt binary syntactic file) converter
ADD_EXECUTABLE(pbrt2pbsf pbrt2pbsf.cpp)
TARGET_LINK_LIBRARIES(pbrt2pbsf pbrtParser_syntactic)

# '.pbrt' to '.pbf' (pbrt binary format) converter
ADD_EXECUTABLE(pbrt2pbf pbrt2pbf.cpp)
TARGET_LINK_LIBRARIES(pbrt2pbf pbrtParser_semantic)

ADD_EXECUTABLE(pbfInfo pbfInfo.cpp)
TARGET_LINK_LIBRARIES(pbfInfo pbrtParser_semantic)

install(TARGETS pbfInfo  DESTINATION bin)
install(TARGETS pbrt2pbf DESTINATION bin)
