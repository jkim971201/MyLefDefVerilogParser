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
set(CXXOPTS_HOME ${PROJECT_SOURCE_DIR}/extern/cxxopts)

set(LEFLIB_HOME ${PROJECT_SOURCE_DIR}/extern/lef)
set(DEFLIB_HOME ${PROJECT_SOURCE_DIR}/extern/def)
set(FFTLIB_HOME ${PROJECT_SOURCE_DIR}/extern/fft)

set(EIGEN_HOME ${PROJECT_SOURCE_DIR}/extern/eigen-git-mirror)

set(GOOGLEHASH_HOME "/usr/local/include/sprasehash")

# Package Settings
# 1. For Boost and Google Hash Map
find_package(Boost REQUIRED)

# 2. For CImg
find_package(X11 REQUIRED)
find_package(JPEG REQUIRED)
find_package(Threads REQUIRED)

## 3. For Eigen
#find_package(Eigen3 REQUIRED)
#if(Eigen3_FOUND)
#	message(STATUS "Eigen3 Found")
#	message(STATUS "${Eigen3_DIR}")
#	message(STATUS "${Eigen3_INCLUDE_DIR}")
#else()
#	message(STATUS "Eigen3 Not Found")
#endif()

# Source Code
set(Parser_SRC
	src/main.cpp
	src/LefDefParser.cpp
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

# Enable CUDA
option(GPU "Enable GPU" ON)
if (GPU)
	message(STATUS "GPU Mode ON")
	find_package(CUDAToolkit)
	if (CUDAToolkit_FOUND)
		message(STATUS "CUDA Found")
		enable_language(CUDA)

		find_package(Thrust)

		set_target_properties(${PROJECT_NAME} 
				                  PROPERTIES CUDA_ARCHITECTURES "75")

		target_sources(${PROJECT_NAME} 
				           PRIVATE
									 )

		target_link_libraries(${PROJECT_NAME} 
				                  PRIVATE
													CUDA::cudart
													CUDA::cublas
													CUDA::cusparse
													CUDA::cufft)

		target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_GPU)

	else()
		message(STATUS "CUDA Not Found")
	endif()
else()
	message(STATUS "GPU Mode Off")
endif()
