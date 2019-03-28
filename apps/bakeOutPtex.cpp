// ======================================================================== //
// Copyright 2018 Ingo Wald                                                 //
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

#include "pbrtParser/Scene.h"
#include <fstream>
#include <sstream>
#include <set>
#include <deque>
#include <queue>
#include "Ptexture.h"

#define PARALLEL_BAKING 1

#if PARALLEL_BAKING
#  include <tbb/parallel_for.h>
template<typename INDEX_T, typename TASK_T>
inline void parallel_for(INDEX_T nTasks, TASK_T&& fcn)
{
  tbb::parallel_for(INDEX_T(0), nTasks, std::forward<TASK_T>(fcn));
}
#endif

using namespace pbrt;

#define LOCK(mtx) std::lock_guard<std::mutex> _lock(mtx)

inline bool endsWith(const std::string &s, const std::string &suffix)
{
  return s.substr(s.size()-suffix.size(),suffix.size()) == suffix;
}


/*! gloabl scene we're splitting */
Scene::SP g_inScene;

inline unsigned short to5(const float channel)
{
  int maxVal = (1<<5)-1;
  return std::max(0,std::min(maxVal,int(maxVal*channel+.5f)));
}
  
inline unsigned short to555(const vec3f color)
{
  return
    (to5(color.z) << 10)+
    (to5(color.y) <<  5)+
    (to5(color.x) <<  0);
}
  
void bakeOut(const std::string &outFileBase,
             const std::string ptexName,
             QuadMesh::SP quads,
             int RES,
             bool bilinear)
{
  std::string error = "";
  Ptex::PtexTexture *tex = Ptex::PtexTexture::open(ptexName.c_str(),error);
  if (error != "")
    throw std::runtime_error("ptex error : "+error);
  Ptex::PtexFilter::Options defaultOptions;
  Ptex::PtexFilter *filter = Ptex::PtexFilter::getFilter(tex,defaultOptions);

  std::cout << " - baking out " << ptexName << std::endl;
  std::vector<unsigned short> ushorts;
  for (int faceID=0;faceID<quads->index.size();faceID++) {
      
    for (int iy=0;iy<RES;iy++)
      for (int ix=0;ix<RES;ix++)
        {
          vec3f color = vec3f(0,0,0);
          if (bilinear) {
            vec3f texel;
            vec2f uv = vec2f(ix,iy)*(1.f/float(RES-1));
            filter->eval(&texel.x,0,3,faceID,uv.x,uv.y,0,0,0,0);
            color = texel;
          } else {
            const int numSamples = 16;
            for (int i=0;i<numSamples;i++) {
              vec3f texel;
              vec2f uv = vec2f(ix+drand48(),iy+drand48())*(1.f/float(RES));
              filter->eval(&texel.x,0,3,faceID,uv.x,uv.y,0,0,0,0);
              color = color + texel;
            }
            color = color*(1./float(numSamples));
          }
          ushorts.push_back(to555(color));
        }
  }
    
  std::stringstream outFileName;
  outFileName << outFileBase;
  for (auto c : ptexName) {
    if (c == '/' || c == '.' || c == ':' || c == '\\')
      outFileName << '_';
    else
      outFileName << c;
  }
  std::ofstream out(outFileName.str());
  out.write((const char*)ushorts.data(),ushorts.size()*sizeof(ushorts[0]));
  std::cout << "\t-> " << outFileName.str() << std::endl;
  filter->release();
  tex->release();
}

void usage(const std::string &msg)
{
  if (!msg.empty()) std::cerr << std::endl << "***Error***: " << msg << std::endl << std::endl;
  std::cout << "Usage: ./bakeOutPtex inFile.pbf <args>" << std::endl;
  std::cout << "Args:" << std::endl;
  std::cout << " --res <resolution> : resolution to bake out with" << std::endl;
  std::cout << " -o outFileBase   : base string for all generated file names (required)" << std::endl;
  exit(msg != "");
}

