// // ======================================================================== //
// // Copyright 2015-2019 Ingo Wald                                            //
// //                                                                          //
// // Licensed under the Apache License, Version 2.0 (the "License");          //
// // you may not use this file except in compliance with the License.         //
// // You may obtain a copy of the License at                                  //
// //                                                                          //
// //     http://www.apache.org/licenses/LICENSE-2.0                           //
// //                                                                          //
// // Unless required by applicable law or agreed to in writing, software      //
// // distributed under the License is distributed on an "AS IS" BASIS,        //
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// // See the License for the specific language governing permissions and      //
// // limitations under the License.                                           //
// // ======================================================================== //

// #include "Parser.h"
// #include "Scene.h"

// #include <fstream>

// /*! \file BinaryIO.h Defines the (internal) binary reader/writer
//     classes. No app should ever need to include this file, it is only
//     for internal code cleanliness (having the class definitions
//     separate from their implementation */

// /*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
// namespace pbrt {
//   /*! namespace for syntactic-only parser - this allows to distringuish
//     high-level objects such as shapes from objects or transforms,
//     but does *not* make any difference between what types of
//     shapes, what their parameters mean, etc. Basically, at this
//     level a triangle mesh is nothing but a geometry that has a string
//     with a given name, and parameters of given names and types */
//   namespace syntactic {
  
//     /*! a simple buffer for binary data. The writer will typically
//         serialzie each object into its own buffer, thus allowing to
//         'insert' other (referenced) entiries before the current one */
//     struct OutBuffer : public std::vector<uint8_t>
//     {
//       typedef std::shared_ptr<OutBuffer> SP;
//     };
  
//     /*! helper class that writes out a PBRT scene graph in a binary form
//       that is much faster to parse */
//     struct BinaryWriter {

//       BinaryWriter(const std::string &fileName) : binFile(fileName) {}

//       /*! our stack of output buffers - each object we're writing might
//         depend on other objects that it references in its paramset, so
//         we use a stack of such buffers - every object writes to its
//         own buffer, and if it needs to write another object before
//         itself that other object can simply push a new buffer on the
//         stack, and first write that one before the child completes its
//         own writes. */
//       std::stack<OutBuffer::SP> outBuffer;

//       void writeRaw(const void *ptr, size_t size);
        
//       template<typename T>
//       void write(const T &t);
      
//       void write(const std::string &t);
      
//       template<typename T>
//       void writeVec(const std::vector<T> &t);

//       void writeVec(const std::vector<bool> &t);

//       void writeVec(const std::vector<std::string> &t);
    
//       /*! start a new write buffer on the stack - all future writes will go into this buffer */
//       void startNewWrite();
    
//       /*! write the topmost write buffer to disk, and free its memory */
//       void executeWrite();
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Texture::SP texture);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Camera::SP camera);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Integrator::SP integrator);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Shape::SP shape);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Material::SP material);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(VolumeIntegrator::SP volumeIntegrator);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(SurfaceIntegrator::SP surfaceIntegrator);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Sampler::SP sampler);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Film::SP film);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(PixelFilter::SP pixelFilter);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Medium::SP medium);
    
//       /*! if this instance has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Object::Instance::SP instance);
    
//       /*! if this object has already been written to file, return a
//         handle; else write it once, create a unique handle, and return
//         that */
//       int emitOnce(Object::SP object);

//       /*! write an enitity's set of parameters */
//       void writeParams(const ParamSet &ps);

//       /*! write the root scene itself, by recusirvely writing all its entities */
//       void emit(Scene::SP scene);
      
//       /*! the file we'll be writing the buffers to */
//       std::ofstream binFile;
//     };



//     /*! equivalent to the binary writer: read a given binary file,
//         translating each written 'outbuffer' into a 'block', and then
//         parse the respective entity within this block */
//     struct BinaryReader {

//       struct Block {
//         inline Block() = default;
//         inline Block(Block &&) = default;
      
//         template<typename T>
//         inline T read();
//         inline void readRaw(void *ptr, size_t size);
//         std::string readString();
//         template<typename T>
//         inline void readVec(std::vector<T> &v);
//         inline void readVec(std::vector<std::string> &v);
//         inline void readVec(std::vector<bool> &v);
//         template<typename T>
//         inline std::vector<T> readVec();
        
//         std::vector<unsigned char> data;
//         size_t pos = 0;
//       };
    
//       BinaryReader(const std::string &fileName);
//       int readHeader();
//       inline Block readBlock();
//       template<typename T>
//       typename ParamArray<T>::SP readParam(const std::string &type, Block &block);
      
//       void readParams(ParamSet &paramSet, Block &block);
    
//       Camera::SP            readCamera(Block &block);
//       Integrator::SP        readIntegrator(Block &block);
//       Shape::SP             readShape(Block &block);
//       Material::SP          readMaterial(Block &block);
//       Texture::SP           readTexture(Block &block);
//       SurfaceIntegrator::SP readSurfaceIntegrator(Block &block);
//       PixelFilter::SP       readPixelFilter(Block &block);
//       VolumeIntegrator::SP  readVolumeIntegrator(Block &block);
//       Sampler::SP           readSampler(Block &block);
//       Film::SP              readFilm(Block &block);
//       Object::SP            readObject(Block &block);
//       Object::Instance::SP  readInstance(Block &block);
//       Scene::SP             readScene();
      
//       /*! our stack of output buffers - each object we're writing might
//         depend on other objects that it references in its paramset, so
//         we use a stack of such buffers - every object writes to its
//         own buffer, and if it needs to write another object before
//         itself that other object can simply push a new buffer on the
//         stack, and first write that one before the child completes its
//         own writes. */
//       std::stack<OutBuffer::SP> outBuffer;

//       /*! the file we'll be writing the buffers to */
//       std::ifstream binFile;

    
//       std::vector<Object::SP> objects;
//       std::vector<Material::SP> materials;
//       std::vector<Shape::SP> shapes;
//       std::vector<Object::Instance::SP> instances;
//       std::vector<Texture::SP> textures;
    
    
    
//     };
  

//   } // ::pbrt::syntactic
// } // ::pbrt
