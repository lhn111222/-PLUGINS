option( PLUGIN_STANDARD_QUDP_TRANSFER "Install qUDPTransfer plugin" ON )

if ( PLUGIN_STANDARD_QUDP_TRANSFER )
    project( qUDPTransfer )
	 
	find_package(Qt5 COMPONENTS Network Widgets REQUIRED)  # 需要Widgets用于UI界面
	AddPlugin( NAME ${PROJECT_NAME} )
	
	
	# 启用自动 MOC/UIC/RCC（关键修复）
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
	
	# 源文件
	file(GLOB_RECURSE SOURCES "src/*.cpp")
	
	file(GLOB_RECURSE HEADERS "include/*.h")
	# 明确指定.ui文件路径，确保CMake能找到它们
	set(UI_FILES 
		${CMAKE_CURRENT_SOURCE_DIR}/ui/qUDPTransfer.ui
	)
	
	# 关键步骤：通过uic工具生成ui_xxx.h，并将其目录加入include路径
	qt5_wrap_ui(UI_HEADERS ${UI_FILES})  # 生成的文件会在build目录下，由UI_HEADERS记录路径
	
	# 汇总所有文件：源文件 + 生成的UI头文件
	target_sources(${PROJECT_NAME} PRIVATE ${SOURCES} ${HEADERS} ${UI_FILES} ${UI_HEADERS})
	
	# 头文件路径：源文件include目录 + 生成的UI头文件目录
	target_include_directories(${PROJECT_NAME} PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include  
		${CMAKE_CURRENT_BINARY_DIR}  # 生成的ui_xxx.h在这里
	)
	
	target_link_libraries( ${PROJECT_NAME} Qt5::Network Qt5::Widgets )  # 链接Widgets库
endif()