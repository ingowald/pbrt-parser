// ======================================================================== //
// Copyright 2015-2019 Ingo Wald & Fabio Pellacini                          //
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

//
// This library code from code from the happly library.
//
// A header-only implementation of the .ply file format.
// https://github.com/nmwsharp/happly
// By Nicholas Sharp - nsharp@cs.cmu.edu
//
// MIT License
//
// Copyright (c) 2018 Nick Sharp
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#include "pbrt_parser.h"

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
      Loc(Loc &&) = default;

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

      //! valid token
      explicit operator bool() { return type != TOKEN_TYPE_NONE; }
    
      //! pretty-print
      std::string toString() const { return loc.toString() + ": '" + text + "'"; }
      
      const Loc         loc = {};
      const Type        type = TOKEN_TYPE_NONE;
      const std::string text = "";
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




/*! \file pbrt/Parser.h *Internal* parser class used by \see
  pbrt_parser::Scene::parseFromFile() - as end user, you should
  never have to look into this file directly, and only use \see
  pbrt_parser::Scene::parseFromFile() */

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




#include <fstream>

/*! \file BinaryIO.h Defines the (internal) binary reader/writer
    classes. No app should ever need to include this file, it is only
    for internal code cleanliness (having the class definitions
    separate from their implementation */

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    /*! a simple buffer for binary data. The writer will typically
        serialzie each object into its own buffer, thus allowing to
        'insert' other (referenced) entiries before the current one */
    struct OutBuffer : public std::vector<uint8_t>
    {
      typedef std::shared_ptr<OutBuffer> SP;
    };
  
    /*! helper class that writes out a PBRT scene graph in a binary form
      that is much faster to parse */
    struct BinaryWriter {

      BinaryWriter(const std::string &fileName) : binFile(fileName) {}

      /*! our stack of output buffers - each object we're writing might
        depend on other objects that it references in its paramset, so
        we use a stack of such buffers - every object writes to its
        own buffer, and if it needs to write another object before
        itself that other object can simply push a new buffer on the
        stack, and first write that one before the child completes its
        own writes. */
      std::stack<OutBuffer::SP> outBuffer;

      void writeRaw(const void *ptr, size_t size);
        
      template<typename T>
      void write(const T &t);
      
      void write(const std::string &t);
      
      template<typename T>
      void writeVec(const std::vector<T> &t);

      void writeVec(const std::vector<bool> &t);

      void writeVec(const std::vector<std::string> &t);
    
      /*! start a new write buffer on the stack - all future writes will go into this buffer */
      void startNewWrite();
    
      /*! write the topmost write buffer to disk, and free its memory */
      void executeWrite();
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Texture::SP texture);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Camera::SP camera);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Integrator::SP integrator);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Shape::SP shape);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Material::SP material);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(VolumeIntegrator::SP volumeIntegrator);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(SurfaceIntegrator::SP surfaceIntegrator);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Sampler::SP sampler);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Film::SP film);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(PixelFilter::SP pixelFilter);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Medium::SP medium);
    
      /*! if this instance has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Object::Instance::SP instance);
    
      /*! if this object has already been written to file, return a
        handle; else write it once, create a unique handle, and return
        that */
      int emitOnce(Object::SP object);

      /*! write an enitity's set of parameters */
      void writeParams(const ParamSet &ps);

      /*! write the root scene itself, by recusirvely writing all its entities */
      void emit(Scene::SP scene);
      
      /*! the file we'll be writing the buffers to */
      std::ofstream binFile;
    };



    /*! equivalent to the binary writer: read a given binary file,
        translating each written 'outbuffer' into a 'block', and then
        parse the respective entity within this block */
    struct BinaryReader {

      struct Block {
        inline Block() = default;
        inline Block(Block &&) = default;
      
        template<typename T>
        inline T read();
        inline void readRaw(void *ptr, size_t size);
        std::string readString();
        template<typename T>
        inline void readVec(std::vector<T> &v);
        inline void readVec(std::vector<std::string> &v);
        inline void readVec(std::vector<bool> &v);
        template<typename T>
        inline std::vector<T> readVec();
        
        std::vector<unsigned char> data;
        size_t pos = 0;
      };
    
      BinaryReader(const std::string &fileName);
      int readHeader();
      inline Block readBlock();
      template<typename T>
      typename ParamArray<T>::SP readParam(const std::string &type, Block &block);
      
      void readParams(ParamSet &paramSet, Block &block);
    
      Camera::SP            readCamera(Block &block);
      Integrator::SP        readIntegrator(Block &block);
      Shape::SP             readShape(Block &block);
      Material::SP          readMaterial(Block &block);
      Texture::SP           readTexture(Block &block);
      SurfaceIntegrator::SP readSurfaceIntegrator(Block &block);
      PixelFilter::SP       readPixelFilter(Block &block);
      VolumeIntegrator::SP  readVolumeIntegrator(Block &block);
      Sampler::SP           readSampler(Block &block);
      Film::SP              readFilm(Block &block);
      Object::SP            readObject(Block &block);
      Object::Instance::SP  readInstance(Block &block);
      Scene::SP             readScene();
      
      /*! our stack of output buffers - each object we're writing might
        depend on other objects that it references in its paramset, so
        we use a stack of such buffers - every object writes to its
        own buffer, and if it needs to write another object before
        itself that other object can simply push a new buffer on the
        stack, and first write that one before the child completes its
        own writes. */
      std::stack<OutBuffer::SP> outBuffer;

      /*! the file we'll be writing the buffers to */
      std::ifstream binFile;

    
      std::vector<Object::SP> objects;
      std::vector<Material::SP> materials;
      std::vector<Shape::SP> shapes;
      std::vector<Object::Instance::SP> instances;
      std::vector<Texture::SP> textures;
    
    
    
    };
  

  } // ::pbrt::syntactic
} // ::pbrt



#include <sstream>

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

    Token Lexer::next() 
    {
      // skip all white space and comments
      int c;

      // Loc startLoc = loc;
      // skip all whitespaces and comments
      while (1) {
        c = get_char();

        if (c < 0) { file->close(); return Token(); }
          
        if (isWhite(c)) {
          continue;
        }
          
        if (c == '#') {
          // startLoc = loc;
          // Loc lastLoc = loc;
          while (c != '\n') {
            // lastLoc = loc;
            c = get_char();
            if (c < 0) return Token();
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
        c = get_char();
        if (c < 0)
          return Token(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
        if (c == '#' || isSpecial(c) || isWhite(c) || c=='"') {
          // cout << "END OF TOKEN AT " << lastLoc.toString() << endl;
          unget_char(c);
          return Token(startLoc,Token::TOKEN_TYPE_LITERAL,ss.str());
        }
        ss << (char)c;
      }
    }

  } // ::pbrt::syntactic
} // ::pbrt



// stl
#include <fstream>
#include <sstream>
#include <stack>
// std
#include <stdio.h>
#include <string.h>

#define _unused(x) ((void)(x))

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {  
    int verbose = 0;

    inline bool operator==(const Token &tk, const std::string &text) { return tk.text == text; }
    inline bool operator==(const Token &tk, const char* text) { return tk.text == text; }
  
    /* split a string on a single character delimiter */
    inline std::vector<std::string> split(const std::string &input, char delim)
    {
      std::stringstream ss(input);
      std::string item;
      std::vector<std::string> elems;
      while (std::getline(ss, item, delim))
        elems.push_back(std::move(item));
      return elems;
    }

    /* split a string on a set of delimiters */
    inline std::vector<std::string> split(const std::string &input, 
                                          const std::string &delim)
    {
      std::vector<std::string> tokens;
      size_t pos = 0;
      while (1) {
        size_t begin = input.find_first_not_of(delim,pos);
        if (begin == input.npos) return tokens;
        size_t end = input.find_first_of(delim,begin);
        tokens.push_back(input.substr(begin,(end==input.npos)?input.npos:(end-begin)));
        pos = end;
      }
    }

  
    inline float Parser::parseFloat()
    {
      Token token = next();
      if (!token)
        throw std::runtime_error("unexpected end of file\n@"+std::string(__PRETTY_FUNCTION__));
      return (float)std::stod(token.text);
    }

    inline vec3f Parser::parseVec3f()
    {
      try {
        const float x = parseFloat();
        const float y = parseFloat();
        const float z = parseFloat();
        return vec3f(x,y,z);
      } catch (std::runtime_error e) {
        throw e.what()+std::string("\n@")+std::string(__PRETTY_FUNCTION__);
      }
    }

    affine3f Parser::parseMatrix()
    {
      const std::string open = next().text;

      assert(open == "[");
      affine3f xfm;
      xfm.l.vx.x = (float)std::stod(next().text.c_str());
      xfm.l.vx.y = (float)std::stod(next().text.c_str());
      xfm.l.vx.z = (float)std::stod(next().text.c_str());
      float vx_w = (float)std::stod(next().text.c_str());
      assert(vx_w == 0.f);
      _unused(vx_w);

      xfm.l.vy.x = (float)std::stod(next().text.c_str());
      xfm.l.vy.y = (float)std::stod(next().text.c_str());
      xfm.l.vy.z = (float)std::stod(next().text.c_str());
      float vy_w = (float)std::stod(next().text.c_str());
      assert(vy_w == 0.f);
      _unused(vy_w);

      xfm.l.vz.x = (float)std::stod(next().text.c_str());
      xfm.l.vz.y = (float)std::stod(next().text.c_str());
      xfm.l.vz.z = (float)std::stod(next().text.c_str());
      float vz_w = (float)std::stod(next().text.c_str());
      assert(vz_w == 0.f);
      _unused(vz_w);

      xfm.p.x    = (float)std::stod(next().text.c_str());
      xfm.p.y    = (float)std::stod(next().text.c_str());
      xfm.p.z    = (float)std::stod(next().text.c_str());
      float p_w  = (float)std::stod(next().text.c_str());
      assert(p_w == 1.f);
      _unused(p_w);

      const std::string close = next().text;
      assert(close == "]");

      return xfm;
    }


    inline std::shared_ptr<Param> Parser::parseParam(std::string &name)
    {
      Token token = peek();

      if (token.type != Token::TOKEN_TYPE_STRING)
        return std::shared_ptr<Param>();

      std::vector<std::string> components = split(next().text,std::string(" \n\t"));

      assert(components.size() == 2);
      std::string type = components[0];
      name = components[1];

      std::shared_ptr<Param> ret; 
      if (type == "float") {
        ret = std::make_shared<ParamArray<float>>(type);
      } else if (type == "color") {
        ret = std::make_shared<ParamArray<float> >(type);
      } else if (type == "blackbody") {
        ret = std::make_shared<ParamArray<float>>(type);
      } else if (type == "rgb") {
        ret = std::make_shared<ParamArray<float> >(type);
      } else if (type == "spectrum") {
        ret = std::make_shared<ParamArray<std::string>>(type);
      } else if (type == "integer") {
        ret = std::make_shared<ParamArray<int>>(type);
      } else if (type == "bool") {
        ret = std::make_shared<ParamArray<bool>>(type);
      } else if (type == "texture") {
        ret = std::make_shared<ParamArray<Texture>>(type);
      } else if (type == "normal") {
        ret = std::make_shared<ParamArray<float> >(type);
      } else if (type == "point") {
        ret = std::make_shared<ParamArray<float>>(type);
      } else if (type == "point2") {
        ret = std::make_shared<ParamArray<float>>(type);
      } else if (type == "point3") {
        ret = std::make_shared<ParamArray<float>>(type);
      } else if (type == "point4") {
        ret = std::make_shared<ParamArray<float>>(type);
      } else if (type == "vector") {
        ret = std::make_shared<ParamArray<float>>(type);
      } else if (type == "string") {
        ret = std::make_shared<ParamArray<std::string>>(type);
      } else {
        throw std::runtime_error("unknown parameter type '"+type+"' "+token.loc.toString()
                                 +std::string("\n@")+std::string(__PRETTY_FUNCTION__));
      }

      std::string value = next().text;
      if (value == "[") {
        std::string p = next().text;
        
        while (p != "]") {
          if (type == "texture") {
            std::dynamic_pointer_cast<ParamArray<Texture>>(ret)->texture 
              = getTexture(p);
          } else {
            ret->add(p);
          }
          p = next().text;
        }
      } else {
        if (type == "texture") {
          std::dynamic_pointer_cast<ParamArray<Texture>>(ret)->texture 
            = getTexture(value);
        } else {
          ret->add(value);
        }
      }
      return ret;
    }

    void Parser::parseParams(std::map<std::string, std::shared_ptr<Param> > &params)
    {
      while (1) {
        std::string name;
        std::shared_ptr<Param> param = parseParam(name);
        if (!param) return;
        params[name] = param;
      }
    }

    Attributes::Attributes()
    {
    }

    std::shared_ptr<Texture> Parser::getTexture(const std::string &name) 
    {
      if (attributesStack.top()->namedTexture.find(name) == attributesStack.top()->namedTexture.end())
        // throw std::runtime_error(lastLoc.toString()+": no texture named '"+name+"'");
        {
          std::cerr << "warning: could not find texture named '" << name << "'" << std::endl;
          return std::shared_ptr<Texture> ();
        }
      return attributesStack.top()->namedTexture[name]; 
    }

    Parser::Parser(const std::string &basePath) 
      : basePath(basePath), scene(std::make_shared<Scene>()), dbg(false)
    {
      ctm.reset();
      attributesStack.push(std::make_shared<Attributes>());
      objectStack.push(scene->world);//scene.cast<Object>());
    }

    std::shared_ptr<Object> Parser::getCurrentObject() 
    {
      if (objectStack.empty())
        throw std::runtime_error("no active object!?");
      return objectStack.top(); 
    }

    std::shared_ptr<Object> Parser::findNamedObject(const std::string &name, bool createIfNotExist)
    {
      if (namedObjects.find(name) == namedObjects.end()) {

        if (createIfNotExist) {
          std::shared_ptr<Object> object = std::make_shared<Object>(name);
          namedObjects[name] = object;
        } else {
          throw std::runtime_error("could not find object named '"+name+"'");
        }
      }
      return namedObjects[name];
    }


    void Parser::pushAttributes() 
    {
      attributesStack.push(std::make_shared<Attributes>(*attributesStack.top()));
      materialStack.push(currentMaterial);
      pushTransform();
      // setTransform(ospcommon::one);

    }

    void Parser::popAttributes() 
    {
      popTransform();
      attributesStack.pop();
      currentMaterial = materialStack.top();
      materialStack.pop();
    }
    
    void Parser::pushTransform() 
    {
      ctm.stack.push(ctm);
    }

    void Parser::popTransform() 
    {
      (Transform&)ctm = ctm.stack.top();
      ctm.stack.pop();
    }
    
    bool Parser::parseTransform(const Token& token)
    {
      if (token == "ActiveTransform") {
        const std::string which = next().text;
        if (which == "All") {
          ctm.startActive = true;
          ctm.endActive = true;
        } else if (which == "StartTime") {
          ctm.startActive = true;
          ctm.endActive = false;
        } else if (which == "EndTime") {
          ctm.startActive = false;
          ctm.endActive = true;
        } else
          throw std::runtime_error("unknown argument '"+which+"' to 'ActiveTransform' command");
          
        pushTransform();
        return true;
      }
      if (token == "TransformBegin") {
        pushTransform();
        return true;
      }
      if (token == "TransformEnd") {
        popTransform();
        return true;
      }
      if (token == "Scale") {
        vec3f scale = parseVec3f();
        addTransform(affine3f::scale(scale));
        return true;
      }
      if (token == "Translate") {
        vec3f translate = parseVec3f();
        addTransform(affine3f::translate(translate));
        return true;
      }
      if (token == "ConcatTransform") {
        addTransform(parseMatrix());
        return true;
      }
      if (token == "Rotate") {
        const float angle = parseFloat();
        const vec3f axis  = parseVec3f();
        addTransform(affine3f::rotate(axis,angle*M_PI/180.f));
        return true;
      }
      if (token == "Transform") {
        next(); // '['
        affine3f xfm;
        xfm.l.vx = parseVec3f(); next();
        xfm.l.vy = parseVec3f(); next();
        xfm.l.vz = parseVec3f(); next();
        xfm.p    = parseVec3f(); next();
        next(); // ']'
        addTransform(xfm);
        return true;
      }
      if (token == "ActiveTransform") {
        std::string time = next().text;
        std::cout << "'ActiveTransform' not implemented" << std::endl;
        return true;
      }
      if (token == "Identity") {
          setTransform(affine3f::identity());
        return true;
      }
      if (token == "ReverseOrientation") {
        /* according to the docs, 'ReverseOrientation' only flips the
           normals, not the actual transform */
        return true;
      }
      if (token == "CoordSysTransform") {
        Token nameOfObject = next();
        std::cout << "ignoring 'CoordSysTransform'" << std::endl;
        return true;
      }
      return false;
    }

    void Parser::parseWorld()
    {
      if (dbg) std::cout << "Parsing PBRT World" << std::endl;
      while (1) {
        Token token = next();
        assert(token);
        if (dbg) std::cout << "World token : " << token.toString() << std::endl;

        // ------------------------------------------------------------------
        // WorldEnd - go back to regular parseScene
        // ------------------------------------------------------------------
        if (token == "WorldEnd") {
          if (dbg) std::cout << "Parsing PBRT World - done!" << std::endl;
          break;
        }
      
        // -------------------------------------------------------
        // LightSource
        // -------------------------------------------------------
        if (token == "LightSource") {
          std::shared_ptr<LightSource> lightSource
            = std::make_shared<LightSource>(next().text);
          parseParams(lightSource->param);
          getCurrentObject()->lightSources.push_back(lightSource);
          continue;
        }

        // ------------------------------------------------------------------
        // AreaLightSource
        // ------------------------------------------------------------------
        if (token == "AreaLightSource") {
          std::shared_ptr<AreaLightSource> lightSource
            = std::make_shared<AreaLightSource>(next().text);
          parseParams(lightSource->param);
          continue;
        }

        // -------------------------------------------------------
        // Material
        // -------------------------------------------------------
        if (token == "Material") {
          std::string type = next().text;
          std::shared_ptr<Material> material
            = std::make_shared<Material>(type);
          parseParams(material->param);
          currentMaterial = material;
          material->attributes = attributesStack.top();
          continue;
        }

        // ------------------------------------------------------------------
        // Texture
        // ------------------------------------------------------------------
        if (token == "Texture") {
          std::string name = next().text;
          std::string texelType = next().text;
          std::string mapType = next().text;
          std::shared_ptr<Texture> texture
            = std::make_shared<Texture>(name,texelType,mapType);
          attributesStack.top()->namedTexture[name] = texture;
          texture->attributes = attributesStack.top();
          parseParams(texture->param);
          continue;
        }

        // ------------------------------------------------------------------
        // MakeNamedMaterial
        // ------------------------------------------------------------------
        if (token == "MakeNamedMaterial") {        
          std::string name = next().text;
          std::shared_ptr<Material> material
            = std::make_shared<Material>("<implicit>");
          attributesStack.top()->namedMaterial[name] = material;
          // scene->namedMaterials[name] = material;
          parseParams(material->param);
          material->attributes = attributesStack.top();
          
          /* named material have the parameter type implicitly as a
             parameter rather than explicitly on the
             'makenamedmaterial' command; so let's parse this here */
          std::shared_ptr<Param> type = material->param["type"];
          if (!type) throw std::runtime_error("named material that does not specify a 'type' parameter!?");
          std::shared_ptr<ParamArray<std::string>> asString
            = std::dynamic_pointer_cast<ParamArray<std::string> >(type);
          if (!asString)
            throw std::runtime_error("named material has a type, but not a string!?");
          assert(asString->getSize() == 1);
          material->type = asString->get(0); //paramVec[0];
          continue;
        }

        // ------------------------------------------------------------------
        // MakeNamedMedium
        // ------------------------------------------------------------------
        if (token == "MakeNamedMedium") {
          std::string name = next().text;
          std::shared_ptr<Medium> medium
            = std::make_shared<Medium>("<implicit>");
          attributesStack.top()->namedMedium[name] = medium;
          parseParams(medium->param);

          /* named medium have the parameter type implicitly as a
             parameter rather than explicitly on the
             'makenamedmedium' command; so let's parse this here */
          std::shared_ptr<Param> type = medium->param["type"];
          if (!type) throw std::runtime_error("named medium that does not specify a 'type' parameter!?");
          std::shared_ptr<ParamArray<std::string>> asString
            = std::dynamic_pointer_cast<ParamArray<std::string> >(type);
          if (!asString)
            throw std::runtime_error("named medium has a type, but not a string!?");
          assert(asString->getSize() == 1);
          medium->type = asString->get(0); //paramVec[0];
          continue;
        }

        // ------------------------------------------------------------------
        // NamedMaterial
        // ------------------------------------------------------------------
        if (token == "NamedMaterial") {
          std::string name = next().text;
        
          currentMaterial = attributesStack.top()->namedMaterial[name];

          // std::cout << "#NamedMaterial " << name << " : " << (currentMaterial?currentMaterial->toString():std::string("null")) << std::endl;
        
          continue;
        }

        // ------------------------------------------------------------------
        // MakeNamedMedium
        // ------------------------------------------------------------------
        if (token == "MakeNamedMedium") {
          std::string name = next().text;
          std::shared_ptr<Medium> medium
            = std::make_shared<Medium>("<implicit>");
          attributesStack.top()->namedMedium[name] = medium;
          parseParams(medium->param);

          /* named medium have the parameter type implicitly as a
             parameter rather than explicitly on the
             'makenamedmedium' command; so let's parse this here */
          std::shared_ptr<Param> type = medium->param["type"];
          if (!type) throw std::runtime_error("named medium that does not specify a 'type' parameter!?");
          ParamArray<std::string>::SP asString = type->as<std::string>();
          if (!asString)
            throw std::runtime_error("named medium has a type, but not a string!?");
          assert(asString->getSize() == 1);
          medium->type = asString->get(0); //paramVec[0];
          continue;
        }

        // ------------------------------------------------------------------
        // MediumInterface
        // ------------------------------------------------------------------
        if (token == "MediumInterface") {
          attributesStack.top()->mediumInterface.first = next().text;
          attributesStack.top()->mediumInterface.second = next().text;
          continue;
        }

        // -------------------------------------------------------
        // AttributeBegin
        // -------------------------------------------------------
        if (token == "AttributeBegin") {
          pushAttributes();
          continue;
        }

        // -------------------------------------------------------
        // AttributeEnd
        // -------------------------------------------------------
        if (token == "AttributeEnd") {
          popAttributes();
          continue;
        }

        // -------------------------------------------------------
        // Shape
        // -------------------------------------------------------
        if (token == "Shape") {
          // if (!currentMaterial) {
          //   std::cout << "warning(pbrt_parser): shape, but no current material!" << std::endl;
          // }
          std::shared_ptr<Shape> shape
            = std::make_shared<Shape>(next().text,
                                      currentMaterial,
                                      attributesStack.top()->clone(),
                                      ctm);
          parseParams(shape->param);
          getCurrentObject()->shapes.push_back(shape);
          continue;
        }
      
        // -------------------------------------------------------
        // Volumes
        // -------------------------------------------------------
        if (token == "Volume") {
          std::shared_ptr<Volume> volume
            = std::make_shared<Volume>(next().text);
          parseParams(volume->param);
          getCurrentObject()->volumes.push_back(volume);
          continue;
        }

        // -------------------------------------------------------
        // Transform
        // -------------------------------------------------------
        if (parseTransform(token))
          continue;
        
        // -------------------------------------------------------
        // ObjectBegin
        // -------------------------------------------------------
        if (token == "ObjectBegin") {
          std::string name = next().text;
          std::shared_ptr<Object> object = findNamedObject(name,1);

          objectStack.push(object);
          continue;
        }
          
        // -------------------------------------------------------
        // ObjectEnd
        // -------------------------------------------------------
        if (token == "ObjectEnd") {
          objectStack.pop();
          continue;
        }

        // -------------------------------------------------------
        // ObjectInstance
        // -------------------------------------------------------
        if (token == "ObjectInstance") {
          std::string name = next().text;
          std::shared_ptr<Object> object = findNamedObject(name,1);
          std::shared_ptr<Object::Instance> inst
            = std::make_shared<Object::Instance>(object,ctm);
          getCurrentObject()->objectInstances.push_back(inst);
          if (verbose)
            std::cout << "adding instance " << inst->toString()
                 << " to object " << getCurrentObject()->toString() << std::endl;
          continue;
        }
          
        // -------------------------------------------------------
        // ERROR - unrecognized token in worldbegin/end!!!
        // -------------------------------------------------------
        throw std::runtime_error("unexpected token '"+token.toString()
                                 +"' at "+token.loc.toString());
      }
    }

    Token Parser::next()
    {
      Token token = peek();
      if (!token)
        throw std::runtime_error("unexpected end of file ...");
      peekQueue.pop_front();
      // lastLoc = token.loc;
      return token;
    }
    
    Token Parser::peek(unsigned int i)
    {
      while (peekQueue.size() <= i) {
        Token token = tokens->next();
        // first, handle the 'Include' statement by inlining such files
        if (token && token == "Include") {
          Token fileNameToken = tokens->next();
          std::string includedFileName = fileNameToken.text;
          if (includedFileName[0] != '/') {
            includedFileName = rootNamePath+"/"+includedFileName;
          }
          // if (dbg)
          std::cout << "... including file '" << includedFileName << " ..." << std::endl;
        
          tokenizerStack.push(tokens);
          tokens = std::make_shared<Lexer>(includedFileName);
          continue;
        }
      
        if (token) {
          peekQueue.push_back(token);
          continue;
        }
      
        // last token was invalid, so encountered at least one end of
        // file - see if we can pop back to another one off the stack
        if (tokenizerStack.empty())
          // nothing to back off to, return eof indicator
          return Token();
      
        tokens = tokenizerStack.top();
        tokenizerStack.pop();
        // token = next();
        continue;
      }
      return peekQueue[i];
    }
    
    void Parser::parseScene()
    {
      while (peek()) {
      
        Token token = next();
        if (!token) break;

        if (dbg) std::cout << token.toString() << std::endl;

        // -------------------------------------------------------
        // Transform
        // -------------------------------------------------------
        if (parseTransform(token))
          continue;
        
        if (token == "LookAt") {
          vec3f v0 = parseVec3f();
          vec3f v1 = parseVec3f();
          vec3f v2 = parseVec3f();
        
          // scene->lookAt = std::make_shared<LookAt>(v0,v1,v2);
          affine3f xfm;
          xfm.l.vz = normalize(v1-v0);
          xfm.l.vx = normalize(cross(xfm.l.vz,v2));
          xfm.l.vy = cross(xfm.l.vx,xfm.l.vz);
          xfm.p    = v0;
        
          addTransform(inverse(xfm));
          continue;
        }

        if (token == "ConcatTransform") {
          next(); // '['
          float mat[16];
          for (int i=0;i<16;i++)
            mat[i] = std::stod(next().text);

          affine3f xfm;
          xfm.l.vx = vec3f(mat[0],mat[1],mat[2]);
          xfm.l.vy = vec3f(mat[4],mat[5],mat[6]);
          xfm.l.vz = vec3f(mat[8],mat[9],mat[10]);
          xfm.p    = vec3f(mat[12],mat[13],mat[14]);
          addTransform(xfm);

          next(); // ']'
          continue;
        }
        if (token == "CoordSysTransform") {
          std::string transformType = next().text;
          continue;
        }


        if (token == "ActiveTransform") {
          std::string type = next().text;
          continue;
        }

        if (token == "Camera") {
          std::shared_ptr<Camera> camera = std::make_shared<Camera>(next().text,ctm);
          parseParams(camera->param);
          scene->cameras.push_back(camera);
          continue;
        }
        if (token == "Sampler") {
          std::shared_ptr<Sampler> sampler = std::make_shared<Sampler>(next().text);
          parseParams(sampler->param);
          scene->sampler = sampler;
          continue;
        }
        if (token == "Integrator") {
          std::shared_ptr<Integrator> integrator = std::make_shared<Integrator>(next().text);
          parseParams(integrator->param);
          scene->integrator = integrator;
          continue;
        }
        if (token == "SurfaceIntegrator") {
          std::shared_ptr<SurfaceIntegrator> surfaceIntegrator
            = std::make_shared<SurfaceIntegrator>(next().text);
          parseParams(surfaceIntegrator->param);
          scene->surfaceIntegrator = surfaceIntegrator;
          continue;
        }
        if (token == "VolumeIntegrator") {
          std::shared_ptr<VolumeIntegrator> volumeIntegrator
            = std::make_shared<VolumeIntegrator>(next().text);
          parseParams(volumeIntegrator->param);
          scene->volumeIntegrator = volumeIntegrator;
          continue;
        }
        if (token == "PixelFilter") {
          std::shared_ptr<PixelFilter> pixelFilter = std::make_shared<PixelFilter>(next().text);
          parseParams(pixelFilter->param);
          scene->pixelFilter = pixelFilter;
          continue;
        }
        if (token == "Accelerator") {
          std::shared_ptr<Accelerator> accelerator = std::make_shared<Accelerator>(next().text);
          parseParams(accelerator->param);
          continue;
        }
        if (token == "Film") {
          scene->film = std::make_shared<Film>(next().text);
          parseParams(scene->film->param);
          continue;
        }
        if (token == "Accelerator") {
          std::shared_ptr<Accelerator> accelerator = std::make_shared<Accelerator>(next().text);
          parseParams(accelerator->param);
          continue;
        }
        if (token == "Renderer") {
          std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(next().text);
          parseParams(renderer->param);
          continue;
        }

        if (token == "WorldBegin") {
          ctm.reset();
          parseWorld();
          continue;
        }

        // ------------------------------------------------------------------
        // MediumInterface
        // ------------------------------------------------------------------
        if (token == "MediumInterface") {
          attributesStack.top()->mediumInterface.first = next().text;
          attributesStack.top()->mediumInterface.second = next().text;
          continue;
        }

        // ------------------------------------------------------------------
        // MakeNamedMedium
        // ------------------------------------------------------------------
        if (token == "MakeNamedMedium") {
          std::string name = next().text;
          std::shared_ptr<Medium> medium
            = std::make_shared<Medium>("<implicit>");
          attributesStack.top()->namedMedium[name] = medium;
          parseParams(medium->param);

          /* named medium have the parameter type implicitly as a
             parameter rather than explicitly on the
             'makenamedmedium' command; so let's parse this here */
          std::shared_ptr<Param> type = medium->param["type"];
          if (!type) throw std::runtime_error("named medium that does not specify a 'type' parameter!?");
          std::shared_ptr<ParamArray<std::string>> asString
            = std::dynamic_pointer_cast<ParamArray<std::string> >(type);
          if (!asString)
            throw std::runtime_error("named medium has a type, but not a string!?");
          assert(asString->getSize() == 1);
          medium->type = asString->get(0); //paramVec[0];
          continue;
        }

        if (token == "Material") {
          throw std::runtime_error("'Material' field not within a WorldBegin/End context. "
                                   "Did you run the parser on the 'shape.pbrt' file directly? "
                                   "(you shouldn't - it should only be included from within a "
                                   "pbrt scene file - typically '*.view')");
          continue;
        }

        throw std::runtime_error("unexpected token '"+token.text
                                 +"' at "+token.loc.toString());
      }
    }


#ifdef _WIN32
    const char path_sep = '\\';
#else
    const char path_sep = '/';
#endif

    std::string pathOf(const std::string &fn)
    {
      size_t pos = fn.find_last_of(path_sep);
      if (pos == std::string::npos) return std::string();
      return fn.substr(0,pos+1);
    }


    /*! parse given file, and add it to the scene we hold */
    void Parser::parse(const std::string &fn)
    {
      rootNamePath
        = basePath==""
        ? (std::string)pathOf(fn)
        : (std::string)basePath;
      this->tokens = std::make_shared<Lexer>(fn);
      parseScene();
      scene->basePath = rootNamePath;
    }

    
  } // ::pbrt::syntx
} // ::pbrt