extern "C" int main(int ac, char **av)
{
  std::string inFileName = "";
  std::string outFileBase = "";
  int res = 8;
  bool bilinear;
      
  for (int i=1;i<ac;i++) {
    const std::string arg = av[i];
    if (arg == "-o") {
      outFileBase = av[++i];
    } else if (arg == "--res") {
      res = atoi(av[++i]);
    } else if (arg == "--bilinear") {
      bilinear = true;
    } else if (arg[0] != '-')
      inFileName = arg;
    else
      usage("unknown cmd line arg '"+arg+"'");
  }

  if (inFileName == "") usage("no input file name specified");
  if (outFileBase == "") usage("no output file name base specified");

  std::cout << "==================================================================" << std::endl;
  std::cout << "loading scene..." << std::endl;
  std::cout << "==================================================================" << std::endl;

  if (endsWith(inFileName,".pbrt"))
    g_inScene = importPBRT(inFileName);
  else if (endsWith(inFileName,".pbf"))
    g_inScene = Scene::loadFrom(inFileName);
  else
    throw std::runtime_error("un-recognized file format '"+inFileName+"'");
  g_inScene->makeSingleLevel();

  std::cout << "==================================================================" << std::endl;
  std::cout << "precomputing global stuff..." << std::endl;
  std::cout << "==================================================================" << std::endl;

  std::set<TriangleMesh::SP> activeMeshes;
  for (auto inst : g_inScene->world->instances)
    for (auto geom : inst->object->shapes) {
      TriangleMesh::SP mesh = std::dynamic_pointer_cast<TriangleMesh>(geom);
      if (!mesh) continue;
      activeMeshes.insert(mesh);
    }
  std::cout << "found a total of " << activeMeshes.size() << " active meshes" << std::endl;

    
  std::cout << "==================================================================" << std::endl;
  std::cout << "extracting per-quad textures..." << std::endl;
  std::cout << "==================================================================" << std::endl;
    
  std::map<std::string,QuadMesh::SP> activePtexs;
  for (auto triMesh : activeMeshes) {
    QuadMesh::SP quads = QuadMesh::makeFrom(triMesh);
    for (auto texture : quads->textures) {
      PtexFileTexture::SP ptex = std::dynamic_pointer_cast<PtexFileTexture>(texture.second);
      if (!ptex)
        continue;

      if (!endsWith(ptex->fileName,".ptex") &&
          !endsWith(ptex->fileName,".ptx")) {
        throw std::runtime_error("ptex texture, but does not end in .ptex or .ptx??? (fileName="+ptex->fileName+")");
        continue;
      }
        
      if (texture.first == "color") {
        activePtexs[ptex->fileName] = quads;
      } else if (texture.first == "bumpmap") {
        std::cout << "ignoring 'bumpmap' (that's supposed to be a displacement, anyway ..." << std::endl;
      } else
        throw std::runtime_error("unknown ptex texture type "+texture.first);
    }
  }

  size_t totalTexturedQuads = 0;
  for (auto active : activePtexs) {
    std::cout << active.first << " : on mesh w/ "
              << active.second->getNumPrims() << " quads" << std::endl;
    totalTexturedQuads += active.second->getNumPrims();
  }
  std::cout << "found a total of " << activePtexs.size() << " 'color'-mapped ptex textures, "
            << "on a total of " << pbrt::math::prettyNumber(totalTexturedQuads) << " quads ..." << std::endl;

#if PARALLEL_BAKING
  {
    std::vector<std::string> fileName;
    std::vector<QuadMesh::SP> quadMesh;
    for (auto active : activePtexs) {
      fileName.push_back(active.first);
      quadMesh.push_back(active.second);
    }
    parallel_for(fileName.size(),[&](int ptexID){
        bakeOut(outFileBase,fileName[ptexID],quadMesh[ptexID],res,bilinear);
      });
  }
#else
  for (auto active : activePtexs) {
    bakeOut(outFileBase,active.first,active.second,res);
  }
#endif
    
  std::cout << std::endl;
  std::cout << "==================================================================" << std::endl;
  std::cout << "done" << std::endl;
  std::cout << "==================================================================" << std::endl;
    
}

