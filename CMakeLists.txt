cmake_minimum_required(VERSION 3.0.2)
project(intergif)

add_library(anim STATIC
	src/animlib.c
	src/animlib.h
	src/colmatch.c
	src/commonpal.c
	src/count.c
	src/count.h
	src/frame.c
	src/frame.h
	src/from.c
	src/fromfile.c
	src/gifdecode.c
	src/gifdecode.h
	src/gifencode.c
	src/gifencode.h
	src/histogram.c
	src/intergif.c
	src/intergif.h
	src/join.c
	src/loadgif.c
	src/loadsprite.c
	src/loadtca.c
	src/mediancut.c
	src/palettemap.c
	src/riemann.c
	src/riemann.h
	src/savegif.c
	src/savesprite.c
	src/split.c
	src/split.h
	src/sprite.h
	src/tca.h
	src/utils.h
	src/version.h
	src/win32.c
	src/workspace.c
	src/workspace.h
)

add_executable(intergif src/main.c)
target_link_libraries(intergif PRIVATE anim)

if(WIN32)
	add_executable(igviewer WIN32
		viewersrc/winview.rc
		viewersrc/winview.c
		viewersrc/resource.h
	)
	target_link_libraries(igviewer PRIVATE anim)
endif(WIN32)