// std
#include <iostream>
#include <sstream>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a shape that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    /*! parse the given file name, return parsed scene */
    std::shared_ptr<Scene> Scene::parseFromFile(const std::string &fileName)
    {
      std::shared_ptr<Parser> parser = std::make_shared<Parser>();
      parser->parse(fileName);
      return parser->getScene();
    }
    

    std::string Object::toString(int depth) const 
    { 
      std::stringstream ss;
      ss << "Object '"<<name<<"' : "<<shapes.size()<<"shps, " << objectInstances.size() << "insts";
      if (depth > 0) {
        ss << " shapes:" << std::endl;
        for (auto &shape : shapes)
          ss << " - " << shape->type << std::endl;
        ss << " insts:" << std::endl;
        for (auto &inst : objectInstances)
          ss << " - " << inst->object->toString(depth-1) << std::endl;
      }
      return ss.str();
    }

    std::string Object::Instance::toString() const
    { 
      std::stringstream ss;
      ss << "Inst: " << object->toString() << " xfm " << (math::affine3f&)xfm.atStart; 
      return ss.str();
    }

    // ==================================================================
    // Param
    // ==================================================================
    template<> void ParamArray<float>::add(const std::string &text)
    { this->push_back(atof(text.c_str())); }

    template<> void ParamArray<int>::add(const std::string &text)
    { this->push_back(atoi(text.c_str())); }

    template<> void ParamArray<std::string>::add(const std::string &text)
    { this->push_back(text); }

    template<> void ParamArray<bool>::add(const std::string &text)
    { 
      if (text == "true")
        this->push_back(true); 
      else if (text == "false")
        this->push_back(false); 
      else
        throw std::runtime_error("invalid value '"+text+"' for bool parameter");
    }


    template<> std::string ParamArray<float>::toString() const
    { 
      std::stringstream ss;
      ss << getType() << " ";
      ss << "[ ";
      for (size_t i=0;i<this->size();i++)
        ss << get(i) << " ";
      ss << "]";
      return ss.str();
    }

    std::string ParamArray<Texture>::toString() const
    { 
      std::stringstream ss;
      ss << getType() << " ";
      ss << "[ ";
      ss << "<TODO>";
      ss << "]";
      return ss.str();
    }

    template<> std::string ParamArray<std::string>::toString() const
    { 
      std::stringstream ss;
      ss << getType() << " ";
      ss << "[ ";
      for (size_t i=0;i<this->size();i++)
        ss << get(i) << " ";
      ss << "]";
      return ss.str();
    }

    template<> std::string ParamArray<int>::toString() const
    { 
      std::stringstream ss;
      ss << getType() << " ";
      ss << "[ ";
      for (size_t i=0;i<this->size();i++)
        ss << get(i) << " ";
      ss << "]";
      return ss.str();
    }

    template<> std::string ParamArray<bool>::toString() const
    { 
      std::stringstream ss;
      ss << getType() << " ";
      ss << "[ ";
      for (size_t i=0;i<this->size();i++)
        ss << get(i) << " ";
      ss << "]";
      return ss.str();
    }

    template struct ParamArray<float>;
    template struct ParamArray<int>;
    template struct ParamArray<bool>;
    template struct ParamArray<std::string>;

    // ==================================================================
    // ParamSet
    // ==================================================================
    bool ParamSet::getParam3f(float *result, const std::string &name) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return 0;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() != 3)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (3f, name='"+name+"'");
      result[0] = p->get(0);
      result[1] = p->get(1);
      result[2] = p->get(2);
      return true;
    }

    vec3f ParamSet::getParam3f(const std::string &name, const vec3f &fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("3f: found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() != 3)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (3f, name='"+name+"'");
      return vec3f(p->get(0),p->get(1),p->get(2));
    }

    float ParamSet::getParam1f(const std::string &name, const float fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<float>> p = std::dynamic_pointer_cast<ParamArray<float>>(pr);
      if (!p)
        throw std::runtime_error("1f: found param of given name, but of wrong type! (name was '"+name+"'");
      if (p->getSize() != 1)
        throw std::runtime_error("found param of given name and type, but wrong number of components!");
      return p->get(0);
    }

    int ParamSet::getParam1i(const std::string &name, const int fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<int>> p = std::dynamic_pointer_cast<ParamArray<int>>(pr);
      if (!p)
        throw std::runtime_error("1i: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1) {
        throw std::runtime_error("found param of given name and type, but wrong number of components! (1i, name='"+name+"'");
      }
      return p->get(0);
    }

    std::string ParamSet::getParamString(const std::string &name) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return "";
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<std::string>> p = std::dynamic_pointer_cast<ParamArray<std::string>>(pr);
      if (!p)
        throw std::runtime_error("str: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (str, name='"+name+"'");
      return p->get(0);
    }

    std::shared_ptr<Texture> ParamSet::getParamTexture(const std::string &name) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return std::shared_ptr<Texture>();
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<Texture>> p = std::dynamic_pointer_cast<ParamArray<Texture>>(pr);
      if (!p)
        throw std::runtime_error("tex: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (tex, name='"+name+"'");
      return p->texture;
    }

    bool ParamSet::getParamBool(const std::string &name, const bool fallBack) const
    {
      std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.find(name);
      if (it == param.end())
        return fallBack;
      std::shared_ptr<Param> pr = it->second;
      const std::shared_ptr<ParamArray<bool>> p = std::dynamic_pointer_cast<ParamArray<bool>>(pr);
      if (!p)
        throw std::runtime_error("bool: found param of given name ("+name+"), but of wrong type!");
      if (p->getSize() != 1)
        throw std::runtime_error("found param of given name and type, but wrong number of components! (bool, name='"+name+"'");
      return p->get(0);
    }

    // ==================================================================
    // Shape
    // ==================================================================

    /*! constructor */
    Shape::Shape(const std::string &type,
                 std::shared_ptr<Material>   material,
                 std::shared_ptr<Attributes> attributes,
                 const Transform &transform) 
      : Node(type), 
        material(material),
        attributes(attributes),
        transform(transform)
    {};

    // ==================================================================
    // Material
    // ==================================================================
    std::string Material::toString() const
    {
      std::stringstream ss;
      ss << "Material type='"<< type << "' {" << std::endl;
      for (std::map<std::string,std::shared_ptr<Param> >::const_iterator it=param.begin(); 
           it != param.end(); it++) {
        ss << " - " << it->first << " : " << it->second->toString() << std::endl;
      }
      ss << "}" << std::endl;
      return ss.str();
    }

    extern "C" PBRT_PARSER_INTERFACE
    void pbrt_syntactic_parse(pbrt::syntactic::Scene::SP &scene,
                            const std::string &fileName)
    {
      scene = pbrt::syntactic::Scene::parseFromFile(fileName);
    }
    
  } // ::pbrt::syntactic
} // ::pbrt



// std
#include <set>
#include <string.h>
#include <algorithm>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for final, *semantic* parser - unlike the syntactic
    parser, the semantic one will distinguish between different
    matieral types, differnet shape types, etc, and it will not only
    store 'name:value' pairs for parameters, but figure out which
    parameters which shape etc have, parse them from the
    parameters, etc */
  namespace semantic {
  
    box3f Shape::getPrimBounds(const size_t primID)
    {
      return getPrimBounds(primID,affine3f::identity());
    }


    box3f TriangleMesh::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f primBounds = box3f::empty_box();
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].x]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].y]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].z]));
      return primBounds;
    }
    
    box3f TriangleMesh::getPrimBounds(const size_t primID) 
    {
      box3f primBounds = box3f::empty_box();
      primBounds.extend(vertex[index[primID].x]);
      primBounds.extend(vertex[index[primID].y]);
      primBounds.extend(vertex[index[primID].z]);
      return primBounds;
    }
    
    box3f TriangleMesh::getBounds() 
    {
      if (!haveComputedBounds) {
        std::lock_guard<std::mutex> lock(mutex);
        if (haveComputedBounds) return bounds;
        bounds = box3f::empty_box();
        for (auto v : vertex) bounds.extend(v);
        haveComputedBounds = true;
        return bounds;
      }
      return bounds;
    }




    box3f Sphere::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f ob(vec3f(-radius),vec3f(+radius));
      affine3f _xfm = xfm * transform;
      
      box3f bounds = box3f::empty_box();
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.upper.y,ob.upper.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.upper.y,ob.upper.z)));
      return bounds;
    }
    
    box3f Sphere::getPrimBounds(const size_t primID) 
    {
      return getPrimBounds(primID,affine3f::identity());
    }
    
    box3f Sphere::getBounds() 
    {
      return getPrimBounds(0);
    }
    





    box3f Disk::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f ob(vec3f(-radius,-radius,0),vec3f(+radius,+radius,height));
      affine3f _xfm = xfm * transform;
      
      box3f bounds = box3f::empty_box();
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.lower.x,ob.upper.y,ob.upper.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(_xfm,vec3f(ob.upper.x,ob.upper.y,ob.upper.z)));
      return bounds;
    }
    
    box3f Disk::getPrimBounds(const size_t primID) 
    {
      return getPrimBounds(primID,affine3f::identity());
    }
    
    box3f Disk::getBounds() 
    {
      return getPrimBounds(0);
    }
    



    
    // ==================================================================
    // QuadMesh
    // ==================================================================

    box3f QuadMesh::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f primBounds = box3f::empty_box();
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].x]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].y]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].z]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].w]));
      return primBounds;
    }

    box3f QuadMesh::getPrimBounds(const size_t primID) 
    {
      box3f primBounds = box3f::empty_box();
      primBounds.extend(vertex[index[primID].x]);
      primBounds.extend(vertex[index[primID].y]);
      primBounds.extend(vertex[index[primID].z]);
      primBounds.extend(vertex[index[primID].w]);
      return primBounds;
    }
    
    box3f QuadMesh::getBounds() 
    {
      if (!haveComputedBounds) {
        std::lock_guard<std::mutex> lock(mutex);
        if (haveComputedBounds) return bounds;
        bounds = box3f::empty_box();
        for (auto v : vertex) bounds.extend(v);
        haveComputedBounds = true;
        return bounds;
      }
      return bounds;
    }




    // ==================================================================
    // Curve
    // ==================================================================

    box3f Curve::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f primBounds = box3f::empty_box();
      for (auto p : P)
        primBounds.extend(xfmPoint(xfm,p));
      vec3f maxWidth = vec3f(std::max(width0,width1));
      primBounds.lower = primBounds.lower - maxWidth;
      primBounds.upper = primBounds.upper + maxWidth;
      return primBounds;
    }

    box3f Curve::getPrimBounds(const size_t primID) 
    {
      box3f primBounds = box3f::empty_box();
      for (auto p : P)
        primBounds.extend(p);
      vec3f maxWidth = vec3f(std::max(width0,width1));
      primBounds.lower = primBounds.lower - maxWidth;
      primBounds.upper = primBounds.upper + maxWidth;
      return primBounds;
    }
    
    box3f Curve::getBounds() 
    {
      return getPrimBounds(0);
    }


    // ==================================================================
    // Object
    // ==================================================================

    box3f Object::getBounds() const
    {
      box3f bounds = box3f::empty_box();
      for (auto inst : instances) {
        if (inst) {
          const box3f ib = inst->getBounds();
          if (!ib.empty())
            bounds.extend(ib);
        }
      }
      for (auto geom : shapes) {
        if (geom) {
          const box3f gb = geom->getBounds();
          if (!gb.empty())
            bounds.extend(gb);
        }
      }
      return bounds;
    };


    /*! compute (conservative but possibly approximate) bbox of this
      instance in world space. This box is not necessarily tight, as
      it getsc omputed by transforming the object's bbox */
    box3f Instance::getBounds() const
    {
      box3f ob = object->getBounds();
      if (ob.empty()) 
        return ob;

      box3f bounds = box3f::empty_box();
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.upper.y,ob.upper.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.upper.y,ob.upper.z)));
      return bounds;
    };
    
    /* compute some _rough_ storage cost esimate for a scene. this will
       allow bricking builders to greedily split only the most egregious
       objects */
    double computeApproximateStorageWeight(Scene::SP scene)
    {
      /*! @{ "guessed" guess-timates of storage cost for a primitive and
        a instance, respectively. For prim, assume 12ish bytes index,
        12ish vertex, 32-ish bvh cost, and a bit for shading data -
        say a round 100?. For a instance, at least the transforms,
        plus bvh overhead, etc .... 4k?. Eventually could/should do
        some fitting of these parameters, and probably make it
        primtype- and material/shading-data dependent (textures ...),
        but for now any approximateion will do */
      static const int primWeight = 100.f;
      static const int instWeight = 4000.f;
      static const int geomWeight = 4000.f;
    
      std::set<Shape::SP> activeShapes;
      for (auto inst : scene->world->instances) 
        for (auto geom : inst->object->shapes) 
          activeShapes.insert(geom);

      double costEstimate
        = scene->world->instances.size() * instWeight
        + activeShapes.size() * geomWeight;
      for (auto geom : activeShapes)
        costEstimate += geom->getNumPrims() * primWeight;
      return costEstimate;
    }

    struct FatVertex {
      vec3f p, n;
    };
    inline bool operator<(const FatVertex &a, const FatVertex &b)
    { return memcmp(&a,&b,sizeof(a))<0; }
    
    QuadMesh::SP remeshVertices(QuadMesh::SP in)
    {
      QuadMesh::SP out = std::make_shared<QuadMesh>(in->material);
      out->textures = in->textures;

      std::vector<int> indexRemap(in->vertex.size());
      std::map<FatVertex,int> vertexID;
      for (size_t i=0;i<in->vertex.size();i++) {
        FatVertex oldVertex;
        oldVertex.p = in->vertex[i];
        oldVertex.n = in->normal.empty()?vec3f(0.f):in->normal[i];

        auto it = vertexID.find(oldVertex);
        if (it == vertexID.end()) {
          int newID = out->vertex.size();
          out->vertex.push_back(in->vertex[i]);
          if (!in->normal.empty())
            out->normal.push_back(in->normal[i]);
          // if (!in->texcoord.empty())
          //   out->texcoord.push_back(in->texcoord[i]);
          vertexID[oldVertex] = newID;
          indexRemap[i] = newID;
        } else {
          indexRemap[i] = it->second;
        }
      }
      for (auto idx : in->index) 
        out->index.push_back(vec4i(idx.x < 0 ? -1 : indexRemap[idx.x],
                                   idx.y < 0 ? -1 : indexRemap[idx.y],
                                   idx.z < 0 ? -1 : indexRemap[idx.z],
                                   idx.w < 0 ? -1 : indexRemap[idx.w]));
      std::cout << "#pbrt:semantic: done remapping quad mesh. #verts: " << in->vertex.size() << " -> " << out->vertex.size() << std::endl;
      return out;
    }
    
    /*! create a new quad mesh by merging triangle pairs in given
      triangle mesh. triangles that cannot be merged into quads will
      be stored as degenerate quads */
    QuadMesh::SP QuadMesh::makeFrom(TriangleMesh::SP tris)
    {
      QuadMesh::SP out = std::make_shared<QuadMesh>(tris->material);
      out->textures = tris->textures;
      // out->texcoord = in->texcoord;
      out->vertex   = tris->vertex;
      out->normal   = tris->normal;
      
      for (size_t i=0;i<tris->index.size();i++) {
        vec3i idx0 = tris->index[i+0];
        if ((i+1) < tris->index.size()) {
          vec3i idx1 = tris->index[i+1];
          if (idx1.x == idx0.x && idx1.y == idx0.z) {
            // could merge!!!
            out->index.push_back(vec4i(idx0.x,idx0.y,idx0.z,idx1.z));
            ++i;
            continue;
          }
          if (idx1.x == idx0.z && idx1.z == idx0.x) {
            // could merge!!!
            out->index.push_back(vec4i(idx0.x,idx0.y,idx0.z,idx1.y));
            ++i;
            continue;
          }
        }
        // could not merge :-( - emit tri as degenerate quad
        out->index.push_back(vec4i(idx0.x,idx0.y,idx0.z,idx0.z));
      }
      
      if (tris->vertex.size() == 3*tris->index.size()) {
        return remeshVertices(out);
      }
      else
        return out;
    }
    

    struct SingleLevelFlattener {
      SingleLevelFlattener(Object::SP world)
        : result(std::make_shared<Object>())
      {
        traverse(world, affine3f::identity());
      }
    
      Object::SP
      getOrCreateEmittedShapeFrom(Object::SP object)
      {
        if (object->shapes.empty()) return Object::SP();
        if (alreadyEmitted[object]) return alreadyEmitted[object];
      
        Object::SP ours = std::make_shared<Object>("ShapeFrom:"+object->name);
        for (auto geom : object->shapes)
          ours->shapes.push_back(geom);
        return alreadyEmitted[object] = ours;
      }
    
      void traverse(Object::SP object, const affine3f &xfm)
      {
        if (!object)
          return;
      
        Object::SP emittedShape = getOrCreateEmittedShapeFrom(object);
        if (emittedShape) {
          Instance::SP inst
            = std::make_shared<Instance>(emittedShape,xfm);
          if (inst)
            result->instances.push_back(inst);
        }
      
        for (auto inst : object->instances) {
          if (inst)
            traverse(inst->object,xfm * inst->xfm);
        }
      }
    
      Object::SP result;
      std::map<Object::SP,Object::SP> alreadyEmitted;
    };

    /*! helper function that flattens a multi-level scene into a
      flattned scene */
    void Scene::makeSingleLevel()
    {
      if (isSingleLevel())
        return;
      
      this->world = SingleLevelFlattener(world).result;
    }

    /*! checks if the scene contains more than one level of instancing */
    bool Scene::isSingleLevel() const
    {
      if (!world->shapes.empty())
        return false;
      for (auto inst : world->instances)
        if (!inst->object->instances.empty())
          return false;
      return true;
    }
  
  } // ::pbrt::semantic
} // ::pbrt



