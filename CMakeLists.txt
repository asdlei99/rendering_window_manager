cmake_minimum_required(VERSION 3.5)

set(EXE_NAME rendering_window)

add_subdirectory(display_manager)

add_executable(${EXE_NAME} WIN32
main.cpp
)


add_definitions(/ZI)

# Add some compile flags to our source files.
if(MSVC)
	#set_source_files_properties(${SOURCE_FILES}
	#	PROPERTIES COMPILE_FLAGS "/W3 /WX")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	
	target_compile_definitions(${EXE_NAME} PUBLIC
	NOMINMAX
)
else()
	set_source_files_properties(${SOURCE_FILES}
		PROPERTIES COMPILE_FLAGS -Wall -Wextra -Wpedantic)
endif()

target_include_directories(${EXE_NAME} PUBLIC
#	"${PROJECT_SOURCE_DIR}/deps/libsdptransform/include"
	"${PROJECT_SOURCE_DIR}/lib/json_parser/"
#	"${PROJECT_SOURCE_DIR}/lib/media_render_util/"
#	"${LIBWEBRTC_INCLUDE_PATH}"
	"${LIBWEBRTC_INCLUDE_PATH}/third_party/abseil-cpp"
)


target_link_directories( ${EXE_NAME} PUBLIC
	$<BUILD_INTERFACE:${BOOST_LIB}/stage/lib>
)



# Public (interface) dependencies.
target_link_libraries(${EXE_NAME} PUBLIC
	display_manager
	websocket_server
	func_thread_handler
	json_parser
	#media_render_util
#	sdptransform
#[==[
	${LIBWEBRTC_BINARY_PATH}/webrtc${CMAKE_STATIC_LIBRARY_SUFFIX}
	${PROJECT_SOURCE_DIR}/lib/mediasoup/Debug/mediasoupclient.lib
	${PROJECT_SOURCE_DIR}/lib/mediasoup/Debug/sdptransform.lib
	signaller
	json_parser
	winmm
	dmoguids
	wmcodecdspuuid
	msdmo
	secur32
	mediasoup_conference
	func_thread_handler
	websocket_server
	video_receiver
	#]==]
#amstrmid.lib
#crypt32.lib
#iphlpapi.lib
#media_render_util
#video_render
#	d3d9
)


