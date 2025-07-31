# components/as_common_deps/CMakeLists.txt

message(STATUS "as_common_deps: -----------------------------------")

  # 首先注册组件，不设置任何源文件或依赖
# idf_component_register()

  # 首先注册组件，不设置任何源文件或依赖
# idf_component_register(
#     INCLUDE_DIRS 
#         "opencv"          # 添加 OpenCV 头文件路径
#         "flat_buffers"    # 添加 FlatBuffers 头文件路径
# )
idf_component_register(
    SRCS ""                                      # 本组件无源文件
    INCLUDE_DIRS 
        "${CMAKE_CURRENT_LIST_DIR}/opencv"       # 暴露 opencv 目录下的头
        "${CMAKE_CURRENT_LIST_DIR}/flat_buffers" # 暴露 flat_buffers 目录下的头
)

# 添加预编译库
add_library(opencv_imgcodecs STATIC IMPORTED)
set_target_properties(opencv_imgcodecs PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/opencv/libopencv_imgcodecs.a"
)

add_library(libpng STATIC IMPORTED)
set_target_properties(libpng PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/opencv/3rdparty/liblibpng.a"
)

add_library(libzlib STATIC IMPORTED)
set_target_properties(libzlib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/opencv/3rdparty/libzlib.a"
)

add_library(opencv_imgproc STATIC IMPORTED)
set_target_properties(opencv_imgproc PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/opencv/libopencv_imgproc.a"
)

add_library(opencv_core STATIC IMPORTED)
set_target_properties(opencv_core PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/opencv/libopencv_core.a"
)

# 设置依赖关系
target_link_libraries(opencv_imgcodecs INTERFACE libpng libzlib)
target_link_libraries(opencv_core INTERFACE libzlib)

# 将预编译库添加到组件的链接目标中
target_link_libraries(${COMPONENT_LIB} INTERFACE 
    opencv_imgcodecs 
    opencv_imgproc 
    opencv_core
)

# 添加头文件搜索路径
# target_include_directories(${COMPONENT_LIB} INTERFACE
#     ${CMAKE_CURRENT_LIST_DIR}/opencv
#      ${CMAKE_CURRENT_LIST_DIR}/opencv/opencv2  # 添加 opencv2 子目录
#     ${CMAKE_CURRENT_LIST_DIR}/flat_buffers
# )

target_include_directories(${COMPONENT_LIB} INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/opencv
     ${CMAKE_CURRENT_LIST_DIR}/opencv/opencv2  # 添加 opencv2 子目录
    ${CMAKE_CURRENT_LIST_DIR}/flat_buffers
)