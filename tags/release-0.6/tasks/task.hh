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

#ifndef FAINT_TASK_HH
#define FAINT_TASK_HH
#include "geo/intrect.hh"
#include "rendering/faintdc.hh"
#include "tools/tool.hh"
#include "util/pending.hh"
#include "util/unique.hh"

enum TaskResult{
  // Return values for Task-methods
  TASK_NONE, // Action had no externally relevant effect
  TASK_DRAW, // Task wants to draw
  TASK_CHANGE, // Task wants to be switched out for return of GetNewTask()
  TASK_COMMIT, // Task has a command ready from GetCommand()
  TASK_COMMIT_AND_CHANGE // Task has a command and a new task
};

class Task{
  // Tasks are used to split Tools into states.
public:
  virtual ~Task();
  virtual void Activate();
  virtual bool AcceptsPastedText() const;
  virtual bool CanRedo() const;
  virtual bool CanUndo() const;
  virtual TaskResult Char( const KeyInfo& );
  virtual bool CopyText(faint::utf8_string& out, bool clear );
  virtual TaskResult Delete();
  virtual TaskResult Deselect();
  virtual bool Draw( FaintDC&, Overlays&, const Point& ) = 0;
  virtual bool DrawBeforeZoom(Layer::type) const = 0;
  virtual Command* GetCommand() = 0;
  virtual Cursor::type GetCursor( const CursorPositionInfo& ) const = 0;
  virtual Task* GetNewTask() = 0;
  virtual IntRect GetRefreshRect( const IntRect&, const Point& ) const = 0;
  virtual std::string GetRedoName() const;
  virtual std::string GetUndoName() const;
  virtual bool HasSelection() const;
  virtual TaskResult LeftDoubleClick( const CursorPositionInfo& );
  virtual TaskResult LeftDown( const CursorPositionInfo& ) = 0;
  virtual TaskResult LeftUp( const CursorPositionInfo& ) = 0;
  virtual TaskResult Motion( const CursorPositionInfo& ) = 0;
  virtual void Paste( const faint::utf8_string& );
  virtual TaskResult Preempt( const CursorPositionInfo& ) = 0;
  virtual void Redo();
  virtual TaskResult SelectAll();
  virtual void SelectionChange();
  virtual void Undo();
  virtual bool UpdateSettings(); // Fixme: Change to void
protected:
  Task* DefaultTask() const;
};

class TaskWrapper;
typedef Unique<Task*, TaskWrapper, 1> default_task;
typedef Unique<Task*, TaskWrapper, 2> initial_task;

class TaskWrapper{
  // Contains an active task and a default task. The active task is
  // accesible via operator->.
  //
  // The Switch() function changes to a new task retrieved from the
  // current task using Task::GetNewTask. If the new task is null, the
  // default task will be activated. If the default task is also null,
  // a benign NullTask will be activated, and Switch will return
  // false.
  //
  // Releases the task memory on destruction.
public:
  explicit TaskWrapper(default_task, initial_task=initial_task(0));
  void Reset(default_task, initial_task initial=initial_task(0));
  ~TaskWrapper();
  bool Switch();
  Task* operator->();
  const Task* operator->() const;
private:
  void Clear();
  TaskWrapper& operator=(const TaskWrapper&); // Prevent assignment
  Task* m_defaultTask;
  Task* m_task;
};

// For tasks to be returned by Task::GetNewTask.
typedef Pending<Task> PendingTask;

#endif
