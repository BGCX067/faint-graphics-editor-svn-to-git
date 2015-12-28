from ifaint import *

#start
# Draw a line on the active frame in the active image
active = get_active_image()
active.line(0,0,100,100)

# Add a line object with the active setting
active.Line((0,0,100,100), get_settings())

# ...however, many functions have forwards to the active
# canvas, so  this is enough:
line(0,0,100,100)
Line((0,0,100,100), get_settings())

# Retrieve the file name of the active image
name = get_active_image().get_filename()

# Get the frame objects for the active image
frames = get_active_image().get_frames()
