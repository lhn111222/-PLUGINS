option( PLUGIN_STANDARD_QVOLUME_MEASURE "Install qVolumeMeasure plugin" ON )

if ( PLUGIN_STANDARD_QVOLUME_MEASURE )
	project( qVolumeMeasure )
	
	find_package(Qt5 COMPONENTS AxContainer Widgets REQUIRED)  # 加 Widgets
	AddPlugin( NAME ${PROJECT_NAME} )
	
	
	 # 启用自动 MOC/UIC/RCC（关键修复）
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
	
	file(GLOB_RECURSE SOURCES "src/*.cpp")
	# 新增：收集所有包含Q_OBJECT的头文件（必须添加，否则MOC无法处理）
	file(GLOB_RECURSE HEADERS "include/*.h")
	
	# 手动指定 .ui 文件
	set(UI_FILES 
		${CMAKE_CURRENT_SOURCE_DIR}/ui/qVolumeMeasure.ui
	)
	
	# 手动生成 UI 头文件
	qt5_wrap_ui(UI_HEADERS ${UI_FILES})
	
	target_sources(${PROJECT_NAME} PRIVATE ${SOURCES} ${HEADERS} ${UI_FILES}  ${UI_HEADERS})
	
	target_include_directories(${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include  
		${CMAKE_CURRENT_BINARY_DIR}  # 生成的 ui_xxx.h 目录
	)
	
	target_link_libraries( ${PROJECT_NAME} Qt5::AxContainer Qt5::Widgets )  # 链接 Widgets
endif()