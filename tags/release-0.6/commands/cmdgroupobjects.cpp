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

#include "commands/cmdgroupobjects.hh"
#include "util/canvasinterface.hh"
#include "util/derefiter.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"

GroupObjects::GroupObjects( const objects_t& objects, const select_added& select )
  : Command( CMD_TYPE_OBJECT ),
    m_group( new ObjComposite(objects, LOANER)),
    m_objects( objects ),
    m_select(select.Get())
{}

GroupObjects::~GroupObjects(){
  delete m_group;
}

void GroupObjects::Do( CommandContext& context ){
  if ( m_objectDepths.empty() ){
    for ( size_t i = 0; i != m_objects.size(); i++ ){
      m_objectDepths.push_back( context.GetObjectZ( m_objects[i] ) );
    }
  }

  size_t depth = 0;
  for ( size_t i = 0; i!= m_group->GetObjectCount(); i++ ){
    Object* obj = m_group->GetObject(i);
    depth = std::max( depth, context.GetObjectZ( obj ) );
    context.Remove(obj);
  }
  context.Add( m_group, depth, select_added(m_select), deselect_old(false) );
}

std::string GroupObjects::Name() const{
  return space_sep("Group", get_collective_name(m_objects));
}

void GroupObjects::Undo( CommandContext& context ){
  context.Remove(m_group);
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    context.Add( m_objects[i], m_objectDepths[i], select_added(false), deselect_old(false) );
  }
}

ObjComposite* GroupObjects::GetComposite(){
  return m_group;
}

/*  Ungroup objects */
UngroupObjects::UngroupObjects( const objects_t& objects, const select_added& select )
  : Command( CMD_TYPE_OBJECT ),
    m_objects(objects),
    m_select(select.Get())
{}

std::string UngroupObjects::Name() const{
  return "Ungroup Objects";
}

void UngroupObjects::Do( CommandContext& context ){
  if ( m_objectDepths.empty() ) {
    for ( size_t i = 0; i!= m_objects.size(); i++ ){
      m_objectDepths.push_back( context.GetObjectZ( m_objects[i] ) );
    }
  }

  // Traverse and ungroup all objects in the command
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Object* obj = m_objects[i];
    size_t numContained = obj->GetObjectCount();
    if ( numContained == 0 ){
      continue;
    }
    const int depth = context.GetObjectZ( obj );
    // Add the contained objects
    for ( size_t j = 0; j != numContained; j++ ){
      Object* contained = obj->GetObject( j );
      context.Add( contained, depth + j, select_added(m_select), deselect_old(false) );
    }
    // Remove the ungrouped composite
    context.Remove( obj );
  }
}

void UngroupObjects::Undo( CommandContext& context ){
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Object* obj = m_objects[i];
    size_t numContained = obj->GetObjectCount();
    if ( numContained == 0 ){
      continue;
    }
    for ( size_t j = 0; j != numContained; j++ ){
      context.Remove( obj->GetObject(j) );
    }
    context.Add( obj, m_objectDepths[i], select_added(m_select), deselect_old(true) );
  }
}
