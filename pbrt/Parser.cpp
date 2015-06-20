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

#include "parser.h"
// stl
#include <fstream>
#include <sstream>
#include <queue>
// std
#include <stdio.h>
#include <string.h>

namespace plib {
  namespace pbrt {
    using namespace std;

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
      Loc(Ref<File> file) : file(file), line(1), col(0) {}
      Loc(const Loc &loc) : file(loc.file), line(loc.line), col(loc.col) {}
      
      std::string toString() const { 
        std::stringstream ss;
        ss << "@" << file->name.str() << ":" << line << "." << col;
        return ss.str();
      }

      Ref<File> file;
      int line, col;
    };

    struct Token : public RefCounted {
      Token(const Loc &loc, const std::string &token) : loc(loc), token(token) {}

      std::string toString() const { 
        std::stringstream ss;
        ss << loc.toString() <<": " << token;
        return ss.str();
      }
      const Loc loc;
      const std::string token;
    };

    struct Tokenizer {

      Tokenizer(const FileName &fn)
        : file(new File(fn)), loc(file), peek(-1) 
      {
      }

      void unget_char(int c)
      {
        if (peek >= 0) 
          THROW_RUNTIME_ERROR("can't push back more than one char ...");
        peek = c;
      }

      int get_char() 
      {
        if (peek >= 0) {
          int c = peek;
          peek = -1;
          // PING; PRINT(c);
          return c;
        }
        
        if (!file->file || feof(file->file)) {
          // PING; 
          return -1;
        }
        
        int c = fgetc(file->file);
        int eol = '\n';
        // PRINT(eol);
        if (c == '\n') {
          // PING;
          // cout << "NEWLINE" << endl;
          loc.line++;
          loc.col = 0;
        } else {
          loc.col++;
        }
        // PING; PRINT(loc.toString()); PRINT(c); PRINT((char)c);
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

      Ref<Token> next() 
      {
        // PING;

        // skip all white space and comments
        int c;
        if (c < 0) return NULL;

        std::stringstream ss;

        Loc startLoc = loc;
        // PING;
        // skip all whitespaces and comments
        while (1) {
          // PING;
          c = get_char();
          // PRINT(c);
          // PRINT((char)c);

          if (c < 0) return NULL;
          
          if (isWhite(c)) {
            // PING; PRINT((char)c); PRINT(loc.toString());
            continue;
          }
          
          if (c == '#') {
            startLoc = loc;
            Loc lastLoc = loc;
            // std::cout << "start of comment at " << startLoc.toString() << std::endl;
            while (c != '\n') {
              lastLoc = loc;
              c = get_char();
              if (c < 0) return NULL;
            }
            // std::cout << "END of comment at " << lastLoc.toString() << std::endl;
            continue;
          }
          break;
        }

        // PING;
        // cout << "end of whitespace at " << loc.toString() << " c = " << (char)c << endl;
        // -------------------------------------------------------
        // STRING LITERAL
        // -------------------------------------------------------
        startLoc = loc;
        Loc lastLoc = loc;
        if (c == '"') {
          // cout << "START OF STRING at " << loc.toString() << endl;
          ss << (char)c;
          do {
            lastLoc = loc;
            c = get_char();
            if (c < 0)
              THROW_RUNTIME_ERROR("could not find end of string literal (found eof instead)");
            ss << (char)c;
          } while (c != '"');
          return new Token(startLoc,ss.str());
        }

        // -------------------------------------------------------
        // special char
        // -------------------------------------------------------
        if (isSpecial(c)) {
          ss << (char)c;
          return new Token(startLoc,ss.str());
        }

        ss << (char)c;
        // cout << "START OF TOKEN at " << loc.toString() << endl;
        while (1) {
          lastLoc = loc;
          c = get_char();
          if (c < 0)
            return new Token(startLoc,ss.str());
          if (c == '#' || isSpecial(c) || isWhite(c)) {
            // cout << "END OF TOKEN AT " << lastLoc.toString() << endl;
            // PRINT((char)c);
            unget_char(c);
            // PRINT(ss.str());
            return new Token(startLoc,ss.str());
          }
          ss << (char)c;
        }
        return NULL;
      }

      Ref<File> file;
      Loc loc;
      int peek;
    };

    /*! parse given file, and add it to the scene we hold */
    void Parser::parse(const FileName &fn)
    {
      Tokenizer tokenizer(fn);
      PING;
      Ref<Token> token;
      while ((token = tokenizer.next()))
        PRINT(token->toString());
      PING;
    }

  }
}
