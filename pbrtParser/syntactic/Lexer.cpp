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

#include "Lexer.h"
#include <sstream>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as geometries from objects or transforms,
    but does *not* make any difference between what types of
    geometries, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    // =======================================================
    // file
    // =======================================================
    File::File(const std::string &fn)
      : name(fn)
    {
      file = fopen(fn.c_str(),"r");
      if (!file)
        throw std::runtime_error("could not open file '"+fn+"'");
    }

    File::~File()
    { 
      if (file) fclose(file);
    }


    // =======================================================
    // loc
    // =======================================================

    //! constructor
    Loc::Loc(const std::shared_ptr<File> &file) : file(file), line(1), col(0) 
    {
    }
    
    // //! copy-constructor
    // Loc::Loc(const Loc &loc) : file(loc.file), line(loc.line), col(loc.col) 
    // {
    // }
    
    //! pretty-print
    std::string Loc::toString() const 
    {
      std::stringstream ss;
      ss << "@" << (file?file->getFileName():"<invalid>") << ":" << line << "." << col;
      return ss.str();
    }

    // =======================================================
    // token
    // =======================================================

    //! constructor
    Token::Token(const Loc &loc, 
                 const Type type,
                 const std::string &text) 
      : loc(loc), type(type), text(text) 
    {}

    //! pretty-print
    std::string Token::toString() const 
    {
      std::stringstream ss;
      ss << loc.toString() <<": '" << text << "'";
      return ss.str();
    }
    
    // =======================================================
    // Lexer
    // =======================================================

    //! constructor
    Lexer::Lexer(const std::string &fn)
      : file(new File(fn)), loc(file), peekedChar(-1) 
    {
    }

    inline void Lexer::unget_char(int c)
    {
      if (peekedChar >= 0) 
        throw std::runtime_error("can't push back more than one char ...");
      peekedChar = c;
    }

    inline int Lexer::get_char() 
    {
      if (peekedChar >= 0) {
        int c = peekedChar;
        peekedChar = -1;
        return c;
      }
        
      if (!file->file || feof(file->file)) {
        return -1;
      }
        
      int c = fgetc(file->file);
      int eol = '\n';
      if (c == '\n') {
        loc.line++;
        loc.col = 0;
      } else {
        loc.col++;
      }
      return c;
    };
      
    inline bool Lexer::isWhite(const char c)
    {
      return (c==' ' || c=='\n' || c=='\t' || c=='\r');
      // return strchr(" \t\n\r",c)!=nullptr;
    }
    inline bool Lexer::isSpecial(const char c)
    {
      return (c=='[' || c==',' || c==']');
      // return strchr("[,]",c)!=nullptr;
    }

    TokenHandle Lexer::next() 
    {
      // skip all white space and comments
      int c;

      // Loc startLoc = loc;
      // skip all whitespaces and comments
      while (1) {
        c = get_char();

        if (c < 0) { file->close(); return TokenHandle(); }
          
        if (isWhite(c)) {
          continue;
        }
          
        if (c == '#') {
          // startLoc = loc;
          // Loc lastLoc = loc;
          while (c != '\n') {
            // lastLoc = loc;
            c = get_char();
            if (c < 0) return TokenHandle();
          }
          continue;
        }
        break;
      }

      std::string s; s.reserve(100);
      std::stringstream ss(s);

    
      Loc startLoc = loc;
      // startLoc = loc;
      // Loc lastLoc = loc;
      if (c == '"') {
        while (1) {
          // lastLoc = loc;
          c = get_char();
          if (c < 0)
            throw std::runtime_error("could not find end of string literal (found eof instead)");
          if (c == '"') 
            break;
          ss << (char)c;
        }
        return std::make_shared<Token>(startLoc,Token::TOKEN_TYPE_STRING,ss.str());
      }

      // -------------------------------------------------------
      // special char
      // -------------------------------------------------------
      if (isSpecial(c)) {
        ss << (char)c;
        return std::make_shared<Token>(startLoc,Token::TOKEN_TYPE_SPECIAL,ss.str());
      }

      ss << (char)c;
      // cout << "START OF TOKEN at " << loc.toString() << endl;
      while (1) {
        // lastLoc = loc;
        c = get_char();
        if (c < 0)
          return std::make_shared<Token>(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
        if (c == '#' || isSpecial(c) || isWhite(c) || c=='"') {
          // cout << "END OF TOKEN AT " << lastLoc.toString() << endl;
          unget_char(c);
          return std::make_shared<Token>(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
        }
        ss << (char)c;
      }
    }

  } // ::pbrt::syntactic
} // ::pbrt
