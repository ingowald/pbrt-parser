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

#include "ospcommon/vec.h"
#include "ospcommon/AffineSpace.h"
  
#define PBRT_PARSER_VECTYPE_NAMESPACE  ospcommon

#include "pbrtParser/semantic/Scene.h"
// std
#include <set>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for final, *semantic* parser - unlike the syntactic
    parser, the semantic one will distinguish between different
    matieral types, differnet shape types, etc, and it will not only
    store 'name:value' pairs for parameters, but figure out which
    parameters which geometry etc have, parse them from the
    parameters, etc */
  namespace semantic {
  
    box3f Geometry::getPrimBounds(const size_t primID)
    {
      return getPrimBounds(primID,affine3f(ospcommon::one));
    }


    box3f TriangleMesh::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f primBounds = ospcommon::empty;
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].x]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].y]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].z]));
      return primBounds;
    }
    
    box3f TriangleMesh::getPrimBounds(const size_t primID) 
    {
      box3f primBounds = ospcommon::empty;
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
      
      box3f bounds = ospcommon::empty;
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
      return getPrimBounds(primID,affine3f(ospcommon::one));
    }
    
    box3f Sphere::getBounds() 
    {
      return getPrimBounds(0);
    }
    





    box3f Disk::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f ob(vec3f(-radius,-radius,0),vec3f(+radius,+radius,height));
      affine3f _xfm = xfm * transform;
      
      box3f bounds = ospcommon::empty;
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
      return getPrimBounds(primID,affine3f(ospcommon::one));
    }
    
    box3f Disk::getBounds() 
    {
      return getPrimBounds(0);
    }
    



    
    box3f QuadMesh::getPrimBounds(const size_t primID, const affine3f &xfm) 
    {
      box3f primBounds = ospcommon::empty;
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].x]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].y]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].z]));
      primBounds.extend(xfmPoint(xfm,vertex[index[primID].w]));
      return primBounds;
    }

    box3f QuadMesh::getPrimBounds(const size_t primID) 
    {
      box3f primBounds = ospcommon::empty;
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
        for (auto v : vertex) bounds.extend(v);
        haveComputedBounds = true;
        return bounds;
      }
      return bounds;
    }



    box3f Object::getBounds() const
    {
      box3f bounds = ospcommon::empty;
      PING;
      PRINT(bounds);
      for (auto inst : instances) {
        if (inst) {
          const box3f ib = inst->getBounds();
          PING; PRINT(ib); 
          bounds.extend(ib);
          PRINT(bounds);
        }
      }
      for (auto geom : geometries) {
        if (geom) {
          const box3f gb = geom->getBounds();
          PING; PRINT(gb);
          bounds.extend(gb);
          PRINT(bounds);
        }
      }
      PING; PRINT(bounds);
      return bounds;
    };


    /*! compute (conservative but possibly approximate) bbox of this
      instance in world space. This box is not necessarily tight, as
      it getsc omputed by transforming the object's bbox */
    box3f Instance::getBounds() const
    {
      box3f ob = object->getBounds();
      box3f bounds = ospcommon::empty;
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.lower.x,ob.upper.y,ob.upper.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.lower.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.lower.y,ob.upper.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.upper.y,ob.lower.z)));
      bounds.extend(xfmPoint(xfm,vec3f(ob.upper.x,ob.upper.y,ob.upper.z)));
      PING; PRINT(bounds);
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
    
      std::set<Geometry::SP> activeGeometries;
      for (auto inst : scene->root->instances) 
        for (auto geom : inst->object->geometries) 
          activeGeometries.insert(geom);

      double costEstimate
        = scene->root->instances.size() * instWeight
        + activeGeometries.size() * geomWeight;
      for (auto geom : activeGeometries)
        costEstimate += geom->getNumPrims() * primWeight;
      return costEstimate;
    }
  
  
    /*! create a new quad mesh by merging triangle pairs in given
      triangle mesh. triangles that cannot be merged into quads will
      be stored as degenerate quads */
    QuadMesh::SP QuadMesh::makeFrom(TriangleMesh::SP tris)
    {
      QuadMesh::SP out = std::make_shared<QuadMesh>(tris->material);
      out->vertex   = tris->vertex;
      out->normal   = tris->normal;
      out->textures = tris->textures;
      // out->texcoord = in->texcoord;

      if (tris->vertex.size() == 3*tris->index.size())
        std::cout << "warning: this looks like a 'fat' triangle mesh - optimize???" << std::endl;
    
      for (int i=0;i<tris->index.size();i++) {
        vec3i idx0 = tris->index[i+0];
        if ((i+1) < tris->index.size()) {
          vec3i idx1 = tris->index[i+1];
          if (idx1.x == idx0.x && idx1.y == idx0.z) {
            // could merge!!!
            out->index.push_back(vec4i(idx0.x,idx0.y,idx0.z,idx1.z));
            ++i;
            continue;
          }
        }
        // could not merge :-( - emit tri as degenerate quad
        out->index.push_back(vec4i(idx0.x,idx0.y,idx0.z,idx0.z));
      }
      return out;
    }
    

    struct SingleLevelFlattener {
      SingleLevelFlattener(Object::SP root)
        : result(std::make_shared<Object>())
      {
        // inlineMultiLevelNodes(root);
      
        traverse(root, affine3f(ospcommon::one));
      }

      // void inlineAllGeometry(Object::SP target,
      //                        Object::SP object,
      //                        const affine3f &xfm, int &numInlined)
      // {
      //   for (auto geom : object->geometries) {
      //     TriangleMesh::SP mesh = std::dynamic_pointer_cast<TriangleMesh>(geom);
      //     if (!geom) continue;

      //     TriangleMesh::SP newMesh = std::make_shared<TriangleMesh>(mesh->material);
      //     newMesh->index = mesh->index;
      //     for (auto v : mesh->vertex)
      //       newMesh->vertex.push_back(xfmPoint(xfm,v));
      //     target->geometries.push_back(newMesh);
      //   }

      //   numInlined++;
      
      //   for (auto inst : object->instances)
      //     if (inst && inst->object) 
      //       inlineAllGeometry(target,inst->object, xfm * inst->xfm, numInlined);
      // }

      // Object::SP inlineAllGeometry(Object::SP object, int &numInlined)
      // {
      //   if (object->instances.empty()) return object;

      //   Object::SP newObj = std::make_shared<Object>("inlinedFrom:"+object->name);
      //   inlineAllGeometry(newObj,object,affine3f(one),numInlined);
      //   return newObj;
      // }

      // void inlineMultiLevelNodes(Object::SP root)
      // {
      //   std::map<Object::SP,std::set<Instance::SP>> parentsOf;

      //   // first, traverse everything to populate 'parentsOf' field
      //   {
      //     std::set<Object::SP> alreadyPushedObjects;
      //     std::stack<Object::SP> workStack;
      //     workStack.push(root);
      //     alreadyPushedObjects.insert(root);
      //     while (!workStack.empty()) {
      //       Object::SP obj = workStack.top(); workStack.pop();
      //       for (auto inst : obj->instances) {
      //         parentsOf[inst->object].insert(inst);
      //         if (alreadyPushedObjects.find(inst->object) == alreadyPushedObjects.end()) {
      //           workStack.push(inst->object);
      //           alreadyPushedObjects.insert(inst->object);
      //         }
      //       }
      //     }
      //   }
      // }
    
      Object::SP
      getOrCreateEmittedGeometryFrom(Object::SP object)
      {
        if (object->geometries.empty()) return Object::SP();
        if (alreadyEmitted[object]) return alreadyEmitted[object];
      
        Object::SP ours = std::make_shared<Object>("GeometryFrom:"+object->name);
        for (auto geom : object->geometries)
          ours->geometries.push_back(geom);
        return alreadyEmitted[object] = ours;
      }
    
      void traverse(Object::SP object, const affine3f &xfm)
      {
        if (!object)
          return;
      
        Object::SP emittedGeometry = getOrCreateEmittedGeometryFrom(object);
        if (emittedGeometry) {
          Instance::SP inst
            = std::make_shared<Instance>(emittedGeometry,xfm);
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
      this->root = SingleLevelFlattener(root).result;
    }

    /*! checks if the scene contains more than one level of instancing */
    bool Scene::isSingleLevel() const
    {
      if (!root->geometries.empty())
        return false;
      for (auto inst : root->instances)
        if (!inst->object->instances.empty())
          return false;
      return true;
    }
  
  } // ::pbrt::semantic
} // ::pbrt
