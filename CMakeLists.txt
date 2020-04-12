cmake_minimum_required(VERSION 3.0.2)
project(intergif)

include(GNUInstallDirs)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
elseif(WATCOM)
	add_compile_options(-oanxh -wx)
else()
	add_compile_options(-O6 --unsigned-char -Wall -Wextra -pedantic -Wno-unused)
endif()

option(ENABLE_PNG "Build with PNG support" ON)
if(ENABLE_PNG)
	find_package(PNG)
	if(PNG_FOUND)
		add_definitions(-DENABLE_PNG)
	else(PNG_FOUND)
		set(ENABLE_PNG OFF)
	endif(PNG_FOUND)
endif(ENABLE_PNG)

add_library(anim STATIC
	src/animlib.c
	src/animlib.h
	src/antialias.h
	src/cfsi.c
	src/cfsi.h
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
	src/loaddraw.c
	src/loadgif.c
	src/loadpng.c
	src/loadsprite.c
	src/loadtca.c
	src/mediancut.c
	src/modes.c
	src/palettemap.c
	src/riemann.c
	src/riemann.h
	src/savegif.c
	src/savesprite.c
	src/split.c
	src/sprite.h
	src/tca.h
	src/utils.c
	src/utils.h
	src/version.h
	src/workspace.c
	src/workspace.h
)
target_include_directories(anim INTERFACE src)
if(ENABLE_PNG)
	target_include_directories(anim PRIVATE ${PNG_INCLUDE_DIRS})
	target_link_libraries(anim PUBLIC ${PNG_LIBRARIES})
endif(ENABLE_PNG)

add_executable(intergif src/main.c)
target_link_libraries(intergif PRIVATE anim)
install(TARGETS intergif DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WIN32)
	add_executable(igviewer WIN32
		viewersrc/winview.rc
		viewersrc/winview.c
		viewersrc/resource.h
	)
	target_link_libraries(igviewer PRIVATE anim)
	install(TARGETS igviewer DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(WIN32)

add_executable(igexample examples/igexample.c)
target_link_libraries(igexample PRIVATE anim)