// std
#include <map>
#include <sstream>

namespace pbrt {
  namespace semantic {

    using PBRTScene = pbrt::syntactic::Scene;
    using pbrt::syntactic::ParamArray;

    inline bool endsWith(const std::string &s, const std::string &suffix)
    {
      return s.substr(s.size()-suffix.size(),suffix.size()) == suffix;
    }

    struct SemanticParser
    {
      Scene::SP result;

      // ==================================================================
      // MATERIALS
      // ==================================================================
      std::map<pbrt::syntactic::Texture::SP,semantic::Texture::SP> textureMapping;

      /*! do create a track representation of given texture, _without_
        checking whether that was already created */
      semantic::Texture::SP createTextureFrom(pbrt::syntactic::Texture::SP in)
      {
        if (in->mapType == "imagemap") {
          const std::string fileName = in->getParamString("filename");
          if (fileName == "")
            std::cerr << "warning: pbrt image texture, but no filename!?" << std::endl;
          return std::make_shared<ImageTexture>(fileName);
        }
        if (in->mapType == "constant") {
          ConstantTexture::SP tex = std::make_shared<ConstantTexture>();
          if (in->hasParam1f("value"))
            tex->value = vec3f(in->getParam1f("value"));
          else
            in->getParam3f(&tex->value.x,"value");
          return tex;
        }
        if (in->mapType == "fbm") {
          FbmTexture::SP tex = std::make_shared<FbmTexture>();
          return tex;
        }
        if (in->mapType == "windy") {
          WindyTexture::SP tex = std::make_shared<WindyTexture>();
          return tex;
        }
        if (in->mapType == "marble") {
          MarbleTexture::SP tex = std::make_shared<MarbleTexture>();
          if (in->hasParam1f("scale"))
            tex->scale = in->getParam1f("scale");
          return tex;
        }
        if (in->mapType == "wrinkled") {
          WrinkledTexture::SP tex = std::make_shared<WrinkledTexture>();
          return tex;
        }
        if (in->mapType == "scale") {
          ScaleTexture::SP tex = std::make_shared<ScaleTexture>();
          if (in->hasParamTexture("tex1"))
            tex->tex1 = findOrCreateTexture(in->getParamTexture("tex1"));
          else if (in->hasParam3f("tex1"))
            in->getParam3f(&tex->scale1.x,"tex1");
          else
            tex->scale1 = vec3f(in->getParam1f("tex1"));
          
          if (in->hasParamTexture("tex2"))
            tex->tex2 = findOrCreateTexture(in->getParamTexture("tex2"));
          else if (in->hasParam3f("tex2"))
            in->getParam3f(&tex->scale2.x,"tex2");
          else
            tex->scale2 = vec3f(in->getParam1f("tex2"));
          return tex;
        }
        if (in->mapType == "mix") {
          MixTexture::SP tex = std::make_shared<MixTexture>();

          if (in->hasParam3f("amount"))
            in->getParam3f(&tex->amount.x,"amount");
          else if (in->hasParam1f("amount"))
            tex->amount = vec3f(in->getParam1f("amount"));
          else 
            tex->map_amount = findOrCreateTexture(in->getParamTexture("amount"));
          
          if (in->hasParamTexture("tex1"))
            tex->tex1 = findOrCreateTexture(in->getParamTexture("tex1"));
          else if (in->hasParam3f("tex1"))
            in->getParam3f(&tex->scale1.x,"tex1");
          else
            tex->scale1 = vec3f(in->getParam1f("tex1"));
          
          if (in->hasParamTexture("tex2"))
            tex->tex2 = findOrCreateTexture(in->getParamTexture("tex2"));
          else if (in->hasParam3f("tex2"))
            in->getParam3f(&tex->scale2.x,"tex2");
          else
            tex->scale2 = vec3f(in->getParam1f("tex2"));
          return tex;
        }
        if (in->mapType == "ptex") {
          const std::string fileName = in->getParamString("filename");
          if (fileName == "")
            std::cerr << "warning: pbrt image texture, but no filename!?" << std::endl;
          return std::make_shared<PtexFileTexture>(fileName);
        }
        throw std::runtime_error("un-handled pbrt texture type '"+in->toString()+"'");
        return std::make_shared<Texture>();
      }
      
      semantic::Texture::SP findOrCreateTexture(pbrt::syntactic::Texture::SP in)
      {
        if (!textureMapping[in]) {
          textureMapping[in] = createTextureFrom(in);
        }
        return textureMapping[in];
      }
    

      // ==================================================================
      // MATERIALS
      // ==================================================================
      std::map<pbrt::syntactic::Material::SP,semantic::Material::SP> materialMapping;

      /*! do create a track representation of given material, _without_
        checking whether that was already created */
      semantic::Material::SP createMaterialFrom(pbrt::syntactic::Material::SP in)
      {
        if (!in) {
          std::cerr << "warning: empty material!" << std::endl;
          return semantic::Material::SP();
        }
      
        const std::string type = in->type=="" ? in->getParamString("type") : in->type;

        // ==================================================================
        if (type == "") {
          return std::make_shared<Material>();
        }
        
        // ==================================================================
        if (type == "plastic") {
          PlasticMaterial::SP mat = std::make_shared<PlasticMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kd") {
              if (in->hasParamTexture(name)) {
                mat->kd = vec3f(1.f);
                mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kd.x,name);
            }
            else if (name == "Ks") {
              if (in->hasParamTexture(name)) {
                mat->ks = vec3f(1.f);
                mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->ks.x,name);
            }
            else if (name == "roughness") {
              if (in->hasParamTexture(name))
                mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
              else
                mat->roughness = in->getParam1f(name);
            }
            else if (name == "remaproughness") {
              mat->remapRoughness = in->getParamBool(name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled plastic-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "matte") {
          MatteMaterial::SP mat = std::make_shared<MatteMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kd") {
              if (in->hasParamTexture(name)) {
                mat->kd = vec3f(1.f);
                mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kd.x,name);
            }
            else if (name == "sigma") {
              if (in->hasParam1f(name))
                mat->sigma = in->getParam1f(name);
              else 
                mat->map_sigma = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            } else
              throw std::runtime_error("un-handled matte-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "metal") {
          MetalMaterial::SP mat = std::make_shared<MetalMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "roughness") {
              if (in->hasParamTexture(name))
                mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
              else
                mat->roughness = in->getParam1f(name);
            }
            else if (name == "uroughness") {
              if (in->hasParamTexture(name))
                mat->map_uRoughness = findOrCreateTexture(in->getParamTexture(name));
              else
                mat->uRoughness = in->getParam1f(name);
            }
            else if (name == "vroughness") {
              if (in->hasParamTexture(name))
                mat->map_vRoughness = findOrCreateTexture(in->getParamTexture(name));
              else
                mat->vRoughness = in->getParam1f(name);
            }
            else if (name == "eta") {
              if (in->hasParam3f(name))
                in->getParam3f(&mat->eta.x,name);
              else
                mat->spectrum_eta = in->getParamString(name);
            }
            else if (name == "k") {
              if (in->hasParam3f(name))
                in->getParam3f(&mat->k.x,name);
              else
                mat->spectrum_k = in->getParamString(name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled metal-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "fourier") {
          FourierMaterial::SP mat = std::make_shared<FourierMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "bsdffile") {
              mat->fileName = in->getParamString(name);
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled fourier-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "mirror") {
          MirrorMaterial::SP mat = std::make_shared<MirrorMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kr") {
              if (in->hasParamTexture(name)) {
                throw std::runtime_error("mapping Kr for mirror materials not implemented");
              } else
                in->getParam3f(&mat->kr.x,name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled mirror-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "uber") {
          UberMaterial::SP mat = std::make_shared<UberMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kd") {
              if (in->hasParamTexture(name)) {
                mat->kd = vec3f(1.f);
                mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kd.x,name);
            }
            else if (name == "Kr") {
              if (in->hasParamTexture(name)) {
                mat->kr = vec3f(1.f);
                mat->map_kr = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kr.x,name);
            }
            else if (name == "Kt") {
              if (in->hasParamTexture(name)) {
                mat->kt = vec3f(1.f);
                mat->map_kt = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kt.x,name);
            }
            else if (name == "Ks") {
              if (in->hasParamTexture(name)) {
                mat->ks = vec3f(1.f);
                mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->ks.x,name);
            }
            else if (name == "alpha") {
              if (in->hasParamTexture(name)) {
                mat->alpha = 1.f;
                mat->map_alpha = findOrCreateTexture(in->getParamTexture(name));
              } else
                mat->alpha = in->getParam1f(name);
            }
            else if (name == "opacity") {
              if (in->hasParamTexture(name)) {
                mat->opacity = vec3f(1.f);
                mat->map_opacity = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->opacity.x,name);
            }
            else if (name == "index") {
              mat->index = in->getParam1f(name);
            }
            else if (name == "roughness") {
              if (in->hasParamTexture(name))
                mat->map_roughness = findOrCreateTexture(in->getParamTexture(name));
              else if (in->hasParam1f(name))
                mat->roughness = in->getParam1f(name);
              else
                throw std::runtime_error("uber::roughness in un-recognized format...");
              // else
              //   in->getParam3f(&mat->roughness.x,name);
            }
            else if (name == "shadowalpha") {
              if (in->hasParamTexture(name)) {
                mat->shadowAlpha = 1.f;
                mat->map_shadowAlpha = findOrCreateTexture(in->getParamTexture(name));
              } else
                mat->shadowAlpha = in->getParam1f(name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled uber-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "substrate") {
          SubstrateMaterial::SP mat = std::make_shared<SubstrateMaterial>();
          for (auto it : in->param) {
            std::string name = it.first;
            if (name == "Kd") {
              if (in->hasParamTexture(name)) {
                mat->kd = vec3f(1.f);
                mat->map_kd = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->kd.x,name);
            }
            else if (name == "Ks") {
              if (in->hasParamTexture(name)) {
                mat->ks = vec3f(1.f);
                mat->map_ks = findOrCreateTexture(in->getParamTexture(name));
              } else
                in->getParam3f(&mat->ks.x,name);
            }
            else if (name == "uroughness") {
              if (in->hasParamTexture(name)) {
                mat->uRoughness = 1.f;
                mat->map_uRoughness = findOrCreateTexture(in->getParamTexture(name));
              } else
                mat->uRoughness = in->getParam1f(name);
            }
            else if (name == "vroughness") {
              if (in->hasParamTexture(name)) {
                mat->vRoughness = 1.f;
                mat->map_vRoughness = findOrCreateTexture(in->getParamTexture(name));
              } else
                mat->vRoughness = in->getParam1f(name);
            }
            else if (name == "remaproughness") {
              mat->remapRoughness = in->getParamBool(name);
            }
            else if (name == "bumpmap") {
              mat->map_bump = findOrCreateTexture(in->getParamTexture(name));
            }
            else if (name == "type") {
              /* ignore */
            } else
              throw std::runtime_error("un-handled substrate-material parameter '"+it.first+"'");
          };
          return mat;
        }
      
        // ==================================================================
        if (type == "disney") {
          DisneyMaterial::SP mat = std::make_shared<DisneyMaterial>();

          in->getParam3f(&mat->color.x,"color");
          mat->anisotropic    = in->getParam1f("anisotropic",    0.f );
          mat->clearCoat      = in->getParam1f("clearcoat",      0.f );
          mat->clearCoatGloss = in->getParam1f("clearcoatgloss", 1.f );
          mat->diffTrans      = in->getParam1f("difftrans",      1.35f );
          mat->eta            = in->getParam1f("eta",            1.2f );
          mat->flatness       = in->getParam1f("flatness",       0.2f );
          mat->metallic       = in->getParam1f("metallic",       0.f );
          mat->roughness      = in->getParam1f("roughness",      0.9f );
          mat->sheen          = in->getParam1f("sheen",          0.3f );
          mat->sheenTint      = in->getParam1f("sheentint",      0.68f );
          mat->specTrans      = in->getParam1f("spectrans",      0.f );
          mat->specularTint   = in->getParam1f("speculartint",   0.f );
          mat->thin           = in->getParamBool("thin",           true);
          return mat;
        }

        // ==================================================================
        if (type == "mix") {
          MixMaterial::SP mat = std::make_shared<MixMaterial>();
          
          if (in->hasParamTexture("amount"))
            mat->map_amount = findOrCreateTexture(in->getParamTexture("amount"));
          else
            in->getParam3f(&mat->amount.x,"amount");
          
          const std::string name0 = in->getParamString("namedmaterial1");
          if (name0 == "")
            throw std::runtime_error("mix material w/o 'namedmaterial1' parameter");
          const std::string name1 = in->getParamString("namedmaterial2");
          if (name1 == "")
            throw std::runtime_error("mix material w/o 'namedmaterial2' parameter");

          assert(in->attributes);
          pbrt::syntactic::Material::SP mat0 = in->attributes->namedMaterial[name0];
          assert(mat0);
          pbrt::syntactic::Material::SP mat1 = in->attributes->namedMaterial[name1];
          assert(mat1);

          mat->material0    = findOrCreateMaterial(mat0);
          mat->material1    = findOrCreateMaterial(mat1);
        
          return mat;
        }

        // ==================================================================
        if (type == "translucent") {
          TranslucentMaterial::SP mat = std::make_shared<TranslucentMaterial>();

          in->getParam3f(&mat->transmit.x,"transmit");
          in->getParam3f(&mat->reflect.x,"reflect");
          if (in->hasParamTexture("Kd"))
            mat->map_kd = findOrCreateTexture(in->getParamTexture("Kd"));
          else
            in->getParam3f(&mat->kd.x,"Kd");
          
          return mat;
        }

        // ==================================================================
        if (type == "glass") {
          GlassMaterial::SP mat = std::make_shared<GlassMaterial>();

          in->getParam3f(&mat->kr.x,"Kr");
          in->getParam3f(&mat->kt.x,"Kt");
          mat->index = in->getParam1f("index");
        
          return mat;
        }

        // ==================================================================
#ifndef NDEBUG
        std::cout << "Warning: un-recognizd material type '"+type+"'" << std::endl;
#endif
        return std::make_shared<semantic::Material>();
      }

      /*! check if this material has already been imported, and if so,
        find what we imported, and reutrn this. otherwise import and
        store for later use.
      
        important: it is perfectly OK for this material to be a null
        object - the area ligths in moana have this features, for
        example */
      semantic::Material::SP findOrCreateMaterial(pbrt::syntactic::Material::SP in)
      {
        // null materials get passed through ...
        if (!in)
          return semantic::Material::SP();
      
        if (!materialMapping[in]) {
          materialMapping[in] = createMaterialFrom(in);
        }
        return materialMapping[in];
      }
    
      /*! counter that tracks, for each un-handled shape type, how many
        "instances" of that shape type it could not create (note those
        are not _real_ instances in the ray tracing sense, just
        "occurrances" of that shape type in the scene graph,
        _before_ object instantiation */
      std::map<std::string,size_t> unhandledShapeTypeCounter;
    
      /*! constructor that also perfoms all the work - converts the
        input 'pbrtScene' to a naivescenelayout, and assings that to
        'result' */
      SemanticParser(PBRTScene::SP pbrtScene)
        : pbrtScene(pbrtScene)
      {
        result        = std::make_shared<Scene>();
        result->world  = findOrEmitObject(pbrtScene->world);

        if (!unhandledShapeTypeCounter.empty()) {
          std::cerr << "WARNING: scene contained some un-handled shapes!" << std::endl;
          for (auto type : unhandledShapeTypeCounter)
            std::cerr << " - " << type.first << " : " << type.second << " occurrances" << std::endl;
        }
      }

    private:
      Instance::SP emitInstance(pbrt::syntactic::Object::Instance::SP pbrtInstance)
      {
        Instance::SP ourInstance
          = std::make_shared<Instance>();
        ourInstance->xfm    = (const affine3f&)pbrtInstance->xfm.atStart;
        ourInstance->object = findOrEmitObject(pbrtInstance->object);
        return ourInstance;
      }

      /*! extract 'texture' parameters from shape, and assign to shape */
      void extractTextures(Shape::SP geom, pbrt::syntactic::Shape::SP shape)
      {
        for (auto param : shape->param) {
          if (param.second->getType() != "texture")
            continue;

          geom->textures[param.first] = findOrCreateTexture(shape->getParamTexture(param.first));//param.second);
        }
      }

      Shape::SP emitPlyMesh(pbrt::syntactic::Shape::SP shape)
      {
        // const affine3f xfm = instanceXfm*(ospcommon::affine3f&)shape->transform.atStart;
        const std::string fileName
          = pbrtScene->makeGlobalFileName(shape->getParamString("filename"));
        TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));
        pbrt_helper_loadPlyTriangles(fileName,ours->vertex,ours->normal,ours->index);

        affine3f xfm = shape->transform.atStart;
        for (vec3f &v : ours->vertex)
          v = xfmPoint(xfm,v);
        for (vec3f &v : ours->normal)
          v = xfmVector(xfm,v);

        extractTextures(ours,shape);
        return ours;
      }

      template<typename T>
      std::vector<T> extractVector(pbrt::syntactic::Shape::SP shape, const std::string &name)
      {
        std::vector<T> result;
        typename ParamArray<typename T::scalar_t>::SP param = shape->findParam<typename T::scalar_t>(name);
        if (param) {

          int dims = sizeof(T)/sizeof(typename T::scalar_t);
          size_t num = param->size() / dims;// / T::dims;
          const T *const data = (const T*)param->data();
        
          result.resize(num);
          std::copy(data,data+num,result.begin());
        }
        return result;
      }
    
      Shape::SP emitTriangleMesh(pbrt::syntactic::Shape::SP shape)
      {
        TriangleMesh::SP ours = std::make_shared<TriangleMesh>(findOrCreateMaterial(shape->material));

        // vertices - param "P", 3x float each
        ours->vertex = extractVector<vec3f>(shape,"P");
        // vertex normals - param "N", 3x float each
        ours->normal = extractVector<vec3f>(shape,"N");
        // triangle vertex indices - param "P", 3x int each
        ours->index = extractVector<vec3i>(shape,"indices");

        affine3f xfm = shape->transform.atStart;
        for (vec3f &v : ours->vertex)
          v = xfmPoint(xfm,v);
        for (vec3f &v : ours->normal)
          v = xfmNormal(xfm,v);
        extractTextures(ours,shape);
      
        return ours;
      }



      Shape::SP emitCurve(pbrt::syntactic::Shape::SP shape)
      {
        Curve::SP ours = std::make_shared<Curve>(findOrCreateMaterial(shape->material));

        // -------------------------------------------------------
        // check 'type'
        const std::string type
          = shape->hasParamString("type")
          ? shape->getParamString("type")
          : std::string("");
        if (type == "cylinder")
          ours->type = Curve::CurveType_Cylinder;
        else if (type == "ribbon")
          ours->type = Curve::CurveType_Ribbon;
        else if (type == "flat")
            ours->type = Curve::CurveType_Flat;
          else 
            ours->type = Curve::CurveType_Unknown;
        
        // -------------------------------------------------------
        // check 'basis'
        const std::string basis
          = shape->hasParamString("basis")
          ? shape->getParamString("basis")
          : std::string("");
        if (basis == "bezier")
          ours->basis = Curve::CurveBasis_Bezier;
        else if (basis == "bspline")
          ours->basis = Curve::CurveBasis_BSpline;
        else 
          ours->basis = Curve::CurveBasis_Unknown;
        
        // -------------------------------------------------------
        // check 'width', 'width0', 'width1'
        if (shape->hasParam1f("width")) 
          ours->width0 = ours->width1 = shape->getParam1f("width");
        
        if (shape->hasParam1f("width0")) 
          ours->width0 = shape->getParam1f("width0");
        if (shape->hasParam1f("width1")) 
          ours->width1 = shape->getParam1f("width1");

        if (shape->hasParam1i("degree")) 
          ours->degree = shape->getParam1i("degree");

        // vertices - param "P", 3x float each
        ours->P = extractVector<vec3f>(shape,"P");
        return ours;
      }



      
      Shape::SP emitSphere(pbrt::syntactic::Shape::SP shape)
      {
        Sphere::SP ours = std::make_shared<Sphere>(findOrCreateMaterial(shape->material));

        ours->transform = shape->transform.atStart;
        ours->radius    = shape->getParam1f("radius");
        extractTextures(ours,shape);
      
        return ours;
      }

      Shape::SP emitDisk(pbrt::syntactic::Shape::SP shape)
      {
        Disk::SP ours = std::make_shared<Disk>(findOrCreateMaterial(shape->material));

        ours->transform = shape->transform.atStart;
        ours->radius    = shape->getParam1f("radius");
        
        if (shape->hasParam1f("height"))
          ours->height    = shape->getParam1f("height");
        
        extractTextures(ours,shape);
      
        return ours;
      }

      Shape::SP emitShape(pbrt::syntactic::Shape::SP shape)
      {
        if (shape->type == "plymesh") 
          return emitPlyMesh(shape);
        if (shape->type == "trianglemesh")
          return emitTriangleMesh(shape);
        if (shape->type == "curve")
          return emitCurve(shape);
        if (shape->type == "sphere")
          return emitSphere(shape);
        if (shape->type == "disk")
          return emitDisk(shape);

        // throw std::runtime_error("un-handled shape "+shape->type);
        unhandledShapeTypeCounter[shape->type]++;
        // std::cout << "WARNING: un-handled shape " << shape->type << std::endl;
        return Shape::SP();
      }

      Shape::SP findOrCreateShape(pbrt::syntactic::Shape::SP pbrtShape)
      {
        if (emittedShapes.find(pbrtShape) != emittedShapes.end())
          return emittedShapes[pbrtShape];

        emittedShapes[pbrtShape] = emitShape(pbrtShape);
        return emittedShapes[pbrtShape];
      }
    
      Object::SP findOrEmitObject(pbrt::syntactic::Object::SP pbrtObject)
      {
        if (emittedObjects.find(pbrtObject) != emittedObjects.end()) {
          return emittedObjects[pbrtObject];
        }
      
        Object::SP ourObject = std::make_shared<Object>();
        ourObject->name = pbrtObject->name;
        for (auto shape : pbrtObject->shapes) {
          Shape::SP ourShape = findOrCreateShape(shape);
          if (ourShape)
            ourObject->shapes.push_back(ourShape);
        }
        for (auto instance : pbrtObject->objectInstances)
          ourObject->instances.push_back(emitInstance(instance));

        emittedObjects[pbrtObject] = ourObject;
        return ourObject;
      }

      std::map<pbrt::syntactic::Object::SP,Object::SP> emittedObjects;
      std::map<pbrt::syntactic::Shape::SP,Shape::SP>   emittedShapes;
    
      PBRTScene::SP pbrtScene;
    };

    void createFilm(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt)
    {
      if (pbrt->film &&
          pbrt->film->findParam<int>("xresolution") &&
          pbrt->film->findParam<int>("yresolution")) {
        vec2i resolution;
        std::string fileName = "";
        if (pbrt->film->hasParamString("filename"))
          fileName = pbrt->film->getParamString("filename");
        
        resolution.x = pbrt->film->findParam<int>("xresolution")->get(0);
        resolution.y = pbrt->film->findParam<int>("yresolution")->get(0);
        ours->film = std::make_shared<Film>(resolution,fileName);
      } else {
        std::cout << "warning: could not determine film resolution from pbrt scene" << std::endl;
      }
    }

    float findCameraFov(pbrt::syntactic::Camera::SP camera)
    {
      if (!camera->findParam<float>("fov")) {
        std::cerr << "warning - pbrt file has camera, but camera has no 'fov' field; replacing with constant 30 degrees" << std::endl;
        return 30;
      }
      return camera->findParam<float>("fov")->get(0);
    }

    /*! create a scene->camera from the pbrt model, if specified, or
      leave unchanged if not */
    void createCamera(Scene::SP scene, pbrt::syntactic::Scene::SP pbrt)
    {
      Camera::SP ours = std::make_shared<Camera>();
      if (pbrt->cameras.empty()) {
        std::cout << "warning: no 'camera'(s) in pbrt file" << std::endl;
        return;
      }

      for (auto camera : pbrt->cameras) {
        if (!camera)
          throw std::runtime_error("invalid pbrt camera");

        const float fov = findCameraFov(camera);

        // TODO: lensradius and focaldistance:

        //     float 	lensradius 	0 	The radius of the lens. Used to render scenes with depth of field and focus effects. The default value yields a pinhole camera.
        const float lensRadius = 0.f;

        // float 	focaldistance 	10^30 	The focal distance of the lens. If "lensradius" is zero, this has no effect. Otherwise, it specifies the distance from the camera origin to the focal plane.
        const float focalDistance = 1.f;
    
        const affine3f frame = inverse(camera->transform.atStart);
    
        ours->simplified.lens_center = frame.p;
        ours->simplified.lens_du = lensRadius * frame.l.vx;
        ours->simplified.lens_dv = lensRadius * frame.l.vy;

        const float fovDistanceToUnitPlane = 0.5f / tanf(fov/2 * M_PI/180.f);
        ours->simplified.screen_center = frame.p + focalDistance * frame.l.vz;
        ours->simplified.screen_du = - focalDistance/fovDistanceToUnitPlane * frame.l.vx;
        ours->simplified.screen_dv = focalDistance/fovDistanceToUnitPlane * frame.l.vy;

        scene->cameras.push_back(ours);
      }
    }

    Camera::SP createCamera(pbrt::syntactic::Camera::SP camera)
    {
      if (!camera) return Camera::SP();

      Camera::SP ours = std::make_shared<Camera>();
      if (camera->hasParam1f("fov"))
        ours->lensRadius = camera->getParam1f("fov");
      if (camera->hasParam1f("lensradius"))
        ours->lensRadius = camera->getParam1f("lensradius");
      if (camera->hasParam1f("focaldistance"))
        ours->focalDistance = camera->getParam1f("focaldistance");

      ours->frame = inverse(camera->transform.atStart);
      
      ours->simplified.lens_center
        = ours->frame.p;
      ours->simplified.lens_du
        = ours->lensRadius * ours->frame.l.vx;
      ours->simplified.lens_dv
        = ours->lensRadius * ours->frame.l.vy;
      
      const float fovDistanceToUnitPlane = 0.5f / tanf(ours->fov/2 * M_PI/180.f);
      ours->simplified.screen_center
        = ours->frame.p + ours->focalDistance * ours->frame.l.vz;
      ours->simplified.screen_du
        = - ours->focalDistance/fovDistanceToUnitPlane * ours->frame.l.vx;
      ours->simplified.screen_dv
        = ours->focalDistance/fovDistanceToUnitPlane * ours->frame.l.vy;

      return ours;
    }

    semantic::Scene::SP importPBRT(const std::string &fileName)
    {
      pbrt::syntactic::Scene::SP pbrt;
      if (endsWith(fileName,".pbsf"))
        pbrt = pbrt::syntactic::readBinary(fileName);
      else if (endsWith(fileName,".pbrt"))
        pbrt = pbrt::syntactic::parse(fileName);
      else
        throw std::runtime_error("could not detect input file format!? (unknown extension in '"+fileName+"')");
      
      semantic::Scene::SP scene = SemanticParser(pbrt).result;
      createFilm(scene,pbrt);
      for (auto cam : pbrt->cameras)
        scene->cameras.push_back(createCamera(cam));
      return scene;
    }

  } // ::pbrt::semantic
} // ::pbrt



// std
#include <iostream>
#include <sstream>
#include <fstream>
#include <stack>
#include <string.h>

namespace pbrt {
  namespace semantic {

#define    PBRT_PARSER_SEMANTIC_FORMAT_MAJOR 0
#define    PBRT_PARSER_SEMANTIC_FORMAT_MINOR 5
  
