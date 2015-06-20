## ======================================================================== ##
## Copyright 2009-2015 Ingo Wald                                            ##
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

PROJECT(PBRTParser)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)

  IF(${CMAKE_MAJOR_VERSION} EQUAL 3)
    CMAKE_POLICY(SET CMP0042 NEW)
  ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 3)
ENDIF(COMMAND cmake_policy)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

ADD_LIBRARY(plib_common
  embree-common/sys/filename.cpp
  embree-common/sys/sysinfo.cpp
)

ADD_LIBRARY(plib_pbrt
  pbrt/parser
)
TARGET_LINK_LIBRARIES(plib_pbrt plib_common)

ADD_EXECUTABLE(pbrtInfo pbrt/info.cpp)
TARGET_LINK_LIBRARIES(pbrtInfo plib_pbrt)

