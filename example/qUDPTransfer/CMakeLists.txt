option( PLUGIN_STANDARD_QUDP_TRANSFER "Install qUDPTransfer plugin" ON )

if ( PLUGIN_STANDARD_QUDP_TRANSFER )
    project( qUDPTransfer )
	 message(STATUS " 32444 PLUGIN CMAKE: ExampleIOPlugin 321")
	find_package(Qt5 COMPONENTS Network Widgets REQUIRED)  # 需要Widgets用于UI界面
	AddPlugin( NAME ${PROJECT_NAME} )
	
	
	# 启用自动 MOC/UIC/RCC（关键修复）
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)  # 自动处理 .qrc 资源文件
	
	# 源文件
	file(GLOB_RECURSE SOURCES "src/*.cpp")
	
	file(GLOB_RECURSE HEADERS "include/*.h")
	# 明确指定.ui文件路径，确保CMake能找到它们
	set(UI_FILES 
		${CMAKE_CURRENT_SOURCE_DIR}/ui/qUDPTransfer.ui
	)
	
	# 关键新增：收集 .qrc 资源文件（包含 info.json 和图标）
	file(GLOB QRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")  # 假设 .qrc 在插件根目录（如 qUDPTransfer.qrc）
	
	# 关键步骤：通过uic工具生成ui_xxx.h，并将其目录加入include路径
	qt5_wrap_ui(UI_HEADERS ${UI_FILES})  # 生成的文件会在build目录下，由UI_HEADERS记录路径
	
	# 汇总所有文件：源文件 + 生成的UI头文件 + .qrc资源
	target_sources(${PROJECT_NAME} PRIVATE 
		${SOURCES} 
		${HEADERS} 
		${UI_FILES} 
		${UI_HEADERS}
		${QRC_FILES}  # 添加 .qrc 资源文件，确保被编译
	)
	
	# 头文件路径：源文件include目录 + 生成的UI头文件目录
	target_include_directories(${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include  
		${CMAKE_CURRENT_BINARY_DIR}  # 生成的ui_xxx.h在这里
	)
	
	target_link_libraries( ${PROJECT_NAME} Qt5::Network Qt5::Widgets )  # 链接Widgets库
	
	# 关键修复：添加安装路径配置（Standard类型对应plugins/standard）
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # Windows .dll
        LIBRARY DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # Linux/macOS .so/.dylib
        ARCHIVE DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # 静态库（可选）
    )
	
    # 安装info.json到插件专属目录（必须包含"type": "Standard"）
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/info.json"
        DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard/${PROJECT_NAME}"
    )
	
    # 若.qrc中包含图标等资源，需确保安装到插件目录（若资源通过.qrc嵌入，此步可省略）
    # install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/"
    #     DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard/${PROJECT_NAME}/resources"
    # )
endif()
    
