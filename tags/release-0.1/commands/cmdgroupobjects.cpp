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

#include "cmdgroupobjects.hh"
#include "faintimage.hh"
#include "canvasinterface.hh"

GroupObjects::GroupObjects( CanvasInterface* canvas, const std::vector<Object*>& objects )
  : Command( CMD_TYPE_OBJECT ),
    m_objects( objects ),
    m_canvas( canvas )
{
  m_comp = new ObjComposite(m_objects, LOANER);
}

GroupObjects::GroupObjects( const std::vector<Object*>& objects )
  : Command( CMD_TYPE_OBJECT ),
    m_objects( objects ),
    m_canvas(0)
{
  m_comp = new ObjComposite(m_objects, LOANER);
}

GroupObjects::~GroupObjects(){
  delete m_comp;
}

void GroupObjects::Do( faint::Image& image ){
  if ( m_objectDepths.empty() ){
    for ( size_t i = 0; i != m_objects.size(); i++ ){
      m_objectDepths.push_back( image.GetObjectZ( m_objects[i] ) );
    }
  }

  int depth = 0;
  for ( size_t i = 0; i!= m_comp->GetObjectCount(); i++ ){
    Object* obj = m_comp->GetObject(i);
    depth = std::max( depth, image.GetObjectZ( obj ) );
    if ( m_canvas != 0 ){
      m_canvas->DeselectObject( obj );
    }
    image.RemoveObject( obj );
  }
  image.AddObject( m_comp, depth );
  if ( m_canvas != 0 ){
    m_canvas->SelectObject( m_comp );
  }
}

void GroupObjects::Undo( faint::Image& image ){
  if ( m_canvas != 0 ){
    m_canvas->DeselectObject(m_comp);
  }
  image.RemoveObject( m_comp );
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Object* obj = m_objects[i];
    image.AddObject( obj, m_objectDepths[i] );
  }
}

ObjComposite* GroupObjects::GetComposite(){
  return m_comp;
}

/*  Ungroup objects */
UngroupObjects::UngroupObjects( CanvasInterface* canvas, const std::vector<Object*>& objects )
  : Command( CMD_TYPE_OBJECT ),
    m_objects(objects),
    m_canvas(canvas)
{}

/*  Ungroup objects */
UngroupObjects::UngroupObjects( const std::vector<Object*>& objects )
  : Command( CMD_TYPE_OBJECT ),
    m_objects(objects),
    m_canvas( 0 )
{}

void UngroupObjects::Do( faint::Image& image ){
  if ( m_objectDepths.empty() ) {
    for ( size_t i = 0; i!= m_objects.size(); i++ ){
      m_objectDepths.push_back( image.GetObjectZ( m_objects[i] ) );
    }
  }

  // Traverse and ungroup all objects in the command
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Object* obj = m_objects[i];
    size_t numContained = obj->GetObjectCount();
    if ( numContained == 0 ){
      continue;
    }
    const int depth = image.GetObjectZ( obj );
    // Add the contained objects
    for ( size_t j = 0; j != numContained; j++ ){
      Object* contained = obj->GetObject( j );
      image.AddObject( contained, depth + j );
      if ( m_canvas != 0 ){
        m_canvas->SelectObject( contained, false );
      }
    }
    // Remove the ungrouped composite
    if ( m_canvas != 0 ){
      m_canvas->DeselectObject( obj );
    }
    image.RemoveObject( obj );
  }
}

void UngroupObjects::Undo( faint::Image& img ){
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Object* obj = m_objects[i];
    size_t numContained = obj->GetObjectCount();
    if ( numContained == 0 ){
      continue;
    }

    for ( size_t j = 0; j != numContained; j++ ){
      Object* subObj = obj->GetObject( j );
      img.RemoveObject( subObj );
      if ( m_canvas != 0 ){
        m_canvas->DeselectObject( subObj );
      }
    }
    img.AddObject( obj, m_objectDepths[i] );
    if ( m_canvas != 0 ){
      m_canvas->SelectObject( obj );
    }
  }
}
