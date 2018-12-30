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

#pragma once

/*! \file pbrt/semantic/Scene.h Defines the final semantically parsed
    scene */

#include "pbrtParser/syntactic/Scene.h"
// std
#include <mutex>

#ifdef _WIN32
#  ifdef pbrtParser_semantic_EXPORTS
#    define PBRT_PARSER_INTERFACE __declspec(dllexport)
#  else
#    define PBRT_PARSER_INTERFACE __declspec(dllimport)
#  endif
#else
#  define PBRT_PARSER_INTERFACE /* ignore on linux */
#endif

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for final, *semantic* parser - unlike the syntactic
    parser, the semantic one will distinguish between different
    matieral types, differnet shape types, etc, and it will not only
    store 'name:value' pairs for parameters, but figure out which
    parameters which geometry etc have, parse them from the
    parameters, etc */
  namespace semantic {

    using vec2f    = syntactic::vec2f;
    using vec3f    = syntactic::vec3f;
    using vec3i    = syntactic::vec3i;
    using vec4i    = syntactic::vec4i;
    using affine3f = syntactic::affine3f;

#if defined(PBRT_PARSER_VECTYPE_NAMESPACE)
    using vec2i    = PBRT_PARSER_VECTYPE_NAMESPACE::vec2i;
    using vec4f    = PBRT_PARSER_VECTYPE_NAMESPACE::vec4f;
    using box3f    = PBRT_PARSER_VECTYPE_NAMESPACE::box3f;
#else
    struct vec2i { int x, y; };
    struct vec4f { float x,y,z,w; };
    struct box3f { vec3f lower, upper; };
#endif
    
    /*! internal class used for serializing a scene graph to/from disk */
    struct BinaryWriter;
    
    /*! internal class used for serializing a scene graph to/from disk */
    struct BinaryReader;

    struct Object;

    /*! base abstraction for any entity in the pbrt scene graph that's
        not a paramter type (eg, it's a geometry/shape, a object, a
        instance, matierla, tetxture, etc */
    struct Entity {
      typedef std::shared_ptr<Entity> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const = 0; // { return "Entity"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) = 0;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) {}
    };

    /*! base abstraction for any material type. Note this is
        intentionally *not* abstract, since we will use it for any
        syntactic material type that we couldn't parse/recognize. This
        allows the app to eventually find non-recognized mateirals and
        replace them with whatever that app wants to use as a default
        material. Note at least one pbrt-v3 model uses a un-named
        material type, which will return this (empty) base class */
    struct Material : public Entity {
      typedef std::shared_ptr<Material> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Material"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct Texture : public Entity {
      typedef std::shared_ptr<Texture> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "(Abstract)Texture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };

    /*! a texture defined by a 2D image. Note we hadnle ptex separately,
      as ptex is not a 2D image format */
    struct ImageTexture : public Texture {
      typedef std::shared_ptr<ImageTexture> SP;

      ImageTexture(const std::string &fileName="") : fileName(fileName) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "ImageTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      std::string fileName;
    };
  
    /*! a texture defined by a disney ptex file. these are kind-of like
      image textures, but also kind-of not, so we handle them
      separately */
    struct PtexFileTexture : public Texture {
      typedef std::shared_ptr<PtexFileTexture> SP;

      PtexFileTexture(const std::string &fileName="") : fileName(fileName) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "PtexFileTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      std::string fileName;
    };
  
    struct FbmTexture : public Texture {
      typedef std::shared_ptr<FbmTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "FbmTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct WindyTexture : public Texture {
      typedef std::shared_ptr<WindyTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "WindyTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct MarbleTexture : public Texture {
      typedef std::shared_ptr<MarbleTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MarbleTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float scale { 1.f };
    };
  
    struct WrinkledTexture : public Texture {
      typedef std::shared_ptr<WrinkledTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "WrinkledTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    };
  
    struct ScaleTexture : public Texture {
      typedef std::shared_ptr<ScaleTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "ScaleTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      Texture::SP tex1, tex2;
      vec3f scale1 { 1.f };
      vec3f scale2 { 1.f };
    };
  
    struct MixTexture : public Texture {
      typedef std::shared_ptr<MixTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MixTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f amount { 1.f };
      Texture::SP map_amount;
      Texture::SP tex1, tex2;
      vec3f scale1 { 1.f };
      vec3f scale2 { 1.f };
    };
  
    struct ConstantTexture : public Texture {
      typedef std::shared_ptr<ConstantTexture> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "ConstantTexture"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f value;
    };
  
    /*! disney 'principled' material, as used in moana model */
    struct DisneyMaterial : public Material {
      typedef std::shared_ptr<DisneyMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "DisneyMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float anisotropic    { 0.f };
      float clearCoat      { 0.f };
      float clearCoatGloss { 1.f };
      vec3f color          { 1.f, 1.f, 1.f };
      float diffTrans      { 1.35f };
      float eta            { 1.2 };
      float flatness       { 0.2f };
      float metallic       { 0.f };
      float roughness      { .9f };
      float sheen          { .3f };
      float sheenTint      { 0.68f };
      float specTrans      { 0.f };
      float specularTint   { 0.f };
      bool  thin           { true };
    };

    struct MixMaterial : public Material
    {
      typedef std::shared_ptr<MixMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MixMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      Material::SP material0, material1;
      vec3f amount;
      Texture::SP map_amount;
    };
    
    struct MetalMaterial : public Material
    {
      typedef std::shared_ptr<MetalMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MetalMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float roughness  { 0.f };
      Texture::SP map_roughness;
      float uRoughness { 0.f };
      Texture::SP map_uRoughness;
      float vRoughness { 0.f };
      Texture::SP map_vRoughness;
      vec3f       eta  { 1.f, 1.f, 1.f };
      std::string spectrum_eta;
      vec3f       k    { 1.f, 1.f, 1.f };
      std::string spectrum_k;
      Texture::SP map_bump;
    };
    
    struct TranslucentMaterial : public Material
    {
      typedef std::shared_ptr<TranslucentMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "TranslucentMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f transmit { 1.f, 1.f, 1.f };
      vec3f reflect  { 0.f, 0.f, 0.f };
      vec3f kd;
      Texture::SP map_kd;
    };
    
    struct PlasticMaterial : public Material
    {
      typedef std::shared_ptr<PlasticMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "PlasticMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kd { .65f };
      Texture::SP map_kd;
      vec3f ks { .0f };
      Texture::SP map_ks;
      Texture::SP map_bump;
      float roughness { 0.00030000001f };
      Texture::SP map_roughness;
      bool remapRoughness { false };
   };
    
    struct SubstrateMaterial : public Material
    {
      typedef std::shared_ptr<SubstrateMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "SubstrateMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float uRoughness { .001 };
      float vRoughness { .001 };

      vec3f kd { .65f };
      Texture::SP map_kd;
      vec3f ks { .0f };
      Texture::SP map_ks;
      Texture::SP map_bump;
    };
    
    struct SubSurfaceMaterial : public Material
    {
      typedef std::shared_ptr<SubSurfaceMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "SubSurfaceMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      float uRoughness { .001 };
      float vRoughness { .001 };
      bool  remapRoughness { false };
      /*! in the one pbrt v3 model that uses that, these materials
          carry 'name' fields like "Apple" etc - not sure if that's
          supposed to specify some sub-surface medium properties!? */
      std::string name;
    };
    
    struct MirrorMaterial : public Material
    {
      typedef std::shared_ptr<MirrorMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MirrorMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kr { .7f };
      Texture::SP map_bump;
    };
    
    
    struct FourierMaterial : public Material
    {
      typedef std::shared_ptr<FourierMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "FourierMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      std::string fileName;
    };
    
    struct MatteMaterial : public Material
    {
      typedef std::shared_ptr<MatteMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "MatteMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kd { .65f };
      Texture::SP map_kd;
      float sigma { 10 };
      Texture::SP map_sigma;
      Texture::SP map_bump;
    };
    
    struct GlassMaterial : public Material
    {
      typedef std::shared_ptr<GlassMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "GlassMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      vec3f kr { 2.5371551514f, 2.5371551514f, 2.5371551514f };
      vec3f kt { 0.8627451062f, 0.9411764741f, 1.f };
      float index { 1.2999999523f };
    };
    
    struct UberMaterial : public Material {
      typedef std::shared_ptr<UberMaterial> SP;
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "UberMaterial"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      vec3f kd { .65f };
      Texture::SP map_kd;
    
      vec3f ks { 0.f };
      Texture::SP map_ks;
    
      vec3f kr { 0.f };
      Texture::SP map_kr;
      
      vec3f kt { 0.f };
      Texture::SP map_kt;
      
      vec3f opacity { 0.f };
      Texture::SP map_opacity;
      
      float alpha { 0.f };
      Texture::SP map_alpha;

      float shadowAlpha { 0.f };
      Texture::SP map_shadowAlpha;

      float index { 1.33333f };
      float roughness { 0.5f };
      Texture::SP map_bump;
    };

    /*! base abstraction for any geometric shape. for pbrt, this
        should actually be called a 'Shape'; we call it geometry for
        historic reasons (it's the term that embree and ospray - as
        well as a good many others - use) */
    struct Geometry : public Entity {
      typedef std::shared_ptr<Geometry> SP;
    
      Geometry(Material::SP material = Material::SP()) : material(material) {}
    
      /*! virtual destructor, to force this to be polymorphic */
      virtual ~Geometry() {}
    
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      virtual size_t getNumPrims() const = 0;
      virtual box3f getBounds() = 0;
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) = 0;
      virtual box3f getPrimBounds(const size_t primID);
      
      /*! the pbrt material assigned to the underlying shape */
      Material::SP material;
      /*! textures directl assigned to the shape (ie, not to the
        material) */
      std::map<std::string,Texture::SP> textures;
    };

    /*! a plain triangle mesh, with vec3f vertex and normal arrays, and
      vec3i indices for triangle vertex indices. normal and texture
      arrays may be empty, but if they exist, will use the same vertex
      indices as vertex positions */
    struct TriangleMesh : public Geometry {
      typedef std::shared_ptr<TriangleMesh> SP;

      /*! constructor */
      TriangleMesh(Material::SP material=Material::SP()) : Geometry(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "TriangleMesh"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
    
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      virtual size_t getNumPrims() const override
      {
        return index.size();
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
    
      virtual box3f getBounds() override;

      std::vector<vec3f> vertex;
      std::vector<vec3f> normal;
      std::vector<vec3i> index;
      /*! mutex to lock anything within this object that might get
        changed by multiple threads (eg, computing bbox */
      std::mutex         mutex;
      bool  haveComputedBounds { false };
      box3f bounds;
    };

    /*! a plain qaud mesh, where every quad is a pair of two
      triangles. "degenerate" quads (in the sense of quads that are
      actually only triangles) are in fact allowed by having index.w
      be equal to index.z. */
    struct QuadMesh : public Geometry {
      typedef std::shared_ptr<QuadMesh> SP;

      /*! constructor */
      QuadMesh(Material::SP material=Material::SP()) : Geometry(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "QuadMesh"; }

      /*! create a new quad mesh by merging triangle pairs in given
        triangle mesh. triangles that cannot be merged into quads will
        be stored as degenerate quads */
      static QuadMesh::SP makeFrom(TriangleMesh::SP triangleMesh);
    
      virtual size_t getNumPrims() const override
      {
        return index.size();
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
      virtual box3f getBounds() override;

      std::vector<vec3f> vertex;
      std::vector<vec3f> normal;
      std::vector<vec4i> index;
      /*! mutex to lock anything within this object that might get
        changed by multiple threads (eg, computing bbox */
      std::mutex         mutex;
      bool  haveComputedBounds { false };
      box3f bounds;
    };
  
    /*! a single sphere, with a radius and a transform. note we do
        _not_ yet apply the transform to update ratius and center, and
        use the pbrt way of storing them individually (in thoery this
        allows ellipsoins, too!?) */
    struct Sphere : public Geometry {
      typedef std::shared_ptr<Sphere> SP;

      /*! constructor */
      Sphere(Material::SP material=Material::SP()) : Geometry(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Sphere"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
    
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      virtual size_t getNumPrims() const override
      {
        return 1;
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
    
      virtual box3f getBounds() override;

      affine3f transform;
      float radius { 1.f };
    };


    /*! a single unit disk, with a radius and a transform. note we do
        _not_ yet apply the transform to update ratius and center, and
        use the pbrt way of storing them individually (in thoery this
        allows ellipsoins, too!?) */
    struct Disk : public Geometry {
      typedef std::shared_ptr<Disk> SP;

      /*! constructor */
      Disk(Material::SP material=Material::SP()) : Geometry(material) {}
    
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Disk"; }

      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
    
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      virtual size_t getNumPrims() const override
      {
        return 1;
      }
      virtual box3f getPrimBounds(const size_t primID, const affine3f &xfm) override;
      virtual box3f getPrimBounds(const size_t primID) override;
    
      virtual box3f getBounds() override;

      affine3f transform;
      float radius { 1.f };
      float height { 0.f };
    };


    struct Instance : public Entity {
      Instance() {}
      Instance(const std::shared_ptr<Object> &object,
               const affine3f                &xfm)
        : object(object), xfm(xfm)
      {}
      
      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Instance"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      virtual box3f getBounds() const;

      typedef std::shared_ptr<Instance> SP;
      
      std::shared_ptr<Object> object;
      affine3f                xfm { PBRT_PARSER_VECTYPE_NAMESPACE::one };
    };

    /*! a logical "NamedObject" that can be instanced */
    struct Object : public Entity {
      
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more concise, and easier to read */
      typedef std::shared_ptr<Object> SP;

      /*! constructor */
      Object(const std::string &name="") : name(name) {}

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Object"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;

      virtual box3f getBounds() const;
    
      std::vector<Geometry::SP>     geometries;
      std::vector<Instance::SP>     instances;
      std::string name = "";
    };

    /*! a camera as specified in the root .pbrt file. Note that unlike
      other entities for the camera we go one step beyond just
      seminatically parsing the objects, and actually already
      transform to a easier camera model.... this should eventually be
      reverted, only storing the actual values in this place */
    struct Camera : public Entity {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
      typedef std::shared_ptr<Camera> SP;

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Camera"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      /*! @{ computed mostly from 'lookat' value - using a _unit_ screen
        (that spans [-1,-1]-[+1,+1) that does not involve either
        screen size, screen resolution, or aspect ratio */
      vec3f screen_center;
      vec3f screen_du;
      vec3f screen_dv;
      /*! @} */
      vec3f lens_center;
      vec3f lens_du;
      vec3f lens_dv;
    };

    /*! a frame buffer of given resolution - for now we'll have this
      framebuffer actually store the pixels, and use a hardcoded vec4f
      format 

      \todo move the pixel array handling out of here - this is the
      scene graph, not renderer state
    */
    struct FrameBuffer : public Entity {
      /*! a "Type::SP" shorthand for std::shared_ptr<Type> - makes code
        more consise, and easier to read */
      typedef std::shared_ptr<FrameBuffer> SP;

      FrameBuffer(const vec2i &resolution)
        : resolution(resolution)
        // ,
        //   pixels(resolution.x*resolution.y)
      {}

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "FrameBuffer"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
    
      /*! helper function that returns total number of pixels in this image */
      inline size_t numPixels() const { return resolution.x*resolution.y; }
    
      // std::vector<vec4f> pixels;
      vec2i              resolution;
    };

    /*! the complete scene - pretty much the 'root' object that
        contains the WorldBegin/WorldEnd entities, plus high-level
        stuff like camera, frame buffer specification, etc */
    struct Scene : public Entity, public std::enable_shared_from_this<Scene> {
      typedef std::shared_ptr<Scene> SP;


      /*! save scene to given file name, and reutrn number of bytes written */
      size_t saveTo(const std::string &outFileName);
      static Scene::SP loadFrom(const std::string &inFileName);

      /*! pretty-printer, for debugging */
      virtual std::string toString() const override { return "Scene"; }
      /*! serialize out to given binary writer */
      virtual int writeTo(BinaryWriter &) override;
      /*! serialize _in_ from given binary file reader */
      virtual void readFrom(BinaryReader &) override;
    
      /*! checks if the scene contains more than one level of instancing */
      bool isMultiLevel() const;
      /*! checks if the scene contains more than one level of instancing */
      bool isSingleLevel() const;
      /*! checks if the scene contains more than one level of instancing */
      bool isSingleObject() const;
    
      /*! helper function that flattens a multi-level scene into a
        flattned scene */
      void makeSingleLevel();

      /*! return bounding box of scene */
      box3f getBounds() const
      { assert(root); return root->getBounds(); }
    
      Camera::SP      camera;
      FrameBuffer::SP frameBuffer;
    
      /*! the worldbegin/worldend content, in our format */
      Object::SP      root;
    };
    
    /* compute some _rough_ storage cost esimate for a scene. this will
       allow bricking builders to greedily split only the most egregious
       objects */
    double computeApproximateStorageWeight(Scene::SP scene);

    /*! parse a pbrt file (using the pbrt_parser project, and convert
      the result over to a naivescenelayout */
    Scene::SP importPBRT(const std::string &fileName);
  
  } // ::pbrt::semantic
} // ::pbrt
