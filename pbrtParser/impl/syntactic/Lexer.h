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

#include "Scene.h"
// stl
#include <queue>
#include <memory>
#include <string.h>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    /*! file name and handle, to be used by tokenizer and loc */
    struct PBRT_PARSER_INTERFACE File {
      File(const std::string &fn): name(fn) {
        file = fopen(fn.c_str(),"r");
        if (!file)
          throw std::runtime_error("could not open file '"+fn+"'");
      }
      void close() { fclose(file); file = NULL; }
      virtual ~File() { 
        if (file) fclose(file);
      }
      /*! get name of the file */
      std::string getFileName() const { return name; }

      friend struct Lexer;

    private:
      std::string name;
      FILE *file;
    };

    /*! struct referring to a 'loc'ation in the input stream, given by file name 
      and line number */
    struct PBRT_PARSER_INTERFACE Loc {
      //! constructor
      Loc(const std::shared_ptr<File> &file=std::shared_ptr<File>()) : file(file), line(1), col(0) { }
      //! copy-constructor
      Loc(const Loc &loc) = default;
      Loc(Loc &&loc) = default;
      //! assignment
      Loc& operator=(const Loc &other) = default;
      Loc& operator=(Loc &&) = default;

      //! pretty-print
      std::string toString() const {
        return "@" + (file?file->getFileName():"<invalid>") + ":" + std::to_string(line) + "." + std::to_string(col);
      }

      friend struct Lexer;
    private:
      std::shared_ptr<File> file;
      int line { -1 };
      int col  { -1 };
    };

    struct PBRT_PARSER_INTERFACE Token {
      typedef enum { TOKEN_TYPE_STRING, TOKEN_TYPE_LITERAL, TOKEN_TYPE_SPECIAL, TOKEN_TYPE_NONE } Type;

      //! constructor
      Token() : loc{}, type{TOKEN_TYPE_NONE}, text{} {}
      Token(const Loc &loc, const Type type, const std::string& text) : loc{loc}, type{type}, text{text} { }
      Token(const Token &other) = default;
      Token(Token &&) = default;

      //! assignment
      Token& operator=(const Token &other) = default;
      Token& operator=(Token &&) = default;

      //! valid token
      explicit operator bool() { return type != TOKEN_TYPE_NONE; }
    
      //! pretty-print
      std::string toString() const { return loc.toString() + ": '" + text + "'"; }
      
      Loc         loc = {};
      Type        type = TOKEN_TYPE_NONE;
      std::string text = "";
    };

    /*! class that does the lexing - ie, the breaking up of an input
      stream of chars into an input stream of tokens.  */
    struct PBRT_PARSER_INTERFACE Lexer {

      //! constructor
      Lexer(const std::string &fn) : file(new File(fn)), loc(file), peekedChar(-1) { }

      Token next();
      
    private:
      Loc getLastLoc() { return loc; }

      inline void unget_char(int c);
      inline int get_char();
      inline bool isWhite(const char c);
      inline bool isSpecial(const char c);

      std::shared_ptr<File> file;
      Loc loc;
      int peekedChar;
    };

  } // ::pbrt::syntactic
} // ::pbrt
