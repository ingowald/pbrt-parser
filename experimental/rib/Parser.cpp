// ======================================================================== //
// Copyright 2019 Ingo Wald                                                 //
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

#include "Parser.h"

extern FILE *yyin;
extern int yydebug;
extern int yyparse();

namespace yacc_state {
  extern rib::RIBParser *parser;
}

namespace rib {

  RIBParser::RIBParser(const std::string &fileName)
  {
    scene = std::make_shared<Scene>();
    currentObject = scene->world = std::make_shared<Object>();
    
    xfmStack.push(affine3f(one));
    yydebug = 0; //1;
    yyin = fopen(fileName.c_str(),"r");
    yacc_state::parser = this;
    yyparse();
    yacc_state::parser = nullptr;
    fclose(yyin);

    std::cout << "done parsing." << std::endl;
    if (!ignored.empty()) {
      std::cout << "warning: there were some statements that we ignored..." << std::endl;
      for (auto it : ignored)
        std::cout << " - '" << it.first << "' \t(" << it.second << " times ...)" << std::endl;
    }
  }

  void RIBParser::add(pbrt::semantic::QuadMesh *qm)
  {
    currentObject->shapes.push_back(pbrt::semantic::QuadMesh::SP(qm));
  }

  semantic::QuadMesh *
  RIBParser::makeSubdivMesh(const std::string &type,
                            const std::vector<int> &faceVertexCount,
                            const std::vector<int> &vertexIndices,
                            const std::vector<int> &ignore0,
                            const std::vector<int> &ignore1,
                            const std::vector<float> &ignore2,
                            Params &params)
  {
    return makePolygonMesh(faceVertexCount,vertexIndices,params);
  }

  /*! note: this creates a POINTER, not a shared-ptr, else the yacc
    stack gets confused! */
  pbrt::semantic::QuadMesh *
  RIBParser::makePolygonMesh(const std::vector<int> &faceVertexCount,
                             const std::vector<int> &vertexIndices,
                             Params &params)
  {
    int nextIdx = 0;
    Param::SP _P = params["P"];
    if (!_P) throw std::runtime_error("mesh without 'P' parameter!?");
    ParamT<float> &P = (ParamT<float>&)*_P;

    int numVertices = P.values.size()/3;
    const vec3f *in_vertex = (const vec3f*)P.values.data();
    
    const affine3f xfm = currentXfm();
    
    pbrt::semantic::QuadMesh *qm = new semantic::QuadMesh();
    for (int i=0;i<numVertices;i++)
      qm->vertex.push_back(xfmPoint(xfm,in_vertex[i]));
    for (auto nextFaceVertices : faceVertexCount) {
      std::vector<int> idx;
      for (int i=0;i<nextFaceVertices;i++)
        idx.push_back(vertexIndices[nextIdx++]);
      while (idx.size() >= 4) {
        qm->index.push_back(vec4i(idx[0],idx[1],idx[2],idx[3]));
        idx.erase(idx.begin()+1,idx.begin()+3);
      }
      if (idx.size() == 3) {
        qm->index.push_back(vec4i(idx[0],idx[1],idx[2],idx[2]));
      }
    }
    // std::cout << "created mesh: First few quads: " << std::endl;
    // for (int i=0;i<std::min(size_t(10),qm->index.size());i++) {
    //   std::cout << "#" << i << ": " << qm->index[i] << "{"
    //             << qm->vertex[qm->index[i].x]
    //             << qm->vertex[qm->index[i].y]
    //             << qm->vertex[qm->index[i].z]
    //             << qm->vertex[qm->index[i].w]
    //             << "}" << std::endl;
    // }
    // box3f bounds = empty;
    // for (auto vtx : qm->vertex)
    //   bounds.extend(vtx);
    // PRINT(bounds);
    return qm;
  }
    

} // ::rib
