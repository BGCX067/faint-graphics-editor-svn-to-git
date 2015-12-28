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
#include <cassert>
#include "app/getappcontext.hh"
#include "bitmap/bitmap.hh"
#include "tasks/null-task.hh"

Task::~Task(){}

void Task::Activate(){
}

bool Task::AcceptsPastedText() const {
  return false;
}

TaskResult Task::Char( const KeyInfo& ){
  return TASK_NONE;
}

bool Task::CopyText( faint::utf8_string&, bool ){
  return false;
}

bool Task::CanRedo() const{
  return false;
}

bool Task::CanUndo() const{
  return false;
}

Task* Task::DefaultTask() const{
  // The null-pointer tells the TaskWrapper to set the default-task or
  // request a tool change if no task is available.
  return 0;
}

TaskResult Task::Delete(){
  return TASK_NONE;
}

TaskResult Task::Deselect(){
  return TASK_NONE;
}

std::string Task::GetRedoName() const{
  return "";
}
std::string Task::GetUndoName() const{
  return "";
}

bool Task::HasSelection() const{
  return false;
}

TaskResult Task::LeftDoubleClick(const CursorPositionInfo&){
  return TASK_NONE;
}

void Task::Paste( const faint::utf8_string& ){
}

void Task::Redo(){
}

void Task::SelectionChange(){
}

TaskResult Task::SelectAll(){
  return TASK_NONE;
}

void Task::Undo(){
}

bool Task::UpdateSettings(){
  return false;
}

TaskWrapper::TaskWrapper(default_task defaultTask, initial_task initialTask)
  : m_defaultTask(0),
    m_task(0)
{
  Reset(defaultTask, initialTask);
}

TaskWrapper::~TaskWrapper(){
  Clear();
}

void TaskWrapper::Clear(){
  if ( m_defaultTask != m_task ){
    delete m_task;
    m_task = 0;
  }
  delete m_defaultTask;
  m_defaultTask = 0;
}

void TaskWrapper::Reset( default_task defaultTask, initial_task initialTask ){
  Clear();
  m_defaultTask = defaultTask.Get();
  m_task = initialTask.Get();
  if ( m_task == 0 ){
    m_task = defaultTask.Get();
  }
  assert( m_task != 0 );
  m_task->Activate();
}

Task* TaskWrapper::operator->(){
  assert( m_task != 0 );
  return m_task;
}

const Task* TaskWrapper::operator->() const{
  assert( m_task != 0 );
  return m_task;
}

bool TaskWrapper::Switch(){
  Task* newTask = m_task->GetNewTask();
  assert( newTask != m_task );
  if ( m_task != m_defaultTask ){
    delete m_task;
  }
  m_task = (newTask == 0 ? m_defaultTask : newTask );
  if ( m_task != 0 ){
    m_task->Activate();

    AppContext& app(GetAppContext());
    CanvasId canvasId( app.GetActiveCanvas().GetId());
    GetAppContext().OnDocumentStateChange(canvasId); // Fixme: Probably necessary, e.g. for selection state on task switch
    return true;
  }

  // No task left
  m_task = new NullTask();
  return false;
}
