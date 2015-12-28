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
#include "null-task.hh"

void NullTask::Draw(FaintDC&, Overlays&, const Point& ){
}

bool NullTask::DrawBeforeZoom(Layer) const {
  return false;
}

Command* NullTask::GetCommand(){
  return nullptr;
}

Cursor NullTask::GetCursor( const CursorPositionInfo& ) const{
  return Cursor::DONT_CARE;
}

Task* NullTask::GetNewTask(){
  return nullptr;
}

IntRect NullTask::GetRefreshRect( const IntRect&, const Point& ) const{
  return IntRect();
}

TaskResult NullTask::LeftDown(const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult NullTask::LeftUp(const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult NullTask::Motion(const CursorPositionInfo& ){
  return TASK_NONE;
}

TaskResult NullTask::Preempt(const CursorPositionInfo& ){
  return TASK_NONE;
}
