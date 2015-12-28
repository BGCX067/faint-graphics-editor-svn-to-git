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

#ifndef FAINT_COLORDATAOBJECT_HH
#define FAINT_COLORDATAOBJECT_HH
#include "wx/dataobj.h"
#include "wx/dnd.h"
#include "geo/color.hh"
// For color drag and drop and clipboard operations
class ColorDataObject : public wxDataObject {
public:
  explicit ColorDataObject( const faint::Color& );
  wxDataFormat GetPreferredFormat( Direction ) const;
  size_t GetFormatCount( Direction ) const;
  void GetAllFormats( wxDataFormat*, Direction) const;
  size_t GetDataSize( const wxDataFormat& ) const;
  bool GetDataHere( const wxDataFormat&, void* buf ) const;
  bool SetData( const wxDataFormat&, size_t len, const void* buf );
  bool IsSupported( const wxDataFormat&, Direction ) const;
  faint::Color GetColor();
  wxDragResult GetResult();
  wxDragResult GetOperationType() const;
  void SetOperationType(wxDragResult );
private:  
  faint::Color m_color;
  static wxDragResult m_dragResult;
};

class ColorDropTarget{
public:
  ColorDropTarget( wxWindow* targetWindow );
  virtual wxDragResult OnDropColor( const wxPoint&, const faint::Color& ) =0;
};

#endif
