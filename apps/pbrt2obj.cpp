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

// ospcommon, which we use for a vector class
#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
// pbrt-parser
#define PBRT_PARSER_VECTYPE_NAMESPACE    ospcommon
#include "pbrtParser/syntax/Scene.h"
// stl
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

namespace pbrt {
  namespace syntax {
    
    using std::cout;
    using std::endl;

    using namespace ospcommon;
  
    // the output file we're writing.
    std::ofstream out;
    // FILE *out = NULL;

    size_t numWritten = 0;
    size_t numVerticesWritten = 0;
  
    std::string exportMaterial(Material::SP material)
    {
      if (!material) 
        // default material
        return "usemtl pbrt_parser_default_material\n\n";

      std::stringstream ss;

      static std::map<std::shared_ptr<Material>,std::string> alreadyExported;
      if (alreadyExported.find(material) != alreadyExported.end()) {
        ss << "usemtl " << alreadyExported[material] << std::endl << std::endl;
        return ss.str();
      }

      const std::string type = material->type;
      if (type == "mix") {
        std::string matName = "pbrt_parser_error_material";
        alreadyExported[material] = matName;
        return "usemtl pbrt_parser_error_material\n\n";
        //      throw std::runtime_error("'mix' material ..."+material->toString());
      } else if (type == "uber") {
        std::string matName = std::string("uber_material__")+std::to_string((unsigned long long)alreadyExported.size());
        ss << "newmtl " << matName << std::endl;
        { //----------- Kd -----------
          vec3f v;
          try {
            v = material->getParam3f("Kd",vec3f(0.0f));
          } catch (std::runtime_error e) {
            v = vec3f(.6f);
          };
          ss << "Kd " << v.x << " " << v.y << " " << v.z << std::endl;
        }
        // ----------- done material -----------
        ss << std::endl << "usemtl " << matName << std::endl << std::endl;
        alreadyExported[material] = matName;
        return ss.str();
      } else {
        std::string matName = "pbrt_parser_error_material";
        alreadyExported[material] = matName;
        return "usemtl pbrt_parser_error_material\n\n";
      }
    }

  
    void writeTriangleMesh(std::shared_ptr<Shape> shape, const affine3f &instanceXfm)
    {
      /*! call 'exportMateiral, which will return a string that properly
        defined and/or activates the given mateirla */
      std::string materialString = exportMaterial(shape->material);
      out << materialString << std::endl;

      const affine3f xfm = instanceXfm*(ospcommon::affine3f&)shape->transform.atStart;
      size_t firstVertexID = numVerticesWritten+1;

      std::shared_ptr<ParamArray<float> > param_st = shape->findParam<float>("st");
      if (param_st) {
        const size_t numPoints = param_st->size() / 2;
        for (int i=0;i<numPoints;i++) {
          vec2f v(param_st->get(2*i+0),
                  param_st->get(2*i+1));
          out << "vt " << v.x << " " << v.y << std::endl;
        }
      }
    
      { // parse "point P"
        std::shared_ptr<ParamArray<float> > param_P = shape->findParam<float>("P");
        if (param_P) {
          const size_t numPoints = param_P->size() / 3;
          for (int i=0;i<numPoints;i++) {
            vec3f v(param_P->get(3*i+0),
                    param_P->get(3*i+1),
                    param_P->get(3*i+2));
            v = xfmPoint(xfm,v);
            out << "v  " << v.x << " " << v.y << " " << v.z << std::endl;
            numVerticesWritten++;
          }
        }
      }

      { // parse "int indices"
        std::shared_ptr<ParamArray<int> > param_indices = shape->findParam<int>("indices");
        if (param_indices) {
          
          const size_t numIndices = param_indices->size() / 3;
          for (int i=0;i<numIndices;i++) {
            vec3i v(param_indices->get(3*i+0),
                    param_indices->get(3*i+1),
                    param_indices->get(3*i+2));
            if (param_st) {
              out << "f " << (firstVertexID+v.x) << "//" << (firstVertexID+v.x)
                  << "\t" << (firstVertexID+v.y) << "//" << (firstVertexID+v.y)
                  << "\t" << (firstVertexID+v.z) << "//" << (firstVertexID+v.z)
                  << std::endl;
            } else {
              out << "f " << (firstVertexID+v.x)
                  << "\t" << (firstVertexID+v.y)
                  << "\t" << (firstVertexID+v.z)
                  << std::endl;
            }
            numWritten++;
          }
        }
      }        
    }

