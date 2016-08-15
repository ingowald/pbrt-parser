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
#include "Lexer.h"
// stl
#include <fstream>
#include <sstream>
#include <stack>
// std
#include <stdio.h>
#include <string.h>

namespace plib {
  namespace pbrt {
    using namespace std;

    int verbose = 0;

    inline float parseFloat(Lexer &tokens)
    {
      Ref<Token> token = tokens.next();
      // if (token == Token::TOKEN_EOF)
      if (!token)
        throw std::runtime_error("unexpected end of file\n@"+std::string(__PRETTY_FUNCTION__));
      return atof(token->text.c_str());
    }

    inline vec3f parseVec3f(Lexer &tokens)
    {
      try {
        const float x = parseFloat(tokens);
        const float y = parseFloat(tokens);
        const float z = parseFloat(tokens);
        return vec3f(x,y,z);
      } catch (std::runtime_error e) {
        // e->addBackTrace(__PRETTY_FUNCTION__);
        throw e.what()+std::string("\n@")+std::string(__PRETTY_FUNCTION__);
        // throw e;
      }
    }

    inline Ref<Param> parseParam(std::string &name, Lexer &tokens)
    {
      Ref<Token> token = tokens.peek();
      if (!token || token->type != Token::TOKEN_TYPE_STRING)
        return NULL;

      token = tokens.next();
      char *_type = strdup(token->text.c_str());
      char *_name = strdup(token->text.c_str());
      int rc = sscanf(token->text.c_str(),"%s %s",_type,_name);
      if (rc != 2)
        throw std::runtime_error("could not parse object parameter's type and name "
                                 +token->loc.toString()
                                 +std::string("\n@")+std::string(__PRETTY_FUNCTION__));
      // throw new ParserException("could not parse object parameter's type and name "+token->loc.toString(),
      //                           __PRETTY_FUNCTION__);
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
        throw std::runtime_error("unknown parameter type '"+type+"' "+token->loc.toString()
                                 +std::string("\n@")+std::string(__PRETTY_FUNCTION__));
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

    void parseParams(std::map<std::string, Ref<Param> > &params, Lexer &tokens)
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

    Parser::Parser(bool dbg, const std::string &basePath) 
      : scene(new Scene), dbg(dbg), basePath(basePath) 
    {
      transformStack.push(affine3f(embree::one));
      attributesStack.push(new Attributes);
      objectStack.push(scene->world);//scene.cast<Object>());
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
          Ref<Object> object = new Object(name);
          namedObjects[name] = object;
        } else {
          throw std::runtime_error("could not find object named '"+name+"'");
        }
      }
      return namedObjects[name];
    }


    void Parser::pushAttributes() 
    {
      attributesStack.push(new Attributes(*attributesStack.top()));
      materialStack.push(currentMaterial);
      pushTransform();
      // setTransform(embree::one);

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
      transformStack.push(transformStack.top());
    }

    void Parser::popTransform() 
    {
      transformStack.pop();
    }
    
    affine3f parseMatrix(Lexer &tokens)
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
      assert(close == "]");

