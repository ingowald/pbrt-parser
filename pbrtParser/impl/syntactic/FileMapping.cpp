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

#include <fstream>
#include <stdexcept>
#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "FileMapping.h"

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {
  /*! namespace for syntactic-only parser - this allows to distringuish
    high-level objects such as shapes from objects or transforms,
    but does *not* make any difference between what types of
    shapes, what their parameters mean, etc. Basically, at this
    level a triangle mesh is nothing but a geometry that has a string
    with a given name, and parameters of given names and types */
  namespace syntactic {

    FileMapping::FileMapping(const std::string &fname) : mapping(nullptr), num_bytes(0) {
#ifdef _WIN32
	  file = CreateFileA(fname.c_str(), GENERIC_READ,
	  		FILE_SHARE_READ, nullptr, OPEN_EXISTING,
	  		FILE_ATTRIBUTE_NORMAL, nullptr);
	  if (file == INVALID_HANDLE_VALUE) {
	  	throw std::runtime_error("Failed to open file " + fname);
	  }

	  LARGE_INTEGER file_size;
	  GetFileSizeEx(file, &file_size);
	  if (file_size.QuadPart == 0) {
	  	throw std::runtime_error("Cannot map 0 size file");
	  }

	  mapping_handle = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
	  if (mapping_handle == INVALID_HANDLE_VALUE) {
	  	throw std::runtime_error("Failed to create file mapping for " + fname);
	  }

	  num_bytes = file_size.QuadPart;
	  mapping = MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, num_bytes);
	  if (!mapping) {
	  	throw std::runtime_error("Failed to create mapped view of file " + fname);
	  }
#else
	  file = open(fname.c_str(), O_RDONLY);
	  if (file == -1) {
	  	perror("failed opening file");
	  	fflush(0);
	  	throw std::runtime_error("Failed to open file " + fname);
	  }

	  struct stat stat_buf;
	  fstat(file, &stat_buf);
	  num_bytes = stat_buf.st_size;

	  mapping = mmap(NULL, num_bytes, PROT_READ, MAP_SHARED, file, 0);
	  if (!mapping) {
	  	throw std::runtime_error("Failed to map file!");
	  }
#endif
}
    FileMapping::FileMapping(FileMapping &&fm)
	  : mapping(fm.mapping), num_bytes(fm.num_bytes), file(fm.file)
#ifdef _WIN32
	  , mapping_handle(fm.mapping_handle)
#endif
    {
	  fm.mapping = nullptr;
	  fm.num_bytes = 0;
#ifdef _WIN32
	  fm.file = INVALID_HANDLE_VALUE;
	  fm.mapping_handle = INVALID_HANDLE_VALUE;
#else
	  fm.file = -1;
#endif
    }
    FileMapping::~FileMapping() {
	  if (mapping) {
#ifdef _WIN32
		UnmapViewOfFile(mapping);
		CloseHandle(mapping_handle);
		CloseHandle(file);
#else
		munmap(mapping, num_bytes);
		close(file);
#endif
	  }
    }
    FileMapping& FileMapping::operator=(FileMapping &&fm) {
	  mapping = fm.mapping;
	  num_bytes = fm.num_bytes;
	  file = fm.file;
#ifdef _WIN32
	  mapping_handle = fm.mapping_handle;
#endif

	  fm.mapping = nullptr;
	  fm.num_bytes = 0;
#ifdef _WIN32
	  fm.file = INVALID_HANDLE_VALUE;
	  fm.mapping_handle = INVALID_HANDLE_VALUE;
#else
	  fm.file = -1;
#endif

	  return *this;
    }
    const uint8_t* FileMapping::data() const {
      return static_cast<uint8_t*>(mapping);
    }
    size_t FileMapping::nbytes() const {
      return num_bytes;
    }
  } // ::syntactic
} // ::pbrt
