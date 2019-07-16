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

#include "pbrtParser/math.h" // export
#include "FileMapping.h"
// stl
#include <ios>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>

/*! namespace for all things pbrt parser, both syntactical *and* semantical parser */
namespace pbrt {

  namespace syntactic {

    /*! RAII wrapper for ANSI-C file */
    struct PBRT_PARSER_INTERFACE File {
      typedef std::shared_ptr<File> SP;

      File(const std::string &fn): name(fn) {
        file = fopen(fn.c_str(),"r");
        if (!file)
          throw std::runtime_error("could not open file '"+fn+"'");
      }
      void close() { fclose(file); file = NULL; }
      virtual ~File() { 
        if (file) fclose(file);
      }

      bool eof() const { return feof(file); }
      int get() { return fgetc(file); }

      /*! get name of the file */
      std::string getFileName() const { return name; }

    private:
      std::string name;
      FILE *file;
    };

    /*! Wrapper for memory mapped files, provides get() and eof() */
    struct PBRT_PARSER_INTERFACE MappedFile {
      typedef std::shared_ptr<MappedFile> SP;

      MappedFile(const std::string &fn)
        : name_(fn)
        , file_(fn)
        , data_(reinterpret_cast<const char*>(file_.data()), file_.nbytes())
        , pos_(data_.cbegin())
      {
      }

      /* check if current position is end of file */
      bool eof() const {
        return pos_ == data_.cend();
      }

      /*! get character according to current file position */
      int get() {
        if (eof())
          return EOF;
        else
          return static_cast<int>(static_cast<unsigned char>(*pos_++));
      }

      /*! get name of the file */
      std::string getFileName() const {
        return name_;
      }

    private:
      std::string name_;

      FileMapping file_;

      StringView data_;
      const char* pos_;
    };

    /*! wrapper for istream, provides eof() */
    template <typename Stream>
    struct PBRT_PARSER_INTERFACE IStream : Stream {
      typedef std::shared_ptr<IStream> SP;
      using Stream::Stream;
      bool eof() const { return Stream::rdstate() & std::ios_base::eofbit; }
    };

    /*! struct referring to a 'loc'ation in the input stream, given by file name
      and line number */
    struct PBRT_PARSER_INTERFACE Loc {
      //! pretty-print
      std::string toString() const {
        return "@" + (file?file->getFileName():"<invalid>") + ":" + std::to_string(line) + "." + std::to_string(col);
      }

      std::shared_ptr<File> file;
      int line;
      int col;
    };

    /*! read buffer with generic data source */
    template <typename DataSourcePtr>
    struct PBRT_PARSER_INTERFACE ReadBuffer {
      //! constructor
      explicit ReadBuffer(DataSourcePtr s);

      //! unget a character
      void unget_char(int c);

      //! get character from source or from peek buffer
      int get_char();

      //! get 'loc'action in input stream
      Loc get_loc() const;

    private:
      DataSourcePtr source;
      int peekBuffer[1] = { -1 };
      int line = 1;
      int lineBuffer[1] = { -1 };
      int col  = 0;
      int colBuffer[1] = { -1 };
    };

  } // ::syntactic
} // ::pbrt

// Implementation
#include "Buffer.inl"
