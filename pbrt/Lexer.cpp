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

#include "Lexer.h"
#include <sstream>

namespace pbrt_parser {

  // =======================================================
  // file
  // =======================================================
  File::File(const FileName &fn)
    : name(fn)
  {
    file = fopen(fn.str().c_str(),"r");
    if (!file)
      throw std::runtime_error("could not open file '"+fn.str()+"'");
  }

  // =======================================================
  // loc
  // =======================================================

  //! constructor
  Loc::Loc(std::shared_ptr<File> file) : file(file), line(1), col(0) 
  {
    assert(file);
  }
    
  //! copy-constructor
  Loc::Loc(const Loc &loc) : file(loc.file), line(loc.line), col(loc.col) 
  {
    assert(loc.file);
  }
    
  //! pretty-print
  std::string Loc::toString() const 
  {
    std::stringstream ss;
    assert(file);
    ss << "@" << file->getFileName() << ":" << line << "." << col;
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
    ss << loc.toString() <<": " << text;
    return ss.str();
  }
    
  // =======================================================
  // Lexer
  // =======================================================

  //! constructor
  Lexer::Lexer(const FileName &fn)
    : file(new File(fn)), loc(file), peekedChar(-1) 
  {
  }

  /*! produce the next token from the input stream; return nullptr if
    end of (all files) is reached */
  inline std::shared_ptr<Token> Lexer::produceNextToken() 
  {
    // skip all white space and comments
    int c;

    std::stringstream ss;

    Loc startLoc = loc;
    // skip all whitespaces and comments
    while (1) {
      c = get_char();

      if (c < 0) return std::shared_ptr<Token>();
          
      if (isWhite(c)) {
        continue;
      }
          
      if (c == '#') {
        startLoc = loc;
        Loc lastLoc = loc;
        // std::cout << "start of comment at " << startLoc.toString() << std::endl;
        while (c != '\n') {
          lastLoc = loc;
          c = get_char();
          if (c < 0) return std::shared_ptr<Token>();
        }
        // std::cout << "END of comment at " << lastLoc.toString() << std::endl;
        continue;
      }
      break;
    }

    startLoc = loc;
    Loc lastLoc = loc;
    if (c == '"') {
      // cout << "START OF STRING at " << loc.toString() << endl;
      while (1) {
        lastLoc = loc;
        c = get_char();
        if (c < 0)
          THROW_RUNTIME_ERROR("could not find end of string literal (found eof instead)");
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
      lastLoc = loc;
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

  std::shared_ptr<Token> Lexer::peek(size_t i)
  {
    while (i >= peekedTokens.size())
      peekedTokens.push_back(produceNextToken());
    return peekedTokens[i];
  }
      
  inline void Lexer::unget_char(int c)
  {
    if (peekedChar >= 0) 
      THROW_RUNTIME_ERROR("can't push back more than one char ...");
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
    return strchr(" \t\n\r",c)!=nullptr;
  }
  inline bool Lexer::isSpecial(const char c)
  {
    return strchr("[,]",c)!=nullptr;
  }

  std::shared_ptr<Token> Lexer::next() 
  {
    if (peekedTokens.empty())
      return produceNextToken();
      
    std::shared_ptr<Token> token = peekedTokens.front();
    peekedTokens.pop_front();
    return token;
  }

} // ::pbrt_parser
