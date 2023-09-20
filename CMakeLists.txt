cmake_minimum_required(VERSION 3.1)

project(Parser VERSION 1 LANGUAGES CXX)

set(CMAKE_C_COMPILER "g++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${Parser_HOME}/cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb -O3 -DIL_STD -ldl")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -g -ggdb -O3 -DIL_STD -ldl")

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON) # For CImg

# Home Directory
set(Parser_HOME ${PROJECT_SOURCE_DIR})
set(CIMG_HOME ${PROJECT_SOURCE_DIR}/extern/CImg)

# For CImg
find_package(X11 REQUIRED)
find_package(JPEG REQUIRED)
find_package(Threads REQUIRED)

# Source Code
set(Parser_SRC
	src/main.cpp
	src/LefDefParser.cpp
	src/CmdInterpreter.cpp
	src/Painter.cpp
)

# Include Directory

include_directories(
  ${X11_INCLUDE_DIR}
  ${CIMG_HOME}
)

# Executable
add_executable(Parser ${Parser_SRC})

# Link Library
target_link_libraries(${PROJECT_NAME} 
	PUBLIC
 	${X11_LIBRARIES}
	Threads::Threads
)
