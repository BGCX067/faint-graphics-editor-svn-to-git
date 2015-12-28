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

#include "objselectbehavior.hh"
#include "resizeobject.hh"
#include "moveobject.hh"
#include "util/util.hh"
#include "objects/objtext.hh"
#include "tools/textbehavior.hh"
#include "tools/rotateobject.hh"
#include "tools/movepoint.hh"
#include "cursors.hh"
#include "commands/addobjectcommand.hh"
#include "commands/cmdchangesetting.hh"
#include "commands/cmdupdatesettings.hh"
#include "commands/commandbunch.hh"
#include "faintdc.hh"
#include "getappcontext.hh"
#include "objects/objcomposite.hh"

const int dragThreshold = 3;

bool Remove_Selection( int modifiers ){
  return fl( TOOLMODIFIER1, modifiers );
}

bool Add_Selection( int modifiers ){
  return !Remove_Selection(modifiers);
}

/* Creates a single object update setting or a CommandBunch if multiple objects are affected */
template<typename T>
Command* UpdateObjectSettings( std::vector<Object*>& objects, const T& s, typename T::ValueType value ){
  if ( objects.size() == 0 ){
    return 0;
  }
  else if ( objects.size() == 1 ){
    Object* obj = objects[0];
    if ( obj->GetSettings().Has( s ) ){
      return new CmdChangeSetting<T>( obj, s, value );
    }
    return 0;
  }
  else {
    CommandBunch* commands = new CommandBunch( CMD_TYPE_OBJECT );
    for ( size_t i = 0; i != objects.size(); i++ ){
      Object* obj = objects[i];
      if ( obj->GetSettings().Has( s ) ){
        commands->Add( new CmdChangeSetting<T>( obj, s, value ) );
      }
    }
    return commands;
  }
}

ObjSettingNotifier::ObjSettingNotifier(){
  m_context = 0;
}

void ObjSettingNotifier::SetTarget( AppContext* context ){
  m_context = context;
}

void ObjSettingNotifier::Notify( const BoolSetting& s, bool value ){
  Command* cmd = UpdateObjectSettings( m_context->GetActiveCanvas().GetSelectedObjects(), s, value );
  RunCommand( cmd );
}
void ObjSettingNotifier::Notify( const IntSetting& s, int value ){
  Command* cmd = UpdateObjectSettings( m_context->GetActiveCanvas().GetSelectedObjects(), s, value );
  RunCommand( cmd );

}
void ObjSettingNotifier::Notify( const StrSetting& s, const std::string& value ){
  Command* cmd = UpdateObjectSettings( m_context->GetActiveCanvas().GetSelectedObjects(), s, value );
  RunCommand( cmd );
}
void ObjSettingNotifier::Notify( const ColorSetting& s, const faint::Color& value ){
  Command* cmd = UpdateObjectSettings( m_context->GetActiveCanvas().GetSelectedObjects(), s, value );
  RunCommand( cmd );
}
void ObjSettingNotifier::Notify( const FloatSetting& s, FloatSetting::ValueType value ){
  Command* cmd = UpdateObjectSettings( m_context->GetActiveCanvas().GetSelectedObjects(), s, value );
  RunCommand( cmd );
}

void ObjSettingNotifier::RunCommand( Command* cmd ){
  if ( cmd ){
    m_context->GetActiveCanvas().RunCommand( cmd );
  }
}

ObjSelectBehavior::ObjSelectBehavior( AppContext& ctx )
  : ToolBehavior( T_OBJ_SEL, m_notifier, true ),
    m_appContext( ctx )
{
  m_notifier.SetTarget( &m_appContext );
  m_mouseDown = false;
  m_drawRectangle = false;
  m_deselectCandidate = 0;
  m_dragCandidate = 0;
  m_otherTool = 0;
  SettingsFromObjects( m_appContext.GetActiveCanvas().GetSelectedObjects() );
  m_fullRefresh = false;
}

bool ObjSelectBehavior::DrawBeforeZoom(Layer) const{
  return false;
}

ToolRefresh ObjSelectBehavior::LeftDown( const CursorPositionInfo& info, int modifiers ){
  m_mouseDown = false;
  m_deselectCandidate = 0;
  m_dragCandidate = 0;

  if ( info.object == 0 ){
    return ClickedNothing( info, modifiers );
  }
  else if ( info.objSelected ){
    return ClickedSelected( info, modifiers );
  }
  return ClickedUnselected( info, modifiers );
}

