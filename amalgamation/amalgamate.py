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

copyright = '''
// ======================================================================== //
// Copyright 2015-2019 Ingo Wald & Fabio Pellacini                          //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

//
// This library code from code from the happly library.
//
// A header-only implementation of the .ply file format.
// https://github.com/nmwsharp/happly
// By Nicholas Sharp - nsharp@cs.cmu.edu
//
// MIT License
//
// Copyright (c) 2018 Nick Sharp
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

'''

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
    first_copyright = True
    for filename in header_files:
        in_copyright = True
        with open(filename) as f:
            for line in f:
                if in_copyright:
                    if line.startswith('//'): continue
                    if first_copyright: output.write(copyright[1:])
                    in_copyright = False
                    first_copyright = False
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
    first_copyright = True
    for filename in source_files:
        in_copyright = True
        with open(filename) as f:
            for line in f:
                if in_copyright:
                    if line.startswith('//'): continue
                    if first_copyright: output.write(copyright[1:])
                    in_copyright = False
                    first_copyright = False
                if line.startswith('#include "'): continue
                if line.startswith('#pragma once'): 
                    if first_pragma:
                        output.write('#include "pbrt_parser.h"\n')
                        first_pragma = False
                    continue
                output.write(line)
        output.write('\n\n')
