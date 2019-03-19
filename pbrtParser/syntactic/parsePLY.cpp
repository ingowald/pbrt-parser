// ======================================================================== //
// Copyright 2015-2019 Ingo Wald                                            //
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

// pbrt
#include "Parser.h"
// stl
#include <iostream>
#include <vector>

#include "3rdParty/happly.h"

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    using std::cout;
    using std::endl;



    namespace ply {
      // Ref<sg::TriangleMesh> readFile(const std::string &fileName)
      void parse(const std::string &fileName,
                  std::vector<vec3f> &pos,
                  std::vector<vec3f> &nor,
                  std::vector<vec3i> &idx)
      {
        happly::PLYData ply(fileName);
        
        if(ply.hasElement("vertex")) {
          happly::Element& elem = ply.getElement("vertex");
          if(elem.hasProperty("x") && elem.hasProperty("y") && elem.hasProperty("z")) {
            std::vector<float> x = elem.getProperty<float>("x");
            std::vector<float> y = elem.getProperty<float>("y");
            std::vector<float> z = elem.getProperty<float>("z");
            pos.resize(x.size());
            for(int i = 0; i < x.size(); i ++) {
              pos[i] = vec3f(x[i], y[i], z[i]);
            }
          } else {
            throw std::runtime_error("missing positions in ply");
          }
          if(elem.hasProperty("nx") && elem.hasProperty("ny") && elem.hasProperty("nz")) {
            std::vector<float> x = elem.getProperty<float>("nx");
            std::vector<float> y = elem.getProperty<float>("ny");
            std::vector<float> z = elem.getProperty<float>("nz");
            nor.resize(x.size());
            for(int i = 0; i < x.size(); i ++) {
              nor[i] = vec3f(x[i], y[i], z[i]);
            }
          }
        } else {
          throw std::runtime_error("missing positions in ply");
        }
        if (ply.hasElement("face")) {
          happly::Element& elem = ply.getElement("face");
          if(elem.hasProperty("vertex_indices")) {
            std::vector<std::vector<int>> fasces = elem.getListProperty<int>("vertex_indices");
            for(int j = 0; j < fasces.size(); j ++) {
              std::vector<int>& face = fasces[j];
              for (int i=2;i<face.size();i++) {
                idx.push_back(vec3i(face[0], face[i-1], face[i]));
              }
            }
          } else {
            throw std::runtime_error("missing faces in ply");
          }          
        } else {
            throw std::runtime_error("missing faces in ply");
        }                        
      }

    } // ::pbrt::syntactic::ply

    
  } // ::pbrt::syntactic
} // ::pbrt

extern "C" 
void pbrt_helper_loadPlyTriangles(const std::string &fileName,
                                  std::vector<pbrt::syntactic::vec3f> &v,
                                  std::vector<pbrt::syntactic::vec3f> &n,
                                  std::vector<pbrt::syntactic::vec3i> &idx)
{
  pbrt::syntactic::ply::parse(fileName,v,n,idx);
}

