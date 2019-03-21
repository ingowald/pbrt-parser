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

#pragma once

/*! \file pbrt/Parser.h *Internal* parser class used by \see
  pbrt_parser::Scene::parseFromFile() - as end user, you should
  never have to look into this file directly, and only use \see
  pbrt_parser::Scene::parseFromFile() */

#include "Scene.h"
#include "Lexer.h"
// std
#include <stack>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    /*! the class that implements PBRT's "current transformation matrix
      (CTM)" stack */
    struct CTM : public Transform {
      void reset()
      {
        startActive = endActive = true;
          (math::affine3f&)atStart = (math::affine3f&)atEnd = math::affine3f::identity();
      }
      bool startActive { true };
      bool endActive   { true };
    
      /*! pbrt's "CTM" (current transformation matrix) handling */
      std::stack<Transform> stack;
    };

  
    /*! parser object that holds persistent state about the parsing
      state (e.g., file paths, named objects, etc), even if they are
      split over multiple files. 

      \note End users should not have to use this class directly; it
      should only ever be used by Scene::parseFromFile()

      \warning Each scene should be parsed with its own instanc of this
      parser class, otherwise left-over state from previous passes may
      mess with the state of later pbrt file parse's */
    struct PBRT_PARSER_INTERFACE Parser {
      /*! constructor */
      Parser(const std::string &basePath="");

      /*! parse given file, and add it to the scene we hold */
      void parse(const std::string &fn);

      /*! parse everything in WorldBegin/WorldEnd */
      void parseWorld();
      /*! parse everything in the root scene file */
      void parseScene();
      
      void pushAttributes();
      void popAttributes();
      
      /*! try parsing this token as some sort of transform token, and
        return true if successful, false if not recognized  */
      bool parseTransform(const Token& token);

      void pushTransform();
      void popTransform();

      vec3f parseVec3f();
      float parseFloat();
      affine3f parseMatrix();


      std::map<std::string,std::shared_ptr<Object> >   namedObjects;

      inline Param::SP parseParam(std::string &name);
      void parseParams(std::map<std::string, Param::SP> &params);

      /*! return the scene we have parsed */
      std::shared_ptr<Scene> getScene() { return scene; }
      std::shared_ptr<Texture> getTexture(const std::string &name);
    private:
      //! stack of parent files' token streams
      std::stack<std::shared_ptr<Lexer>> tokenizerStack;
      std::deque<Token> peekQueue;
    
      //! token stream of currently open file
      std::shared_ptr<Lexer> tokens;
    
      /*! get the next token to process (either from current file, or
        parent file(s) if current file is EOL!); return NULL if
        complete end of input */
      Token next();

      /*! peek ahead by N tokens, (either from current file, or
        parent file(s) if current file is EOL!); return NULL if
        complete end of input */
      Token peek(unsigned int ahead=0);

      // add additional transform to current transform
      void addTransform(const affine3f &xfm) {
        if (ctm.startActive) (math::affine3f&)ctm.atStart = (math::affine3f&)ctm.atStart * (const math::affine3f&)xfm;
        if (ctm.endActive)   (math::affine3f&)ctm.atEnd   = (math::affine3f&)ctm.atEnd * (const math::affine3f&)xfm;
      }
      void setTransform(const affine3f &xfm) {
        if (ctm.startActive) (math::affine3f&)ctm.atStart = (const math::affine3f&)xfm;
        if (ctm.endActive) (math::affine3f&)ctm.atEnd = (const math::affine3f&)xfm;
      }

      std::stack<std::shared_ptr<Material> >   materialStack;
      std::stack<std::shared_ptr<Attributes> > attributesStack;
      std::stack<std::shared_ptr<Object> >     objectStack;

      CTM ctm;
      // Transform              getCurrentXfm() { return transformtack.top(); }
      std::shared_ptr<Object> getCurrentObject();
      std::shared_ptr<Object> findNamedObject(const std::string &name, bool createIfNotExist=false);

      // emit debug status messages...
      const std::string basePath;
      std::string rootNamePath;
      std::shared_ptr<Scene>    scene;
      std::shared_ptr<Object>   currentObject;
      std::shared_ptr<Material> currentMaterial;
    
      bool dbg;
      /*! tracks the location of the last token gotten through next() (for debugging) */
      // Loc lastLoc;
    
    };

    PBRT_PARSER_INTERFACE void parsePLY(const std::string &fileName,
                                        std::vector<vec3f> &v,
                                        std::vector<vec3f> &n,
                                        std::vector<vec3i> &idx);
    
  } // ::pbrt::syntx
} // ::pbrt
