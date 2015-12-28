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
#include <vector>

class Object;
class wxDataObject;

namespace faint{
// Clipboard support for Faint objects
class ObjectDataObject : public wxDataObject {
public:
  ObjectDataObject();
  explicit ObjectDataObject( const std::vector<Object*>& );
  ~ObjectDataObject();

  // Inherited
  wxDataFormat GetPreferredFormat( Direction ) const;
  size_t GetFormatCount( Direction ) const;
  void GetAllFormats( wxDataFormat*, Direction) const;
  size_t GetDataSize( const wxDataFormat& ) const;
  bool GetDataHere( const wxDataFormat&, void* buf ) const;
  bool SetData( const wxDataFormat&, size_t len, const void* buf );
  bool IsSupported( const wxDataFormat&, Direction ) const;

  // Custom
  // Note: Calling code is responsible for deleting the returned
  // objects
  std::vector<Object*> GetObjects() const;

private:
  void DeleteObjects();
  std::vector<Object*> m_objects;
};
}

#endif
