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

#include "pbrt/common.h"
// stl
#include <queue>

namespace plib {
  namespace pbrt {

    struct File : public RefCounted {
      File(const FileName &fn)
        : name(fn)
      {
        file = fopen(fn.str().c_str(),"r");
      }

      FileName name;
      FILE *file;
    };

    struct Loc {
      Loc(Ref<File> file) : file(file), line(1), col(0) 
      {
        assert(file);
      }
      Loc(const Loc &loc) : file(loc.file), line(loc.line), col(loc.col) 
      {
        assert(loc.file);
      }
      
      std::string toString() const { 
        std::stringstream ss;
        assert(file);
        ss << "@" << file->name.str() << ":" << line << "." << col;
        return ss.str();
      }

      Ref<File> file;
      int line, col;
    };

    struct Token : public RefCounted {
      // static Ref<Token> TOKEN_EOF;

      typedef enum { TOKEN_TYPE_STRING, TOKEN_TYPE_LITERAL, TOKEN_TYPE_SPECIAL } Type;

      Token(const Loc &loc, 
            const Type type,
            const std::string &text) 
        : loc(loc), type(type), text(text) 
      {}
      
      inline operator std::string() const { return toString(); }
      inline const char *c_str() const { return text.c_str(); }

      std::string toString() const { 
        std::stringstream ss;
        ss << loc.toString() <<": " << text;
        return ss.str();
      }
      const Loc         loc;
      const std::string text;
      const Type        type;
    };

    // Ref<Token> Token::TOKEN_EOF = new Token(Loc(NULL),Token::TOKEN_TYPE_EOF,"<EOF>");

    struct Tokenizer {

      Tokenizer(const FileName &fn)
        : file(new File(fn)), loc(file), peekedChar(-1) 
      {
      }

      std::deque<Ref<Token> > peekedTokens;

      Loc getLastLoc() { return loc; }

      inline Ref<Token> peek(size_t i=0)
      {
        while (i >= peekedTokens.size())
          peekedTokens.push_back(produceNextToken());
        return peekedTokens[i];
      }
      
      inline void unget_char(int c)
      {
        if (peekedChar >= 0) 
          THROW_RUNTIME_ERROR("can't push back more than one char ...");
        peekedChar = c;
      }

      inline int get_char() 
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
      
      inline bool isWhite(const char c)
      {
        return strchr(" \t\n\r",c)!=NULL;
      }
      inline bool isSpecial(const char c)
      {
        return strchr("[,]",c)!=NULL;
      }

      inline Ref<Token> next() 
      {
        if (peekedTokens.empty())
          return produceNextToken();

        Ref<Token> token = peekedTokens.front();
        peekedTokens.pop_front();
        return token;
      }

      inline Ref<Token> produceNextToken() 
      {
        // skip all white space and comments
        int c;

        std::stringstream ss;

        Loc startLoc = loc;
        // skip all whitespaces and comments
        while (1) {
          c = get_char();

          if (c < 0) return NULL; //Token::TOKEN_EOF;
          
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
              if (c < 0) return NULL; //Token::TOKEN_EOF;
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
          return new Token(startLoc,Token::TOKEN_TYPE_STRING,ss.str());
        }

        // -------------------------------------------------------
        // special char
        // -------------------------------------------------------
        if (isSpecial(c)) {
          ss << (char)c;
          return new Token(startLoc,Token::TOKEN_TYPE_SPECIAL,ss.str());
        }

        ss << (char)c;
        // cout << "START OF TOKEN at " << loc.toString() << endl;
        while (1) {
          lastLoc = loc;
          c = get_char();
          if (c < 0)
            return new Token(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
          if (c == '#' || isSpecial(c) || isWhite(c) || c=='"') {
            // cout << "END OF TOKEN AT " << lastLoc.toString() << endl;
            unget_char(c);
            return new Token(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
          }
          ss << (char)c;
        }
      }

      Ref<File> file;
      Loc loc;
      int peekedChar;
    };

  }
}
