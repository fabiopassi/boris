#!/bin/bash

gcc boris.c -o boris -Wall -O2 -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lm

if [ "$?" == "0" ]; then
	./boris
fi
