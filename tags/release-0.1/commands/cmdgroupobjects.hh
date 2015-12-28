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
#include <vector>
#include "command.hh"
#include "objcomposite.hh"

class CanvasInterface;

class GroupObjects : public Command {
public:
  // Uses CanvasInterface to alter selection
  GroupObjects( CanvasInterface*, const std::vector<Object*>& );

  // Doesn't fiddle with selection at all
  GroupObjects( const std::vector<Object*>& );
  ~GroupObjects();
  void Do( faint::Image& );
  void Undo( faint::Image& );

  // Necessary for returning the group as a Python-object
  // when grouping via Python
  ObjComposite* GetComposite();
private:
  std::vector<Object*> m_objects;
  std::vector<int> m_objectDepths;
  ObjComposite* m_comp;
  CanvasInterface* m_canvas;
};

class UngroupObjects : public Command {
public:
  // Uses CanvasInterface to alter selection
  UngroupObjects( CanvasInterface*, const std::vector<Object*>& );

  // Doesn't fiddle with selection at all
  UngroupObjects( const std::vector<Object*>& );
  void Do( faint::Image& );
  void Undo( faint::Image& );

private:
  std::vector<Object*> m_objects;
  std::vector<int> m_objectDepths;
  CanvasInterface* m_canvas;
};

#endif
