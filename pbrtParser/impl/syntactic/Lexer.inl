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

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
      
    // =======================================================
    // Lexer
    // =======================================================

    inline bool isWhite(const char c)
    {
      return (c==' ' || c=='\n' || c=='\t' || c=='\r');
      // return strchr(" \t\n\r",c)!=nullptr;
    }
    inline bool isSpecial(const char c)
    {
      return (c=='[' || c==',' || c==']');
      // return strchr("[,]",c)!=nullptr;
    }

    template <typename Buffer>
    Token BasicLexer<Buffer>::next() 
    {
      // skip all white space and comments
      int c;

      // Loc startLoc = loc;
      // skip all whitespaces and comments
      while (1) {
        c = buffer.get_char();

        if (c < 0) return Token();
          
        if (isWhite(c)) {
          continue;
        }
          
        if (c == '#') {
          // startLoc = loc;
          // Loc lastLoc = loc;
          while (c != '\n') {
            // lastLoc = loc;
            c = buffer.get_char();
            if (c < 0) return Token();
          }
          continue;
        }
        break;
      }


      ss.clear();
    
      Loc startLoc = buffer.get_loc();
      // startLoc = loc;
      // Loc lastLoc = loc;
      if (c == '"') {
        while (1) {
          // lastLoc = loc;
          c = buffer.get_char();
          if (c < 0)
            throw std::runtime_error("could not find end of string literal (found eof instead)");
          if (c == '"') 
            break;
          ss << (char)c;
        }
        return Token(startLoc,Token::TOKEN_TYPE_STRING,ss.str());
      }

      // -------------------------------------------------------
      // special char
      // -------------------------------------------------------
      if (isSpecial(c)) {
        ss << (char)c;
        return Token(startLoc,Token::TOKEN_TYPE_SPECIAL,ss.str());
      }

      ss << (char)c;
      // cout << "START OF TOKEN at " << loc.toString() << endl;
      while (1) {
        // lastLoc = loc;
        c = buffer.get_char();
        if (c < 0)
          return Token(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
        if (c == '#' || isSpecial(c) || isWhite(c) || c=='"') {
          // cout << "END OF TOKEN AT " << lastLoc.toString() << endl;
          buffer.unget_char(c);
          return Token(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
        }
        ss << (char)c;
      }
    }

  } // ::pbrt::syntactic
} // ::pbrt