    void writePlyMesh(std::shared_ptr<Scene> scene,
                      std::shared_ptr<Shape> shape,
                      const affine3f &instanceXfm)
    {
      /*! call 'exportMateiral, which will return a string that properly
        defined and/or activates the given mateirla */
      std::string materialString = exportMaterial(shape->material);
      out << materialString << std::endl;
      // fprintf(out,"%s\n",materialString.c_str());


      std::vector<vec3f> p, n;
      std::vector<vec3i> idx;
      
      // std::shared_ptr<ParamArray<std::string> > param_fileName = shape->findParam<std::string>("filename");
      std::string fn = scene->makeGlobalFileName(shape->getParamString("filename"));
      // param_fileName->get(0));
      // std::string fn = basePath + "/" + param_fileName->get(0);
      pbrt_helper_loadPlyTriangles(fn,p,n,idx);

      const affine3f xfm = instanceXfm*(ospcommon::affine3f&)shape->transform.atStart;
      size_t firstVertexID = numVerticesWritten+1;

      for (int i=0;i<p.size();i++) {
        vec3f v = xfmPoint(xfm,p[i]);
        out << "v  " << v.x << " " << v.y << " " << v.z << std::endl;
        // fprintf(out,"v %f %f %f\n",v.x,v.y,v.z);
        numVerticesWritten++;
      }

      for (int i=0;i<idx.size();i++) {
        vec3i v = idx[i];
        out << "f " << (firstVertexID+v.x)
            << "\t" << (firstVertexID+v.y)
            << "\t" << (firstVertexID+v.z)
            << std::endl;
        numWritten++;
      }
    }

    void defineDefaultMaterials(std::ofstream &file)
    {
      file << "newmtl pbrt_parser_error_material" << std::endl;
      file << "Kd 1 0 0" << std::endl;
      file << "" << std::endl;

      file << "newmtl pbrt_parser_default_material" << std::endl;
      file << "Kd .6 .6 .6" << std::endl;
      file << "Ka .1 .1 .1" << std::endl;
      file << "" << std::endl;
    }
  
    void writeObject(Scene::SP scene,
                     Object::SP object, 
                     const affine3f &instanceXfm)
    {
      cout << "writing " << object->toString() << endl;
      for (int shapeID=0;shapeID<object->shapes.size();shapeID++) {
        std::shared_ptr<Shape> shape = object->shapes[shapeID];
        if (shape->type == "trianglemesh") {
          writeTriangleMesh(shape,instanceXfm);
        } else if (shape->type == "plymesh") {
          writePlyMesh(scene,shape,instanceXfm);
        } else 
          cout << "**** invalid shape #" << shapeID << " : " << shape->type << endl;
      }
      for (int instID=0;instID<object->objectInstances.size();instID++) {
        writeObject(scene,
                    object->objectInstances[instID]->object,
                    instanceXfm*(ospcommon::affine3f&)object->objectInstances[instID]->xfm.atStart);
      }
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
            THROW_RUNTIME_ERROR("invalid argument '"+arg+"'");
        } else {
          inFileName = arg;
        }          
      }
      out = std::ofstream(outFileName);
      // out = fopen(outFileName.c_str(),"w");
      assert(out.good());

      defineDefaultMaterials(out);
    
  
      std::cout << "-------------------------------------------------------" << std::endl;
      std::cout << "parsing: " << inFileName << std::endl;
      
      try {
        std::cout << "==> parsing successful (grammar only for now)" << std::endl;
        std::shared_ptr<Scene> scene = pbrt::syntax::parse(inFileName);
        std::cout << "done parsing, now exporting (triangular geometry from) scene" << std::endl;
        writeObject(scene,scene->world,ospcommon::one);
        // fclose(out);
        cout << "Done exporting to OBJ; wrote a total of " << numWritten << " triangles" << endl;
      } catch (std::runtime_error e) {
        std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
        exit(1);
      }
    }
  
    extern "C" int main(int ac, char **av)
    {
      pbrt2obj(ac,av);
      return 0;
    }

  } // ::pbrt::syntax
} // ::pbrt
