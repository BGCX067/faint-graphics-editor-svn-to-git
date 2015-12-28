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

#ifndef FAINT_CMDGROUPOBJECTS_HH
#define FAINT_CMDGROUPOBJECTS_HH
#include "commands/command.hh"
#include "objects/objcomposite.hh"

class CanvasInterface;

class GroupObjects : public Command {
public:
  GroupObjects( const objects_t&, const select_added& );
  ~GroupObjects();
  void Do( CommandContext& );
  std::string Name() const;
  void Undo( CommandContext& );

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
  UngroupObjects( const objects_t& );
  void Do( CommandContext& );
  std::string Name() const;
  void Undo( CommandContext& );
private:
  objects_t m_objects;
  std::vector<int> m_objectDepths;
  bool m_select;
};

#endif
