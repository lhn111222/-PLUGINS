option( PLUGIN_IO_XYZI "Install xyzi I/O plugin" ON )

if( PLUGIN_IO_XYZI )
    project( qXYZILoader )

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

endif()