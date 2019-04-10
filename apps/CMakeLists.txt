## ======================================================================== ##
## Copyright 2009-2019 Ingo Wald                                            ##
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

ADD_EXECUTABLE(pbrt2obj pbrt2obj.cpp)
TARGET_LINK_LIBRARIES(pbrt2obj pbrtParser)

# '.pbrt' to '.pbf' (pbrt binary format) converter
ADD_EXECUTABLE(pbrt2pbf pbrt2pbf.cpp)
TARGET_LINK_LIBRARIES(pbrt2pbf pbrtParser)

# simple example of loading a pbrt or pbf file, finding all triangle
# meshes, and dumping all triangles in obj format. Note this is a
# intentionally simplisitic example that lacks support for many things
# (including, among other, vertex normals, texture coordinates,
# materials, textures, etc) - it ONLY dumps triangles, nothing
# else. Also be warned: since OBJ doesn't support instancing this can
# produce preeeeetty big output files for heavily instanced scenes
# (ie, do not try this on landscape, ecosys, or moana ...)
ADD_EXECUTABLE(pbrtInfo pbrtInfo.cpp)
TARGET_LINK_LIBRARIES(pbrtInfo pbrtParser)

install(TARGETS pbrtInfo DESTINATION bin)
install(TARGETS pbrt2pbf DESTINATION bin)
