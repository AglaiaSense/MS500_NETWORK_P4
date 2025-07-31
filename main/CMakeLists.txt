#  main/CMakeLists.txt
file(GLOB_RECURSE SOURCES ./*.cpp ./*.c
                            )

idf_component_register(SRCS ${SOURCES}
                     INCLUDE_DIRS "" ./bsp  ./bsp_video  ./ai_drivers  ./ai_process  ./bsp_network
                )
  
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")


if(NOT CONFIG_SOC_SDMMC_HOST_SUPPORTED)
    fail_at_build_time(sdmmc ""
                             "SDMMC host is not supported."
                             "Please refer README.md for more details")
endif()


# 主存储分区
spiffs_create_partition_image(storage ${CMAKE_CURRENT_SOURCE_DIR}/../spiffs_data FLASH_IN_PROJECT)

# 下载专用分区 
# fatfs_create_spiflash_image(storage_dl ${CMAKE_CURRENT_SOURCE_DIR}/../spiffs_download FLASH_IN_PROJECT )

target_link_libraries(${COMPONENT_LIB} INTERFACE "-u decode_dnn_output")


target_link_libraries(${COMPONENT_LIB} INTERFACE
    "${CMAKE_SOURCE_DIR}/components/as_common_deps/opencv/libopencv_core.a"
    "${CMAKE_SOURCE_DIR}/components/as_common_deps/opencv/libopencv_imgproc.a"
    "${CMAKE_SOURCE_DIR}/components/as_common_deps/opencv/libopencv_imgcodecs.a"
    "${CMAKE_SOURCE_DIR}/components/as_common_deps/opencv/3rdparty/liblibpng.a"
    "${CMAKE_SOURCE_DIR}/components/as_common_deps/opencv/3rdparty/libzlib.a"
)
