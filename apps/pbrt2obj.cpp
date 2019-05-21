// ======================================================================== //
// Copyright 2015-2017 Ingo Wald                                            //
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

#include "pbrtParser/Scene.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

namespace pbrt {
    
  using std::cout;
  using std::endl;

  // the output file we're writing.
  std::ofstream out;
  // FILE *out = NULL;

  size_t numWritten = 0;
  size_t numVerticesWritten = 0;
  
  
  void writeTriangleMesh(TriangleMesh::SP mesh, const affine3f &xfm)
  {
    /* TODO: export the material of that shape, and use it */
    size_t firstVertexID = numVerticesWritten+1;

    /* TODO: ignoring texture coordinates for now */

    /* TODO: ignoring shading normals for now */

    for (auto v : mesh->vertex) {
      v = xfmPoint(xfm,v);
      out << "v  " << v.x << " " << v.y << " " << v.z << std::endl;
      numVerticesWritten++;
    }

    for (auto v : mesh->index) {
      /* TODO : for texcoord and normal arrays, use x//x an x/x/x formats */
      out << "f " << (firstVertexID+v.x)
          << "\t" << (firstVertexID+v.y)
          << "\t" << (firstVertexID+v.z)
          << std::endl;
    }
    numWritten++;
  }

  void defineDefaultMaterials(std::ofstream &file)
  {
    file << "newmtl pbrt_parser_no_materials_yet" << std::endl;
    file << "Kd .6 .6 .6" << std::endl;
    file << "Ka .1 .1 .1" << std::endl;
    file << "usematerial pbrt_parser_no_materials_yet" << std::endl << std::endl;
  }
  
  void writeObject(Object::SP object, 
                   const affine3f &instanceXfm)
  {
    cout << "writing " << object->toString() << endl;
    for (auto shape : object->shapes) {
      TriangleMesh::SP mesh = std::dynamic_pointer_cast<TriangleMesh>(shape);
      if (mesh) {
        std::cout << " - found mesh w/ " << mesh->index.size() << " tris" << std::endl;
        writeTriangleMesh(mesh,instanceXfm);
      } else
        std::cout << " - warning: shape is not a triangle mesh : " << shape->toString() << std::endl;
    }
      
    for (auto inst : object->instances) {
      writeObject(inst->object,
                  instanceXfm*inst->xfm);
    }
  }

  inline bool endsWith(const std::string &s, const std::string &suffix)
  {
    return s.substr(s.size()-suffix.size(),suffix.size()) == suffix;
  }


  void pbrt2obj(int ac, char **av)
  {
    std::string inFileName;
    std::string outFileName = "a.obj";
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg[0] == '-') {
        if (arg == "-o")
          outFileName = av[++i];
        else
          throw std::runtime_error("invalid argument '"+arg+"'");
      } else {
        inFileName = arg;
      }          
    }
    out.open(outFileName);
    // out = fopen(outFileName.c_str(),"w");
    assert(out.good());

    defineDefaultMaterials(out);
    
  
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "parsing: " << inFileName << std::endl;
      
    std::shared_ptr<Scene> scene;
    try {
      if (endsWith(inFileName,".pbrt"))
        scene = importPBRT(inFileName);
      else if (endsWith(inFileName,".pbf"))
        scene = Scene::loadFrom(inFileName);
      else
        throw std::runtime_error("un-recognized input file extension");
        
      std::cout << " => yay! parsing successful..." << std::endl;
        
      std::cout << "done parsing, now exporting (triangular geometry from) scene" << std::endl;
      writeObject(scene->world,affine3f::identity());
    } catch (std::runtime_error e) {
      std::cerr << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      std::cerr << "(this means that either there's something wrong with that PBRT file, or that the parser can't handle it)" << std::endl;
      exit(1);
    }
  }
  
  extern "C" int main(int ac, char **av)
  {
    pbrt2obj(ac,av);
    return 0;
  }

} // ::pbrt
