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

#include "pbrtParser/Scene.h"
// std
#include <set>
#include <string.h>
#include <algorithm>

#ifndef PRINT
# define PRINT(var) std::cout << #var << "=" << var << std::endl;
# define PING std::cout << __FILE__ << "::" << __LINE__ << ": " << __PRETTY_FUNCTION__ << std::endl;
#endif

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  
  box3f Shape::getPrimBounds(const size_t primID)
  {
    return getPrimBounds(primID,affine3f::identity());
  }

  std::string TriangleMesh::toString() const 
  {
    return "TriangleMesh";
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




  box3f Sphere::getPrimBounds(const size_t /*unused: primID*/, const affine3f &xfm) 
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
    





  box3f Disk::getPrimBounds(const size_t /*unused: primID*/, const affine3f &xfm) 
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

  box3f Curve::getPrimBounds(const size_t /*unused: primID*/, const affine3f &xfm) 
  {
    box3f primBounds = box3f::empty_box();
    for (auto p : P)
      primBounds.extend(xfmPoint(xfm,p));
    vec3f maxWidth = vec3f(std::max(width0,width1));
    primBounds.lower = primBounds.lower - maxWidth;
    primBounds.upper = primBounds.upper + maxWidth;
    return primBounds;
  }

  box3f Curve::getPrimBounds(const size_t /*unused: primID */) 
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
  }


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
  }
    
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
      for (auto lightSource : object->lightSources)
        ours->lightSources.push_back(lightSource);
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
  
} // ::pbrt