ToolRefresh ObjSelectBehavior::LeftDoubleClick( const CursorPositionInfo& info, int ){
  if ( info.layerType != LAYER_OBJECT || info.object == 0 ) {
    return TOOL_NONE;
  }

  // If a text object is double-clicked, edit the text object
  ObjText* textObject = 0;
  if ( 0 != ( textObject = dynamic_cast<ObjText*>( info.object ) ) ){
    m_otherTool = new TextBehavior( textObject, this );
    return TOOL_CHANGE;
  }

  // If a composite containing a text object is double clicked, start
  // editing the text object
  ObjComposite* groupObject = 0;
  if ( 0 != ( groupObject = dynamic_cast<ObjComposite*>( info.object ) ) ){
    size_t numObjects = groupObject->GetObjectCount();
    for ( size_t i = 0; i != numObjects; i++ ){
      Object* object = groupObject->GetObject(i);
      textObject = dynamic_cast<ObjText*>( object );
      if ( textObject != 0 ){
        m_otherTool = new TextBehavior( textObject, this );
        return TOOL_CHANGE;
      }
    }
  }
  return TOOL_NONE;
}

ToolRefresh ObjSelectBehavior::ClickedNothing( const CursorPositionInfo& info, int modifiers ){
  if ( fl(TOOLMODIFIER1, modifiers) || fl(TOOLMODIFIER2, modifiers) ){
    // Do not deselect objects if a modifier is held, as this probably
    // means either that the user mis-clicked or intends to draw a
    // selection or deselection-rectangle
    m_mouseDown = true;
    m_clickPos = info.pos;
    return TOOL_NONE;
  }

  info.canvas->DeselectObjects();
  SettingsFromObjects( info.canvas->GetSelectedObjects() );
  m_appContext.UpdateShownSettings();
  m_mouseDown = true;
  m_clickPos = info.pos;
  m_fullRefresh = true;
  return TOOL_OVERLAY;
}

ToolRefresh ObjSelectBehavior::ClickedSelected( const CursorPositionInfo& info, int modifiers ){
  Object* object = info.object;
  int hit = info.hitStatus;
  if ( hit == HIT_RESIZE_POINT ){
    bool copy = fl( RIGHT_MOUSE, modifiers );
    // < 4 - i.e. can't rotate around UD/LR scaling
    if ( fl( TOOLMODIFIER1, modifiers ) && info.handleIndex < 4 ){
      m_otherTool = new ObjRotateBehavior( object, info.handleIndex, this, copy );
    }
    else {
      m_otherTool = new ObjResizeBehavior( object, info.handleIndex, this, copy );
    }
    return TOOL_CHANGE;
  }
  else if ( hit == HIT_MOVABLE_POINT ){
    bool copy = fl( RIGHT_MOUSE, modifiers );
    if ( copy ){
      info.canvas->DeselectObject( object );
    }
    m_otherTool = new MovePointBehavior( object, info.handleIndex, this, copy );
    return TOOL_CHANGE;
  }
  else if ( hit == HIT_INSIDE || hit == HIT_NEAR  || hit == HIT_BOUNDARY  ){
    if ( modifiers & TOOLMODIFIER1 ){
      // Ctrl-held while clicking a selected object - This can
      // either mean deselect or drag and copy, depending on
      // drag-distance (see Motion(...))
      m_clickPos = info.pos;
      m_mouseDown = true;
      m_deselectCandidate = object;
      return TOOL_NONE;
    }
    m_otherTool = new ObjMoveBehavior( info.canvas, object, info.canvas->GetSelectedObjects(), this, info.pos - object->GetTri().P0(), false );
    return TOOL_CHANGE;
  }
  assert( false );
  return TOOL_NONE;
}

ToolRefresh ObjSelectBehavior::ClickedUnselected( const CursorPositionInfo& info, int modifiers ){
  m_dragCandidate = info.object;
  m_clickPos = info.pos;
  m_mouseDown = true;
  bool deselectOld = !(fl( TOOLMODIFIER1, modifiers ) );
  info.canvas->SelectObject( info.object, deselectOld );
  SettingsFromObjects( info.canvas->GetSelectedObjects() );
  m_appContext.UpdateShownSettings();
  m_fullRefresh = true;
  return TOOL_OVERLAY;
}

