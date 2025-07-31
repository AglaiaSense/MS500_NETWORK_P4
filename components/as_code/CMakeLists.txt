# components/as_core/CMakeLists.txt
# ================================================================
# 开发 → 准备分发 → 分发
# (OFF, OFF) → (OFF, ON) → (ON, ON)

set(AS_CORE_LIB_USE OFF)      # OFF = 源码模式, ON = 库模式
set(AS_CORE_CLEAR OFF)       # OFF = 保留源码, ON = 清除源码

# ================================================================
message(STATUS "as_core: -----------------------------------")

message(STATUS "as_core: LIB_USE = ${AS_CORE_LIB_USE}, CLEAR = ${AS_CORE_CLEAR}")

# 源文件列表（始终定义）
# 使用GLOB_RECURSE自动收集所有源文件
file(GLOB_RECURSE srcs 
    "src/*.c"
    "src/*.cpp"
)

list(APPEND EXTRA_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}/../as_common_deps/opencv"
    "${CMAKE_CURRENT_LIST_DIR}/../as_common_deps/opencv/opencv2"
    "${CMAKE_CURRENT_LIST_DIR}/../as_common_deps/flat_buffers"
)

set(include_dirs "include" ${EXTRA_INCLUDES} )
set(priv_include_dirs "private")
set(requires "esp_common as_common_deps")

# 选择使用模式
if(AS_CORE_LIB_USE)
    message(STATUS "as_core: Using precompiled library mode")
    
    # 确保库文件存在
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/lib/as_core_lib.a")
        message(STATUS "Precompiled library not found, falling back to source mode")
        set(use_lib_mode FALSE)
    else()
        set(use_lib_mode TRUE)
    endif()
else()
    message(STATUS "as_core: Using source code mode")
    set(use_lib_mode FALSE)
endif()

# 注册组件
if(use_lib_mode)
    
    idf_component_register(
        SRCS ""
        INCLUDE_DIRS ${include_dirs} 
        REQUIRES ${requires}
    )
    target_link_libraries(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/lib/as_core_lib.a")
else()
    idf_component_register(
        SRCS ${srcs}
        INCLUDE_DIRS ${include_dirs}
        PRIV_INCLUDE_DIRS ${priv_include_dirs}
        REQUIRES ${requires}
    )
    
    # 在源码模式下，添加清除源码的功能
    if(AS_CORE_CLEAR)
        message(STATUS "as_core: Will clear source files after build")
        
        # 添加后构建步骤：生成库后清除源码
        add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_LIST_DIR}/src"
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_LIST_DIR}/private"
            COMMAND ${CMAKE_COMMAND} -E echo "Source directories cleared"
            COMMENT "Clearing source directories for distribution"
        )
        
        # 添加后构建步骤：自动修改配置为库模式
        add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Please set AS_CORE_LIB_USE to ON for future builds"
        )
    endif()


endif()


    # 添加库文件生成目标（仅在OFF, OFF模式下）
if(NOT AS_CORE_LIB_USE)
    # 编译目标对象文件列表
    add_library(as_core_objects OBJECT ${srcs})
    target_include_directories(as_core_objects PRIVATE ${include_dirs} ${priv_include_dirs})
    target_link_libraries(as_core_objects PRIVATE ${requires})

    # 确保lib目录存在
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib)

    # 修复：添加COMMAND_EXPAND_LISTS处理对象文件列表
    add_custom_command(OUTPUT ${CMAKE_CURRENT_LIST_DIR}/lib/as_core_lib.a
        COMMAND ${CMAKE_AR} rcs ${CMAKE_CURRENT_LIST_DIR}/lib/as_core_lib.a $<TARGET_OBJECTS:as_core_objects>
        COMMAND_EXPAND_LISTS  # 展开列表为单独参数
        DEPENDS as_core_objects
        COMMENT "Archiving as_core_lib.a"
    )

    add_custom_target(as_core_lib_archive ALL
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/lib/as_core_lib.a
    )
endif()