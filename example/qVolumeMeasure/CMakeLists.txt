option( PLUGIN_STANDARD_QVOLUME_MEASURE "Install qVolumeMeasure plugin" ON )

if ( PLUGIN_STANDARD_QVOLUME_MEASURE )
	project( qVolumeMeasure )
	message(STATUS " 32555 PLUGIN CMAKE: ExampleIOPlugin 321")
	find_package(Qt5 COMPONENTS AxContainer Widgets REQUIRED)  # 加 Widgets
	AddPlugin( NAME ${PROJECT_NAME} )
	
	
	 # 启用自动 MOC/UIC/RCC（关键修复）
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)  # 自动处理 .qrc 资源文件
	
	file(GLOB_RECURSE SOURCES "src/*.cpp")
	# 新增：收集所有包含Q_OBJECT的头文件（必须添加，否则MOC无法处理）
	file(GLOB_RECURSE HEADERS "include/*.h")
	
	# 手动指定 .ui 文件
	set(UI_FILES 
		${CMAKE_CURRENT_SOURCE_DIR}/ui/qVolumeMeasure.ui
	)
	
	# 关键新增：收集 .qrc 资源文件（包含 info.json 和图标）
	file(GLOB QRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc")  # 假设 .qrc 在插件根目录
	
	# 手动生成 UI 头文件
	qt5_wrap_ui(UI_HEADERS ${UI_FILES})
	
	# 关键修改：将 .qrc 文件添加到源文件列表，确保 AUTORCC 处理
	target_sources(${PROJECT_NAME} PRIVATE 
		${SOURCES} 
		${HEADERS} 
		${UI_FILES}  
		${UI_HEADERS}
		${QRC_FILES}  # 添加 .qrc 资源
	)
	
	target_include_directories(${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include  
		${CMAKE_CURRENT_BINARY_DIR}  # 生成的 ui_xxx.h 目录
	)
	
	target_link_libraries( ${PROJECT_NAME} Qt5::AxContainer Qt5::Widgets )  # 链接 Widgets
	
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # Windows .dll
		LIBRARY DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # Linux/macOS .so/.dylib
		ARCHIVE DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard"  # 静态库（可选）
	)
	
	# 安装info.json到插件专属目录（需包含"type": "Standard"）
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/info.json"
		DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard/${PROJECT_NAME}"
	)
	
	# 若有额外资源（如图标文件夹），需同步安装（.qrc嵌入的资源可省略）
	# install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/icons/"
	# 	DESTINATION "${CLOUDCOMPARE_DEST_FOLDER}/plugins/standard/${PROJECT_NAME}/icons"
	# )
endif()
    