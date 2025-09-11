option( PLUGIN_EXAMPLE_GL "Install example GL plugin" ON )

if( PLUGIN_EXAMPLE_GL )

    project( ExampleGLPlugin )
    message(STATUS " 32222 PLUGIN CMAKE: ExampleIOPlugin 321") 
    AddPlugin(
        NAME ${PROJECT_NAME}
        TYPE gl
    )
        
    add_subdirectory( include )
    add_subdirectory( src )
    add_subdirectory( shaders )
	
	 install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/gl"  # Windows .dll
        LIBRARY DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/gl"  # Linux/macOS .so/.dylib
        ARCHIVE DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/gl"  # 静态库（可选）
    )

    # 安装资源文件（info.json、shaders 等）
    # 1. 安装 info.json 到插件目录
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/info.json"
        DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/gl/${PROJECT_NAME}"
    )

    # 2. 安装着色器文件到插件的 shaders 子目录
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
        DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/gl/${PROJECT_NAME}/shaders"
        FILES_MATCHING PATTERN "*.vert" PATTERN "*.frag"  # 只安装着色器文件
    )
endif()
