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

#include "Parser.h"
#include "Lexer.h"
// stl
#include <fstream>
#include <sstream>
#include <stack>
// std
#include <stdio.h>
#include <string.h>

namespace pbrt_parser {
    using namespace std;

    int verbose = 0;

    inline float parseFloat(Lexer &tokens)
    {
      std::shared_ptr<Token> token = tokens.next();
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
        throw e.what()+std::string("\n@")+std::string(__PRETTY_FUNCTION__);
      }
    }

  inline std::shared_ptr<Param> Parser::parseParam(std::string &name, Lexer &tokens)
    {
      std::shared_ptr<Token> token = tokens.peek();
      if (!token || token->type != Token::TOKEN_TYPE_STRING)
        return std::shared_ptr<Param>();

      token = tokens.next();
      char *_type = strdup(token->text.c_str());
      char *_name = strdup(token->text.c_str());
      int rc = sscanf(token->text.c_str(),"%s %s",_type,_name);
      if (rc != 2)
        throw std::runtime_error("could not parse object parameter's type and name "
                                 +token->loc.toString()
                                 +std::string("\n@")+std::string(__PRETTY_FUNCTION__));
      string type = _type;
      name = _name;
      free(_name);
      free(_type);

      std::shared_ptr<Param> ret; 
      if (type == "float") {
        ret = std::make_shared<ParamT<float>>(type);
      } else if (type == "color") {
        ret = std::make_shared<ParamT<float> >(type);
      } else if (type == "blackbody") {
        ret = std::make_shared<ParamT<float>>(type);
      } else if (type == "rgb") {
        ret = std::make_shared<ParamT<float> >(type);
      } else if (type == "spectrum") {
        ret = std::make_shared<ParamT<std::string>>(type);
      } else if (type == "integer") {
        ret = std::make_shared<ParamT<int>>(type);
      } else if (type == "bool") {
        ret = std::make_shared<ParamT<bool>>(type);
      } else if (type == "texture") {
        ret = std::make_shared<ParamT<Texture>>(type);
      } else if (type == "normal") {
        ret = std::make_shared<ParamT<float> >(type);
      } else if (type == "point") {
        ret = std::make_shared<ParamT<float>>(type);
      } else if (type == "point2") {
        ret = std::make_shared<ParamT<float>>(type);
      } else if (type == "point3") {
        ret = std::make_shared<ParamT<float>>(type);
      } else if (type == "point4") {
        ret = std::make_shared<ParamT<float>>(type);
      } else if (type == "vector") {
        ret = std::make_shared<ParamT<float>>(type);
      } else if (type == "string") {
        ret = std::make_shared<ParamT<std::string>>(type);
      } else {
        throw std::runtime_error("unknown parameter type '"+type+"' "+token->loc.toString()
                                 +std::string("\n@")+std::string(__PRETTY_FUNCTION__));
      }

      std::shared_ptr<Token> value = tokens.next();
      if (value->text == "[") {
        std::shared_ptr<Token> p = tokens.next();
        
        while (p->text != "]") {
          if (type == "texture") {
            std::dynamic_pointer_cast<ParamT<Texture>>(ret)->texture 
              = getTexture(p->text);
          } else {
            ret->add(p->text);
          }
          p = tokens.next();
        }
      } else {
        if (type == "texture") {
          std::dynamic_pointer_cast<ParamT<Texture>>(ret)->texture 
            = getTexture(value->text);
        } else {
          ret->add(value->text);
        }
      }
      return ret;
    }

  void Parser::parseParams(std::map<std::string, std::shared_ptr<Param> > &params, Lexer &tokens)
    {
      while (1) {
        std::string name;
        std::shared_ptr<Param> param = parseParam(name,tokens);
        if (!param) return;
        params[name] = param;
      }
    }

    Attributes::Attributes()
    {
    }

  // Attributes::Attributes(const Attributes &other)
  //   {
  //   }


  std::shared_ptr<Texture> Parser::getTexture(const std::string &name) 
  {
    // PING; PRINT(attributesStack.top()->namedTexture.size());
    if (attributesStack.top()->namedTexture.find(name) == attributesStack.top()->namedTexture.end())
      throw std::runtime_error("no texture named '"+name+"'");
    return attributesStack.top()->namedTexture[name]; 
  }

    Parser::Parser(bool dbg, const std::string &basePath) 
      : scene(std::make_shared<Scene>()), dbg(dbg), basePath(basePath) 
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
      (Transforms&)ctm = ctm.stack.top();
      ctm.stack.pop();
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

    bool Parser::parseTransforms(std::shared_ptr<Token> token)
    {
      if (token->text == "ActiveTransform") {
        const std::string which = tokens->next()->text;
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
        setTransform(affine3f(ospcommon::one));
        return true;
      }
      if (token->text == "ReverseOrientation") {
        /* according to the docs, 'ReverseOrientation' only flips the
           normals, not the actual transform */
        return true;
      }
      if (token->text == "CoordSysTransform") {
        std::shared_ptr<Token> nameOfObject = tokens->next();
        cout << "ignoring 'CoordSysTransform'" << endl;
        return true;
      }
      return false;
    }

    void Parser::parseWorld()
    {
      if (dbg) cout << "Parsing PBRT World" << endl;
      while (1) {
        std::shared_ptr<Token> token = getNextToken();
        assert(token);
        if (token->text == "WorldEnd") {
          if (dbg) cout << "Parsing PBRT World - done!" << endl;
          break;
        }
        // -------------------------------------------------------
        // LightSource
        // -------------------------------------------------------
        if (token->text == "LightSource") {
          std::shared_ptr<LightSource> lightSource
            = std::make_shared<LightSource>(tokens->next()->text);
          parseParams(lightSource->param,*tokens);
          getCurrentObject()->lightSources.push_back(lightSource);
          continue;
        }
        if (token->text == "AreaLightSource") {
          std::shared_ptr<AreaLightSource> lightSource
            = std::make_shared<AreaLightSource>(tokens->next()->text);
          parseParams(lightSource->param,*tokens);
          continue;
        }
        // -------------------------------------------------------
        // Material
        // -------------------------------------------------------
        if (token->text == "Material") {
          std::string type = tokens->next()->text;
          std::shared_ptr<Material> material
            = std::make_shared<Material>(type);
          parseParams(material->param,*tokens);
          currentMaterial = material;
          continue;
        }
        if (token->text == "Texture") {
          std::string name = tokens->next()->text;
          std::string texelType = tokens->next()->text;
          std::string mapType = tokens->next()->text;
          // if (mapType == "imagemap") {
          //   /* ok, everythng else are params */
          // } else if (mapType == "scale") {
          //   // scale texture: two more parameters
          // }
          std::shared_ptr<Texture> texture
            = std::make_shared<Texture>(name,texelType,mapType);
          attributesStack.top()->namedTexture[name] = texture;
          parseParams(texture->param,*tokens);
          continue;
        }
        if (token->text == "MakeNamedMaterial") {
          std::string name = tokens->next()->text;
          std::shared_ptr<Material> material
            = std::make_shared<Material>("<implicit>");
          attributesStack.top()->namedMaterial[name] = material;
          parseParams(material->param,*tokens);

          /* named material have the parameter type implicitly as a
             parameter rather than explicitly on the
             'makenamedmaterial' command; so let's parse this here */
          std::shared_ptr<Param> type = material->param["type"];
          if (!type) throw std::runtime_error("named material that does not specify a 'type' parameter!?");
          std::shared_ptr<ParamT<std::string>> asString
            = std::dynamic_pointer_cast<ParamT<std::string> >(type);
          if (!asString)
            throw std::runtime_error("named material has a type, but not a string!?");
          assert(asString->getSize() == 1);
          material->type = asString->paramVec[0];
          continue;
        }

        if (token->text == "NamedMaterial") {
          // USE named material
          std::string name = tokens->next()->text;
          currentMaterial = attributesStack.top()->namedMaterial[name];
          continue;
        }


        if (token->text == "MakeNamedMedium") {
          std::string name = tokens->next()->text;
          std::shared_ptr<Medium> medium
            = std::make_shared<Medium>("<implicit>");
          attributesStack.top()->namedMedium[name] = medium;
          parseParams(medium->param,*tokens);

          /* named medium have the parameter type implicitly as a
             parameter rather than explicitly on the
             'makenamedmedium' command; so let's parse this here */
          std::shared_ptr<Param> type = medium->param["type"];
          if (!type) throw std::runtime_error("named medium that does not specify a 'type' parameter!?");
          std::shared_ptr<ParamT<std::string>> asString
            = std::dynamic_pointer_cast<ParamT<std::string> >(type);
          if (!asString)
            throw std::runtime_error("named medium has a type, but not a string!?");
          assert(asString->getSize() == 1);
          medium->type = asString->paramVec[0];
          continue;
        }

        if (token->text == "MediumInterface") {
          attributesStack.top()->mediumInterface.first = tokens->next()->text;
          attributesStack.top()->mediumInterface.second = tokens->next()->text;
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
          std::shared_ptr<Shape> shape
            = std::make_shared<Shape>(tokens->next()->text,
                                      currentMaterial,
                                      attributesStack.top()->clone(),
                                      ctm);
          parseParams(shape->param,*tokens);
          getCurrentObject()->shapes.push_back(shape);
          continue;
        }
        // -------------------------------------------------------
        // Volumes
        // -------------------------------------------------------
        if (token->text == "Volume") {
          std::shared_ptr<Volume> volume
            = std::make_shared<Volume>(tokens->next()->text);
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
          std::shared_ptr<Object> object = findNamedObject(name,1);

          objectStack.push(object);
          continue;
        }
          
        if (token->text == "ObjectEnd") {
          objectStack.pop();
          continue;
        }

        if (token->text == "ObjectInstance") {
          std::string name = tokens->next()->text;
          std::shared_ptr<Object> object = findNamedObject(name,1);
          std::shared_ptr<Object::Instance> inst
            = std::make_shared<Object::Instance>(object,ctm);
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

    std::shared_ptr<Token> Parser::getNextToken()
    {
      std::shared_ptr<Token> token = tokens->next();
      while (!token) {
        if (tokenizerStack.empty())
          return std::shared_ptr<Token>();
        tokens = tokenizerStack.top();
        tokenizerStack.pop();
        token = tokens->next();
      }
      assert(token);
      if (token->text == "Include") {
        std::shared_ptr<Token> fileNameToken = tokens->next();
        FileName includedFileName = fileNameToken->text;
        if (includedFileName.str()[0] != '/') {
          includedFileName = rootNamePath+includedFileName;
        }
        if (dbg) cout << "... including file '" << includedFileName.str() << " ..." << endl;
        
        tokenizerStack.push(tokens);
        tokens = std::make_shared<Lexer>(includedFileName);
        return getNextToken();
      }
      else
        return token;
    }
    
    void Parser::parseScene()
    {
      while (1) {
        std::shared_ptr<Token> token = getNextToken();
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
          scene->lookAt = std::make_shared<LookAt>(v0,v1,v2);
          continue;
        }
        if (token->text == "Camera") {
          std::shared_ptr<Camera> camera = std::make_shared<Camera>(tokens->next()->text);
          parseParams(camera->param,*tokens);
          scene->cameras.push_back(camera);
          continue;
        }
        if (token->text == "Sampler") {
          std::shared_ptr<Sampler> sampler = std::make_shared<Sampler>(tokens->next()->text);
          parseParams(sampler->param,*tokens);
          scene->sampler = sampler;
          continue;
        }
        if (token->text == "Integrator") {
          std::shared_ptr<Integrator> integrator = std::make_shared<Integrator>(tokens->next()->text);
          parseParams(integrator->param,*tokens);
          scene->integrator = integrator;
          continue;
        }
        if (token->text == "SurfaceIntegrator") {
          std::shared_ptr<SurfaceIntegrator> surfaceIntegrator
            = std::make_shared<SurfaceIntegrator>(tokens->next()->text);
          parseParams(surfaceIntegrator->param,*tokens);
          scene->surfaceIntegrator = surfaceIntegrator;
          continue;
        }
        if (token->text == "VolumeIntegrator") {
          std::shared_ptr<VolumeIntegrator> volumeIntegrator
            = std::make_shared<VolumeIntegrator>(tokens->next()->text);
          parseParams(volumeIntegrator->param,*tokens);
          scene->volumeIntegrator = volumeIntegrator;
          continue;
        }
        if (token->text == "PixelFilter") {
          std::shared_ptr<PixelFilter> pixelFilter = std::make_shared<PixelFilter>(tokens->next()->text);
          parseParams(pixelFilter->param,*tokens);
          scene->pixelFilter = pixelFilter;
          continue;
        }
        if (token->text == "Accelerator") {
          std::shared_ptr<Accelerator> accelerator = std::make_shared<Accelerator>(tokens->next()->text);
          parseParams(accelerator->param,*tokens);
          continue;
        }
        if (token->text == "Film") {
          std::shared_ptr<Film> film = std::make_shared<Film>(tokens->next()->text);
          parseParams(film->param,*tokens);
          continue;
        }
        if (token->text == "Accelerator") {
          std::shared_ptr<Accelerator> accelerator = std::make_shared<Accelerator>(tokens->next()->text);
          parseParams(accelerator->param,*tokens);
          continue;
        }
        if (token->text == "Renderer") {
          std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(tokens->next()->text);
          parseParams(renderer->param,*tokens);
          continue;
        }

        if (token->text == "WorldBegin") {
          ctm.reset();
          parseWorld();
          continue;
        }

        if (token->text == "Material") {
          throw std::runtime_error("'Material' field not within a WorldBegin/End context. "
                                   "Did you run the parser on the 'geometry.pbrt' file directly? "
                                   "(you shouldn't - it should only be included from within a "
                                   "pbrt scene file - typically '*.view')");
          continue;
        }

        
        throw std::runtime_error("unexpected token '"+token->text
                                 +"' at "+token->loc.toString());
      }
    }

    
    /*! parse given file, and add it to the scene we hold */
    void Parser::parse(const FileName &fn)
    {
      rootNamePath
        = basePath==""
        ? (std::string)fn.path()
        : (std::string)FileName(basePath);
      this->tokens = std::make_shared<Lexer>(fn);
      parseScene();      
    }

} // ::pbrt_parser
