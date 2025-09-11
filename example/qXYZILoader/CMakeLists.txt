option( PLUGIN_IO_XYZI "Install xyzi I/O plugin" ON )

if( PLUGIN_IO_XYZI )
    project( qXYZILoader )
message(STATUS " 32666 PLUGIN CMAKE: ExampleIOPlugin 321")
    # 启用自动 MOC/UIC/RCC
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
    
    AddPlugin( NAME ${PROJECT_NAME} TYPE io )
    
    # 查找源文件和资源文件
    file(GLOB_RECURSE SOURCES "src/*.cpp")  
    file(GLOB_RECURSE QRCS "*.qrc")         # 查找所有.qrc文件
	
	 # ：显式收集包含 Q_OBJECT 的头文件 
    file(GLOB_RECURSE HEADERS "include/*.h") 
    
    target_sources(${PROJECT_NAME} PRIVATE ${SOURCES} ${QRCS}  ${HEADERS} )  
    
    # 指定头文件目录
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include  
    )

    
    #链接 Qt 核心库（解决元对象链接错误）
    
    target_link_libraries(${PROJECT_NAME} 
        Qt5::Core        # 必须：Qt 核心库，提供元对象系统支持
        Qt5::Widgets     # 必须：Qt 界面库，插件 UI 依赖
    )
	
	install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io"  # Windows .dll
        LIBRARY DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io"  # Linux/macOS .so/.dylib
        ARCHIVE DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io"  # 静态库（可选）
    )
    
    # 安装info.json到插件专属目录（需包含"type": "I/O"）
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/info.json"
        DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io/${PROJECT_NAME}"
    )
    
    # 若.qrc中包含独立资源文件（如图标），需同步安装（.qrc嵌入的资源可省略）
    # install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/icons/"
    #     DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/io/${PROJECT_NAME}/icons"
    # )

endif()