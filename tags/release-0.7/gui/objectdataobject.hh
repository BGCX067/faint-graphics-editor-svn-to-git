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

#ifndef FAINT_OBJECTDATAOBJECT_HH
#define FAINT_OBJECTDATAOBJECT_HH
#include "util/commonfwd.hh"

class wxDataObject;

namespace faint{
// Clipboard support for Faint objects
class ObjectDataObject : public wxDataObject {
public:
  ObjectDataObject();
  explicit ObjectDataObject( const objects_t& );
  ~ObjectDataObject();
  void GetAllFormats( wxDataFormat*, Direction) const; // Inherited
  size_t GetDataSize( const wxDataFormat& ) const; // Inherited
  bool GetDataHere( const wxDataFormat&, void* buf ) const; // Inherited
  size_t GetFormatCount( Direction ) const; // Inherited

  // Note: Calling code is responsible for deleting the returned
  // objects
  objects_t GetObjects() const;
  wxDataFormat GetPreferredFormat( Direction ) const; // Inherited
  bool IsSupported( const wxDataFormat&, Direction ) const; // Inherited
  bool SetData( const wxDataFormat&, size_t len, const void* buf ); // Inherited
private:
  objects_t m_objects;
};

}

#endif
