import os
def changed( source, target ):
    return not os.path.exists( target ) or os.path.getmtime( target ) < os.path.getmtime( source )