    const uint32_t ourFormatID = (PBRT_PARSER_SEMANTIC_FORMAT_MAJOR << 16) + PBRT_PARSER_SEMANTIC_FORMAT_MINOR;

    enum {
      TYPE_ERROR,
      TYPE_SCENE,
      TYPE_OBJECT,
      TYPE_SHAPE,
      TYPE_INSTANCE,
      TYPE_CAMERA,
      TYPE_MATERIAL,
      TYPE_DISNEY_MATERIAL,
      TYPE_UBER_MATERIAL,
      TYPE_MIX_MATERIAL,
      TYPE_GLASS_MATERIAL,
      TYPE_MIRROR_MATERIAL,
      TYPE_MATTE_MATERIAL,
      TYPE_SUBSTRATE_MATERIAL,
      TYPE_SUBSURFACE_MATERIAL,
      TYPE_FOURIER_MATERIAL,
      TYPE_METAL_MATERIAL,
      TYPE_PLASTIC_MATERIAL,
      TYPE_TRANSLUCENT_MATERIAL,
      TYPE_TEXTURE,
      TYPE_IMAGE_TEXTURE,
      TYPE_SCALE_TEXTURE,
      TYPE_PTEX_FILE_TEXTURE,
      TYPE_CONSTANT_TEXTURE,
      TYPE_WINDY_TEXTURE,
      TYPE_FBM_TEXTURE,
      TYPE_MARBLE_TEXTURE,
      TYPE_MIX_TEXTURE,
      TYPE_WRINKLED_TEXTURE,
      TYPE_FILM,
      TYPE_TRIANGLE_MESH,
      TYPE_QUAD_MESH,
      TYPE_SPHERE,
      TYPE_DISK,
      TYPE_CURVE,
    };
    
    /*! a simple buffer for binary data */
    struct SerializedEntity : public std::vector<uint8_t>
    {
      typedef std::shared_ptr<SerializedEntity> SP;
    };

    struct BinaryReader {

      BinaryReader(const std::string &fileName)
        : binFile(fileName)
      {
        int formatTag;
      
        binFile.read((char*)&formatTag,sizeof(formatTag));
        while (1) {
          size_t size;
          binFile.read((char*)&size,sizeof(size));
          if (!binFile.good())
            break;
          int tag;
          binFile.read((char*)&tag,sizeof(tag));
          currentEntityData.resize(size);
          binFile.read((char *)currentEntityData.data(),size);
          currentEntityOffset = 0;

          Entity::SP newEntity = createEntity(tag);
          readEntities.push_back(newEntity);
          if (newEntity) newEntity->readFrom(*this);
          currentEntityData.clear();
        }
        // exit(0);
      }

      template<typename T>
      inline void copyBytes(T *t, size_t numBytes)
      {
        if ((currentEntityOffset + numBytes) > currentEntityData.size())
          throw std::runtime_error("invalid read attempt by entity - not enough data in data block!");
        memcpy((void *)t,(void *)((char*)currentEntityData.data()+currentEntityOffset),numBytes);
        currentEntityOffset += numBytes;
      }
                                        
      template<typename T> T read();
      template<typename T> std::vector<T> readVector();
      template<typename T> void read(std::vector<T> &vt) { vt = readVector<T>(); }
      template<typename T> void read(std::vector<std::shared_ptr<T>> &vt)
      {
        vt = readVector<std::shared_ptr<T>>(); 
      }
      template<typename T> void read(T &t) { t = read<T>(); }


      template<typename T1, typename T2>
      void read(std::map<T1,T2> &result)
      {
        int size = read<int>();
        result.clear();
        for (int i=0;i<size;i++) {
          T1 t1; T2 t2;
          read(t1);
          read(t2);
          result[t1] = t2;
        }
      }
      
      template<typename T> inline void read(std::shared_ptr<T> &t)
      {
        int ID = read<int>();
        t = getEntity<T>(ID);
      }

      Entity::SP createEntity(int typeTag)
      {
        switch (typeTag) {
        case TYPE_SCENE:
          return std::make_shared<Scene>();
        case TYPE_TEXTURE:
          return std::make_shared<Texture>();
        case TYPE_IMAGE_TEXTURE:
          return std::make_shared<ImageTexture>();
        case TYPE_SCALE_TEXTURE:
          return std::make_shared<ScaleTexture>();
        case TYPE_PTEX_FILE_TEXTURE:
          return std::make_shared<PtexFileTexture>();
        case TYPE_CONSTANT_TEXTURE:
          return std::make_shared<ConstantTexture>();
        case TYPE_WINDY_TEXTURE:
          return std::make_shared<WindyTexture>();
        case TYPE_FBM_TEXTURE:
          return std::make_shared<FbmTexture>();
        case TYPE_MARBLE_TEXTURE:
          return std::make_shared<MarbleTexture>();
        case TYPE_MIX_TEXTURE:
          return std::make_shared<MixTexture>();
        case TYPE_WRINKLED_TEXTURE:
          return std::make_shared<WrinkledTexture>();
        case TYPE_MATERIAL:
          return std::make_shared<Material>();
        case TYPE_DISNEY_MATERIAL:
          return std::make_shared<DisneyMaterial>();
        case TYPE_UBER_MATERIAL:
          return std::make_shared<UberMaterial>();
        case TYPE_MIX_MATERIAL:
          return std::make_shared<MixMaterial>();
        case TYPE_TRANSLUCENT_MATERIAL:
          return std::make_shared<TranslucentMaterial>();
        case TYPE_GLASS_MATERIAL:
          return std::make_shared<GlassMaterial>();
        case TYPE_PLASTIC_MATERIAL:
          return std::make_shared<PlasticMaterial>();
        case TYPE_MIRROR_MATERIAL:
          return std::make_shared<MirrorMaterial>();
        case TYPE_SUBSTRATE_MATERIAL:
          return std::make_shared<SubstrateMaterial>();
        case TYPE_SUBSURFACE_MATERIAL:
          return std::make_shared<SubSurfaceMaterial>();
        case TYPE_MATTE_MATERIAL:
          return std::make_shared<MatteMaterial>();
        case TYPE_FOURIER_MATERIAL:
          return std::make_shared<FourierMaterial>();
        case TYPE_METAL_MATERIAL:
          return std::make_shared<MetalMaterial>();
        case TYPE_FILM:
          return std::make_shared<Film>(vec2i(0));
        case TYPE_CAMERA:
          return std::make_shared<Camera>();
        case TYPE_TRIANGLE_MESH:
          return std::make_shared<TriangleMesh>();
        case TYPE_QUAD_MESH:
          return std::make_shared<QuadMesh>();
        case TYPE_SPHERE:
          return std::make_shared<Sphere>();
        case TYPE_DISK:
          return std::make_shared<Disk>();
        case TYPE_CURVE:
          return std::make_shared<Curve>();
        case TYPE_INSTANCE:
          return std::make_shared<Instance>();
        case TYPE_OBJECT:
          return std::make_shared<Object>();
        default:
          std::cerr << "unknown entity type tag " << typeTag << " in binary file" << std::endl;
          return Entity::SP();
        };
      }
    
      /*! return entity with given ID, dynamic-casted to given type. if
        entity wasn't recognized during parsing, a null pointer will
        be returned. if entity _was_ recognized in parser, but is not
        of this type, an exception will be thrown; if ID is -1, a null
        pointer will be returned */
      template<typename T>
      std::shared_ptr<T> getEntity(int ID)
      {
        // rule 1: ID -1 is a valid identifier for 'null object'
        if (ID == -1)
          return std::shared_ptr<T>();
        // assertion: only values with ID 0 ... N-1 are allowed
        assert(ID < (int)readEntities.size() && ID >= 0);

        // rule 2: if object _was_ a null pointer, return it (that was
        // an error during object creation)
        if (!readEntities[ID])
          return std::shared_ptr<T>();

        std::shared_ptr<T> t = std::dynamic_pointer_cast<T>(readEntities[ID]);
      
        // rule 3: if object was of different type, throw an exception
        if (!t)
          throw std::runtime_error("error in reading binary file - given entity is not of expected type!");

        // rule 4: otherwise, return object cast to that type
        return t;
      }

      std::vector<uint8_t> currentEntityData;
      size_t currentEntityOffset;
      std::vector<Entity::SP> readEntities;
      std::ifstream binFile;
    };

    template<typename T>
    T BinaryReader::read()
    {
      T t;
      copyBytes(&t,sizeof(t));
      return t;
    }

    template<>
    std::string BinaryReader::read()
    {
      int length = read<int>();
      std::vector<char> cv(length);
      copyBytes(&cv[0],length);
      std::string s = std::string(cv.begin(),cv.end());
      return s;
    }

    template<typename T>
    std::vector<T> BinaryReader::readVector()
    {
      size_t length = read<size_t>();
      std::vector<T> vt(length);
      for (size_t i=0;i<length;i++) {
        read(vt[i]);
      }
      return vt;
    }
  
    

    /*! helper class that writes out a PBRT scene graph in a binary form
      that is much faster to parse */
    struct BinaryWriter {

      BinaryWriter(const std::string &fileName)
        : binFile(fileName)
      {
        int formatTag = ourFormatID;
        binFile.write((char*)&formatTag,sizeof(formatTag));
      }

      /*! our stack of output buffers - each object we're writing might
        depend on other objects that it references in its paramset, so
        we use a stack of such buffers - every object writes to its
        own buffer, and if it needs to write another object before
        itself that other object can simply push a new buffer on the
        stack, and first write that one before the child completes its
        own writes. */
      std::stack<SerializedEntity::SP> serializedEntity;

      /*! the file we'll be writing the buffers to */
      std::ofstream binFile;

      void writeRaw(const void *ptr, size_t size)
      {
        assert(ptr);
        serializedEntity.top()->insert(serializedEntity.top()->end(),(uint8_t*)ptr,(uint8_t*)ptr + size);
      }
    
      template<typename T>
      void write(const T &t)
      {
        writeRaw(&t,sizeof(t));
      }

      template<typename T>
      void write(std::shared_ptr<T> t)
      {
        write(serialize(t));
      }

      void write(const std::string &t)
      {
        write((int32_t)t.size());
        writeRaw(&t[0],t.size());
      }
    
      template<typename T>
      void write(const std::vector<T> &t)
      {
        const void *ptr = (const void *)t.data();
        size_t size = t.size();
        write(size);
        if (!t.empty())
          writeRaw(ptr,t.size()*sizeof(T));
      }

      template<typename T>
      void write(const std::vector<std::shared_ptr<T>> &t)
      {
        size_t size = t.size();
        write(size);
        for (size_t i=0;i<size;i++)
          write(t[i]);
      }


      
      template<typename T1, typename T2>
      void write(const std::map<T1,std::shared_ptr<T2>> &values)
      {
        int size = values.size();
        write(size);
        for (auto it : values) {
          write(it.first);
          write(it.second);
          // write(serialize(it.second));
        }
      }


      void write(const std::vector<bool> &t)
      {
        std::vector<unsigned char> asChar(t.size());
        for (size_t i=0;i<t.size();i++)
          asChar[i] = t[i]?1:0;
        write(asChar);
      }

      void write(const std::vector<std::string> &t)
      {
        write(t.size());
        for (auto &s : t) {
          write(s);
        }
      }
    
