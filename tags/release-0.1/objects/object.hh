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
#include <cassert>
#include "settings.hh"
#include "util/idtypes.hh"
#include "geo/tri.hh"

class FaintDC;

enum HitInfo{
  HIT_NONE=0,
  HIT_INSIDE=1,
  HIT_NEAR,
  HIT_ATTACH_POINT,
  HIT_RESIZE_POINT,
  HIT_MOVABLE_POINT,
  HIT_BOUNDARY
};

enum sides{
  INVALID=-1,
  TOP_LEFT=0,
  TOP_RIGHT=1,
  BOTTOM_RIGHT=2,
  BOTTOM_LEFT=3
};

extern const std::string s_TypeObject;
extern const FaintSettings NullObjectSettings;
extern const faint::Color mask_outside;
extern const faint::Color mask_fill;
extern const faint::Color mask_edge;

class Object {
public:
  Object( const std::string* type, const Tri&, const FaintSettings& settings=NullObjectSettings );
  virtual ~Object();
  bool Active();
  void SetActive();
  void ClearActive();
  ObjectId GetId() const;
  Rect GetRect() const;
  const FaintSettings& GetSettings() const;
  Tri GetTri() const;
  const char* GetType() const;
  virtual bool HitTest( const Point& ) = 0;
  void SetTri( const Tri& );
  bool UpdateSettings( const FaintSettings& );

  virtual Object* Clone() const = 0;
  virtual void Draw( FaintDC& ) = 0;
  virtual void DrawMask( FaintDC& ) = 0;
  virtual std::vector<Point> GetAttachPoints();
  virtual std::vector<Point> GetMovablePoints() const;
  virtual std::vector<Point> GetResizePoints() = 0;
  virtual size_t GetObjectCount() const;
  virtual Object* GetObject(int);
  virtual const Object* GetObject(int) const;
  virtual Point GetPoint( size_t index ) const;
  
  virtual IntRect GetRefreshRect() = 0;
  virtual void SetPoint( const Point&, size_t index );
  virtual std::string StatusString() const;

  template<typename T>
  void Set( const T& s, typename T::ValueType v ){
    m_settings.Set( s, v );
  }
protected:
  FaintSettings m_settings;
private:
  virtual void OnSetTri();
  Object( const Object& );
  std::string m_type;
  ObjectId m_id;
  Tri m_tri;
  bool m_active;
};

Point Delta( Object*, const Point& to );
void MoveTo( Object*,  const Point& );
void OffsetBy( Object*, const Point& delta );
void OffsetBy( Object*, const IntPoint& delta );

#endif
