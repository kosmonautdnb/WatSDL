gpp -c -O3 -ogl.obj glimpl.cpp
gpp -c -O3 -osdl.obj sdl.cpp
gpp -c -O3 -otest.obj sdltest.cpp
gpp -o SDLTEST.EXE gl.obj sdl.obj test.obj
@echo off
del gl.obj
del sdl.obj
del test.obj
