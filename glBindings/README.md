# Generate gl bindings for rift

This is just a note for asking to remove all contents of glad.h until line saying `typedef void (APIENTRY *GLVULKANPROCNV)(void);` including itself. The contents above this line will be added automatically by gen.py also remove the last seven lines of the file.
NOTE: The bindings are generated for compatibility profile, OpenGL 4.6, C