import re

def parse_args( s ):
    assert( s.startswith("(") )
    assert( s.endswith(")") )
    
    args = [ float(arg) for arg in s.replace(" ", ",")[1:-1].split(",") ]
    return args

def parse_transform( s ):
    items = re.findall( "\w+\(.*?\)", s )
    transform = []

    for item in items:
        op, args = item.split("(")
        transform.append( (op, parse_args( '(' + args )) )
    return transform

if __name__ == '__main__':
    print parse_transform( 'rotate(20,1,2)')
    print parse_transform( 'rotate(20,1,2) scale(1,2)')
