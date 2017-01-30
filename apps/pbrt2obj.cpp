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

// pbrt
#include "pbrt/Parser.h"
// stl
#include <iostream>
#include <vector>

namespace pbrt_parser {

  using std::cout;
  using std::endl;

  FileName basePath = "";

  // the output file we're writing.
  FILE *out = NULL;

  size_t numWritten = 0;
  size_t numVerticesWritten = 0;

  void writeTriangleMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
    const affine3f xfm = instanceXfm*shape->transform;
    size_t firstVertexID = numVerticesWritten+1;

    { // parse "point P"
      std::shared_ptr<ParamT<float> > param_P = shape->findParam<float>("P");
      if (param_P) {
        const size_t numPoints = param_P->paramVec.size() / 3;
        for (int i=0;i<numPoints;i++) {
          vec3f v(param_P->paramVec[3*i+0],
                  param_P->paramVec[3*i+1],
                  param_P->paramVec[3*i+2]);
          v = xfmPoint(xfm,v);
          fprintf(out,"v %f %f %f\n",v.x,v.y,v.z);
          numVerticesWritten++;
        }
      }
    }

    { // parse "int indices"
      std::shared_ptr<ParamT<int> > param_indices = shape->findParam<int>("indices");
      if (param_indices) {
          
        const size_t numIndices = param_indices->paramVec.size() / 3;
        for (int i=0;i<numIndices;i++) {
          vec3i v(param_indices->paramVec[3*i+0],
                  param_indices->paramVec[3*i+1],
                  param_indices->paramVec[3*i+2]);
          fprintf(out,"f %lu %lu %lu\n",firstVertexID+v.x,firstVertexID+v.y,firstVertexID+v.z);
          numWritten++;
        }
      }
    }        
  }

  void parsePLY(const std::string &fileName,
                std::vector<vec3f> &v,
                std::vector<vec3f> &n,
                std::vector<vec3i> &idx);

  void writePlyMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
  {
    std::vector<vec3f> p, n;
    std::vector<vec3i> idx;
      
    std::shared_ptr<ParamT<std::string> > param_fileName = shape->findParam<std::string>("filename");
    FileName fn = FileName(basePath) + param_fileName->paramVec[0];
    parsePLY(fn.str(),p,n,idx);

    const affine3f xfm = instanceXfm*shape->transform;
    size_t firstVertexID = numVerticesWritten+1;

    for (int i=0;i<p.size();i++) {
      vec3f v = xfmPoint(xfm,p[i]);
      fprintf(out,"v %f %f %f\n",v.x,v.y,v.z);
      numVerticesWritten++;
    }

    for (int i=0;i<idx.size();i++) {
      vec3i v = idx[i];
      fprintf(out,"f %lu %lu %lu\n",
              firstVertexID+v.x,
              firstVertexID+v.y,
              firstVertexID+v.z);
      numWritten++;
    }
  }

  void writeObject(std::shared_ptr<Object> object, 
                   const affine3f &instanceXfm)
  {
    cout << "writing " << object->toString() << endl;
    for (int shapeID=0;shapeID<object->shapes.size();shapeID++) {
      std::shared_ptr<Shape> shape = object->shapes[shapeID];
      if (shape->type == "trianglemesh") {
        writeTriangleMesh(shape,instanceXfm);
      } else if (shape->type == "plymesh") {
        writePlyMesh(shape,instanceXfm);
      } else 
        cout << "**** invalid shape #" << shapeID << " : " << shape->type << endl;
    }
    for (int instID=0;instID<object->objectInstances.size();instID++) {
      writeObject(object->objectInstances[instID]->object,
                  instanceXfm*object->objectInstances[instID]->xfm);
    }
  }


  void pbrt2obj(int ac, char **av)
  {
    std::vector<std::string> fileName;
    bool dbg = false;
    std::string outFileName = "a.obj";
    for (int i=1;i<ac;i++) {
      const std::string arg = av[i];
      if (arg[0] == '-') {
        if (arg == "-dbg" || arg == "--dbg")
          dbg = true;
        else if (arg == "--path" || arg == "-path")
          basePath = av[++i];
        else if (arg == "-o")
          outFileName = av[++i];
        else
          THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
      } else {
        fileName.push_back(arg);
      }          
    }
    out = fopen(outFileName.c_str(),"w");
    assert(out);
  
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "parsing:";
    for (int i=0;i<fileName.size();i++)
      std::cout << " " << fileName[i];
    std::cout << std::endl;

    if (basePath.str() == "")
      basePath = FileName(fileName[0]).path();
  
    pbrt_parser::Parser *parser = new pbrt_parser::Parser(dbg,basePath);
    try {
      for (int i=0;i<fileName.size();i++)
        parser->parse(fileName[i]);
    
      std::cout << "==> parsing successful (grammar only for now)" << std::endl;
    
      std::shared_ptr<Scene> scene = parser->getScene();
      writeObject(scene->world,ospcommon::one);
      fclose(out);
      cout << "Done exporting to OBJ; wrote a total of " << numWritten << " triangles" << endl;
    } catch (std::runtime_error e) {
      std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
      exit(1);
    }
  }

} // ::pbrt_parser

int main(int ac, char **av)
{
  pbrt_parser::pbrt2obj(ac,av);
  return 0;
}
