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

#include "commands/group-objects-cmd.hh"
#include "objects/objcomposite.hh"
#include "util/canvasinterface.hh"
#include "util/formatting.hh"
#include "util/objutil.hh"

class GroupObjects : public Command {
public:
  GroupObjects( const objects_t&, const select_added& );
  ~GroupObjects();
  void Do( CommandContext& ) override;
  std::string Name() const override;
  void Undo( CommandContext& ) override;

  // Necessary for returning the group as a Python-object
  // when grouping via Python
  ObjComposite* GetComposite();
private:
  ObjComposite* m_group;
  std::vector<size_t> m_objectDepths;
  objects_t m_objects;
  bool m_select;
};

class UngroupObjects : public Command {
public:
  UngroupObjects( const objects_t&, const select_added& );
  void Do( CommandContext& ) override;
  std::string Name() const override;
  void Undo( CommandContext& ) override;
private:
  objects_t m_objects;
  std::vector<int> m_objectDepths;
  bool m_select;
};

GroupObjects::GroupObjects( const objects_t& objects, const select_added& select )
  : Command( CommandType::OBJECT ),
    m_group( new ObjComposite(objects, Ownership::LOANER)),
    m_objects( objects ),
    m_select(select.Get())
{}

GroupObjects::~GroupObjects(){
  delete m_group;
}

void GroupObjects::Do( CommandContext& context ){
  // Initialize on the first run
  if ( m_objectDepths.empty() ){
    for ( const Object* obj : m_objects ){
      m_objectDepths.push_back( context.GetObjectZ(obj));
    }
  }

  // Traverse and remove objects, adding them instead as a group
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

static bool all_are_groups( const objects_t& objects ){
  for ( const Object* obj : objects ){
    if ( obj->GetObjectCount() == 0 ){
      return false;
    }
  }
  return true;
}

/*  Ungroup objects */
UngroupObjects::UngroupObjects( const objects_t& objects, const select_added& select )
  : Command( CommandType::OBJECT ),
    m_objects(objects),
    m_select(select.Get())
{}

std::string UngroupObjects::Name() const{
  return "Ungroup Objects";
}

void UngroupObjects::Do( CommandContext& context ){
  // Initialize object depths for Undo on the first run
  if ( m_objectDepths.empty() ) {
    for ( const Object* obj : m_objects ){
      m_objectDepths.push_back( context.GetObjectZ( obj ) );
    }
  }

  // Traverse and ungroup all groups in the command
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Object* group = m_objects[i];
    size_t numContained = group->GetObjectCount();
    const int depth = context.GetObjectZ( group );
    // Add the contained objects
    for ( size_t j = 0; j != numContained; j++ ){
      Object* contained = group->GetObject( j );
      context.Add( contained, depth + j, select_added(m_select), deselect_old(false) ); // Fixme: Causes loads of refresh-es for no good reason
    }
    // Remove the disbanded group
    context.Remove( group );
  }
}

void UngroupObjects::Undo( CommandContext& context ){
  for ( size_t i = 0; i!= m_objects.size(); i++ ){
    Object* group = m_objects[i];
    size_t numContained = group->GetObjectCount();
    if ( numContained == 0 ){
      continue;
    }
    for ( size_t j = 0; j != numContained; j++ ){
      context.Remove( group->GetObject(j) );
    }
    context.Add( group, m_objectDepths[i], select_added(m_select), deselect_old(true) );
  }
}

std::pair<Command*, Object*> group_objects_command( const objects_t& objects, const select_added& select ){
  assert(!objects.empty());
  GroupObjects* cmd = new GroupObjects(objects, select);
  return std::make_pair(cmd, cmd->GetComposite());
}

Command* ungroup_objects_command( const objects_t& objects, const select_added& select ){
  assert(!objects.empty());
  assert(all_are_groups(objects));
  return new UngroupObjects(objects, select);
}
