// ======================================================================== //
// Copyright 2016 Ingo Wald
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

using namespace plib::pbrt;

namespace plib {
  namespace pbrt {

    using std::cout;
    using std::endl;

    FileName basePath = "";

    // the output file we're writing.
    FILE *out = NULL;
    FILE *bin = NULL;

    size_t numUniqueTriangles = 0;
    size_t numInstancedTriangles = 0;

    std::map<Shape *,int> alreadyExported;
    //! transform used when original instance was emitted
    std::map<int,affine3f> transformOfFirstInstance;
    std::map<int,int> numTrisOfInstance;

    size_t nextTransformID = 0;

    std::vector<int> rootObjects;


    int nextNodeID = 0;
    
    int writeTriangleMesh(Ref<Shape> shape, const affine3f &instanceXfm)
    {
      int thisID = nextNodeID++;
      const affine3f xfm = instanceXfm*shape->transform;
      alreadyExported[shape.ptr] = thisID;
      transformOfFirstInstance[thisID] = instanceXfm;

      fprintf(out,"<Mesh id=\"%i\">\n",thisID);
      fprintf(out,"  <materiallist>0</materiallist>\n");
      { // parse "point P"
        Ref<ParamT<float> > param_P = shape->findParam<float>("P");
        if (param_P) {
          size_t ofs = ftell(bin);
          const size_t numPoints = param_P->paramVec.size() / 3;
          for (int i=0;i<numPoints;i++) {
            vec3f v(param_P->paramVec[3*i+0],
                    param_P->paramVec[3*i+1],
                    param_P->paramVec[3*i+2]);
            v = xfmPoint(xfm,v);
            fwrite(&v,sizeof(v),1,bin);
            // fprintf(out,"v %f %f %f\n",v.x,v.y,v.z);
            // numVerticesWritten++;
          }
          fprintf(out,"  <vertex num=\"%li\" ofs=\"%li\"/>\n",
                  numPoints,ofs);
        }
        
      }
      
      { // parse "int indices"
        Ref<ParamT<int> > param_indices = shape->findParam<int>("indices");
        if (param_indices) {
          size_t ofs = ftell(bin);
          const size_t numIndices = param_indices->paramVec.size() / 3;
          numTrisOfInstance[thisID] = numIndices;
          numUniqueTriangles+=numIndices;
          for (int i=0;i<numIndices;i++) {
            vec4i v(param_indices->paramVec[3*i+0],
                    param_indices->paramVec[3*i+1],
                    param_indices->paramVec[3*i+2],
                    0);
            fwrite(&v,sizeof(v),1,bin);
          }
          fprintf(out,"  <prim num=\"%li\" ofs=\"%li\"/>\n",
                  numIndices,ofs);
        }
      }        
      fprintf(out,"</Mesh>\n");
      return thisID;
    }

    void parsePLY(const std::string &fileName,
                  std::vector<vec3f> &v,
                  std::vector<vec3f> &n,
                  std::vector<vec3i> &idx);

    int writePlyMesh(Ref<Shape> shape, const affine3f &instanceXfm)
    {
      std::vector<vec3f> p, n;
      std::vector<vec3i> idx;
      
      Ref<ParamT<std::string> > param_fileName = shape->findParam<std::string>("filename");
      FileName fn = FileName(basePath) + param_fileName->paramVec[0];
      parsePLY(fn.str(),p,n,idx);

      int thisID = nextNodeID++;
      const affine3f xfm = instanceXfm*shape->transform;
      alreadyExported[shape.ptr] = thisID;
      transformOfFirstInstance[thisID] = xfm;
      
      // -------------------------------------------------------
      fprintf(out,"<Mesh id=\"%i\">\n",thisID);
      fprintf(out,"  <materiallist>0</materiallist>\n");

      // -------------------------------------------------------
      fprintf(out,"  <vertex num=\"%li\" ofs=\"%li\"/>\n",
              p.size(),ftell(bin));
      for (int i=0;i<p.size();i++) {
        vec3f v = xfmPoint(xfm,p[i]);
        fwrite(&v,sizeof(v),1,bin);
      }

      // -------------------------------------------------------
      fprintf(out,"  <prim num=\"%li\" ofs=\"%li\"/>\n",
              idx.size(),ftell(bin));
      for (int i=0;i<idx.size();i++) {
        vec3i v = idx[i];
        fwrite(&v,sizeof(v),1,bin);
        int z = 0.f;
        fwrite(&z,sizeof(z),1,bin);
      }
      numTrisOfInstance[thisID] = idx.size();
      numUniqueTriangles += idx.size();
      // -------------------------------------------------------
      fprintf(out,"</Mesh>\n");
      // -------------------------------------------------------
      return thisID;
    }

