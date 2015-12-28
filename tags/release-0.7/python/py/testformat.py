import ifaint
def load( filename, props ):
    #print "HELLO WORLD", filename, props
    #print "load: ", filename, props
    #props.test()
    s = ifaint.Settings()
    s.linewidth = 2.5
    props.Rect((0.0,0.0,50.0,50.50),s)
    props.Ellipse((20.0,20.20, 100.0, 100.0),s)
    props.Polygon((20.0,0.0,100.0,0.0,100.0,200.0),s)
    id1 = props.Path("M150 0 L75 200 L225 200 Z",s)
    id2 = props.Spline((0.0,0.0,50,300,150,200,190,190,0,0), s)
    id3 = props.Text((100,100,100,100), "Hello", s)
    id4 = props.Group(id1, id2)
    try:
        props.Group(id1, id3)
    except ValueError, e:
        print "Caught error: ", e
    try:
        props.Group(id4, id4)
    except ValueError, e:
        print "Caught error: ", e

    

def save( filename, props ):
    print "save: ", filename, props
