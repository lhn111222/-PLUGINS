option( PLUGIN_EXAMPLE_IO "Install example I/O plugin" ON )

if( PLUGIN_EXAMPLE_IO )
	project( ExampleIOPlugin )
	
	message(STATUS " 32111 PLUGIN CMAKE: ExampleIOPlugin 321") 
    AddPlugin( NAME ${PROJECT_NAME} TYPE io )
    
    add_subdirectory( include )
    add_subdirectory( src )
	
	install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io"  # Windows .dll
        LIBRARY DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io"  # Linux/macOS .so/.dylib
        ARCHIVE DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io"  # 静态库（可选）
    )

    # 安装 info.json 到插件目录（必须与库文件同目录）
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/info.json"
        DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io/${PROJECT_NAME}"
    )
endif()