      /*! start a new write buffer on the stack - all future writes will go into this buffer */
      void startNewEntity()
      { serializedEntity.push(std::make_shared<SerializedEntity>()); }
    
      /*! write the topmost write buffer to disk, and free its memory */
      void executeWrite(int tag)
      {
        size_t size = serializedEntity.top()->size();
        // std::cout << "writing block of size " << size << std::endl;
        binFile.write((const char *)&size,sizeof(size));
        binFile.write((const char *)&tag,sizeof(tag));
        binFile.write((const char *)serializedEntity.top()->data(),size);
        serializedEntity.pop();
      
      }

      std::map<Entity::SP,int> emittedEntity;

      int serialize(Entity::SP entity)
      {
        if (!entity) {
          // std::cout << "warning: null entity" << std::endl;
          return -1;
        }
      
        if (emittedEntity.find(entity) != emittedEntity.end())
          return emittedEntity[entity];

        startNewEntity();
        int tag = entity->writeTo(*this);
        executeWrite(tag);
        return (emittedEntity[entity] = emittedEntity.size());
      }
    };

    /*! serialize out to given binary writer */
    int Scene::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(film));
      binary.write(cameras);
      binary.write(binary.serialize(world));
      return TYPE_SCENE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Scene::readFrom(BinaryReader &binary) 
    {
      binary.read(film);
      binary.read(cameras);
      binary.read(world);
    }

    /*! serialize out to given binary writer */
    int Camera::writeTo(BinaryWriter &binary) 
    {
      binary.write(fov);
      binary.write(focalDistance);
      binary.write(lensRadius);
      binary.write(frame);
      binary.write(simplified);
      return TYPE_CAMERA;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Camera::readFrom(BinaryReader &binary) 
    {
      binary.read(fov);
      binary.read(focalDistance);
      binary.read(lensRadius);
      binary.read(frame);
      binary.read(simplified);
    }



    /*! serialize out to given binary writer */
    int Film::writeTo(BinaryWriter &binary) 
    {
      binary.write(resolution);
      binary.write(fileName);
      return TYPE_FILM;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Film::readFrom(BinaryReader &binary) 
    {
      binary.read(resolution);
      binary.read(fileName);
    }



    /*! serialize out to given binary writer */
    int Shape::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(material));
      binary.write(textures);
      return TYPE_SHAPE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Shape::readFrom(BinaryReader &binary) 
    {
      binary.read(material);
      binary.read(textures);
    }


    // ==================================================================
    // TriangleMesh
    // ==================================================================

    /*! serialize out to given binary writer */
    int TriangleMesh::writeTo(BinaryWriter &binary) 
    {
      Shape::writeTo(binary);
      binary.write(vertex);
      binary.write(normal);
      binary.write(index);
      return TYPE_TRIANGLE_MESH;
    }
  
    /*! serialize _in_ from given binary file reader */
    void TriangleMesh::readFrom(BinaryReader &binary) 
    {
      Shape::readFrom(binary);
      binary.read(vertex);
      binary.read(normal);
      binary.read(index);
    }


    // ==================================================================
    // QuadMesh
    // ==================================================================

    /*! serialize out to given binary writer */
    int QuadMesh::writeTo(BinaryWriter &binary) 
    {
      Shape::writeTo(binary);
      binary.write(vertex);
      binary.write(normal);
      binary.write(index);
      return TYPE_QUAD_MESH;
    }
  
    /*! serialize _in_ from given binary file reader */
    void QuadMesh::readFrom(BinaryReader &binary) 
    {
      Shape::readFrom(binary);
      binary.read(vertex);
      binary.read(normal);
      binary.read(index);
    }


    // ==================================================================
    // Curve
    // ==================================================================

    /*! serialize out to given binary writer */
    int Curve::writeTo(BinaryWriter &binary) 
    {
      Shape::writeTo(binary);
      binary.write(width0);
      binary.write(width1);
      binary.write(basis);
      binary.write(type);
      binary.write(degree);
      binary.write(P);
      return TYPE_CURVE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Curve::readFrom(BinaryReader &binary) 
    {
      Shape::readFrom(binary);
      binary.read(width0);
      binary.read(width1);
      binary.read(basis);
      binary.read(type);
      binary.read(degree);
      binary.read(P);
    }


    // ==================================================================
    // Disk
    // ==================================================================

    /*! serialize out to given binary writer */
    int Disk::writeTo(BinaryWriter &binary) 
    {
      Shape::writeTo(binary);
      binary.write(radius);
      binary.write(height);
      binary.write(transform);
      return TYPE_DISK;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Disk::readFrom(BinaryReader &binary) 
    {
      Shape::readFrom(binary);
      binary.read(radius);
      binary.read(height);
      binary.read(transform);
    }




    // ==================================================================
    // Sphere
    // ==================================================================

    /*! serialize out to given binary writer */
    int Sphere::writeTo(BinaryWriter &binary) 
    {
      Shape::writeTo(binary);
      binary.write(radius);
      binary.write(transform);
      return TYPE_SPHERE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Sphere::readFrom(BinaryReader &binary) 
    {
      Shape::readFrom(binary);
      binary.read(radius);
      binary.read(transform);
    }






  
    /*! serialize out to given binary writer */
    int Texture::writeTo(BinaryWriter &binary) 
    {
      return TYPE_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Texture::readFrom(BinaryReader &binary) 
    {
    }






    /*! serialize out to given binary writer */
    int ConstantTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      binary.write(value);
      return TYPE_CONSTANT_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void ConstantTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
      binary.read(value);
    }




    /*! serialize out to given binary writer */
    int ImageTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      binary.write(fileName);
      return TYPE_IMAGE_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void ImageTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
      binary.read(fileName);
    }




    /*! serialize out to given binary writer */
    int ScaleTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      binary.write(binary.serialize(tex1));
      binary.write(binary.serialize(tex2));
      binary.write(scale1);
      binary.write(scale2);
      return TYPE_SCALE_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void ScaleTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
      binary.read(tex1);
      binary.read(tex2);
      binary.read(scale1);
      binary.read(scale2);
    }


    /*! serialize out to given binary writer */
    int MixTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      binary.write(binary.serialize(map_amount));
      binary.write(binary.serialize(tex1));
      binary.write(binary.serialize(tex2));
      binary.write(scale1);
      binary.write(scale2);
      binary.write(amount);
      return TYPE_MIX_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MixTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
      binary.read(map_amount);
      binary.read(tex1);
      binary.read(tex2);
      binary.read(scale1);
      binary.read(scale2);
      binary.read(amount);
    }




    /*! serialize out to given binary writer */
    int PtexFileTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      binary.write(fileName);
      return TYPE_PTEX_FILE_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void PtexFileTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
      binary.read(fileName);
    }




   /*! serialize out to given binary writer */
    int WindyTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      return TYPE_WINDY_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void WindyTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
    }




   /*! serialize out to given binary writer */
    int FbmTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      return TYPE_FBM_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void FbmTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
    }




   /*! serialize out to given binary writer */
    int MarbleTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      binary.write(scale);
      return TYPE_MARBLE_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MarbleTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
      binary.read(scale);
    }





    /*! serialize out to given binary writer */
    int WrinkledTexture::writeTo(BinaryWriter &binary) 
    {
      Texture::writeTo(binary);
      return TYPE_WRINKLED_TEXTURE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void WrinkledTexture::readFrom(BinaryReader &binary) 
    {
      Texture::readFrom(binary);
    }






    /*! serialize out to given binary writer */
    int Material::writeTo(BinaryWriter &binary) 
    {
      /*! \todo serialize materials (can only do once mateirals are fleshed out) */
      return TYPE_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Material::readFrom(BinaryReader &binary) 
    {
      /*! \todo serialize materials (can only do once mateirals are fleshed out) */
    }





    /*! serialize out to given binary writer */
    int DisneyMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(anisotropic);
      binary.write(clearCoat);
      binary.write(clearCoatGloss);
      binary.write(color);
      binary.write(diffTrans);
      binary.write(eta);
      binary.write(flatness);
      binary.write(metallic);
      binary.write(roughness);
      binary.write(sheen);
      binary.write(sheenTint);
      binary.write(specTrans);
      binary.write(specularTint);
      binary.write(thin);
      return TYPE_DISNEY_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void DisneyMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(anisotropic);
      binary.read(clearCoat);
      binary.read(clearCoatGloss);
      binary.read(color);
      binary.read(diffTrans);
      binary.read(eta);
      binary.read(flatness);
      binary.read(metallic);
      binary.read(roughness);
      binary.read(sheen);
      binary.read(sheenTint);
      binary.read(specTrans);
      binary.read(specularTint);
      binary.read(thin);
    }






    /*! serialize out to given binary writer */
    int UberMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(kd);
      binary.write(binary.serialize(map_kd));
      binary.write(ks);
      binary.write(binary.serialize(map_ks));
      binary.write(kr);
      binary.write(binary.serialize(map_kr));
      binary.write(kt);
      binary.write(binary.serialize(map_kt));
      binary.write(opacity);
      binary.write(binary.serialize(map_opacity));
      binary.write(alpha);
      binary.write(binary.serialize(map_alpha));
      binary.write(shadowAlpha);
      binary.write(binary.serialize(map_shadowAlpha));
      binary.write(index);
      binary.write(roughness);
      binary.write(binary.serialize(map_roughness));
      binary.write(binary.serialize(map_bump));
      return TYPE_UBER_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void UberMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(kd);
      binary.read(map_kd);
      binary.read(ks);
      binary.read(map_ks);
      binary.read(kr);
      binary.read(map_kr);
      binary.read(kt);
      binary.read(map_kt);
      binary.read(opacity);
      binary.read(map_opacity);
      binary.read(alpha);
      binary.read(map_alpha);
      binary.read(shadowAlpha);
      binary.read(map_shadowAlpha);
      binary.read(index);
      binary.read(roughness);
      binary.read(map_roughness);
      binary.read(map_bump);
    }




    /*! serialize out to given binary writer */
    int SubstrateMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(kd);
      binary.write(binary.serialize(map_kd));
      binary.write(ks);
      binary.write(binary.serialize(map_ks));
      binary.write(binary.serialize(map_bump));
      binary.write(uRoughness);
      binary.write(binary.serialize(map_uRoughness));
      binary.write(vRoughness);
      binary.write(binary.serialize(map_vRoughness));
      binary.write(remapRoughness);
      return TYPE_SUBSTRATE_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void SubstrateMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(kd);
      binary.read(map_kd);
      binary.read(ks);
      binary.read(map_ks);
      binary.read(map_bump);
      binary.read(uRoughness);
      binary.read(map_uRoughness);
      binary.read(vRoughness);
      binary.read(map_vRoughness);
      binary.read(remapRoughness);
    }




        /*! serialize out to given binary writer */
    int SubSurfaceMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(uRoughness);
      binary.write(vRoughness);
      binary.write(remapRoughness);
      binary.write(name);
      return TYPE_SUBSURFACE_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void SubSurfaceMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(uRoughness);
      binary.read(vRoughness);
      binary.read(remapRoughness);
      binary.read(name);
    }




    /*! serialize out to given binary writer */
    int MixMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(material0));
      binary.write(binary.serialize(material1));
      binary.write(binary.serialize(map_amount));
      binary.write(amount);
      return TYPE_MIX_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MixMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(material0);
      binary.read(material1);
      binary.read(map_amount);
      binary.read(amount);
    }





    /*! serialize out to given binary writer */
    int TranslucentMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(map_kd));
      binary.write(reflect);
      binary.write(transmit);
      binary.write(kd);
      return TYPE_TRANSLUCENT_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void TranslucentMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(map_kd);
      binary.read(reflect);
      binary.read(transmit);
      binary.read(kd);
    }





    /*! serialize out to given binary writer */
    int GlassMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(kr);
      binary.write(kt);
      binary.write(index);
      return TYPE_GLASS_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void GlassMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(kr);
      binary.read(kt);
      binary.read(index);
    }



    /*! serialize out to given binary writer */
    int MatteMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(map_kd));
      binary.write(kd);
      binary.write(sigma);
      binary.write(binary.serialize(map_sigma));
      binary.write(binary.serialize(map_bump));
      return TYPE_MATTE_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MatteMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(map_kd);
      binary.read(kd);
      binary.read(sigma);
      binary.read(map_sigma);
      binary.read(map_bump);
    }




    /*! serialize out to given binary writer */
    int FourierMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(fileName);
      return TYPE_FOURIER_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void FourierMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(fileName);
    }




    /*! serialize out to given binary writer */
    int MetalMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(roughness);
      binary.write(uRoughness);
      binary.write(vRoughness);
      binary.write(spectrum_eta);
      binary.write(spectrum_k);
      binary.write(eta);
      binary.write(k);
      binary.write(binary.serialize(map_bump));
      binary.write(binary.serialize(map_roughness));
      binary.write(binary.serialize(map_uRoughness));
      binary.write(binary.serialize(map_vRoughness));
      return TYPE_METAL_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MetalMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(roughness);
      binary.read(uRoughness);
      binary.read(vRoughness);
      binary.read(spectrum_eta);
      binary.read(spectrum_k);
      binary.read(eta);
      binary.read(k);
      binary.read(map_bump);
      binary.read(map_roughness);
      binary.read(map_uRoughness);
      binary.read(map_vRoughness);
    }




    /*! serialize out to given binary writer */
    int MirrorMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(map_bump));
      binary.write(kr);
      return TYPE_MIRROR_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void MirrorMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(map_bump);
      binary.read(kr);
    }




  
    /*! serialize out to given binary writer */
    int PlasticMaterial::writeTo(BinaryWriter &binary) 
    {
      binary.write(binary.serialize(map_kd));
      binary.write(binary.serialize(map_ks));
      binary.write(kd);
      binary.write(ks);
      binary.write(roughness);
      binary.write(remapRoughness);
      binary.write(binary.serialize(map_roughness));
      binary.write(binary.serialize(map_bump));
      return TYPE_PLASTIC_MATERIAL;
    }
  
    /*! serialize _in_ from given binary file reader */
    void PlasticMaterial::readFrom(BinaryReader &binary) 
    {
      Material::readFrom(binary);
      binary.read(map_kd);
      binary.read(map_ks);
      binary.read(kd);
      binary.read(ks);
      binary.read(roughness);
      binary.read(remapRoughness);
      binary.read(map_roughness);
      binary.read(map_bump);
    }




    // ==================================================================
    // Instance
    // ==================================================================

    /*! serialize out to given binary writer */
    int Instance::writeTo(BinaryWriter &binary) 
    {
      binary.write(xfm);
      binary.write(binary.serialize(object));
      return TYPE_INSTANCE;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Instance::readFrom(BinaryReader &binary) 
    {
      binary.read(xfm);
      binary.read(object);
    }



  
    // ==================================================================
    // Object
    // ==================================================================

    /*! serialize out to given binary writer */
    int Object::writeTo(BinaryWriter &binary) 
    {
      binary.write(name);
      binary.write((int)shapes.size());
      for (auto geom : shapes) {
        binary.write(binary.serialize(geom));
      }
      binary.write((int)instances.size());
      for (auto inst : instances) {
        binary.write(binary.serialize(inst));
      }
      return TYPE_OBJECT;
    }
  
    /*! serialize _in_ from given binary file reader */
    void Object::readFrom(BinaryReader &binary) 
    {
      name = binary.read<std::string>();
      // read shapes
      int numShapes = binary.read<int>();
      assert(shapes.empty());
      for (int i=0;i<numShapes;i++)
        shapes.push_back(binary.getEntity<Shape>(binary.read<int>()));
      // read instances
      int numInstances = binary.read<int>();
      assert(instances.empty());
      for (int i=0;i<numInstances;i++)
        instances.push_back(binary.getEntity<Instance>(binary.read<int>()));
    }


  


    /*! save scene to given file name, and reutrn number of bytes written */
    size_t Scene::saveTo(const std::string &outFileName)
    {
      BinaryWriter binary(outFileName);
      Entity::SP sp = shared_from_this();
      binary.serialize(sp);
      return binary.binFile.tellp();
    }

    Scene::SP Scene::loadFrom(const std::string &inFileName)
    {
      BinaryReader binary(inFileName);
      if (binary.readEntities.empty())
        throw std::runtime_error("error in Scene::load - no entities");
      Scene::SP scene = std::dynamic_pointer_cast<Scene>(binary.readEntities.back());
      assert(scene);
      return scene;
    }
  
  } // ::pbrt::scene
} // ::pbrt



// std
#include <iostream>
#include <sstream>
#include <fstream>
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
  
