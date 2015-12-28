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

#include "addobjectcommand.hh"
#include "object.hh"
#include "faintimage.hh"
#include "getappcontext.hh"
#include "canvasinterface.hh"
#include <algorithm>
#include <vector>

AddObjectCommand::AddObjectCommand( Object* object, bool select )
  : Command( CMD_TYPE_OBJECT )
{
  m_object = object;
  m_select = select;
}

AddObjectCommand::~AddObjectCommand(){
  delete m_object;
}

void AddObjectCommand::Do( faint::Image& img ){
  img.AddObject( m_object );
  if ( m_select ){
    CanvasInterface& canvas = GetAppContext().GetActiveCanvas();    
    canvas.SelectObject( m_object, false );
    m_select = false;
  }
}

void AddObjectCommand::Undo( faint::Image& img ){
  CanvasInterface& canvas = GetAppContext().GetActiveCanvas();
  const std::vector<Object*>& selected = canvas.GetSelectedObjects();
  if ( std::find( selected.begin(), selected.end(), m_object ) != selected.end() ){
    canvas.DeselectObject( m_object );
  }
  img.RemoveObject( m_object );
}
