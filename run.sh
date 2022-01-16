#!/bin/sh

rm ./bin/generated

gcc -o bin/generated -DGL_GLEXT_PROTOTYPES generated.c -g -lm -lglfw3 -lglad -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl || exit
./bin/generated
