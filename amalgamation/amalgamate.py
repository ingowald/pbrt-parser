#! /usr/bin/env python3

# ======================================================================== #
# Copyright 2015-2019 Ingo Wald & Fabio Pellacini                          #
#                                                                          #
# Licensed under the Apache License, Version 2.0 (the "License");          #
# you may not use this file except in compliance with the License.         #
# You may obtain a copy of the License at                                  #
#                                                                          #
#     http:#www.apache.org/licenses/LICENSE-2.0                            #
#                                                                          #
# Unless required by applicable law or agreed to in writing, software      #
# distributed under the License is distributed on an "AS IS" BASIS,        #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
# See the License for the specific language governing permissions and      #
# limitations under the License.                                           #
# ======================================================================== #

#
# Amalgamates the current files into a single header and C++ file.
# This is hack and should not be used externally.
#

header_files = [
    'pbrtParser/common/math.h',
    'pbrtParser/syntactic/Scene.h',
    'pbrtParser/semantic/Scene.h',
]

source_files = [
    'pbrtParser/syntactic/Lexer.h',
    'pbrtParser/syntactic/Parser.h',
    'pbrtParser/syntactic/BinaryIO.h',
    'pbrtParser/syntactic/Lexer.cpp',
    'pbrtParser/syntactic/Parser.cpp',
    'pbrtParser/syntactic/Scene.cpp',
    'pbrtParser/semantic/Scene.cpp',
    'pbrtParser/semantic/importPBRT.cpp',
    'pbrtParser/semantic/BinaryFileFormat.cpp',
    'pbrtParser/syntactic/BinaryIO.cpp',
    '3rdParty/happly.h',
    'pbrtParser/syntactic/parsePLY.cpp',
]

with open('amalgamation/pbrt_parser.h', 'wt') as output:
    first_pragma = True
    for filename in header_files:
        with open(filename) as f:
            for line in f:
                if line.startswith('#include "'): continue
                if line.startswith('#pragma once'): 
                    if first_pragma:
                        output.write(line)
                        first_pragma = False
                    else:
                        continue
                output.write(line)
        output.write('\n\n')

with open('amalgamation/pbrt_parser.cpp', 'wt') as output:
    first_pragma = True
    for filename in source_files:
        with open(filename) as f:
            for line in f:
                if line.startswith('#include "'): continue
                if line.startswith('#pragma once'): 
                    if first_pragma:
                        output.write('#include "pbrt_parser.h"\n')
                        first_pragma = False
                    continue
                output.write(line)
        output.write('\n\n')
