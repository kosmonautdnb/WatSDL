wcl386.exe /c /mf -fosdl.obj sdl.cpp
wcl386.exe /c /mf -fogl.obj glimpl.cpp
wcl386.exe /c /mf -fotest.obj sdltest.cpp
wlink FILE GL.OBJ FILE SDL.OBJ FILE TEST.OBJ OPTION QUIET OPTION STack = 65536 NAME SDLTEST.EXE
@echo off
del sdl.obj
del gl.obj
del test.obj