#define    PBRT_PARSER_SYNTACTIC_FORMAT_MAJOR 1
#define    PBRT_PARSER_SYNTACTIC_FORMAT_MINOR 0
  
    const uint32_t ourFormatID = (PBRT_PARSER_SYNTACTIC_FORMAT_MAJOR << 16) + PBRT_PARSER_SYNTACTIC_FORMAT_MINOR;

    typedef enum : uint8_t {
      Type_eof=0,
        
        // param types
        Type_float=10,
        Type_integer,
        Type_bool,
        Type_string,
        Type_texture,
        Type_rgb,
        Type_spectrum,
        Type_point,
        Type_point2,
        Type_point3,
        Type_normal,
        Type_color,
        // object types
        Type_object=100,
        Type_instance,
        Type_shape,
        Type_material,
        Type_camera,
        Type_film,
        Type_medium,
        Type_sampler,
        Type_pixelFilter,
        Type_integrator,
        Type_volumeIntegrator,
        Type_surfaceIntegrator,
        /* add more here */
        END_OF_LIST
        } TypeTag;

    TypeTag typeOf(const std::string &type)
    {
      if (type == "bool")     return Type_bool;
      if (type == "integer")  return Type_integer;

      if (type == "float")    return Type_float;
      if (type == "point")    return Type_point;
      if (type == "point2")   return Type_point2;
      if (type == "point3")   return Type_point3;
      if (type == "normal")   return Type_normal;

      if (type == "rgb")      return Type_rgb;
      if (type == "color")    return Type_color;
      if (type == "spectrum") return Type_spectrum;

      if (type == "string")   return Type_string;

      if (type == "texture")  return Type_texture;

      throw std::runtime_error("un-recognized type '"+type+"'");
    }

    std::string typeToString(TypeTag tag)
    {
      switch(tag) {
      default:
      case Type_bool     : return "bool";
      case Type_integer  : return "integer";
      case Type_rgb      : return "rgb";
      case Type_float    : return "float";
      case Type_point    : return "point";
      case Type_point2   : return "point2";
      case Type_point3   : return "point3";
      case Type_normal   : return "normal";
      case Type_string   : return "string";
      case Type_spectrum : return "spectrum";
      case Type_texture  : return "texture";
        throw std::runtime_error("typeToString() : type tag '"+std::to_string((int)tag)+"' not implmemented");
      }
    }
  
    void BinaryWriter::writeRaw(const void *ptr, size_t size)
    {
      assert(ptr);
      outBuffer.top()->insert(outBuffer.top()->end(),(uint8_t*)ptr,(uint8_t*)ptr + size);
    }
    
    
    template<typename T>
    void BinaryWriter::write(const T &t)
    {
      writeRaw(&t,sizeof(t));
    }
    
    void BinaryWriter::write(const std::string &t)
    {
      write((int32_t)t.size());
      writeRaw(&t[0],t.size());
    }

    template<typename T>
    void BinaryWriter::writeVec(const std::vector<T> &t)
    {
      const void *ptr = (const void *)t.data();
      write(t.size());
      if (!t.empty())
        writeRaw(ptr,t.size()*sizeof(T));
    }

    void BinaryWriter::writeVec(const std::vector<bool> &t)
    {
      std::vector<unsigned char> asChar(t.size());
      for (size_t i=0;i<t.size();i++)
        asChar[i] = t[i]?1:0;
      writeVec(asChar);
    }

    void BinaryWriter::writeVec(const std::vector<std::string> &t)
    {
      write(t.size());
      for (auto &s : t) {
        write(s);
      }
    }

    /*! start a new write buffer on the stack - all future writes will go into this buffer */
    void BinaryWriter::startNewWrite()
    { outBuffer.push(std::make_shared<OutBuffer>()); }

    /*! write the topmost write buffer to disk, and free its memory */
    void BinaryWriter::executeWrite()
    {
      size_t size = outBuffer.top()->size();
      if (size) {
        // std::cout << "writing block of size " << size << std::endl;
        binFile.write((const char *)&size,sizeof(size));
        binFile.write((const char *)outBuffer.top()->data(),size);
      }
      outBuffer.pop();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Camera::SP camera)
    {
      if (!camera) return -1;
      
      static std::map<std::shared_ptr<Camera>,int> alreadyEmitted;
      if (alreadyEmitted.find(camera) != alreadyEmitted.end())
        return alreadyEmitted[camera];
      
      startNewWrite(); {
        write(Type_camera);
        write(camera->type);
        write(camera->transform);
        writeParams(*camera);
      } executeWrite();
      return alreadyEmitted[camera] = alreadyEmitted.size();
    }

    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Integrator::SP integrator)
    {
      if (!integrator) return -1;

      static std::map<std::shared_ptr<Integrator>,int> alreadyEmitted;
      if (alreadyEmitted.find(integrator) != alreadyEmitted.end())
        return alreadyEmitted[integrator];
      
      startNewWrite(); {
        write(Type_integrator);
        write(integrator->type);
        writeParams(*integrator);
      } executeWrite();
      return alreadyEmitted[integrator] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Shape::SP shape)
    {
      if (!shape) return -1;

      static std::map<std::shared_ptr<Shape>,int> alreadyEmitted;
      if (alreadyEmitted.find(shape) != alreadyEmitted.end())
        return alreadyEmitted[shape];
      
      startNewWrite(); {
        write(Type_shape);
        write(shape->type);
        write(shape->transform);
        write((int)emitOnce(shape->material));
        writeParams(*shape);
      } executeWrite();
      return alreadyEmitted[shape] = alreadyEmitted.size();
    }

    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Material::SP material)
    {
      if (!material) return -1;

      static std::map<std::shared_ptr<Material>,int> alreadyEmitted;
      if (alreadyEmitted.find(material) != alreadyEmitted.end())
        return alreadyEmitted[material];
      
      startNewWrite(); {
        write(Type_material);
        write(material->type);
        writeParams(*material);
      } executeWrite();
      return alreadyEmitted[material] = alreadyEmitted.size();
    }

    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(VolumeIntegrator::SP volumeIntegrator)
    {
      if (!volumeIntegrator) return -1;

      static std::map<std::shared_ptr<VolumeIntegrator>,int> alreadyEmitted;
      if (alreadyEmitted.find(volumeIntegrator) != alreadyEmitted.end())
        return alreadyEmitted[volumeIntegrator];
      
      startNewWrite(); {
        write(Type_volumeIntegrator);
        write(volumeIntegrator->type);
        writeParams(*volumeIntegrator);
      } executeWrite();
      return alreadyEmitted[volumeIntegrator] = alreadyEmitted.size();
    }

        
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(SurfaceIntegrator::SP surfaceIntegrator)
    {
      if (!surfaceIntegrator) return -1;

      static std::map<std::shared_ptr<SurfaceIntegrator>,int> alreadyEmitted;
      if (alreadyEmitted.find(surfaceIntegrator) != alreadyEmitted.end())
        return alreadyEmitted[surfaceIntegrator];
      
      startNewWrite(); {
        write(Type_surfaceIntegrator);
        write(surfaceIntegrator->type);
        writeParams(*surfaceIntegrator);
      } executeWrite();
      return alreadyEmitted[surfaceIntegrator] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Sampler::SP sampler)
    {
      if (!sampler) return -1;
      
      static std::map<std::shared_ptr<Sampler>,int> alreadyEmitted;
      if (alreadyEmitted.find(sampler) != alreadyEmitted.end())
        return alreadyEmitted[sampler];
      
      startNewWrite(); {
        write(Type_sampler);
        write(sampler->type);
        writeParams(*sampler);
      } executeWrite();
      return alreadyEmitted[sampler] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Film::SP film)
    {
      if (!film) return -1;

      static std::map<std::shared_ptr<Film>,int> alreadyEmitted;
      if (alreadyEmitted.find(film) != alreadyEmitted.end())
        return alreadyEmitted[film];
      
      startNewWrite(); {
        write(Type_film);
        write(film->type);
        writeParams(*film);
      } executeWrite();
      return alreadyEmitted[film] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(PixelFilter::SP pixelFilter)
    {
      if (!pixelFilter) return -1;
      
      static std::map<std::shared_ptr<PixelFilter>,int> alreadyEmitted;
      if (alreadyEmitted.find(pixelFilter) != alreadyEmitted.end())
        return alreadyEmitted[pixelFilter];
      
      startNewWrite(); {
        write(Type_pixelFilter);
        write(pixelFilter->type);
        writeParams(*pixelFilter);
      } executeWrite();
      return alreadyEmitted[pixelFilter] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Medium::SP medium)
    {
      if (!medium) return -1;
      
      static std::map<std::shared_ptr<Medium>,int> alreadyEmitted;
      if (alreadyEmitted.find(medium) != alreadyEmitted.end())
        return alreadyEmitted[medium];
      
      startNewWrite(); {
        write(Type_medium);
        write(medium->type);
        writeParams(*medium);
      } executeWrite();
      return alreadyEmitted[medium] = alreadyEmitted.size();
    }
    
    /*! if this instance has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Object::Instance::SP instance)
    {
      if (!instance) return -1;

      static std::map<std::shared_ptr<Object::Instance>,int> alreadyEmitted;
      if (alreadyEmitted.find(instance) != alreadyEmitted.end())
        return alreadyEmitted[instance];
      
      startNewWrite(); {
        write(Type_instance);
        write(instance->xfm);
        write((int)emitOnce(instance->object));
      } executeWrite();
      
      return alreadyEmitted[instance] = alreadyEmitted.size();
    }
    
    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Object::SP object)
    {
      if (!object) return -1;
      
      static std::map<std::shared_ptr<Object>,int> alreadyEmitted;
      if (alreadyEmitted.find(object) != alreadyEmitted.end())
        return alreadyEmitted[object];
      
      startNewWrite(); {
        write(Type_object);
        write(object->name);
        
        std::vector<int> shapeIDs;
        for (auto shape : object->shapes) shapeIDs.push_back(emitOnce(shape));
        writeVec(shapeIDs);

        std::vector<int> instanceIDs;
        for (auto instance : object->objectInstances) instanceIDs.push_back(emitOnce(instance));
        writeVec(instanceIDs);

      } executeWrite();
      return alreadyEmitted[object] = alreadyEmitted.size();
    }
    

    /*! write an enitity's set of parameters */
    void BinaryWriter::writeParams(const ParamSet &ps)
    {
      write((uint16_t)ps.param.size());
      for (auto it : ps.param) {
        const std::string name = it.first;
        Param::SP param = it.second;
        
        TypeTag typeTag = typeOf(param->getType());
        write(typeTag);
        write(name);

        switch(typeTag) {
        case Type_bool: {
          writeVec(*param->as<bool>());
        } break;
        case Type_float: 
        case Type_rgb: 
        case Type_color:
        case Type_point:
        case Type_point2:
        case Type_point3:
        case Type_normal:
          writeVec(*param->as<float>());
          break;
        case Type_integer: {
          writeVec(*param->as<int>());
        } break;
        case Type_spectrum:
        case Type_string: 
          writeVec(*param->as<std::string>());
          break;
        case Type_texture: {
          int32_t texID = emitOnce(param->as<Texture>()->texture);
          write(texID);
        } break;
        default:
          throw std::runtime_error("save-to-binary of parameter type '"
                                   +param->getType()+"' not implemented yet");
        }
      }
    }

    /*! if this object has already been written to file, return a
      handle; else write it once, create a unique handle, and return
      that */
    int BinaryWriter::emitOnce(Texture::SP texture)
    {
      if (!texture) return -1;
      
      static std::map<std::shared_ptr<Texture>,int> alreadyEmitted;
      if (alreadyEmitted.find(texture) != alreadyEmitted.end())
        return alreadyEmitted[texture];
      
      startNewWrite(); {
        write(Type_texture);
        write(texture->name);
        write(texture->texelType);
        write(texture->mapType);
        writeParams(*texture);
      } executeWrite();
      return alreadyEmitted[texture] = alreadyEmitted.size();
    }

    /*! write the root scene itself, by recusirvely writing all its entities */
    void BinaryWriter::emit(Scene::SP scene)
    {
      uint32_t formatID = ourFormatID;
      binFile.write((const char *)&formatID,sizeof(formatID));
      
      for (auto cam : scene->cameras) 
        emitOnce(cam);
      
      emitOnce(scene->film);
      emitOnce(scene->sampler);
      emitOnce(scene->integrator);
      emitOnce(scene->volumeIntegrator);
      emitOnce(scene->surfaceIntegrator);
      emitOnce(scene->pixelFilter);
      emitOnce(scene->world);

      startNewWrite(); {
        TypeTag eof = Type_eof;
        write(eof);
      } executeWrite();
    }
    
    void writeBinary(pbrt::syntactic::Scene::SP scene,
                     const std::string &fileName)
    {
      BinaryWriter writer(fileName);
      writer.emit(scene);
    }





    // ==================================================================
    // BinaryReader::Block
    // ==================================================================

    template<typename T>
    inline T BinaryReader::Block::read() {
      T t;
      readRaw(&t,sizeof(t));
      return t;
    }

    inline void BinaryReader::Block::readRaw(void *ptr, size_t size)
    {
      assert((pos + size) <= data.size());
      memcpy((char*)ptr,data.data()+pos,size);
      pos += size;
    };
      
    std::string BinaryReader::Block::readString()
    {
      int32_t size = read<int32_t>();
      std::string s(size,' ');
      readRaw(&s[0],size);
      return s;
    }

    template<typename T>
    inline void BinaryReader::Block::readVec(std::vector<T> &v)
    {
      size_t size = read<size_t>();
      v.resize(size);
      if (size) {
        readRaw((char*)v.data(),size*sizeof(T));
      }
    }
    inline void BinaryReader::Block::readVec(std::vector<std::string> &v)
    {
      size_t size = read<size_t>();
      v.resize(size);
      for (size_t i=0;i<size;i++)
        v[i] = readString();
      // v.push_back(readString());
    }
    inline void BinaryReader::Block::readVec(std::vector<bool> &v)
    {
      std::vector<unsigned char> asChar;
      readVec(asChar);
      v.resize(asChar.size());
      for (size_t i=0;i<asChar.size();i++)
        v[i] = asChar[i];
    }

    template<typename T>
    inline std::vector<T> BinaryReader::Block::readVec()
    {
      std::vector<T> t;
      readVec(t);
      return t;
    }
        


    // ==================================================================
    // BinaryReader
    // ==================================================================


    
    BinaryReader::BinaryReader(const std::string &fileName)
      : binFile(fileName)
    {
      assert(binFile.good());
      binFile.sync_with_stdio(false);
    }

    int BinaryReader::readHeader()
    {
      int formatID;
      assert(binFile.good());
      binFile.read((char*)&formatID,sizeof(formatID));
      assert(binFile.good());
      if (formatID != ourFormatID)
        throw std::runtime_error("file formats/versions do not match!");
      return formatID;
    }

    inline BinaryReader::Block BinaryReader::readBlock()
    {
      Block block;
      size_t size;
      assert(binFile.good());
      binFile.read((char*)&size,sizeof(size));
      assert(binFile.good());

      assert(size > 0);
      block.data.resize(size);
      assert(binFile.good());
      binFile.read((char*)block.data.data(),size);
      assert(binFile.good());

      return block;
    }
    
    template<typename T>
    typename ParamArray<T>::SP BinaryReader::readParam(const std::string &type, Block &block)
    {
      typename ParamArray<T>::SP param = std::make_shared<ParamArray<T>>(type);
      block.readVec(*param);
      return param;
    }
    
    void BinaryReader::readParams(ParamSet &paramSet, Block &block)
    {
      uint16_t numParams = block.read<uint16_t>();
      for (int paramID=0;paramID<numParams;paramID++) {
        TypeTag typeTag = block.read<TypeTag>();
        const std::string name = block.readString();
        switch(typeTag) {
        case Type_bool:
          paramSet.param[name] = readParam<bool>(typeToString(typeTag),block);
          break;
        case Type_integer:
          paramSet.param[name] = readParam<int>(typeToString(typeTag),block);
          break;
        case Type_float: 
        case Type_rgb: 
        case Type_color:
        case Type_point:
        case Type_point2:
        case Type_point3:
        case Type_normal:
          paramSet.param[name] = readParam<float>(typeToString(typeTag),block);
          break;
        case Type_spectrum: 
        case Type_string: 
          paramSet.param[name] = readParam<std::string>(typeToString(typeTag),block);
          break;
        case Type_texture: {
          int texID = block.read<int32_t>();
          typename ParamArray<Texture>::SP param = std::make_shared<ParamArray<Texture>>(typeToString(typeTag));
          param->texture = textures[texID];
          paramSet.param[name] = param;
        } break;
          //   int32_t texID = emitOnce(param->as<Texture>()->texture);
          //   write(texID);
        default:
          throw std::runtime_error("unknown parameter type "+std::to_string((int)typeTag));
        }
      }
    }
    
    Camera::SP BinaryReader::readCamera(Block &block)
    {
      std::string type = block.readString();
      Transform transform = block.read<Transform>();
      
      Camera::SP camera = std::make_shared<Camera>(type,transform);
      
      readParams(*camera,block);
      return camera;
    }
    
    Integrator::SP BinaryReader::readIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      Integrator::SP integrator = std::make_shared<Integrator>(type);
      
      readParams(*integrator,block);
      return integrator;
    }
    
    Shape::SP BinaryReader::readShape(Block &block)
    {
      std::string type = block.readString();
      Attributes::SP attributes = std::make_shared<Attributes>(); // not stored or read right now!?
      Transform transform = block.read<Transform>();

      Material::SP material;
      int matID = block.read<int32_t>();
      if (matID >= 0 && matID < (int)materials.size())
        material = materials[matID];
        
      Shape::SP shape = std::make_shared<Shape>(type,material,attributes,transform);
      
      readParams(*shape,block);
      return shape;
    }
    
    Material::SP BinaryReader::readMaterial(Block &block)
    {
      std::string type = block.readString();
      
      Material::SP material = std::make_shared<Material>(type);
      
      readParams(*material,block);
      return material;
    }
    
    Texture::SP BinaryReader::readTexture(Block &block)
    {
      std::string name = block.readString();
      std::string texelType = block.readString();
      std::string mapType = block.readString();
      
      Texture::SP texture = std::make_shared<Texture>(name,texelType,mapType);
      
      readParams(*texture,block);
      return texture;
    }
    
    SurfaceIntegrator::SP BinaryReader::readSurfaceIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      SurfaceIntegrator::SP surfaceIntegrator = std::make_shared<SurfaceIntegrator>(type);
      
      readParams(*surfaceIntegrator,block);
      return surfaceIntegrator;
    }
    
    PixelFilter::SP BinaryReader::readPixelFilter(Block &block)
    {
      std::string type = block.readString();
      
      PixelFilter::SP pixelFilter = std::make_shared<PixelFilter>(type);
      
      readParams(*pixelFilter,block);
      return pixelFilter;
    }
    
    VolumeIntegrator::SP BinaryReader::readVolumeIntegrator(Block &block)
    {
      std::string type = block.readString();
      
      VolumeIntegrator::SP volumeIntegrator = std::make_shared<VolumeIntegrator>(type);
      
      readParams(*volumeIntegrator,block);
      return volumeIntegrator;
    }
    
    Sampler::SP BinaryReader::readSampler(Block &block)
    {
      std::string type = block.readString();
      Sampler::SP sampler = std::make_shared<Sampler>(type);
      
      readParams(*sampler,block);
      return sampler;
    }
    
    Film::SP BinaryReader::readFilm(Block &block)
    {
      std::string type = block.readString();
      Film::SP film = std::make_shared<Film>(type);

      readParams(*film,block);
      return film;
    }

    Object::SP BinaryReader::readObject(Block &block)
    {
      const std::string name = block.readString();
      Object::SP object = std::make_shared<Object>(name);

      std::vector<int> shapeIDs = block.readVec<int>();
      for (int shapeID : shapeIDs)
        object->shapes.push_back(shapes[shapeID]);
      std::vector<int> instanceIDs = block.readVec<int>();
      for (int instanceID : instanceIDs)
        object->objectInstances.push_back(instances[instanceID]);
      
      return object;
    }



    Object::Instance::SP BinaryReader::readInstance(Block &block)
    {
      Transform transform = block.read<Transform>();
      int objectID = block.read<int>();
      Object::SP object = objects[objectID];
      return std::make_shared<Object::Instance>(object,transform);
    }

    


    Scene::SP BinaryReader::readScene()
    {
      Scene::SP scene = std::make_shared<Scene>();
      readHeader();

      while (1) {
        BinaryReader::Block block = readBlock();
        TypeTag typeTag = block.read<TypeTag>();

        switch(typeTag) {
        case Type_camera:
          scene->cameras.push_back(readCamera(block));
          break;
        case Type_pixelFilter:
          scene->pixelFilter = readPixelFilter(block);
          break;
        case Type_material:
          materials.push_back(readMaterial(block));
          break;
        case Type_shape:
          shapes.push_back(readShape(block));
          break;
        case Type_object:
          objects.push_back(readObject(block));
          break;
        case Type_instance:
          instances.push_back(readInstance(block));
          break;
        case Type_texture:
          textures.push_back(readTexture(block));
          break;
        case Type_integrator:
          scene->integrator = readIntegrator(block);
          break;
        case Type_surfaceIntegrator:
          scene->surfaceIntegrator = readSurfaceIntegrator(block);
          break;
        case Type_volumeIntegrator:
          scene->volumeIntegrator = readVolumeIntegrator(block);
          break;
        case Type_sampler:
          scene->sampler = readSampler(block);
          break;
        case Type_film:
          scene->film = readFilm(block);
          break;
        case Type_eof: {
          std::cout << "Done parsing!" << std::endl;
          assert(!objects.empty());
          scene->world = objects.back();
        } return scene;
        default:
          throw std::runtime_error("unsupported type tag '"+std::to_string((int)typeTag)+"'");
        };
      }
    }








    



    /*! given an already created scene, read given binary file, and populate this scene */
    extern "C" PBRT_PARSER_INTERFACE
    void pbrt_syntactic_readBinary(pbrt::syntactic::Scene::SP &scene,
                                const std::string &fileName)
    {
      BinaryReader reader(fileName);
      scene = reader.readScene();
    }


    extern "C" PBRT_PARSER_INTERFACE
    void pbrt_syntactic_writeBinary(pbrt::syntactic::Scene::SP scene,
                                 const std::string &fileName)
    { pbrt::syntactic::writeBinary(scene,fileName); }
  
    
  } // ::pbrt::syntactic
} // ::pbrt



/* A header-only implementation of the .ply file format.
 * https://github.com/nmwsharp/happly
 * By Nicholas Sharp - nsharp@cs.cmu.edu
 */

/*
MIT License

Copyright (c) 2018 Nick Sharp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

// General namespace wrapping all Happly things.
namespace happly {

// Enum specifying binary or ASCII filetypes. Binary is always little-endian.
enum class DataFormat { ASCII, Binary };

// Type name strings
// clang-format off
template <typename T> std::string typeName()                { return "unknown"; }
template<> inline std::string typeName<int8_t>()            { return "char";    }
template<> inline std::string typeName<uint8_t>()           { return "uchar";   }
template<> inline std::string typeName<int16_t>()           { return "short";   }
template<> inline std::string typeName<uint16_t>()          { return "ushort";  }
template<> inline std::string typeName<int32_t>()           { return "int";     }
template<> inline std::string typeName<uint32_t>()          { return "uint";    }
template<> inline std::string typeName<float>()             { return "float";   }
template<> inline std::string typeName<double>()            { return "double";  }
// clang-format on

// Template hackery that makes getProperty<T>() and friends pretty while automatically picking up smaller types
namespace {

// A pointer for the equivalent/smaller equivalent of a type (eg. when a double is requested a float works too, etc)
// long int is intentionally absent to avoid platform confusion
// clang-format off
template <class T> struct TypeChain                 { bool hasChildType = false;   typedef T            type; };
template <> struct TypeChain<int64_t>               { bool hasChildType = true;    typedef int32_t      type; };
template <> struct TypeChain<int32_t>               { bool hasChildType = true;    typedef int16_t      type; };
template <> struct TypeChain<int16_t>               { bool hasChildType = true;    typedef int8_t       type; };
template <> struct TypeChain<uint64_t>              { bool hasChildType = true;    typedef uint32_t     type; };
template <> struct TypeChain<uint32_t>              { bool hasChildType = true;    typedef uint16_t     type; };
template <> struct TypeChain<uint16_t>              { bool hasChildType = true;    typedef uint8_t      type; };
template <> struct TypeChain<double>                { bool hasChildType = true;    typedef float        type; };
// clang-format on

// clang-format off
template <class T> struct CanonicalName                     { typedef T         type; };
template <> struct CanonicalName<char>                      { typedef int8_t    type; };
template <> struct CanonicalName<unsigned char>             { typedef uint8_t   type; };
template <> struct CanonicalName<size_t>                    { typedef std::conditional<std::is_same<std::make_signed<size_t>::type, int>::value, uint32_t, uint64_t>::type type; };
// clang-format on
} // namespace

/**
 * @brief A generic property, which is associated with some element. Can be plain Property or a ListProperty, of some
 * type.  Generally, the user should not need to interact with these directly, but they are exposed in case someone
 * wants to get clever.
 */
class Property {

public:
  /**
   * @brief Create a new Property with the given name.
   *
   * @param name_
   */
  Property(const std::string& name_) : name(name_){};
  virtual ~Property(){};

  std::string name;

  /**
   * @brief Reserve memory.
   * 
   * @param capacity Expected number of elements.
   */
  virtual void reserve(size_t capacity) = 0;

  /**
   * @brief (ASCII reading) Parse out the next value of this property from a list of tokens.
   *
   * @param tokens The list of property tokens for the element.
   * @param currEntry Index in to tokens, updated after this property is read.
   */
  virtual void parseNext(const std::vector<std::string>& tokens, size_t& currEntry) = 0;

  /**
   * @brief (binary reading) Copy the next value of this property from a stream of bits.
   *
   * @param stream Stream to read from.
   */
  virtual void readNext(std::ifstream& stream) = 0;

  /**
   * @brief (reading) Write a header entry for this property.
   *
   * @param outStream Stream to write to.
   */
  virtual void writeHeader(std::ofstream& outStream) = 0;

  /**
   * @brief (ASCII writing) write this property for some element to a stream in plaintext
   *
   * @param outStream Stream to write to.
   * @param iElement index of the element to write.
   */
  virtual void writeDataASCII(std::ofstream& outStream, size_t iElement) = 0;

  /**
   * @brief (binary writing) copy the bits of this property for some element to a stream
   *
   * @param outStream Stream to write to.
   * @param iElement index of the element to write.
   */
  virtual void writeDataBinary(std::ofstream& outStream, size_t iElement) = 0;

  /**
   * @brief Number of element entries for this property
   *
   * @return
   */
  virtual size_t size() = 0;

  /**
   * @brief A string naming the type of the property
   *
   * @return
   */
  virtual std::string propertyTypeName() = 0;
};

/**
 * @brief A property which takes a single value (not a list).
 */
template <class T>
class TypedProperty : public Property {

public:
  /**
   * @brief Create a new Property with the given name.
   *
   * @param name_
   */
  TypedProperty(const std::string& name_) : Property(name_) {
    if (typeName<T>() == "unknown") {
      // TODO should really be a compile-time error
      throw std::runtime_error("Attempted property type does not match any type defined by the .ply format.");
    }
  };

  /**
   * @brief Create a new property and initialize with data.
   *
   * @param name_
   * @param data_
   */
  TypedProperty(const std::string& name_, const std::vector<T>& data_) : Property(name_), data(data_) {
    if (typeName<T>() == "unknown") {
      throw std::runtime_error("Attempted property type does not match any type defined by the .ply format.");
    }
  };

  virtual ~TypedProperty() override{};

  /**
   * @brief Reserve memory.
   * 
   * @param capacity Expected number of elements.
   */
  virtual void reserve(size_t capacity) override {
    data.reserve(capacity);
  }