ToolRefresh ObjSelectBehavior::CompleteRectangle( const CursorPositionInfo& info, int modifiers ){
  m_drawRectangle = false;
  m_mouseDown = false;
  std::vector<Object*>& objects = info.canvas->GetObjects();
  std::vector<Object*> inside;

  Rect r( m_p1, m_p2 );
  for ( size_t i = 0; i != objects.size(); i++ ){
    if ( Intersects( objects[i]->GetRect(), r ) ){
      inside.push_back( objects[i] );
    }
  }

  if ( Add_Selection( modifiers ) ){
    for ( size_t i = 0; i != inside.size(); i++ ){
      info.canvas->SelectObject( inside[i], false );
    }
  }
  else if ( Remove_Selection( modifiers ) ){
    for ( size_t i = 0; i != inside.size(); i++ ){
      info.canvas->DeselectObject( inside[i] );
    }
    m_fullRefresh = true;
  }

  SettingsFromObjects( info.canvas->GetSelectedObjects() );
  m_appContext.UpdateShownSettings();
  return TOOL_OVERLAY;
}

ToolRefresh ObjSelectBehavior::LeftUp( const CursorPositionInfo& info, int modifiers ){
  if ( m_drawRectangle ){
    return CompleteRectangle( info, modifiers );
  }
  else if ( m_mouseDown && fl( TOOLMODIFIER1, modifiers ) && distance( m_clickPos, info.pos ) < dragThreshold ) {
    // Mouse click without movement with ctrl held means deselect
    // clicked object
    info.canvas->DeselectObject( m_deselectCandidate );
    m_fullRefresh = true;
    m_mouseDown = false;
    return TOOL_OVERLAY;
  }

  m_mouseDown = false;
  return TOOL_NONE;
}

ToolRefresh ObjSelectBehavior::Motion( const CursorPositionInfo& info, int modifiers ){
  m_p2 = info.pos;
  StatusInterface& status = GetAppContext().GetStatusInfo();
  status.SetMainText( "" );
  status.SetText( StrPoint( info.pos ) );
  if ( !m_mouseDown ){
    return TOOL_NONE;
  }

  bool cloneBtn = fl( TOOLMODIFIER1, modifiers );
  if ( m_deselectCandidate != 0 && cloneBtn &&( distance( m_clickPos, info.pos ) >= dragThreshold ) ){
    // Clone the selected object(s)
    m_mouseDown = false;
    m_otherTool = new ObjMoveBehavior( info.canvas, m_deselectCandidate, info.canvas->GetSelectedObjects(), this, info.pos - m_deselectCandidate->GetTri().P0(), true );
    return TOOL_CHANGE;
  }
  else if ( m_dragCandidate != 0 && !cloneBtn && distance(m_clickPos, info.pos ) >= dragThreshold ){
    // Move the selected object(s)
    m_mouseDown = false;
    m_otherTool = new ObjMoveBehavior( info.canvas, m_dragCandidate, info.canvas->GetSelectedObjects(), this, info.pos - m_dragCandidate->GetTri().P0(), false );
    return TOOL_CHANGE;
  }
  else if ( !m_drawRectangle && m_mouseDown ){
    faint::coord dist = distance( m_clickPos, info.pos );
    if ( abs(dist >= dragThreshold ) ){
      m_drawRectangle = true;
      m_p1 = m_clickPos;
      m_p2 = info.pos;
      m_deselectCandidate = 0;
      if ( m_dragCandidate != 0 ){
        // Deselect the ctrl-clicked object
        info.canvas->DeselectObject( m_dragCandidate );
        m_fullRefresh = true;
        m_dragCandidate = 0;
      }
      return TOOL_OVERLAY;
    }
  }
  else if ( m_drawRectangle ){
    return TOOL_OVERLAY;
  }

  return TOOL_NONE;
}

