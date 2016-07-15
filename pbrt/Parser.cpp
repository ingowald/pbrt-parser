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

#include "Parser.h"
// stl
#include <fstream>
#include <sstream>
#include <queue>
#include <stack>
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
      static Ref<Token> TOKEN_EOF;

      typedef enum { TOKEN_TYPE_EOF, TOKEN_TYPE_STRING, TOKEN_TYPE_LITERAL, TOKEN_TYPE_SPECIAL } Type;

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

    Ref<Token> Token::TOKEN_EOF = new Token(Loc(NULL),Token::TOKEN_TYPE_EOF,"<EOF>");

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

      inline Ref<Token> next(bool consumePeeks=false) 
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

          if (c < 0) return Token::TOKEN_EOF;
          
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
              if (c < 0) return Token::TOKEN_EOF;
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


    struct ParserException {
      ParserException(const std::string &error,
                      const std::string &where="")
        : error(error)
      { if (where != "") addBackTrace(where); }
      void addBackTrace(const std::string &where) 
      { backTrace.push_back(where); }
      std::string toString() const {
        if (backTrace.empty()) return error;
        std::string msg = error + "\nBacktrace:\n";
        for (int i=0;i<backTrace.size();i++)
          msg += (backTrace[i]+"\n");
        return msg;
      }
      std::string error;
      std::vector<std::string> backTrace;
    };


    inline float parseFloat(Tokenizer &tokens)
    {
      Ref<Token> token = tokens.next();
      if (token == Token::TOKEN_EOF)
        throw new ParserException("unexpected end of file",__PRETTY_FUNCTION__);
      return atof(token->text.c_str());
    }

    inline vec3f parseVec3f(Tokenizer &tokens)
    {
      try {
        const float x = parseFloat(tokens);
        const float y = parseFloat(tokens);
        const float z = parseFloat(tokens);
        return vec3f(x,y,z);
      } catch (ParserException *e) {
        e->addBackTrace(__PRETTY_FUNCTION__);
        throw e;
      }
    }

    inline Ref<Param> parseParam(std::string &name, Tokenizer &tokens)
    {
      Ref<Token> token = tokens.peek();
      if (token->type != Token::TOKEN_TYPE_STRING)
        return NULL;

      token = tokens.next();
      char *_type = strdup(token->text.c_str());
      char *_name = strdup(token->text.c_str());
      int rc = sscanf(token->text.c_str(),"%s %s",_type,_name);
      if (rc != 2)
        throw new ParserException("could not parse object parameter's type and name "+token->loc.toString(),
                                  __PRETTY_FUNCTION__);
      string type = _type;
      name = _name;
      free(_name);
      free(_type);

      Ref<Param> ret = NULL;
      if (type == "float") {
        ret = new ParamT<float>(type);
      } else if (type == "color") {
        ret = new ParamT<float>(type);
      } else if (type == "rgb") {
        ret = new ParamT<float>(type);
      } else if (type == "spectrum") {
        ret = new ParamT<std::string>(type);
      } else if (type == "integer") {
        ret = new ParamT<int>(type);
      } else if (type == "bool") {
        ret = new ParamT<bool>(type);
      } else if (type == "texture") {
        ret = new ParamT<std::string>(type);
      } else if (type == "normal") {
        ret = new ParamT<float>(type);
      } else if (type == "point") {
        ret = new ParamT<float>(type);
      } else if (type == "vector") {
        ret = new ParamT<float>(type);
      } else if (type == "string") {
        ret = new ParamT<std::string>(type);
      } else {
        throw new ParserException("unknown parameter type '"+type+"' "+token->loc.toString(),
                                  __PRETTY_FUNCTION__);
      }

      Ref<Token> value = tokens.next();
      if (value->text == "[") {
        Ref<Token> p = tokens.next();
        
        while (p->text != "]") {
          ret->add(p->text);
          p = tokens.next();
        }
      } else {
        ret->add(value->text);
      }
      return ret;
    }

    void parseParams(std::map<std::string, Ref<Param> > &params, Tokenizer &tokens)
    {
      while (1) {
        std::string name;
        Ref<Param> param = parseParam(name,tokens);
        if (!param) return;
        params[name] = param;
      }
    }

    Attributes::Attributes()
    {
    }

    Attributes::Attributes(const Attributes &other)
    {
    }

    Parser::Parser(bool dbg) 
      : scene(new Scene), dbg(dbg) 
    {
      transformStack.push(affine3f(embree::one));
      attributesStack.push(new Attributes);
      objectStack.push(scene.cast<Object>());
    }

    Ref<Object> Parser::getCurrentObject() 
    {
      if (objectStack.empty())
        throw std::runtime_error("no active object!?");
      return objectStack.top(); 
    }

    Ref<Object> Parser::findNamedObject(const std::string &name, bool createIfNotExist)
    {
      if (namedObjects.find(name) == namedObjects.end()) {

        if (createIfNotExist) {
          Ref<Object> object = new Object;
          object->name = name;
          // if (namedObjects.find(name) != namedObjects.end())
          //   // throw new ParserException("named object '"+name+"' already defined!? (at "+token->loc.toString()+")");
          //   // cout << token->loc.toString() << ": warning - named object '"+name+"' already defined!? (redefining!)" << endl;
          
          namedObjects[name] = object;
          // cout << "defining object '" << name << "'" << endl;
          
        } else {
          PING;
          PRINT(name);
          throw std::runtime_error("could not find object named '"+name+"'");
        }
      }
      return namedObjects[name];
    }


    void Parser::pushAttributes() 
    {
      attributesStack.push(new Attributes(*attributesStack.top()));
    }

    void Parser::popAttributes() 
    {
      attributesStack.pop();
    }
    
    void Parser::pushTransform() 
    {
      transformStack.push(transformStack.top());
    }

    void Parser::popTransform() 
    {
      transformStack.pop();
    }
    
    affine3f parseMatrix(Tokenizer &tokens)
    {
      const std::string open = *tokens.next();
      assert(open == "[");

      affine3f xfm;
      xfm.l.vx.x = atof(tokens.next()->c_str());
      xfm.l.vx.y = atof(tokens.next()->c_str());
      xfm.l.vx.z = atof(tokens.next()->c_str());
      float vx_w = atof(tokens.next()->c_str());
      assert(vx_w == 0.f);

      xfm.l.vy.x = atof(tokens.next()->c_str());
      xfm.l.vy.y = atof(tokens.next()->c_str());
      xfm.l.vy.z = atof(tokens.next()->c_str());
      float vy_w = atof(tokens.next()->c_str());
      assert(vy_w == 0.f);

      xfm.l.vz.x = atof(tokens.next()->c_str());
      xfm.l.vz.y = atof(tokens.next()->c_str());
      xfm.l.vz.z = atof(tokens.next()->c_str());
      float vz_w = atof(tokens.next()->c_str());
      assert(vz_w == 0.f);

      xfm.p.x    = atof(tokens.next()->c_str());
      xfm.p.y    = atof(tokens.next()->c_str());
      xfm.p.z    = atof(tokens.next()->c_str());
      float p_w  = atof(tokens.next()->c_str());
      assert(p_w == 1.f);

      const std::string close = *tokens.next();
      assert(open == "=");

      return xfm;
    }

    /*! parse given file, and add it to the scene we hold */
    void Parser::parse(const FileName &fn)
    {
      try {
        std::stack<Tokenizer *> tokenizerStack;
        Tokenizer *tokens = new Tokenizer(fn);

        while (1) {
          Ref<Token> token = tokens->next();
          if (!token || token->type == Token::TOKEN_TYPE_EOF) {
            if (tokenizerStack.empty())
              break;
            tokens = tokenizerStack.top();
            tokenizerStack.pop();
            continue;
          }

          if (dbg) 
            cout << token->toString() << endl;
          if (token->text == "Include") {
            Ref<Token> fileNameToken = tokens->next();
            FileName includedFileName = fileNameToken->text;
            if (includedFileName.str()[0] != '/') {
              includedFileName = tokens->file->name.path()+includedFileName;
            }
            cout << "... including file '" << includedFileName.str() << " ..." << endl;
            tokenizerStack.push(tokens);
            tokens = new Tokenizer(includedFileName);
            continue;
          }

          if (token->text == "Identity") {
            setTransform(affine3f(embree::one));
            continue;
          }
          if (token->text == "ReverseOrientation") {
            addTransform(affine3f::scale(vec3f(1.f,1.f,-1.f)));
            continue;
          }
          if (token->text == "CoordSysTransform") {
            Ref<Token> nameOfObject = tokens->next();
            cout << "ignoring 'CoordSysTransform'" << endl;
            continue;
          }
          if (token->text == "Scale") {
            vec3f scale = parseVec3f(*tokens);
            addTransform(affine3f::scale(scale));
            continue;
          }
          if (token->text == "Translate") {
            vec3f translate = parseVec3f(*tokens);
            addTransform(affine3f::translate(translate));
            continue;
          }
          if (token->text == "ConcatTransform") {
            addTransform(parseMatrix(*tokens));
            continue;
          }
          if (token->text == "Rotate") {
            vec3f axis = parseVec3f(*tokens);
            float angle = parseFloat(*tokens);
            addTransform(affine3f::rotate(axis,angle));
            continue;
          }
          if (token->text == "Transform") {
            tokens->next(); // '['
            float mat[16];
            for (int i=0;i<16;i++)
              mat[i] = atof(tokens->next()->text.c_str());
            tokens->next(); // ']'
            continue;
          }
          if (token->text == "ActiveTransform") {
            std::string time = tokens->next()->text;
            std::cout << "'ActiveTransform' not implemented" << endl;
            continue;
          }
          if (token->text == "Identity") {
            continue;
          }
          if (token->text == "ReverseOrientation") {
            continue;
          }
          if (token->text == "ConcatTransform") {
            tokens->next(); // '['
            float mat[16];
            for (int i=0;i<16;i++)
              mat[i] = atof(tokens->next()->text.c_str());

            affine3f xfm;
            xfm.l.vx = vec3f(mat[0],mat[1],mat[2]);
            xfm.l.vy = vec3f(mat[4],mat[5],mat[6]);
            xfm.l.vz = vec3f(mat[8],mat[9],mat[10]);
            xfm.p    = vec3f(mat[12],mat[13],mat[14]);
            addTransform(xfm);

            tokens->next(); // ']'
            continue;
          }
          if (token->text == "CoordSysTransform") {
            std::string transformType = tokens->next()->text;
            continue;
          }


          if (token->text == "ActiveTransform") {
            std::string time = tokens->next()->text;
            continue;
          }

          if (token->text == "LookAt") {
            vec3f v0 = parseVec3f(*tokens);
            vec3f v1 = parseVec3f(*tokens);
            vec3f v2 = parseVec3f(*tokens);
            scene->lookAt = new LookAt(v0,v1,v2);
            continue;
          }
          if (token->text == "Camera") {
            Ref<Camera> camera = new Camera(tokens->next()->text);
            parseParams(camera->param,*tokens);
            scene->cameras.push_back(camera);
            continue;
          }
          if (token->text == "Sampler") {
            Ref<Sampler> sampler = new Sampler(tokens->next()->text);
            parseParams(sampler->param,*tokens);
            scene->sampler = sampler;
            continue;
          }
          if (token->text == "Integrator") {
            Ref<Integrator> integrator = new Integrator(tokens->next()->text);
            parseParams(integrator->param,*tokens);
            scene->integrator = integrator;
            continue;
          }
          if (token->text == "SurfaceIntegrator") {
            Ref<SurfaceIntegrator> surfaceIntegrator
              = new SurfaceIntegrator(tokens->next()->text);
            parseParams(surfaceIntegrator->param,*tokens);
            scene->surfaceIntegrator = surfaceIntegrator;
            continue;
          }
          if (token->text == "VolumeIntegrator") {
            Ref<VolumeIntegrator> volumeIntegrator
              = new VolumeIntegrator(tokens->next()->text);
            parseParams(volumeIntegrator->param,*tokens);
            scene->volumeIntegrator = volumeIntegrator;
            continue;
          }
          if (token->text == "PixelFilter") {
            Ref<PixelFilter> pixelFilter = new PixelFilter(tokens->next()->text);
            parseParams(pixelFilter->param,*tokens);
            scene->pixelFilter = pixelFilter;
            continue;
          }
          if (token->text == "Accelerator") {
            Ref<Accelerator> accelerator = new Accelerator(tokens->next()->text);
            parseParams(accelerator->param,*tokens);
            continue;
          }
          if (token->text == "Shape") {
            Ref<Shape> shape = new Shape(tokens->next()->text,
                                         attributesStack.top()->clone(),
                                         transformStack.top());
            parseParams(shape->param,*tokens);
            scene->shapes.push_back(shape);
            continue;
          }
          if (token->text == "Volume") {
            Ref<Volume> volume = new Volume(tokens->next()->text);
            parseParams(volume->param,*tokens);
            scene->volumes.push_back(volume);
            continue;
          }
          if (token->text == "LightSource") {
            Ref<LightSource> lightSource = new LightSource(tokens->next()->text);
            parseParams(lightSource->param,*tokens);
            cout << "'lightsource' not implemented" << endl;
            continue;
          }
          if (token->text == "AreaLightSource") {
            Ref<AreaLightSource> lightSource = new AreaLightSource(tokens->next()->text);
            parseParams(lightSource->param,*tokens);
            continue;
          }
          if (token->text == "Film") {
            Ref<Film> film = new Film(tokens->next()->text);
            parseParams(film->param,*tokens);
            continue;
          }
          if (token->text == "Accelerator") {
            Ref<Accelerator> accelerator = new Accelerator(tokens->next()->text);
            parseParams(accelerator->param,*tokens);
            continue;
          }
          if (token->text == "Renderer") {
            Ref<Renderer> renderer = new Renderer(tokens->next()->text);
            parseParams(renderer->param,*tokens);
            continue;
          }

          if (token->text == "MakeNamedMaterial") {
            std::string name = tokens->next()->text;
            Ref<Material> material = new Material(name);
            namedMaterial[name] = material;
            parseParams(material->param,*tokens);
            continue;
          }
          if (token->text == "Material") {
            std::string name = tokens->next()->text;
            Ref<Material> material = new Material(name);
            parseParams(material->param,*tokens);
            continue;
          }

          if (token->text == "NamedMaterial") {
            // USE named material
            std::string which = tokens->next()->text;
            continue;
          }
          if (token->text == "Texture") {
            std::string name = tokens->next()->text;
            std::string texelType = tokens->next()->text;
            std::string mapType = tokens->next()->text;
            Ref<Texture> texture = new Texture(name,texelType,mapType);
            namedTexture[name] = texture;
            parseParams(texture->param,*tokens);
            continue;
          }

          if (token->text == "AttributeBegin") {
            pushAttributes();
            continue;
          }
          if (token->text == "AttributeEnd") {
            popAttributes();
            continue;
          }

          if (token->text == "TransformBegin") {
            pushTransform();
            continue;
          }
          if (token->text == "TransformEnd") {
            popTransform();
            continue;
          }
        
          if (token->text == "WorldBegin") {
            continue;
          }
          if (token->text == "WorldEnd") {
            continue;
          }

          if (token->text == "ObjectBegin") {
            PING;
            std::string name = tokens->next()->text;
            PRINT(name);
            Ref<Object> object = findNamedObject(name,1);

            objectStack.push(object);
            transformStack.push(embree::one);
            continue;
          }
          
          if (token->text == "ObjectEnd") {
            objectStack.pop();
            transformStack.pop();
            continue;
          }

          if (token->text == "ObjectInstance") {
            std::string name = tokens->next()->text;
            Ref<Object> object = findNamedObject(name,1);
            Ref<Object::Instance> inst = new Object::Instance(object,getCurrentXfm());
            getCurrentObject()->objectInstances.push_back(inst);
            continue;
          }
          
        
          throw new ParserException("unexpected token '"+token->text+"' at "+token->loc.toString());
        }
      } catch (ParserException *e) {
        THROW_RUNTIME_ERROR("plib::pbrt parser fatal error:\n"+e->toString());
      }
    }

  }
}
