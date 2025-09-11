# CloudCompare example for standard plugins

# REPLACE ALL 'ExamplePlugin' OCCURENCES BY YOUR PLUGIN NAME
# AND ADAPT THE CODE BELOW TO YOUR OWN NEEDS!

# Add an option to CMake to control whether we build this plugin or not
option( PLUGIN_EXAMPLE_STANDARD "Install example plugin" ON )

if ( PLUGIN_EXAMPLE_STANDARD )
	project( ExamplePlugin )
	 message(STATUS " 32333 PLUGIN CMAKE: ExampleIOPlugin 321") 
	AddPlugin( NAME ${PROJECT_NAME} )
		
	add_subdirectory( include )
	add_subdirectory( src )
	
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # Windows .dll
		LIBRARY DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # Linux/macOS .so/.dylib
		ARCHIVE DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # 静态库（可选）
	)
	
	# 安装 info.json 到插件专属目录（必须包含 "type": "Standard"）
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/info.json"
		DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard/${PROJECT_NAME}"
	)
	
	# 若有其他资源文件（如图标、配置文件），也需安装到对应目录
	# install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
	# 	DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard/${PROJECT_NAME}/resources"
	# )
	
	# set dependencies to necessary libraries
	# target_link_libraries( ${PROJECT_NAME} LIB1 )
	
	# set dependencies to necessary libraries
	# target_link_libraries( ${PROJECT_NAME} LIB1 )
endif()