  /**
   * @brief (ASCII reading) Parse out the next value of this property from a list of tokens.
   *
   * @param tokens The list of property tokens for the element.
   * @param currEntry Index in to tokens, updated after this property is read.
   */
  virtual void parseNext(const std::vector<std::string>& tokens, size_t& currEntry) override {
    data.emplace_back();
    std::istringstream iss(tokens[currEntry]);
    iss >> data.back();
    currEntry++;
  };

  /**
   * @brief (binary reading) Copy the next value of this property from a stream of bits.
   *
   * @param stream Stream to read from.
   */
  virtual void readNext(std::ifstream& stream) override {
    data.emplace_back();
    stream.read((char*)&data.back(), sizeof(T));
  }

  /**
   * @brief (reading) Write a header entry for this property.
   *
   * @param outStream Stream to write to.
   */
  virtual void writeHeader(std::ofstream& outStream) override {
    outStream << "property " << typeName<T>() << " " << name << "\n";
  }

  /**
   * @brief (ASCII writing) write this property for some element to a stream in plaintext
   *
   * @param outStream Stream to write to.
   * @param iElement index of the element to write.
   */
  virtual void writeDataASCII(std::ofstream& outStream, size_t iElement) override {
    outStream.precision(std::numeric_limits<T>::max_digits10);
    outStream << data[iElement];
  }

  /**
   * @brief (binary writing) copy the bits of this property for some element to a stream
   *
   * @param outStream Stream to write to.
   * @param iElement index of the element to write.
   */
  virtual void writeDataBinary(std::ofstream& outStream, size_t iElement) override {
    outStream.write((char*)&data[iElement], sizeof(T));
  }

  /**
   * @brief Number of element entries for this property
   *
   * @return
   */
  virtual size_t size() override { return data.size(); }


  /**
   * @brief A string naming the type of the property
   *
   * @return
   */
  virtual std::string propertyTypeName() override { return typeName<T>(); }

  /**
   * @brief The actual data contained in the property
   */
  std::vector<T> data;
};

// outstream doesn't do what we want with chars, these specializations supersede the general behavior to ensure chars
// get written correctly.
template <>
inline void TypedProperty<uint8_t>::writeDataASCII(std::ofstream& outStream, size_t iElement) {
  outStream << (int)data[iElement];
}
template <>
inline void TypedProperty<int8_t>::writeDataASCII(std::ofstream& outStream, size_t iElement) {
  outStream << (int)data[iElement];
}
template <>
inline void TypedProperty<uint8_t>::parseNext(const std::vector<std::string>& tokens, size_t& currEntry) {
  std::istringstream iss(tokens[currEntry]);
  int intVal;
  iss >> intVal;
  data.push_back((uint8_t)intVal);
  currEntry++;
}
template <>
inline void TypedProperty<int8_t>::parseNext(const std::vector<std::string>& tokens, size_t& currEntry) {
  std::istringstream iss(tokens[currEntry]);
  int intVal;
  iss >> intVal;
  data.push_back((int8_t)intVal);
  currEntry++;
}


/**
 * @brief A property which is a list of value (eg, 3 doubles). Note that lists are always variable length per-element.
 */
template <class T>
class TypedListProperty : public Property {

public:
  /**
   * @brief Create a new Property with the given name.
   *
   * @param name_
   */
  TypedListProperty(const std::string& name_, int listCountBytes_) : Property(name_), listCountBytes(listCountBytes_) {
    if (typeName<T>() == "unknown") {
      throw std::runtime_error("Attempted property type does not match any type defined by the .ply format.");
    }
  };

  /**
   * @brief Create a new property and initialize with data
   *
   * @param name_
   * @param data_
   */
  TypedListProperty(const std::string& name_, const std::vector<std::vector<T>>& data_) : Property(name_), data(data_) {
    if (typeName<T>() == "unknown") {
      throw std::runtime_error("Attempted property type does not match any type defined by the .ply format.");
    }
  };

  virtual ~TypedListProperty() override{};

  /**
   * @brief Reserve memory.
   * 
   * @param capacity Expected number of elements.
   */
  virtual void reserve(size_t capacity) override {
    data.reserve(capacity);
    for (size_t i = 0; i < data.size(); i++) {
      data[i].reserve(3); // optimize for triangle meshes
    }
  }

  /**
   * @brief (ASCII reading) Parse out the next value of this property from a list of tokens.
   *
   * @param tokens The list of property tokens for the element.
   * @param currEntry Index in to tokens, updated after this property is read.
   */
  virtual void parseNext(const std::vector<std::string>& tokens, size_t& currEntry) override {

    std::istringstream iss(tokens[currEntry]);
    size_t count;
    iss >> count;
    currEntry++;

    data.emplace_back();
    data.back().resize(count);
    for (size_t iCount = 0; iCount < count; iCount++) {
      std::istringstream iss(tokens[currEntry]);
      iss >> data.back()[iCount];
      currEntry++;
    }
  }

  /**
   * @brief (binary reading) Copy the next value of this property from a stream of bits.
   *
   * @param stream Stream to read from.
   */
  virtual void readNext(std::ifstream& stream) override {

    // Read the size of the list
    size_t count = 0;
    stream.read(((char*)&count), listCountBytes);

    // Read list elements
    data.emplace_back();
    data.back().resize(count);
    stream.read((char*)&data.back().front(), count*sizeof(T));
  }

  /**
   * @brief (reading) Write a header entry for this property. Note that we already use "uchar" for the list count type.
   *
   * @param outStream Stream to write to.
   */
  virtual void writeHeader(std::ofstream& outStream) override {
    // NOTE: We ALWAYS use uchar as the list count output type
    outStream << "property list uchar " << typeName<T>() << " " << name << "\n";
  }

  /**
   * @brief (ASCII writing) write this property for some element to a stream in plaintext
   *
   * @param outStream Stream to write to.
   * @param iElement index of the element to write.
   */
  virtual void writeDataASCII(std::ofstream& outStream, size_t iElement) override {
    std::vector<T>& elemList = data[iElement];
  
    // Get the number of list elements as a uchar, and ensure the value fits
    uint8_t count = elemList.size();
    if(count != elemList.size()) {
      throw std::runtime_error("List property has an element with more entries than fit in a uchar. See note in README.");
    }

    outStream << elemList.size();
    outStream.precision(std::numeric_limits<T>::max_digits10);
    for (size_t iEntry = 0; iEntry < elemList.size(); iEntry++) {
      outStream << " " << elemList[iEntry];
    }
  }

  /**
   * @brief (binary writing) copy the bits of this property for some element to a stream
   *
   * @param outStream Stream to write to.
   * @param iElement index of the element to write.
   */
  virtual void writeDataBinary(std::ofstream& outStream, size_t iElement) override {
    std::vector<T>& elemList = data[iElement];

    // Get the number of list elements as a uchar, and ensure the value fits
    uint8_t count = elemList.size();
    if(count != elemList.size()) {
      throw std::runtime_error("List property has an element with more entries than fit in a uchar. See note in README.");
    }

    outStream.write((char*)&count, sizeof(uint8_t));
    for (size_t iEntry = 0; iEntry < elemList.size(); iEntry++) {
      outStream.write((char*)&elemList[iEntry], sizeof(T));
    }
  }

  /**
   * @brief Number of element entries for this property
   *
   * @return
   */
  virtual size_t size() override { return data.size(); }


  /**
   * @brief A string naming the type of the property
   *
   * @return
   */
  virtual std::string propertyTypeName() override { return typeName<T>(); }

  /**
   * @brief The actualy data lists for the property
   */
  std::vector<std::vector<T>> data;

  /**
   * @brief The number of bytes used to store the count for lists of data.
   */
  int listCountBytes = -1;
};

// outstream doesn't do what we want with int8_ts, these specializations supersede the general behavior to ensure
// int8_ts get written correctly.
template <>
inline void TypedListProperty<uint8_t>::writeDataASCII(std::ofstream& outStream, size_t iElement) {
  std::vector<uint8_t>& elemList = data[iElement];
  outStream << elemList.size();
  outStream.precision(std::numeric_limits<uint8_t>::max_digits10);
  for (size_t iEntry = 0; iEntry < elemList.size(); iEntry++) {
    outStream << " " << (int)elemList[iEntry];
  }
}
template <>
inline void TypedListProperty<int8_t>::writeDataASCII(std::ofstream& outStream, size_t iElement) {
  std::vector<int8_t>& elemList = data[iElement];
  outStream << elemList.size();
  outStream.precision(std::numeric_limits<int8_t>::max_digits10);
  for (size_t iEntry = 0; iEntry < elemList.size(); iEntry++) {
    outStream << " " << (int)elemList[iEntry];
  }
}
template <>
inline void TypedListProperty<uint8_t>::parseNext(const std::vector<std::string>& tokens, size_t& currEntry) {

  std::istringstream iss(tokens[currEntry]);
  size_t count;
  iss >> count;
  currEntry++;

  std::vector<uint8_t> thisVec;
  for (size_t iCount = 0; iCount < count; iCount++) {
    std::istringstream iss(tokens[currEntry]);
    int intVal;
    iss >> intVal;
    thisVec.push_back((uint8_t)intVal);
    currEntry++;
  }
  data.push_back(thisVec);
}
template <>
inline void TypedListProperty<int8_t>::parseNext(const std::vector<std::string>& tokens, size_t& currEntry) {

  std::istringstream iss(tokens[currEntry]);
  size_t count;
  iss >> count;
  currEntry++;

  std::vector<int8_t> thisVec;
  for (size_t iCount = 0; iCount < count; iCount++) {
    std::istringstream iss(tokens[currEntry]);
    int intVal;
    iss >> intVal;
    thisVec.push_back((int8_t)intVal);
    currEntry++;
  }
  data.push_back(thisVec);
}

/**
 * @brief Helper function to construct a new property of the appropriate type.
 *
 * @param name The name of the property to construct.
 * @param typeStr A string naming the type according to the format.
 * @param isList Is this a plain property, or a list property?
 * @param listCountTypeStr If a list property, the type of the count varible.
 *
 * @return A new Property with the proper type.
 */
inline std::unique_ptr<Property> createPropertyWithType(const std::string& name, const std::string& typeStr, bool isList,
                                                        const std::string& listCountTypeStr) {

  // == Figure out how many bytes the list count field has, if this is a list type
  // Note: some files seem to use signed types here, we read the width but always parse as if unsigned
  int listCountBytes = -1;
  if (isList) {
    if (listCountTypeStr == "uchar" || listCountTypeStr == "uint8" || listCountTypeStr == "char" ||
        listCountTypeStr == "int8") {
      listCountBytes = 1;
    } else if (listCountTypeStr == "ushort" || listCountTypeStr == "uint16" || listCountTypeStr == "short" ||
               listCountTypeStr == "int16") {
      listCountBytes = 2;
    } else if (listCountTypeStr == "uint" || listCountTypeStr == "uint32" || listCountTypeStr == "int" ||
               listCountTypeStr == "int32") {
      listCountBytes = 4;
    } else {
      throw std::runtime_error("Unrecognized list count type: " + listCountTypeStr);
    }
  }

  // = Unsigned int

  // 8 bit unsigned
  if (typeStr == "uchar" || typeStr == "uint8") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<uint8_t>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<uint8_t>(name));
    }
  }

  // 16 bit unsigned
  else if (typeStr == "ushort" || typeStr == "uint16") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<uint16_t>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<uint16_t>(name));
    }
  }

  // 32 bit unsigned
  else if (typeStr == "uint" || typeStr == "uint32") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<uint32_t>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<uint32_t>(name));
    }
  }

  // = Signed int

  // 8 bit signed
  if (typeStr == "char" || typeStr == "int8") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<int8_t>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<int8_t>(name));
    }
  }

  // 16 bit signed
  else if (typeStr == "short" || typeStr == "int16") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<int16_t>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<int16_t>(name));
    }
  }

  // 32 bit signed
  else if (typeStr == "int" || typeStr == "int32") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<int32_t>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<int32_t>(name));
    }
  }

  // = Float

  // 32 bit float
  else if (typeStr == "float" || typeStr == "float32") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<float>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<float>(name));
    }
  }

  // 64 bit float
  else if (typeStr == "double" || typeStr == "float64") {
    if (isList) {
      return std::unique_ptr<Property>(new TypedListProperty<double>(name, listCountBytes));
    } else {
      return std::unique_ptr<Property>(new TypedProperty<double>(name));
    }
  }

  else {
    throw std::runtime_error("Data type: " + typeStr + " cannot be mapped to .ply format");
  }
}

/**
 * @brief An element (more properly an element type) in the .ply object. Tracks the name of the elemnt type (eg,
 * "vertices"), the number of elements of that type (eg, 1244), and any properties associated with that element (eg,
 * "position", "color").
 */
class Element {

public:
  /**
   * @brief Create a new element type.
   *
   * @param name_ Name of the element type (eg, "vertices")
   * @param count_ Number of instances of this element.
   */
  Element(const std::string& name_, size_t count_) : name(name_), count(count_) {}

  std::string name;
  size_t count;
  std::vector<std::unique_ptr<Property>> properties;

