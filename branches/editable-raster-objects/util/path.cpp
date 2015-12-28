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

#include "wx/filename.h"
#include "util/path.hh"
#include "util/convertwx.hh"

namespace faint{

class PathImpl{
public:
  PathImpl( const wxFileName& in_path )
    : path(in_path)
  {}
  wxFileName path;
};

FilePath FilePath::FromAbsoluteWx( const wxFileName& wxFn ){
  assert(wxFn.IsAbsolute());
  assert(!wxFn.IsDir());
  return FilePath( new PathImpl(wxFn) );
}

FilePath FilePath::FromAbsolute( const faint::utf8_string& filename ){
  wxFileName wxFn( to_wx(filename) );
  assert(wxFn.IsAbsolute());
  assert(!wxFn.IsDir());
  return FilePath( new PathImpl(wxFn) );
}

FilePath::FilePath(PathImpl* impl){
  m_impl = impl;
}

FilePath::FilePath( const FilePath& other ){
  m_impl = new PathImpl(*other.m_impl);
}

std::string FilePath::GetExtension() const{
  return std::string(m_impl->path.GetExt());
}

std::string FilePath::ToAscii() const{
  return std::string(m_impl->path.GetFullPath());
}

wxFileName FilePath::ToWx() const{
  return m_impl->path;
}

wxString FilePath::StripPath() const{
  wxString name;
  wxString ext;
  bool hasExt;
  wxFileName::SplitPath(ToWx().GetLongPath(),
    0, // Volume
    0, // Path
    &name,
    &ext,
    &hasExt );

  if ( !hasExt ){
    return name;
  }
  return name + wxString(".") + ext;
}

FilePath::~FilePath(){
  delete m_impl;
}

FilePath& FilePath::operator=( const FilePath& other ){
  if ( this == &other ){
    return *this;
  }

  delete m_impl;
  m_impl = new PathImpl(*other.m_impl);
  return *this;
}

std::string FilePath::DirPath() const{
  return std::string(m_impl->path.GetPath());
}

FileList::FileList(){
}

FileList::FileList( const FileList& other ){
  for ( const FilePath* path : other.m_files ){
    push_back( *path );
  }
}

FileList::~FileList(){
  clear();
}

  const faint::FilePath& FileList::back() const{
  assert( !m_files.empty() );
  return *m_files.back();
}

void FileList::clear(){
  for ( FilePath* path : m_files ){
    delete path;
  }
  m_files.clear();
}

bool FileList::empty() const{
  return m_files.empty();
}

const faint::FilePath& FileList::operator[](size_t i) const{
  assert( i < m_files.size() );
  return *m_files[i];
}

void FileList::push_back( const faint::FilePath& path ){
  m_files.push_back(new faint::FilePath(path) );
}

size_t FileList::size() const{
  return m_files.size();
}

bool exists( const FilePath& path ){
  return path.ToWx().FileExists();
}

bool is_absolute_path( const utf8_string& path ){
  return wxFileName(to_wx(path)).IsAbsolute();
}

bool is_file_path( const utf8_string& path ){
  return !wxFileName(to_wx(path)).IsDir();
}

std::wstring to_wstring( const FilePath& path ){
  wxString strWx( path.ToWx().GetLongPath() );
  return strWx.ToStdWstring();
}

faint::utf8_string to_utf8( const FilePath& path ){
  wxString strWx( path.ToWx().GetLongPath() );
  return to_faint(strWx);
}
} // namespace
