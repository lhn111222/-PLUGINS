# 最低 CMake 版本（与 CloudCompare 主项目一致）
cmake_minimum_required(VERSION 3.10)
project(MyCCPlugins)

# ========================
# 关键修复：允许跨目录链接库（解决 "CMP0079" 错误）
# ========================
cmake_policy(SET CMP0079 NEW)  # 新增这行，解除跨目录链接限制


# ========================
# 1. 复用主项目的 CCCoreLib 和 Qt 配置（原逻辑保留）
# ========================
if(NOT TARGET CCCoreLib)
    find_package(CCCoreLib REQUIRED 
        PATHS "${CMAKE_BINARY_DIR}/libs/qCC_db/extern/CCCoreLib" 
        NO_DEFAULT_PATH
    )
endif()

find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)


# ========================
# 2. 插件列表（原逻辑保留）
# ========================
set(MY_PLUGINS 
    qUDPTransfer 
    qVolumeMeasure 
    qXYZILoader
)


# ========================
# 3. 插件编译配置（原逻辑保留，确保语法统一）
# ========================
foreach(PLUGIN IN LISTS MY_PLUGINS)
    add_subdirectory(${PLUGIN})
    
    set_target_properties(${PLUGIN} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"  # Windows DLL
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"  # Linux/Mac 库
    )
    
    
    target_link_libraries(${PLUGIN} 
        CCCoreLib          # CloudCompare 核心库
        Qt5::Core          # Qt 核心
        Qt5::Widgets       # Qt 界面
        Qt5::Network       # qUDPTransfer 需网络功能
    )
endforeach()