cmake_minimum_required(VERSION 3.12)

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif(APPLE)

project("BetterMouseYoke")

if(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_C_STANDARD 17)

FILE(GLOB SRC 
	${CMAKE_SOURCE_DIR}/BetterMouseYoke/detours.h
	${CMAKE_SOURCE_DIR}/BetterMouseYoke/plugin.h	
	${CMAKE_SOURCE_DIR}/BetterMouseYoke/plugin.c
	${CMAKE_SOURCE_DIR}/Util/*.c
	${CMAKE_SOURCE_DIR}/Util/*.h
	)

LIST(SORT SRC)

if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC})

if(WIN32)
	set(PLAT_SHORT "win64")
	set(PLAT_LONG "win-64")
	set(PLAT_ONLY "win")
	set(PLUGIN_BIN_OUTDIR "win_x64")
elseif(APPLE)
	set(PLAT_SHORT "mac64")
	set(PLAT_LONG "mac-64")
	set(PLAT_ONLY "mac")
else()
	set(PLAT_SHORT "lin64")
	set(PLAT_LONG "linux-64")
	set(PLAT_ONLY "lin")
endif()

set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

if(MSVC)
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D DEBUG /Gy- /Od /Z7 /D _USRDLL /LDd /MDd /MTd /D __MSVC__")
else()
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
endif()

set(PLUGIN_VERSION "1.7")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLUGIN_VERSION=\"\"${PLUGIN_VERSION}\"\"")

set(XPSDK "${CMAKE_SOURCE_DIR}/SDK") 

add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1 -DXPLM303=1 -DXPLM400=1)


if(WIN32) 
	add_definitions(-DIBM=1 -DAPL=0 -DLIN=0 -D_X86_)
elseif(APPLE)
	add_definitions(-DIBM=0 -DAPL=1 -DLIN=0 -DGL_SILENCE_DEPRECATION)
else()
	add_definitions(-DIBM=0 -DAPL=0 -DLIN=1)
endif()

if(NOT MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -fvisibility=hidden -fpermissive")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c++17 /Gy- /LD /MD /MT /D __MSVC__  /D _AMD64_")
endif()


include_directories(
    "${XPSDK}/CHeaders/XPLM"
    "${XPSDK}/CHeaders/Widgets"
    "${XPSDK}/CHeaders/Wrappers"
	"${CMAKE_SOURCE_DIR}/Util"
)

# linking

if(WIN32)
	find_library(XPLM_LIBRARY XPLM_64 "${XPSDK}/Libraries/Win")
elseif(APPLE)
	find_library(XPLM_LIBRARY XPLM "${XPSDK}/Libraries/Mac")
else()
	set(XPLM_LIBRARY "")
endif()

target_link_libraries(${CMAKE_PROJECT_NAME}
		${XPLM_LIBRARY}
		"${CMAKE_SOURCE_DIR}/BetterMouseYoke/detours.lib"
    )

if(UNIX AND NOT APPLE)
	target_link_libraries(${CMAKE_PROJECT_NAME} X11 Xi Xcursor Xfixes)
endif()

if(UNIX AND NOT APPLE)
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS
		"${CMAKE_SHARED_LINKER_FLAGS} -m64 -static-libgcc -static-libstdc++ -shared")
elseif(APPLE)
	#set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS
	#	"${CMAKE_SHARED_LINKER_FLAGS} ")
elseif(WIN32 AND NOT MSVC)
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS
		"${CMAKE_SHARED_LINKER_FLAGS} -m64 -static-libgcc -shared")	
elseif(MSVC)
	set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS
			"${CMAKE_SHARED_LINKER_FLAGS} /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO")	
else()

endif()
# /WHOLEARCHIVE:OPENGL32 /WHOLEARCHIVE:GLFW3 /NODEFAULTLIB:MSVCRT

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX "")
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
	"${CMAKE_SOURCE_DIR}/Deploy/${CMAKE_PROJECT_NAME}/64/$<0:>" )
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
	"${CMAKE_SOURCE_DIR}/Deploy/${CMAKE_PROJECT_NAME}/64/$<0:>" )
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PLAT_ONLY}.xpl")