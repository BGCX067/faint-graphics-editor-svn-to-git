#ifndef FAINT_IDTYPES_HH
#define FAINT_IDTYPES_HH

template<int group>
class FaintID{
public:
  FaintID(){
    static int max_id = 1007;
    m_rawId = max_id++;
  }

  bool operator==( const FaintID<group>& other ) const{
    return other.m_rawId == m_rawId;
  }

  int Raw() const{
    return m_rawId;
  }
private:
  int m_rawId;
};

typedef FaintID<107> CanvasId;
typedef FaintID<108> ObjectId;

#endif
