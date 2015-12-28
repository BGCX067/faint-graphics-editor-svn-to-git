#ifndef FAINT_IDTYPES_HH
#define FAINT_IDTYPES_HH

template<int group>
class FaintID{
public:
  FaintID(){
    static int max_id = 1007;
    m_rawId = max_id++;
  }

  static FaintID DefaultID(){
    return FaintID<group>(0);
  }

  bool operator==( const FaintID<group>& other ) const{
    return other.m_rawId == m_rawId;
  }

  bool operator!=( const FaintID<group>& other ) const{
    return !operator==(other);
  }

  bool operator<( const FaintID<group>& other ) const{
    return m_rawId < other.m_rawId;
  }

  int Raw() const{
    return m_rawId;
  }
private:
  FaintID(int id){
    m_rawId = id;
  }
  int m_rawId;
};

typedef FaintID<107> CanvasId;
typedef FaintID<108> ObjectId;
typedef FaintID<109> CommandId;
// 110 reserved
typedef FaintID<111> FrameId;

#endif
