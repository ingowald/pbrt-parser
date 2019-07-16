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

#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {

    class FileMapping {
      void *mapping;
      size_t num_bytes;
#ifdef _WIN32
      HANDLE file;
      HANDLE mapping_handle;
#else
      int file;
#endif

    public:
      // Map the file into memory
      FileMapping(const std::string &fname);
      FileMapping(FileMapping &&fm);
      ~FileMapping();
      FileMapping& operator=(FileMapping &&fm);

      FileMapping(const FileMapping &) = delete;
      FileMapping& operator=(const FileMapping&) = delete;

      const uint8_t* data() const;
      size_t nbytes() const;
    };

    template<typename T>
    class BasicStringView {
      const T *ptr;
      size_t count;

      public:
      BasicStringView() : ptr(nullptr), count(0) {}

      /* Create a typed view into a string. The count is in
       * number of elements of T in the view.
       */
      BasicStringView(const T* ptr, size_t count)
          : ptr(ptr), count(count)
      {}
      const T& operator[](const size_t i) const {
          return ptr[i];
      }
      const T* data() const {
          return ptr;
      }
      size_t size() const {
          return count;
      }
      const T* cbegin() const {
          return ptr;
      }
      const T* cend() const {
          return ptr + count;
      }
    };

    typedef BasicStringView<char> StringView;
  } // ::syntactic
} // ::pbrt
