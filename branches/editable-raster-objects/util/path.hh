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

#ifndef FAINT_PATH_HH
#define FAINT_PATH_HH
#include <vector>
#include "util/char.hh"

class wxFileName;
class wxString;

namespace faint{
class PathImpl;

class FilePath{
  // This class chokes on non-absolute paths and tries to choke on
  // non-file paths as well, as an attempt to keep all stored file
  // paths in Faint absolute.
public:
  // Creates a FilePath. Asserts if the passed in path is not absolute
  // or does not refer to a file.
  static FilePath FromAbsoluteWx( const wxFileName& );
  static FilePath FromAbsolute( const faint::utf8_string& );
  FilePath( const FilePath& );
  ~FilePath();
  FilePath& operator=( const FilePath& other );
  std::string DirPath() const; // Fixme: Use a type
  std::string GetExtension() const; // Fixme: Use a type, std::string loses truncates utf-16 etc.
  std::string ToAscii() const;
  wxFileName ToWx() const;
  wxString StripPath() const;
private:
  explicit FilePath(PathImpl*);
  PathImpl* m_impl;
};

class FileList{
  // Simple vector-like container for FilePath:s, since I don't want
  // to add a default constructor to FilePath
public:
  FileList();
  FileList( const FileList& );
  ~FileList();
  const faint::FilePath& back() const;
  void clear();
  const faint::FilePath& operator[](size_t) const;
  bool empty() const;
  void push_back( const faint::FilePath& );
  size_t size() const;
private:
  FileList& operator=( const FileList&); // Prevent assignment
  std::vector<FilePath*> m_files;
};

bool exists( const FilePath& );
bool is_absolute_path( const utf8_string& );
bool is_file_path( const utf8_string& );
std::wstring to_wstring( const FilePath& );
faint::utf8_string to_utf8( const FilePath& );
} // namespace
#endif
