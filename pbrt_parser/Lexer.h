// ======================================================================== //
// Copyright 2015-2018 Ingo Wald                                            //
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

namespace pbrt_parser {

  /*! file name and handle, to be used by tokenizer and loc */
  struct PBRT_PARSER_INTERFACE File {
    File(const std::string &fn);
    void close() { fclose(file); file = NULL; }
    virtual ~File();
    /*! get name of the file */
    std::string getFileName() const { return name; }

    friend class Lexer;

  private:
    std::string name;
    FILE *file;
  };

  /*! struct referring to a 'loc'ation in the input stream, given by
    file name and line number */
  struct PBRT_PARSER_INTERFACE Loc {
    //! constructor
    Loc(std::shared_ptr<File> file=std::shared_ptr<File>());
    //! copy-constructor
    Loc(const Loc &loc);

    //! pretty-print
    std::string toString() const;

    friend class Lexer;
  private:
    std::shared_ptr<File> file;
    int line { -1 };
    int col  { -1 };
  };

  struct PBRT_PARSER_INTERFACE Token {

    typedef enum { TOKEN_TYPE_STRING, TOKEN_TYPE_LITERAL, TOKEN_TYPE_SPECIAL } Type;

    //! constructor
    Token(const Loc &loc, 
          const Type type,
          const std::string &text);
    //! pretty-print
    std::string toString() const;
      
    /*! auto-cast this to a string, so we can compare it as
        'next()=="match"' without first having to write
        'next()->text=="match"' */
    operator std::string () const { return text; }
    
    inline const char *c_str() const { return text.c_str(); }

    const Loc         loc;
    const std::string text;
    const Type        type;
  };

#if 1
  struct TokenHandle : public std::shared_ptr<Token> {
    TokenHandle() = default;
    TokenHandle(std::shared_ptr<Token> handle) : std::shared_ptr<Token>(handle) {}
    TokenHandle(const TokenHandle &other) : std::shared_ptr<Token>(other) {}
    // TokenHandle(TokenHandle &&) = default;
    TokenHandle &operator=(const TokenHandle &) = default;
    
    inline operator std::string() const { return get()->text; }
  };
#else
  struct TokenHandle {
    TokenHandle() = default;
    TokenHandle(std::shared_ptr<Token> handle) : handle(handle) {}
    TokenHandle(const TokenHandle &) = default;
    TokenHandle(TokenHandle &&) = default;
    
    inline operator std::string() const { return handle->text; }
    inline operator std::shared_ptr<Token>() const { return handle; }
    inline operator bool() const { return (bool)handle; }
    std::shared_ptr<Token> handle;
  };
#endif
  

  /*! class that does the lexing - ie, the breaking up of an input
    stream of chars into an input stream of tokens.  */
  struct PBRT_PARSER_INTERFACE Lexer {

    //! constructor
    Lexer(const std::string &fn);

    TokenHandle next();
      
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

} // ::pbrt_parser
