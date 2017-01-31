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

#pragma once

#include "pbrt/Scene.h"
// std
#include <stack>

namespace pbrt_parser {

  struct Lexer;
  struct Token;

  /*! parser object that holds persistent state about the parsing
    state (e.g., file paths, named objects, etc), even if they are
    split over multiple files. To parse different scenes, use
    different instances of this class. */
  struct PBRT_PARSER_INTERFACE Parser {
    /*! constructor */
    Parser(bool dbg, const std::string &basePath="");

    /*! parse given file, and add it to the scene we hold */
    void parse(const FileName &fn);

    /*! parse everything in WorldBegin/WorldEnd */
    void parseWorld();
    /*! parse everything in the root scene file */
    void parseScene();
      
    void pushAttributes();
    void popAttributes();
      
    /*! try parsing this token as some sort of transform token, and
      return true if successful, false if not recognized  */
    bool parseTransforms(std::shared_ptr<Token> token);

    void pushTransform();
    void popTransform();

    /*! return the scene we have parsed */
    std::shared_ptr<Scene> getScene() { return scene; }
      
  private:
    //! stack of parent files' token streams
    std::stack<std::shared_ptr<Lexer> > tokenizerStack;
    //! token stream of currently open file
    std::shared_ptr<Lexer> tokens;
    /*! get the next token to process (either from current file, or
      parent file(s) if current file is EOL!); return NULL if
      complete end of input */
    std::shared_ptr<Token> getNextToken();

    // add additional transform to current transform
    void addTransform(const affine3f &add)
    {
      transformStack.top() *= add; 
    }
    void setTransform(const affine3f &xfm)
    { transformStack.top() = xfm; }

    std::stack<std::shared_ptr<Material> >   materialStack;
    std::stack<std::shared_ptr<Attributes> > attributesStack;
    std::stack<affine3f>                     transformStack;
    std::stack<std::shared_ptr<Object> >     objectStack;

    affine3f                getCurrentXfm() { return transformStack.top(); }
    std::shared_ptr<Object> getCurrentObject();
    std::shared_ptr<Object> findNamedObject(const std::string &name, bool createIfNotExist=false);

    // emit debug status messages...
    bool dbg;
    const std::string basePath;
    FileName rootNamePath;
    std::shared_ptr<Scene>    scene;
    std::shared_ptr<Object>   currentObject;
    std::shared_ptr<Material> currentMaterial;
    std::map<std::string,std::shared_ptr<Object> >   namedObjects;
    std::map<std::string,std::shared_ptr<Material> > namedMaterial;
    std::map<std::string,std::shared_ptr<Texture> >  namedTexture;
  };

  PBRT_PARSER_INTERFACE void parsePLY(const std::string &fileName,
                                      std::vector<vec3f> &v,
                                      std::vector<vec3f> &n,
                                      std::vector<vec3i> &idx);
}
