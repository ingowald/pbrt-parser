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

  inline bool operator==(const TokenHandle &tk, const std::string &text)
  { return (std::string)tk == text; }
  
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
    TokenHandle token = next();
    if (!token)
      throw std::runtime_error("unexpected end of file\n@"+std::string(__PRETTY_FUNCTION__));
    return std::stod(token);
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
    const std::string open = *next();

    assert(open == "[");
    affine3f xfm;
    xfm.l.vx.x = std::stod(next()->c_str());
    xfm.l.vx.y = std::stod(next()->c_str());
    xfm.l.vx.z = std::stod(next()->c_str());
    float vx_w = std::stod(next()->c_str());
    assert(vx_w == 0.f);

    xfm.l.vy.x = std::stod(next()->c_str());
    xfm.l.vy.y = std::stod(next()->c_str());
    xfm.l.vy.z = std::stod(next()->c_str());
    float vy_w = std::stod(next()->c_str());
    assert(vy_w == 0.f);

    xfm.l.vz.x = std::stod(next()->c_str());
    xfm.l.vz.y = std::stod(next()->c_str());
    xfm.l.vz.z = std::stod(next()->c_str());
    float vz_w = std::stod(next()->c_str());
    assert(vz_w == 0.f);

    xfm.p.x    = std::stod(next()->c_str());
    xfm.p.y    = std::stod(next()->c_str());
    xfm.p.z    = std::stod(next()->c_str());
    float p_w  = std::stod(next()->c_str());
    assert(p_w == 1.f);

    const std::string close = *next();
    assert(close == "]");

    return xfm;
  }


  inline std::shared_ptr<Param> Parser::parseParam(std::string &name)
  {
    TokenHandle token = peek();

    if (token->type != Token::TOKEN_TYPE_STRING)
      return std::shared_ptr<Param>();

    std::vector<std::string> components = split(*next(),std::string(" \n\t"));

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
      throw std::runtime_error("unknown parameter type '"+type+"' "+token->loc.toString()
                               +std::string("\n@")+std::string(__PRETTY_FUNCTION__));
    }

    std::string value = *next();
    if (value == "[") {
      std::string p = *next();
        
      while (p != "]") {
        if (type == "texture") {
          std::dynamic_pointer_cast<ParamArray<Texture>>(ret)->texture 
            = getTexture(p);
        } else {
          ret->add(p);
        }
        p = *next();
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
    : scene(std::make_shared<Scene>()), dbg(false), basePath(basePath) 
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
    
  bool Parser::parseTransforms(TokenHandle token)
  {
    if (token == "ActiveTransform") {
      const std::string which = *next();
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
      std::string time = *next();
      std::cout << "'ActiveTransform' not implemented" << endl;
      return true;
    }
    if (token == "Identity") {
      setTransform(affine3f(ospcommon::one));
      return true;
    }
    if (token == "ReverseOrientation") {
      /* according to the docs, 'ReverseOrientation' only flips the
         normals, not the actual transform */
      return true;
    }
    if (token == "CoordSysTransform") {
      TokenHandle nameOfObject = next();
      cout << "ignoring 'CoordSysTransform'" << endl;
      return true;
    }
    return false;
  }

  void Parser::parseWorld()
  {
    if (dbg) cout << "Parsing PBRT World" << endl;
    while (1) {
      TokenHandle token = next();
      assert(token);
      if (dbg) std::cout << "World token : " << token->toString() << std::endl;

      // ------------------------------------------------------------------
      // WorldEnd - go back to regular parseScene
      // ------------------------------------------------------------------
      if (token == "WorldEnd") {
        if (dbg) cout << "Parsing PBRT World - done!" << endl;
        break;
      }
      
      // -------------------------------------------------------
      // LightSource
      // -------------------------------------------------------
      if (token == "LightSource") {
        std::shared_ptr<LightSource> lightSource
          = std::make_shared<LightSource>(next());
        parseParams(lightSource->param);
        getCurrentObject()->lightSources.push_back(lightSource);
        continue;
      }

      // ------------------------------------------------------------------
      // AreaLightSource
      // ------------------------------------------------------------------
      if (token == "AreaLightSource") {
        std::shared_ptr<AreaLightSource> lightSource
          = std::make_shared<AreaLightSource>(next());
        parseParams(lightSource->param);
        continue;
      }

      // -------------------------------------------------------
      // Material
      // -------------------------------------------------------
      if (token == "Material") {
        std::string type = *next();
        std::shared_ptr<Material> material
          = std::make_shared<Material>(type);
        parseParams(material->param);
        currentMaterial = material;
        continue;
      }

      // ------------------------------------------------------------------
      // Texture
      // ------------------------------------------------------------------
      if (token == "Texture") {
        std::string name = *next();
        std::string texelType = *next();
        std::string mapType = *next();
        std::shared_ptr<Texture> texture
          = std::make_shared<Texture>(name,texelType,mapType);
        attributesStack.top()->namedTexture[name] = texture;
        parseParams(texture->param);
        continue;
      }

      // ------------------------------------------------------------------
      // MakeNamedMaterial
      // ------------------------------------------------------------------
      if (token == "MakeNamedMaterial") {
        std::string name = *next();
        std::shared_ptr<Material> material
          = std::make_shared<Material>("<implicit>");
        attributesStack.top()->namedMaterial[name] = material;
        parseParams(material->param);

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
        std::string name = *next();
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
        std::string name = *next();
        currentMaterial = attributesStack.top()->namedMaterial[name];
        continue;
      }

      // ------------------------------------------------------------------
      // MakeNamedMedium
      // ------------------------------------------------------------------
      if (token == "MakeNamedMedium") {
        std::string name = *next();
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
        attributesStack.top()->mediumInterface.first = *next();
        attributesStack.top()->mediumInterface.second = *next();
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
        std::shared_ptr<Shape> shape
          = std::make_shared<Shape>(next(),
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
          = std::make_shared<Volume>(next());
        parseParams(volume->param);
        getCurrentObject()->volumes.push_back(volume);
        continue;
      }

      // -------------------------------------------------------
      // Transforms
      // -------------------------------------------------------
      if (parseTransforms(token))
        continue;
        
      // -------------------------------------------------------
      // ObjectBegin
      // -------------------------------------------------------
      if (token == "ObjectBegin") {
        std::string name = *next();
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
        std::string name = *next();
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
      throw std::runtime_error("unexpected token '"+token->toString()
                               +"' at "+token->loc.toString());
    }
  }

  TokenHandle Parser::next()
  {
    TokenHandle token = peek();
    if (!token)
      throw std::runtime_error("unexpected end of file ...");
    peekQueue.pop_front();
    lastLoc = token->loc;
    return token;
  }
    
  TokenHandle Parser::peek(int i)
  {
    while (peekQueue.size() <= i) {
      TokenHandle token = tokens->next();
      // first, handle the 'Include' statement by inlining such files
      if (token && token == "Include") {
        TokenHandle fileNameToken = tokens->next();
        std::string includedFileName = (std::string)fileNameToken;
        if (includedFileName[0] != '/') {
          includedFileName = rootNamePath+"/"+includedFileName;
        }
        if (dbg) cout << "... including file '" << includedFileName << " ..." << endl;
        
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
        return TokenHandle();
      
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
      
      TokenHandle token = next();
      if (!token)
        break;

      if (dbg) 
        cout << token->toString() << endl;

      // -------------------------------------------------------
      // Transforms
      // -------------------------------------------------------
      if (parseTransforms(token))
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
        
        addTransform(rcp(xfm));
        continue;
      }

      if (token == "ConcatTransform") {
        next(); // '['
        float mat[16];
        for (int i=0;i<16;i++)
          mat[i] = std::stod(next());

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
        std::string transformType = *next();
        continue;
      }


      if (token == "ActiveTransform") {
        std::string type = *next();
        continue;
      }

      if (token == "Camera") {
        std::shared_ptr<Camera> camera = std::make_shared<Camera>(next(),ctm);
        parseParams(camera->param);
        scene->cameras.push_back(camera);
        continue;
      }
      if (token == "Sampler") {
        std::shared_ptr<Sampler> sampler = std::make_shared<Sampler>(next());
        parseParams(sampler->param);
        scene->sampler = sampler;
        continue;
      }
      if (token == "Integrator") {
        std::shared_ptr<Integrator> integrator = std::make_shared<Integrator>(next());
        parseParams(integrator->param);
        scene->integrator = integrator;
        continue;
      }
      if (token == "SurfaceIntegrator") {
        std::shared_ptr<SurfaceIntegrator> surfaceIntegrator
          = std::make_shared<SurfaceIntegrator>(next());
        parseParams(surfaceIntegrator->param);
        scene->surfaceIntegrator = surfaceIntegrator;
        continue;
      }
      if (token == "VolumeIntegrator") {
        std::shared_ptr<VolumeIntegrator> volumeIntegrator
          = std::make_shared<VolumeIntegrator>(next());
        parseParams(volumeIntegrator->param);
        scene->volumeIntegrator = volumeIntegrator;
        continue;
      }
      if (token == "PixelFilter") {
        std::shared_ptr<PixelFilter> pixelFilter = std::make_shared<PixelFilter>(next());
        parseParams(pixelFilter->param);
        scene->pixelFilter = pixelFilter;
        continue;
      }
      if (token == "Accelerator") {
        std::shared_ptr<Accelerator> accelerator = std::make_shared<Accelerator>(next());
        parseParams(accelerator->param);
        continue;
      }
      if (token == "Film") {
        scene->film = std::make_shared<Film>(next());
        parseParams(scene->film->param);
        continue;
      }
      if (token == "Accelerator") {
        std::shared_ptr<Accelerator> accelerator = std::make_shared<Accelerator>(next());
        parseParams(accelerator->param);
        continue;
      }
      if (token == "Renderer") {
        std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(next());
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
        attributesStack.top()->mediumInterface.first = *next();
        attributesStack.top()->mediumInterface.second = *next();
        continue;
      }

      // ------------------------------------------------------------------
      // MakeNamedMedium
      // ------------------------------------------------------------------
      if (token == "MakeNamedMedium") {
        std::string name = *next();
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
                                 "Did you run the parser on the 'geometry.pbrt' file directly? "
                                 "(you shouldn't - it should only be included from within a "
                                 "pbrt scene file - typically '*.view')");
        continue;
      }

      throw std::runtime_error("unexpected token '"+token->text
                               +"' at "+token->loc.toString());
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

} // ::pbrt_parser
