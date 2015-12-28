// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#include "util/path.hh"

namespace faint{
class BinaryReaderImpl;
class BinaryReader{
public:
  BinaryReader( const FilePath& );
  ~BinaryReader();
  void read( char*, size_t );
  bool eof() const;
  bool good() const;
  void seekg(std::streampos) const;
  std::streampos tellg() const;
private:
  BinaryReader( const BinaryReader& );
  BinaryReader& operator=( const BinaryReader& );
  BinaryReaderImpl* m_impl;
};

class BinaryWriterImpl;
class BinaryWriter{
public:
  BinaryWriter( const FilePath& );
  ~BinaryWriter();
  bool eof() const;
  bool good() const;
  void write( const char*, size_t );
private:
  BinaryWriter( const BinaryWriter& );
  BinaryWriter& operator=( const BinaryWriter& );
  BinaryWriterImpl* m_impl;
};

} // namespace