      return xfm;
    }

    bool Parser::parseTransforms(Ref<Token> token)
    {
      if (token->text == "TransformBegin") {
        pushTransform();
        return true;
      }
      if (token->text == "TransformEnd") {
        popTransform();
        return true;
      }
      if (token->text == "Scale") {
        vec3f scale = parseVec3f(*tokens);
        addTransform(affine3f::scale(scale));
        return true;
      }
      if (token->text == "Translate") {
        vec3f translate = parseVec3f(*tokens);
        addTransform(affine3f::translate(translate));
        return true;
      }
      if (token->text == "ConcatTransform") {
        addTransform(parseMatrix(*tokens));
        return true;
      }
      if (token->text == "Rotate") {
        const float angle = parseFloat(*tokens);
        const vec3f axis  = parseVec3f(*tokens);
        addTransform(affine3f::rotate(axis,angle*M_PI/180.f));
        return true;
      }
      if (token->text == "Transform") {
        tokens->next(); // '['
        affine3f xfm;
        xfm.l.vx = parseVec3f(*tokens); tokens->next();
        xfm.l.vy = parseVec3f(*tokens); tokens->next();
        xfm.l.vz = parseVec3f(*tokens); tokens->next();
        xfm.p    = parseVec3f(*tokens); tokens->next();
        tokens->next(); // ']'
        addTransform(xfm);
        return true;
      }
      if (token->text == "ActiveTransform") {
        std::string time = tokens->next()->text;
        std::cout << "'ActiveTransform' not implemented" << endl;
        return true;
      }
      if (token->text == "Identity") {
        setTransform(affine3f(embree::one));
        return true;
      }
      if (token->text == "ReverseOrientation") {
        /* according to the docs, 'ReverseOrientation' only flips the
           normals, not the actual transform */
        return true;
      }
      if (token->text == "CoordSysTransform") {
        Ref<Token> nameOfObject = tokens->next();
        cout << "ignoring 'CoordSysTransform'" << endl;
        return true;
      }
      return false;
    }

    void Parser::parseWorld()
    {
      cout << "Parsing PBRT World" << endl;
      while (1) {
        Ref<Token> token = getNextToken();
        assert(token);
        if (token->text == "WorldEnd") {
          cout << "Parsing PBRT World - done!" << endl;
          break;
        }
        // -------------------------------------------------------
        // LightSource
        // -------------------------------------------------------
        if (token->text == "LightSource") {
          Ref<LightSource> lightSource = new LightSource(tokens->next()->text);
          parseParams(lightSource->param,*tokens);
          getCurrentObject()->lightSources.push_back(lightSource);
          continue;
        }
        if (token->text == "AreaLightSource") {
          Ref<AreaLightSource> lightSource = new AreaLightSource(tokens->next()->text);
          parseParams(lightSource->param,*tokens);
          continue;
        }
        // -------------------------------------------------------
        // Material
        // -------------------------------------------------------
        if (token->text == "Material") {
          std::string type = tokens->next()->text;
          Ref<Material> material = new Material(type);
          parseParams(material->param,*tokens);
          currentMaterial = material;
          PING;
          PRINT(currentMaterial);
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
        if (token->text == "MakeNamedMaterial") {
          std::string name = tokens->next()->text;
          Ref<Material> material = new Material("<implicit>");
          namedMaterial[name] = material;
          parseParams(material->param,*tokens);

          /* named material have the parameter type implicitly as a
             parameter rather than explicitly on the
             'makenamedmaterial' command; so let's parse this here */
          Ref<Param> type = material->param["type"];
          if (!type) throw std::runtime_error("named material that does not specify a 'type' parameter!?");
          ParamT<std::string> *asString = dynamic_cast<ParamT<std::string> *>(type.ptr);
          if (!asString)
            throw std::runtime_error("named material has a type, but not a string!?");
          assert(asString->getSize() == 1);
          material->type = asString->paramVec[0];
          continue;
        }

        if (token->text == "NamedMaterial") {
          // USE named material
          std::string name = tokens->next()->text;
          currentMaterial = namedMaterial[name];
          continue;
        }

        // -------------------------------------------------------
        // Attributes
        // -------------------------------------------------------
        if (token->text == "AttributeBegin") {
          pushAttributes();
          continue;
        }
        if (token->text == "AttributeEnd") {
          popAttributes();
          continue;
        }
        // -------------------------------------------------------
        // Shapes
        // -------------------------------------------------------
        if (token->text == "Shape") {
          Ref<Shape> shape = new Shape(tokens->next()->text,
                                       currentMaterial,
                                       attributesStack.top()->clone(),
                                       transformStack.top());
          parseParams(shape->param,*tokens);
          getCurrentObject()->shapes.push_back(shape);
          continue;
        }
        // -------------------------------------------------------
        // Volumes
        // -------------------------------------------------------
        if (token->text == "Volume") {
          Ref<Volume> volume = new Volume(tokens->next()->text);
          parseParams(volume->param,*tokens);
          getCurrentObject()->volumes.push_back(volume);
          continue;
        }

        // -------------------------------------------------------
        // Transforms
        // -------------------------------------------------------
        if (parseTransforms(token))
          continue;
        

        // -------------------------------------------------------
        // Objects
        // -------------------------------------------------------

        if (token->text == "ObjectBegin") {
          std::string name = tokens->next()->text;
          Ref<Object> object = findNamedObject(name,1);

          objectStack.push(object);
          // if (verbose)
          //   cout << "pushing object " << object->toString() << endl;
          // transformStack.push(embree::one);
          continue;
        }
          
        if (token->text == "ObjectEnd") {
          objectStack.pop();
          // transformStack.pop();
          continue;
        }

        if (token->text == "ObjectInstance") {
          std::string name = tokens->next()->text;
          Ref<Object> object = findNamedObject(name,1);
          Ref<Object::Instance> inst = new Object::Instance(object,getCurrentXfm());
          getCurrentObject()->objectInstances.push_back(inst);
          if (verbose)
            cout << "adding instance " << inst->toString()
                 << " to object " << getCurrentObject()->toString() << endl;
          continue;
        }
          
        // -------------------------------------------------------
        // ERROR - unrecognized token in worldbegin/end!!!
        // -------------------------------------------------------
        throw std::runtime_error("unexpected token '"+token->text
                                 +"' at "+token->loc.toString());
      }
    }

    Ref<Token> Parser::getNextToken()
    {
      Ref<Token> token = tokens->next();
      while (!token) {
        if (tokenizerStack.empty())
          return NULL; 
        tokens = tokenizerStack.top();
        tokenizerStack.pop();
        token = tokens->next();
      }
      assert(token);
      if (token->text == "Include") {
        Ref<Token> fileNameToken = tokens->next();
        FileName includedFileName = fileNameToken->text;
        if (includedFileName.str()[0] != '/') {
          includedFileName = rootNamePath+includedFileName;
        }
        cout << "... including file '" << includedFileName.str() << " ..." << endl;
        
        tokenizerStack.push(tokens);
        tokens = new Lexer(includedFileName);
        return getNextToken();
      }
      else
        return token;
    }
    
    void Parser::parseScene()
    {
      while (1) {
        Ref<Token> token = getNextToken();
        if (!token)
          break;

        if (dbg) 
          cout << token->toString() << endl;

        // -------------------------------------------------------
        // Transforms
        // -------------------------------------------------------
        if (parseTransforms(token))
          continue;
        

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

        if (token->text == "WorldBegin") {
          parseWorld();
          continue;
        }

        
        throw std::runtime_error("unexpected token '"+token->text
                                 +"' at "+token->loc.toString());
      }
    }

    
    /*! parse given file, and add it to the scene we hold */
    void Parser::parse(const FileName &fn)
    {
      rootNamePath = basePath==""?fn.path():FileName(basePath);
      this->tokens = new Lexer(fn);
      parseScene();      
    }

  }
}
