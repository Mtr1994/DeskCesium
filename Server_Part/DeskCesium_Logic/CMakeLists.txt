cmake_minimum_required(VERSION 3.24)

PROJECT(DeskCesium_Logic)

SET(CMAKE_BUILD_TYPE "Release")
set(ROOT_DIR "/home/mtr1994/Project/PSS_ASIO-3.0.0")

if(UNIX)
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
	set(LIB_OUTPUT_DIR ${ROOT_DIR}/Build/Linux/)
else()
	set(BIN_OUTPUT_DIR ${ROOT_DIR}/Build/Windows/)
	set(LIB_OUTPUT_DIR ${ROOT_DIR}/Build/Windows/)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${BIN_OUTPUT_DIR}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${BIN_OUTPUT_DIR}")
endif()

file(GLOB MAIN_FILES *.cpp ./Proto/*.cc ./MySQL/*.cpp)
source_group("main_file" FILES ${MAIN_FILES})

include_directories(
	${PROJECT_SOURCE_DIR}/../../Common
	${PROJECT_SOURCE_DIR}/../../third_include
	${PROJECT_SOURCE_DIR}/../../third_include/protobuf
	${PROJECT_SOURCE_DIR}/../../Module_Logic/DeskCesium_Logic/Proto
	${PROJECT_SOURCE_DIR}/../../Module_Logic/DeskCesium_Logic/MySQL
	/usr/include/mysql)

ADD_LIBRARY(${PROJECT_NAME} SHARED 
	${MAIN_FILES}
	)

#ifwindows	
#__DEBUG;_USRDLL;DESKCESIUM_LOGIC_EXPORTS;_WINDOWS;%(PreprocessorDefinitions)
if(WIN32)
	message("OS is windows...")
	if(CMAKE_BUILD_TYPE MATCHES "debug")
		add_definitions(
		-D_DEBUG
		-D_USRDLL
		-DTEST_LOGIC_EXPORTS
		-D_WINDOWS)
	else()
		add_definitions(
		-DNDEBUG
		-D_USRDLL
		-DTEST_LOGIC_EXPORTS
		-D_WINDOWS)		
	endif()
endif()

if(gcov_check STREQUAL "1")
	message("[DeskCesium_Logic]set GCOV_TEST...")
	add_definitions(-DGCOV_TEST)
endif()

if(UNIX)
	target_link_libraries(${PROJECT_NAME} -lpthread -lmysqlclient)
	target_link_libraries(${PROJECT_NAME} /home/mtr1994/Project/protobuf-3.21.9/build/lib/libprotobuf.so)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES 
	OUTPUT_NAME_DEBUG ${PROJECT_NAME}
	ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
