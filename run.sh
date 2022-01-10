#!/bin/sh

rm ./bin/generated

gcc -o bin/generated generated.c || exit
./bin/generated
