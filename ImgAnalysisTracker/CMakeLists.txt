set(lib_name "ImgAnalysisTracker")

add_subdirectory(pipeline)

file(GLOB src_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file(GLOB hdr_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h *.hpp)
file(GLOB ui_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.ui)
file(GLOB src_legacy RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} legacy/*.cpp)
file(GLOB hdr_legacy RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} legacy/*.h *.hpp)

qt5_wrap_ui(UI_HEADERS_LOCAL ${ui_main})

add_definitions(-DUSE_DEEPLOCALIZER)

include_directories(${biotracker-includes})

add_library(${lib_name} STATIC
    ${src_main} ${hdr_main} ${src_legacy} ${hdr_legacy} ${UI_HEADERS_LOCAL}
)
link_directories(${DEEPLOCALIZER_CLASSIFIER_LIBRARY_DIR})

get_target_property(PIPELINE_INCLUDES Pipeline PIPELINE_INCLUDES)

include_directories(${PIPELINE_INCLUDES})

target_link_libraries(${lib_name}
    "deeplocalizer-classifier"
    Pipeline
    Qt5::Widgets
    Qt5::OpenGL
    ${OPENGL_LIBRARIES}
    ${OpenCV_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${Caffe_LIBRARIES}
    ${LMDB_LIBRARIES}
    ${CUDA_LIBRARIES}
)

list(APPEND ALGORITHM_LIBRARIES ${lib_name})
set(ALGORITHM_LIBRARIES "${ALGORITHM_LIBRARIES}" CACHE INTERNAL "")

list(APPEND ALGORITHM_LINK_DIRECTORIES ${DEEPLOCALIZER_CLASSIFIER_LIBRARY_DIR})
set(ALGORITHM_LINK_DIRECTORIES "${ALGORITHM_LINK_DIRECTORIES}" CACHE INTERNAL "")