    void writeObject(const Ref<Object> &object, 
                     const affine3f &instanceXfm)
    {
      cout << "writing " << object->toString() << endl;
      // std::vector<int> child;

      for (int shapeID=0;shapeID<object->shapes.size();shapeID++) {
        Ref<Shape> shape = object->shapes[shapeID];

        if (alreadyExported.find(shape.ptr) != alreadyExported.end()) {
          
          int childID = alreadyExported[shape.ptr];
          affine3f xfm = instanceXfm * rcp(transformOfFirstInstance[childID]);
          numInstancedTriangles += numTrisOfInstance[childID];

          int thisID = nextNodeID++;
          fprintf(out,"<Transform id=\"%i\" child=\"%i\">\n",
                  thisID,
                  childID);
          fprintf(out,"  %f %f %f\n",
                  xfm.l.vx.x,
                  xfm.l.vx.y,
                  xfm.l.vx.z);
          fprintf(out,"  %f %f %f\n",
                  xfm.l.vy.x,
                  xfm.l.vy.y,
                  xfm.l.vy.z);
          fprintf(out,"  %f %f %f\n",
                  xfm.l.vz.x,
                  xfm.l.vz.y,
                  xfm.l.vz.z);
          fprintf(out,"  %f %f %f\n",
                  xfm.p.x,
                  xfm.p.y,
                  xfm.p.z);
          fprintf(out,"</Transform>\n");
          rootObjects.push_back(thisID);
          continue;
        } 
      
        if (shape->type == "trianglemesh") {
          int thisID = writeTriangleMesh(shape,instanceXfm);
          rootObjects.push_back(thisID);
          continue;
        }

        if (shape->type == "plymesh") {
          int thisID = writePlyMesh(shape,instanceXfm);
          rootObjects.push_back(thisID);
          continue;
        }

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
      std::string outFileName = "a.xml";
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
      bin = fopen((outFileName+".bin").c_str(),"w");
      assert(out);
      assert(bin);

      fprintf(out,"<?xml version=\"1.0\"?>\n");
      fprintf(out,"<BGFscene>\n");

      int thisID = nextNodeID++;
      fprintf(out,"<Material name=\"default\" type=\"OBJMaterial\" id=\"%i\">\n",thisID);
      fprintf(out,"  <param name=\"kd\" type=\"float3\">0.7 0.7 0.7</param>\n");
      fprintf(out,"</Material>\n");
  
      std::cout << "-------------------------------------------------------" << std::endl;
      std::cout << "parsing:";
      for (int i=0;i<fileName.size();i++)
        std::cout << " " << fileName[i];
      std::cout << std::endl;

      if (basePath.str() == "")
        basePath = FileName(fileName[0]).path();
  
      plib::pbrt::Parser *parser = new plib::pbrt::Parser(dbg,basePath);
      try {
        for (int i=0;i<fileName.size();i++)
          parser->parse(fileName[i]);
    
        std::cout << "==> parsing successful (grammar only for now)" << std::endl;
    
        embree::Ref<Scene> scene = parser->getScene();
        writeObject(scene->world.ptr,embree::one);

        {
          int thisID = nextNodeID++;
          fprintf(out,"<Group id=\"%i\" numChildren=\"%i\">\n",thisID,rootObjects.size());
          for (int i=0;i<rootObjects.size();i++)
            fprintf(out,"%i ",rootObjects[i]);
          fprintf(out,"\n</Group>\n");
        }

        fprintf(out,"</BGFscene>");

        fclose(out);
        fclose(bin);
        cout << "Done exporting to OSP file" << endl;
        cout << " - unique triangles written " << numUniqueTriangles << endl;
        cout << " - instanced tris written   " << numInstancedTriangles << endl;
      } catch (std::runtime_error e) {
        std::cout << "**** ERROR IN PARSING ****" << std::endl << e.what() << std::endl;
        exit(1);
      }
    }

  }
}

int main(int ac, char **av)
{
  plib::pbrt::pbrt2obj(ac,av);
  return 0;
}
