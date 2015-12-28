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

#ifndef FAINT_OBJECT_HH
#define FAINT_OBJECT_HH
#include <vector>
#include "geo/tri.hh"
#include "util/idtypes.hh"
#include "util/settings.hh"

class FaintDC;

extern const std::string s_TypeObject;
extern const Settings NullObjectSettings;
extern const faint::Color mask_outside;
extern const faint::Color mask_fill;
extern const faint::Color mask_no_fill;
extern const faint::Color mask_edge;

class Object {
public:
  Object( const std::string* type, const Tri&, const Settings& settings=NullObjectSettings );
  virtual ~Object();
  bool Active() const;
  virtual bool CanRemovePoint() const;
  void ClearActive();
  virtual Object* Clone() const = 0;

  // True if the object is closed, so that points at the opposite ends
  // should be considered adjacent
  virtual bool CyclicPoints() const;
  virtual void Draw( FaintDC& ) = 0;
  virtual void DrawMask( FaintDC& ) = 0;
  virtual bool Extendable() const;

  // Gets the points in this objects that other objects and tools can
  // snap to
  virtual std::vector<Point> GetAttachPoints() const;

  ObjectId GetId() const;
  Rect GetRect() const;

  // Gets points at which additional points can be inserted
  virtual std::vector<Point> GetExtensionPoints() const;

  // Gets the points that can be moved individually
  virtual std::vector<Point> GetMovablePoints() const;
  virtual size_t GetObjectCount() const;
  virtual Object* GetObject(int);
  virtual const Object* GetObject(int) const;
  virtual Point GetPoint( size_t index ) const;
  virtual IntRect GetRefreshRect() = 0;
  const Settings& GetSettings() const;

  // Gets the points in this object that wish to snap to points in
  // other points when resizing/moving
  virtual std::vector<Point> GetSnappingPoints() const;
  Tri GetTri() const;
  const char* GetType() const;
  virtual bool HitTest( const Point& );
  bool Inactive() const;
  virtual void InsertPoint(const Point&, size_t index );
  virtual size_t NumPoints() const;
  virtual void RemovePoint( size_t index );
  template<typename T> void Set( const T& s, typename T::ValueType );
  void SetActive(bool active=true);
  virtual void SetPoint( const Point&, size_t index );
  void SetTri( const Tri& );
  void SetVisible( bool );
  virtual bool ShowSizeBox() const;
  virtual std::string StatusString() const;
  bool UpdateSettings( const Settings& );
  bool Visible() const;
protected:
  Settings m_settings;
private:
  virtual void OnSetTri();
  Object( const Object& );
  std::string m_type;
  ObjectId m_id;
  Tri m_tri;
  bool m_active;
  bool m_visible;
};

template<typename T>
void Object::Set( const T& s, typename T::ValueType v ){
  m_settings.Set( s, v );
}

#endif
