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

PROJECT(PBRTParser)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)

  IF(${CMAKE_MAJOR_VERSION} EQUAL 3)
    CMAKE_POLICY(SET CMP0042 NEW)
  ENDIF(${CMAKE_MAJOR_VERSION} EQUAL 3)
ENDIF(COMMAND cmake_policy)

SET(CMAKE_CXX_STANDARD 11)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

ADD_SUBDIRECTORY(ospcommon)

set(pbrt-parser_VERSION_MAJOR 1)
set(pbrt-parser_VERSION_MINOR 0)
set(pbrt-parser_VERSION_PATCH 0)

# syntax-only parser
ADD_SUBDIRECTORY(pbrtParser/syntax)

ADD_SUBDIRECTORY(apps)