Command* CreateUpdateCommand( const std::vector<Object*>& objects, const FaintSettings& newSettings ){
  // Gather commands for all updated objects
  std::vector<Command*> commands;
  for ( size_t i = 0; i != objects.size(); i++ ){
    Object* obj = objects[i];
    const FaintSettings& oldSettings = obj->GetSettings();
    if ( oldSettings.WouldUpdate( newSettings ) ){
      commands.push_back( new CmdUpdateSettings( obj, oldSettings, newSettings ) );
    }
  }

  if ( commands.size() == 1 ){
    return commands.back();
  }
  else if ( commands.size() > 1 ) {
    CommandBunch* commandBunch = new CommandBunch( CMD_TYPE_OBJECT );
    for ( size_t i = 0; i != commands.size(); i++ ){
      commandBunch->Add( commands[i] );
    }
    return commandBunch;
  }

  // No object was updated
  return 0;
}

ToolRefresh ObjSelectBehavior::ToolSettingUpdate( const FaintSettings& newSettings ){
  std::vector<Object*>& objects = m_appContext.GetActiveCanvas().GetSelectedObjects();
  Command* command = CreateUpdateCommand( objects, newSettings );
  if ( command != 0 ){
    GetAppContext().GetActiveCanvas().RunCommand( command );
    SettingsFromObjects( objects );
    return TOOL_OVERLAY;
  }
  return TOOL_NONE;
}

ToolRefresh ObjSelectBehavior::Preempt(){
  if ( m_drawRectangle ){
    m_drawRectangle = false;
    return TOOL_CANCEL;
  }
  return TOOL_NONE;
}

bool ObjSelectBehavior::Draw( FaintDC&, Overlays& overlays, const Point& p ){
  if ( m_drawRectangle ){
    m_p2 = p;
    Rect r( m_p1, m_p2 );
    overlays.Rectangle(r);
  }
  return true;
}

ToolBehavior* ObjSelectBehavior::GetNewTool(){
  return m_otherTool;
}

IntRect ObjSelectBehavior::GetRefreshRect( const IntRect& visible, const Point& ){
  if ( m_fullRefresh ){
    m_fullRefresh = false;
    return visible;
  }
  else if ( m_drawRectangle ){
    return truncated( Inflated(Rect(m_p1, m_p2), 2) );
  }
  
  return IntRect(IntPoint(0, 0), IntSize(0, 0));
}

int ObjSelectBehavior::GetCursor( const CursorPositionInfo& info, int modifiers ){
  if ( m_drawRectangle ){
    return CURSOR_CROSSHAIR;
  }
  if ( info.objSelected ){
    if ( info.hitStatus == HIT_RESIZE_POINT ){
      if ( fl( TOOLMODIFIER1, modifiers ) && info.handleIndex < 4 ){
        return CURSOR_ROTATE_RIGHT;
      }
      if ( info.handleIndex == 0 || info.handleIndex == 3 ){
        return CURSOR_RESIZE_NE;
      }

      else if ( info.handleIndex == 1 || info.handleIndex == 2 ) {
        return CURSOR_RESIZE_NW;
      }
      else if ( info.handleIndex == 4 || info.handleIndex == 5 ){
        return CURSOR_RESIZE_WE;
      }
      else if ( info.handleIndex == 6 || info.handleIndex == 7 ){
        return CURSOR_RESIZE_NS;
      }
    }
    else if ( info.hitStatus == HIT_INSIDE || info.hitStatus == HIT_NEAR || info.hitStatus == HIT_BOUNDARY ){
      return fl( TOOLMODIFIER1, modifiers ) ? CURSOR_CLONE : CURSOR_MOVE;
    }
    else if ( info.hitStatus == HIT_MOVABLE_POINT ){
      return CURSOR_MOVE_POINT;
    }
  }

  return CURSOR_ARROW;
}

void ObjSelectBehavior::SettingsFromObjects( const std::vector<Object*>& objects){
  m_settings.Clear();
  for ( size_t i = 0; i != objects.size(); i++ ){
    m_settings.UpdateAll( objects[i]->GetSettings() );
  }
}

const FaintSettings& ObjSelectBehavior::GetSettings(){
  return m_settings;
}

unsigned int ObjSelectBehavior::GetStatusFieldCount(){
  return 1;
}

void ObjSelectBehavior::SelectionChange(){
  SettingsFromObjects( m_appContext.GetActiveCanvas().GetSelectedObjects() );
  m_appContext.UpdateShownSettings();
}
