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

namespace rib {

      RIBParser::RIBParser(const std::string &fileName)
    {
      scene = std::make_shared<Scene>();
      
      xfmStack.push(affine3f());
      yydebug = 0; //1;
      yyin = fopen(fileName.c_str(),"r");
      yyparse();
      fclose(yyin);
    }

  void RIBParser::makeSubdivMesh(const std::string &type,
                        const std::vector<int> &faceVertexCount,
                        const std::vector<int> &vertexIndices,
                        const std::vector<int> &ignore0,
                        const std::vector<int> &ignore1,
                        const std::vector<float> &ignore2,
                        Params &params)
    {
      int nextIdx = 0;
      Param::SP _P = params["P"];
      if (!_P) return;
      ParamT<float> &P = (ParamT<float>&)*_P;
      
      const vec3f *vertex = (const vec3f*)P.values.data();
      for (auto nextFaceVertices : faceVertexCount) {
        std::vector<int> faceIndices;
        for (int i=0;i<nextFaceVertices;i++)
          faceIndices.push_back(vertexIndices[nextIdx++]);
      }
    }

} // ::rib
