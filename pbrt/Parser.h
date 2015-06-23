// ======================================================================== //
// Copyright 2015 Ingo Wald
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

#pragma once

#include "pbrt/Scene.h"

namespace plib {
  namespace pbrt {

    struct Tokenizer;

    /*! parser object that holds persistent state about the parsing
        state (e.g., file paths, named objects, etc), even if they are
        split over multiple files. To parse different scenes, use
        different instances of this class. */
    struct Parser {
      /*! constructor */
      Parser(bool dbg) : scene(new Scene), dbg(dbg) {};

      /*! parse given file, and add it to the scene we hold */
      void parse(const FileName &fn);
      
      void pushAttributes() {}
      void popAttributes() {}

      /*! return the scene we have parsed */
      Ref<Scene> getScene() { return scene; }
      
    private:
      // emit debug status messages...
      bool dbg;
      Ref<Scene> scene;
      std::map<std::string,Ref<Material> > namedMaterial;
      std::map<std::string,Ref<Texture> > namedTexture;
    };

  }
}