  /**
   * @brief Check if a property exists.
   * 
   * @param target The name of the property to get.
   *
   * @return Whether the target property exists.
   */
  bool hasProperty(const std::string& target) {
    for (std::unique_ptr<Property>& prop : properties) {
      if (prop->name == target) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Low-level method to get a pointer to a property. Users probably don't need to call this.
   *
   * @param target The name of the property to get.
   *
   * @return A (unique_ptr) pointer to the property.
   */
  std::unique_ptr<Property>& getPropertyPtr(const std::string& target) {
    for (std::unique_ptr<Property>& prop : properties) {
      if (prop->name == target) {
        return prop;
      }
    }
    throw std::runtime_error("PLY parser: element " + name + " does not have property " + target);
  }

  /**
   * @brief Add a new (plain, not list) property for this element type.
   *
   * @tparam T The type of the property
   * @param propertyName The name of the property
   * @param data The data for the property. Must have the same length as the number of elements.
   */
  template <class T>
  void addProperty(const std::string& propertyName, const std::vector<T>& data) {

    if (data.size() != count) {
      throw std::runtime_error("PLY write: new property " + propertyName + " has size which does not match element");
    }

    // If there is already some property with this name, remove it
    for (size_t i = 0; i < properties.size(); i++) {
      if (properties[i]->name == propertyName) {
        properties.erase(properties.begin() + i);
        i--;
      }
    }

    // Copy to canonical type. Often a no-op, but takes care of standardizing widths across platforms.
    std::vector<typename CanonicalName<T>::type> canonicalVec(data.begin(), data.end());

    properties.push_back(
        std::unique_ptr<Property>(new TypedProperty<typename CanonicalName<T>::type>(propertyName, canonicalVec)));
  }

  /**
   * @brief Add a new list property for this element type.
   *
   * @tparam T The type of the property (eg, "double" for a list of doubles)
   * @param propertyName The name of the property
   * @param data The data for the property. Outer vector must have the same length as the number of elements.
   */
  template <class T>
  void addListProperty(const std::string& propertyName, const std::vector<std::vector<T>>& data) {

    if (data.size() != count) {
      throw std::runtime_error("PLY write: new property " + propertyName + " has size which does not match element");
    }

    // If there is already some property with this name, remove it
    for (size_t i = 0; i < properties.size(); i++) {
      if (properties[i]->name == propertyName) {
        properties.erase(properties.begin() + i);
        i--;
      }
    }

    // Copy to canonical type. Often a no-op, but takes care of standardizing widths across platforms.
    std::vector<std::vector<typename CanonicalName<T>::type>> canonicalListVec;
    for (const std::vector<T>& subList : data) {
      canonicalListVec.emplace_back(subList.begin(), subList.end());
    }

    properties.push_back(std::unique_ptr<Property>(
        new TypedListProperty<typename CanonicalName<T>::type>(propertyName, canonicalListVec)));
  }

  /**
   * @brief Get a vector of a data from a property for this element. Automatically promotes to larger types. Throws if
   * requested data is unavailable.
   *
   * @tparam T The type of data requested
   * @param propertyName The name of the property to get.
   *
   * @return The data.
   */
  template <class T>
  std::vector<T> getProperty(const std::string& propertyName) {

    // Find the property
    std::unique_ptr<Property>& prop = getPropertyPtr(propertyName);

    // Get a copy of the data with auto-promoting type magic
    return getDataFromPropertyRecursive<T, T>(prop.get());
  }

  /**
   * @brief Get a vector of lists of data from a property for this element. Automatically promotes to larger types.
   * Throws if requested data is unavailable.
   *
   * @tparam T The type of data requested
   * @param propertyName The name of the property to get.
   *
   * @return The data.
   */
  template <class T>
  std::vector<std::vector<T>> getListProperty(const std::string& propertyName) {

    // Find the property
    std::unique_ptr<Property>& prop = getPropertyPtr(propertyName);

    // Get a copy of the data with auto-promoting type magic
    return getDataFromListPropertyRecursive<T, T>(prop.get());
  }


  /**
   * @brief Get a vector of lists of data from a property for this element. Automatically promotes to larger types.
   * Unlike getListProperty(), this method will additionally convert between types of different sign (eg, requesting and
   * int32 would get data from a uint32); doing so naively converts between signed and unsigned types. This is typically
   * useful for data representing indices, which might be stored as signed or unsigned numbers.
   *
   * @tparam T The type of data requested
   * @param propertyName The name of the property to get.
   *
   * @return The data.
   */
  template <class T>
  std::vector<std::vector<T>> getListPropertyAnySign(const std::string& propertyName) {

    // Find the property
    std::unique_ptr<Property>& prop = getPropertyPtr(propertyName);

    // Get a copy of the data with auto-promoting type magic
    try {
      // First, try the usual approach, looking for a version of the property with the same signed-ness and possibly
      // smaller size
      return getDataFromListPropertyRecursive<T, T>(prop.get());
    } catch (std::runtime_error orig_e) {

      // If the usual approach fails, look for a version with opposite signed-ness
      try {

        // This type has the oppopsite signeness as the input type
        typedef typename CanonicalName<T>::type Tcan;
        typedef typename std::conditional<std::is_signed<Tcan>::value, typename std::make_unsigned<Tcan>::type,
                                          typename std::make_signed<Tcan>::type>::type OppsignType;

        std::vector<std::vector<OppsignType>> oppSignedResult = getListProperty<OppsignType>(propertyName);

        // Very explicitly convert while copying
        std::vector<std::vector<T>> origSignResult;
        for (std::vector<OppsignType>& l : oppSignedResult) {
          std::vector<T> newL;
          for (OppsignType& v : l) {
            newL.push_back(static_cast<T>(v));
          }
          origSignResult.push_back(newL);
        }

        return origSignResult;
      } catch (std::runtime_error new_e) {
        throw orig_e;
      }

      throw orig_e;
    }
  }


  /**
   * @brief Performs sanity checks on the element, throwing if any fail.
   */
  void validate() {

    // Make sure no properties have duplicate names, and no names have whitespace
    for (size_t iP = 0; iP < properties.size(); iP++) {
      for (char c : properties[iP]->name) {
        if (std::isspace(c)) {
          throw std::runtime_error("Ply validate: illegal whitespace in name " + properties[iP]->name);
        }
      }
      for (size_t jP = iP + 1; jP < properties.size(); jP++) {
        if (properties[iP]->name == properties[jP]->name) {
          throw std::runtime_error("Ply validate: multiple properties with name " + properties[iP]->name);
        }
      }
    }

    // Make sure all properties have right length
    for (size_t iP = 0; iP < properties.size(); iP++) {
      if (properties[iP]->size() != count) {
        throw std::runtime_error("Ply validate: property has wrong size. " + properties[iP]->name +
                                 " does not match element size.");
      }
    }
  }

  /**
   * @brief Writes out this element's information to the file header.
   *
   * @param outStream The stream to use.
   */
  void writeHeader(std::ofstream& outStream) {

    outStream << "element " << name << " " << count << "\n";

    for (std::unique_ptr<Property>& p : properties) {
      p->writeHeader(outStream);
    }
  }

  /**
   * @brief (ASCII writing) Writes out all of the data for every element of this element type to the stream, including
   * all contained properties.
   *
   * @param outStream The stream to write to.
   */
  void writeDataASCII(std::ofstream& outStream) {
    // Question: what is the proper output for an element with no properties? Here, we write a blank line, so there is
    // one line per element no matter what.
    for (size_t iE = 0; iE < count; iE++) {
      for (size_t iP = 0; iP < properties.size(); iP++) {
        properties[iP]->writeDataASCII(outStream, iE);
        if (iP < properties.size() - 1) {
          outStream << " ";
        }
      }
      outStream << "\n";
    }
  }


  /**
   * @brief (binary writing) Writes out all of the data for every element of this element type to the stream, including
   * all contained properties.
   *
   * @param outStream The stream to write to.
   */
  void writeDataBinary(std::ofstream& outStream) {
    for (size_t iE = 0; iE < count; iE++) {
      for (size_t iP = 0; iP < properties.size(); iP++) {
        properties[iP]->writeDataBinary(outStream, iE);
      }
    }
  }


  /**
   * @brief Helper function which does the hard work to implement type promotion for data getters. Throws if type
   * conversion fails.
   *
   * @tparam D The desired output type
   * @tparam T The current attempt for the actual type of the property
   * @param prop The property to get (does not delete nor share pointer)
   *
   * @return The data, with the requested type
   */
  template <class D, class T>
  std::vector<D> getDataFromPropertyRecursive(Property* prop) {

    typedef typename CanonicalName<T>::type Tcan;

    { // Try to return data of type D from a property of type T
      TypedProperty<Tcan>* castedProp = dynamic_cast<TypedProperty<Tcan>*>(prop);
      if (castedProp) {
        // Succeeded, return a buffer of the data (copy while converting type)
        std::vector<D> castedVec;
        for (Tcan& v : castedProp->data) {
          castedVec.push_back(static_cast<D>(v));
        }
        return castedVec;
      }
    }

    TypeChain<Tcan> chainType;
    if (chainType.hasChildType) {
      return getDataFromPropertyRecursive<D, typename TypeChain<Tcan>::type>(prop);
    } else {
      // No smaller type to try, failure
      throw std::runtime_error("PLY parser: property " + prop->name + " cannot be coerced to requested type " +
                               typeName<D>() + ". Has type " + prop->propertyTypeName());
    }
  }


  /**
   * @brief Helper function which does the hard work to implement type promotion for list data getters. Throws if type
   * conversion fails.
   *
   * @tparam D The desired output type
   * @tparam T The current attempt for the actual type of the property
   * @param prop The property to get (does not delete nor share pointer)
   *
   * @return The data, with the requested type
   */
  template <class D, class T>
  std::vector<std::vector<D>> getDataFromListPropertyRecursive(Property* prop) {
    typedef typename CanonicalName<T>::type Tcan;

    TypedListProperty<Tcan>* castedProp = dynamic_cast<TypedListProperty<Tcan>*>(prop);
    if (castedProp) {
      // Succeeded, return a buffer of the data (copy while converting type)
      std::vector<std::vector<D>> castedListVec;
      for (std::vector<Tcan>& l : castedProp->data) {
        std::vector<D> newL;
        for (Tcan& v : l) {
          newL.push_back(static_cast<D>(v));
        }
        castedListVec.push_back(newL);
      }
      return castedListVec;
    }

    TypeChain<Tcan> chainType;
    if (chainType.hasChildType) {
      return getDataFromListPropertyRecursive<D, typename TypeChain<Tcan>::type>(prop);
    } else {
      // No smaller type to try, failure
      throw std::runtime_error("PLY parser: list property " + prop->name +
                               " cannot be coerced to requested type list " + typeName<D>() + ". Has type list " +
                               prop->propertyTypeName());
    }
  }
};


// Some string helpers
namespace {

inline std::string trimSpaces(const std::string& input) {
  size_t start = 0;
  while (start < input.size() && input[start] == ' ') start++;
  size_t end = input.size();
  while (end > start && (input[end - 1] == ' ' || input[end - 1] == '\n' || input[end - 1] == '\r')) end--;
  return input.substr(start, end - start);
}

inline std::vector<std::string> tokenSplit(const std::string& input) {
  std::vector<std::string> result;
  size_t curr = 0;
  size_t found = 0;
  while ((found = input.find_first_of(' ', curr)) != std::string::npos) {
    std::string token = input.substr(curr, found - curr);
    token = trimSpaces(token);
    if (token.size() > 0) {
      result.push_back(token);
    }
    curr = found + 1;
  }
  std::string token = input.substr(curr);
  token = trimSpaces(token);
  if (token.size() > 0) {
    result.push_back(token);
  }

  return result;
}

inline bool startsWith(const std::string& input, const std::string& query) { return input.compare(0, query.length(), query) == 0; }
}; // namespace


/**
 * @brief Primary class; represents a set of data in the .ply format.
 */
class PLYData {

public:
  /**
   * @brief Create an empty PLYData object.
   */
  PLYData(){};

  /**
   * @brief Initialize a PLYData by reading from a file. Throws if any failures occur.
   *
   * @param filename The file to read from.
   * @param verbose If true, print useful info about the file to stdout
   */
  PLYData(const std::string& filename, bool verbose = false) {

    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;

    if (verbose) cout << "PLY parser: Reading ply file: " << filename << endl;

    // Open a file in binary always, in case it turns out to have binary data.
    std::ifstream inStream(filename, std::ios::binary);
    if (inStream.fail()) {
      throw std::runtime_error("PLY parser: Could not open file " + filename);
    }


    // == Process the header
    parseHeader(inStream, verbose);


    // === Parse data from a binary file
    if (inputDataFormat == DataFormat::Binary) {
      parseBinary(inStream, verbose);
    }
    // === Parse data from an ASCII file
    else if (inputDataFormat == DataFormat::ASCII) {
      parseASCII(inStream, verbose);
    }


    if (verbose) {
      cout << "  - Finished parsing file." << endl;
    }
  }

  /**
   * @brief Perform sanity checks on the file, throwing if any fail.
   */
  void validate() {

    for (size_t iE = 0; iE < elements.size(); iE++) {
      for (char c : elements[iE].name) {
        if (std::isspace(c)) {
          throw std::runtime_error("Ply validate: illegal whitespace in element name " + elements[iE].name);
        }
      }
      for (size_t jE = iE + 1; jE < elements.size(); jE++) {
        if (elements[iE].name == elements[jE].name) {
          throw std::runtime_error("Ply validate: duplcate element name " + elements[iE].name);
        }
      }
    }

    // Do a quick validation sanity check
    for (Element& e : elements) {
      e.validate();
    }
  }

  /**
   * @brief Write this data to a .ply file.
   *
   * @param filename The file to write to.
   * @param format The format to use (binary or ascii?)
   */
  void write(const std::string& filename, DataFormat format = DataFormat::ASCII) {
    outputDataFormat = format;

    validate();

    // Open stream for writing
    std::ofstream outStream(filename, std::ios::out | std::ios::binary);
    if (!outStream.good()) {
      throw std::runtime_error("Ply writer: Could not open output file " + filename + " for writing");
    }

    writeHeader(outStream);

    // Write all elements
    for (Element& e : elements) {
      if (outputDataFormat == DataFormat::Binary) {
        e.writeDataBinary(outStream);
      } else if (outputDataFormat == DataFormat::ASCII) {
        e.writeDataASCII(outStream);
      }
    }
  }

  /**
   * @brief Get an element type by name ("vertices")
   *
   * @param target The name of the element type to get
   *
   * @return A reference to the element type.
   */
  Element& getElement(const std::string& target) {
    for (Element& e : elements) {
      if (e.name == target) return e;
    }
    throw std::runtime_error("PLY parser: no element with name: " + target);
  }


  /**
   * @brief Check if an element type exists
   *
   * @param target The name to check for.
   *
   * @return True if exists.
   */
  bool hasElement(const std::string& target) {
    for (Element& e : elements) {
      if (e.name == target) return true;
    }
    return false;
  }


  /**
   * @brief Add a new element type to the object
   *
   * @param name The name of the new element type ("vertices").
   * @param count The number of elements of this type.
   */
  void addElement(const std::string& name, size_t count) { elements.emplace_back(name, count); }

  // === Common-case helpers


  /**
   * @brief Common-case helper get mesh vertex positions
   *
   * @param vertexElementName The element name to use (default: "vertex")
   *
   * @return A vector of vertex positions.
   */
  std::vector<std::array<double, 3>> getVertexPositions(const std::string& vertexElementName = "vertex") {

    std::vector<double> xPos = getElement(vertexElementName).getProperty<double>("x");
    std::vector<double> yPos = getElement(vertexElementName).getProperty<double>("y");
    std::vector<double> zPos = getElement(vertexElementName).getProperty<double>("z");

    std::vector<std::array<double, 3>> result(xPos.size());
    for (size_t i = 0; i < result.size(); i++) {
      result[i][0] = xPos[i];
      result[i][1] = yPos[i];
      result[i][2] = zPos[i];
    }

    return result;
  }

  /**
   * @brief Common-case helper get mesh vertex colors
   *
   * @param vertexElementName The element name to use (default: "vertex")
   *
   * @return A vector of vertex colors (unsigned chars [0,255]).
   */
  std::vector<std::array<unsigned char, 3>> getVertexColors(const std::string& vertexElementName = "vertex") {

    std::vector<unsigned char> r = getElement(vertexElementName).getProperty<unsigned char>("red");
    std::vector<unsigned char> g = getElement(vertexElementName).getProperty<unsigned char>("green");
    std::vector<unsigned char> b = getElement(vertexElementName).getProperty<unsigned char>("blue");

    std::vector<std::array<unsigned char, 3>> result(r.size());
    for (size_t i = 0; i < result.size(); i++) {
      result[i][0] = r[i];
      result[i][1] = g[i];
      result[i][2] = b[i];
    }

    return result;
  }

  /**
   * @brief Common-case helper to get face indices for a mesh. If not template type is given, size_t is used. Naively
   * converts to requested signedness, which may lead to unexpected values if an unsigned type is used and file contains
   * negative values.
   *
   * @return The indices into the vertex elements for each face. Usually 0-based, though there are no formal rules.
   */
  template <typename T = size_t>
  std::vector<std::vector<T>> getFaceIndices() {

    for (const std::string& f : std::vector<std::string>{"face"}) {
      for (const std::string& p : std::vector<std::string>{"vertex_indices", "vertex_index"}) {
        try {
          return getElement(f).getListPropertyAnySign<T>(p);
        } catch (std::runtime_error e) {
          // that's fine
        }
      }
    }
    throw std::runtime_error("PLY parser: could not find face vertex indices attribute under any common name.");
  }


  /**
   * @brief Common-case helper set mesh vertex positons. Creates vertex element, if necessary.
   *
   * @param vertexPositions A vector of vertex positions
   */
  void addVertexPositions(std::vector<std::array<double, 3>>& vertexPositions) {

    std::string vertexName = "vertex";
    size_t N = vertexPositions.size();

    // Create the element
    if (!hasElement(vertexName)) {
      addElement(vertexName, N);
    }

    // De-interleave
    std::vector<double> xPos(N);
    std::vector<double> yPos(N);
    std::vector<double> zPos(N);
    for (size_t i = 0; i < vertexPositions.size(); i++) {
      xPos[i] = vertexPositions[i][0];
      yPos[i] = vertexPositions[i][1];
      zPos[i] = vertexPositions[i][2];
    }

    // Store
    getElement(vertexName).addProperty<double>("x", xPos);
    getElement(vertexName).addProperty<double>("y", yPos);
    getElement(vertexName).addProperty<double>("z", zPos);
  }

  /**
   * @brief Common-case helper set mesh vertex colors. Creates a vertex element, if necessary.
   *
   * @param colors A vector of vertex colors (unsigned chars [0,255]).
   */
  void addVertexColors(std::vector<std::array<unsigned char, 3>>& colors) {

    std::string vertexName = "vertex";
    size_t N = colors.size();

    // Create the element
    if (!hasElement(vertexName)) {
      addElement(vertexName, N);
    }

    // De-interleave
    std::vector<unsigned char> r(N);
    std::vector<unsigned char> g(N);
    std::vector<unsigned char> b(N);
    for (size_t i = 0; i < colors.size(); i++) {
      r[i] = colors[i][0];
      g[i] = colors[i][1];
      b[i] = colors[i][2];
    }

    // Store
    getElement(vertexName).addProperty<unsigned char>("red", r);
    getElement(vertexName).addProperty<unsigned char>("green", g);
    getElement(vertexName).addProperty<unsigned char>("blue", b);
  }

  /**
   * @brief Common-case helper set mesh vertex colors. Creates a vertex element, if necessary.
   *
   * @param colors A vector of vertex colors as floating point [0,1] values. Internally converted to [0,255] chars.
   */
  void addVertexColors(std::vector<std::array<double, 3>>& colors) {

    std::string vertexName = "vertex";
    size_t N = colors.size();

    // Create the element
    if (!hasElement(vertexName)) {
      addElement(vertexName, N);
    }

    auto toChar = [](double v) {
      if (v < 0.0) v = 0.0;
      if (v > 1.0) v = 1.0;
      return static_cast<unsigned char>(v * 255.);
    };

    // De-interleave
    std::vector<unsigned char> r(N);
    std::vector<unsigned char> g(N);
    std::vector<unsigned char> b(N);
    for (size_t i = 0; i < colors.size(); i++) {
      r[i] = toChar(colors[i][0]);
      g[i] = toChar(colors[i][1]);
      b[i] = toChar(colors[i][2]);
    }

    // Store
    getElement(vertexName).addProperty<unsigned char>("red", r);
    getElement(vertexName).addProperty<unsigned char>("green", g);
    getElement(vertexName).addProperty<unsigned char>("blue", b);
  }


  /**
   * @brief Common-case helper to set face indices. Creates a face element if needed. The input type will be casted to a
   * 32 bit integer of the same signedness.
   *
   * @param indices The indices into the vertex list around each face.
   */
  template <typename T>
  void addFaceIndices(std::vector<std::vector<T>>& indices) {

    std::string faceName = "face";
    size_t N = indices.size();

    // Create the element
    if (!hasElement(faceName)) {
      addElement(faceName, N);
    }

    // Cast to 32 bit
    typedef typename std::conditional<std::is_signed<T>::value, int32_t, uint32_t>::type IndType;
    std::vector<std::vector<IndType>> intInds;
    for (std::vector<T>& l : indices) {
      std::vector<IndType> thisInds;
      for (T& val : l) {
        IndType valConverted = static_cast<IndType>(val);
        if (valConverted != val) {
          throw std::runtime_error("Index value " + std::to_string(val) +
                                   " could not be converted to a .ply integer without loss of data. Note that .ply "
                                   "only supports 32-bit ints.");
        }
        thisInds.push_back(valConverted);
      }
      intInds.push_back(thisInds);
    }

    // Store
    getElement(faceName).addListProperty<IndType>("vertex_indices", intInds);
  }


  /**
   * @brief Comments for the file. When writing, each entry will be written as a sequential comment line.
   */
  std::vector<std::string> comments;

private:
  std::vector<Element> elements;
  const int majorVersion = 1; // I'll buy you a drink if these ever get bumped
  const int minorVersion = 0;

  DataFormat inputDataFormat = DataFormat::ASCII;  // set when reading from a file
  DataFormat outputDataFormat = DataFormat::ASCII; // option for writing files


  // === Reading ===

  /**
   * @brief Read the header for a file
   *
   * @param inStream
   * @param verbose
   */
  void parseHeader(std::ifstream& inStream, bool verbose) {

    using std::cout;
    using std::endl;
    using std::string;
    using std::vector;

    // First two lines are predetermined
    { // First line is magic constant
      string plyLine;
      std::getline(inStream, plyLine);
      if (trimSpaces(plyLine) != "ply") {
        throw std::runtime_error("PLY parser: File does not appear to be ply file. First line should be 'ply'");
      }
    }

    { // second line is version
      string styleLine;
      std::getline(inStream, styleLine);
      vector<string> tokens = tokenSplit(styleLine);
      if (tokens.size() != 3) throw std::runtime_error("PLY parser: bad format line");
      std::string formatStr = tokens[0];
      std::string typeStr = tokens[1];
      std::string versionStr = tokens[2];

      // "format"
      if (formatStr != "format") throw std::runtime_error("PLY parser: bad format line");

      // ascii/binary
      if (typeStr == "ascii") {
        inputDataFormat = DataFormat::ASCII;
        if (verbose) cout << "  - Type: ascii" << endl;
      } else if (typeStr == "binary_little_endian") {
        inputDataFormat = DataFormat::Binary;
        if (verbose) cout << "  - Type: binary" << endl;
      } else if (typeStr == "binary_big_endian") {
        throw std::runtime_error("PLY parser: encountered scary big endian file. Don't know how to parse that");
      } else {
        throw std::runtime_error("PLY parser: bad format line");
      }

      // version
      if (versionStr != "1.0") {
        throw std::runtime_error("PLY parser: encountered file with version != 1.0. Don't know how to parse that");
      }
      if (verbose) cout << "  - Version: " << versionStr << endl;
    }

    // Consume header line by line
    while (inStream.good()) {
      string line;
      std::getline(inStream, line);

      // Parse a comment
      if (startsWith(line, "comment")) {
        string comment = line.substr(7);
        if (verbose) cout << "  - Comment: " << comment << endl;
        comments.push_back(comment);
        continue;
      }

      // Parse an element
      else if (startsWith(line, "element")) {
        vector<string> tokens = tokenSplit(line);
        if (tokens.size() != 3) throw std::runtime_error("PLY parser: Invalid element line");
        string name = tokens[1];
        size_t count;
        std::istringstream iss(tokens[2]);
        iss >> count;
        elements.emplace_back(name, count);
        if (verbose) cout << "  - Found element: " << name << " (count = " << count << ")" << endl;
        continue;
      }

      // Parse a property list
      else if (startsWith(line, "property list")) {
        vector<string> tokens = tokenSplit(line);
        if (tokens.size() != 5) throw std::runtime_error("PLY parser: Invalid property list line");
        if (elements.size() == 0) throw std::runtime_error("PLY parser: Found property list without previous element");
        string countType = tokens[2];
        string type = tokens[3];
        string name = tokens[4];
        elements.back().properties.push_back(createPropertyWithType(name, type, true, countType));
        if (verbose)
          cout << "    - Found list property: " << name << " (count type = " << countType << ", data type = " << type
               << ")" << endl;
        continue;
      }

      // Parse a property
      else if (startsWith(line, "property")) {
        vector<string> tokens = tokenSplit(line);
        if (tokens.size() != 3) throw std::runtime_error("PLY parser: Invalid property line");
        if (elements.size() == 0) throw std::runtime_error("PLY parser: Found property without previous element");
        string type = tokens[1];
        string name = tokens[2];
        elements.back().properties.push_back(createPropertyWithType(name, type, false, ""));
        if (verbose) cout << "    - Found property: " << name << " (type = " << type << ")" << endl;
        continue;
      }

      // Parse end of header
      else if (startsWith(line, "end_header")) {
        break;
      }

      // Error!
      else {
        throw std::runtime_error("Unrecognized header line: " + line);
      }
    }
  }

  /**
   * @brief Read the actual data for a file, in ASCII
   *
   * @param inStream
   * @param verbose
   */
  void parseASCII(std::ifstream& inStream, bool verbose) {

    using std::string;
    using std::vector;

    // Read all elements
    for (Element& elem : elements) {

      if (verbose) {
        std::cout << "  - Processing element: " << elem.name << std::endl;
      }

      for (size_t iP = 0; iP < elem.properties.size(); iP++) {
        elem.properties[iP]->reserve(elem.count);
      }
      for (size_t iEntry = 0; iEntry < elem.count; iEntry++) {

        string line;
        std::getline(inStream, line);

        vector<string> tokens = tokenSplit(line);
        size_t iTok = 0;
        for (size_t iP = 0; iP < elem.properties.size(); iP++) {
          elem.properties[iP]->parseNext(tokens, iTok);
        }
      }
    }
  }

  /**
   * @brief Read the actual data for a file, in binary.
   *
   * @param inStream
   * @param verbose
   */
  void parseBinary(std::ifstream& inStream, bool verbose) {

    using std::string;
    using std::vector;

    // Read all elements
    for (Element& elem : elements) {

      if (verbose) {
        std::cout << "  - Processing element: " << elem.name << std::endl;
      }

      for (size_t iP = 0; iP < elem.properties.size(); iP++) {
        elem.properties[iP]->reserve(elem.count);
      }
      for (size_t iEntry = 0; iEntry < elem.count; iEntry++) {
        for (size_t iP = 0; iP < elem.properties.size(); iP++) {
          elem.properties[iP]->readNext(inStream);
        }
      }
    }
  }

  // === Writing ===


  /**
   * @brief Write out a header for a file
   *
   * @param outStream
   */
  void writeHeader(std::ofstream& outStream) {

    // Magic line
    outStream << "ply\n";

    // Type line
    outStream << "format ";
    if (outputDataFormat == DataFormat::Binary) {
      outStream << "binary_little_endian ";
    } else if (outputDataFormat == DataFormat::ASCII) {
      outStream << "ascii ";
    }

    // Version number
    outStream << majorVersion << "." << minorVersion << "\n";

    // Write comments
    for (const std::string& comment : comments) {
      outStream << "comment " << comment << "\n";
    }
    outStream << "comment "
              << "Written with hapPLY (https://github.com/nmwsharp/happly)"
              << "\n";

    // Write elements (and their properties)
    for (Element& e : elements) {
      e.writeHeader(outStream);
    }

    // End header
    outStream << "end_header\n";
  }
};

} // namespace happly



// pbrt
// stl
#include <iostream>
#include <vector>


/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {
  
    using std::cout;
    using std::endl;



    namespace ply {
      // Ref<sg::TriangleMesh> readFile(const std::string &fileName)
      void parse(const std::string &fileName,
                  std::vector<vec3f> &pos,
                  std::vector<vec3f> &nor,
                  std::vector<vec3i> &idx)
      {
        happly::PLYData ply(fileName);
        
        if(ply.hasElement("vertex")) {
          happly::Element& elem = ply.getElement("vertex");
          if(elem.hasProperty("x") && elem.hasProperty("y") && elem.hasProperty("z")) {
            std::vector<float> x = elem.getProperty<float>("x");
            std::vector<float> y = elem.getProperty<float>("y");
            std::vector<float> z = elem.getProperty<float>("z");
            pos.resize(x.size());
            for(int i = 0; i < x.size(); i ++) {
              pos[i] = vec3f(x[i], y[i], z[i]);
            }
          } else {
            throw std::runtime_error("missing positions in ply");
          }
          if(elem.hasProperty("nx") && elem.hasProperty("ny") && elem.hasProperty("nz")) {
            std::vector<float> x = elem.getProperty<float>("nx");
            std::vector<float> y = elem.getProperty<float>("ny");
            std::vector<float> z = elem.getProperty<float>("nz");
            nor.resize(x.size());
            for(int i = 0; i < x.size(); i ++) {
              nor[i] = vec3f(x[i], y[i], z[i]);
            }
          }
        } else {
          throw std::runtime_error("missing positions in ply");
        }
        if (ply.hasElement("face")) {
          happly::Element& elem = ply.getElement("face");
          if(elem.hasProperty("vertex_indices")) {
            std::vector<std::vector<int>> fasces = elem.getListProperty<int>("vertex_indices");
            for(int j = 0; j < fasces.size(); j ++) {
              std::vector<int>& face = fasces[j];
              for (int i=2;i<face.size();i++) {
                idx.push_back(vec3i(face[0], face[i-1], face[i]));
              }
            }
          } else {
            throw std::runtime_error("missing faces in ply");
          }          
        } else {
            throw std::runtime_error("missing faces in ply");
        }                        
      }

    } // ::pbrt::syntactic::ply

    
  } // ::pbrt::syntactic
} // ::pbrt

extern "C" 
void pbrt_helper_loadPlyTriangles(const std::string &fileName,
                                  std::vector<pbrt::syntactic::vec3f> &v,
                                  std::vector<pbrt::syntactic::vec3f> &n,
                                  std::vector<pbrt::syntactic::vec3i> &idx)
{
  pbrt::syntactic::ply::parse(fileName,v,n,idx);
}



