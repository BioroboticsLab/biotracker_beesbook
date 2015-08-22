set(lib_name "TagMatcher")

file(GLOB src_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file(GLOB hdr_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h *.hpp)
file(GLOB ui_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.ui)

qt5_wrap_ui(UI_HEADERS_LOCAL ${ui_main})

add_definitions(-DUSE_DEEPLOCALIZER)

include_directories(${biotracker-includes})

add_library(${lib_name} STATIC
    ${src_main} ${hdr_main} ${UI_HEADERS_LOCAL}
)

get_target_property(PIPELINE_INCLUDES Pipeline PIPELINE_INCLUDES)
get_target_property(DEEPLOCALIZER_CLASSIFIER_LIBRARY_DIR Pipeline PIPELINE_DEEPLOCALIZER_LIBRARY_DIR)

link_directories(${DEEPLOCALIZER_CLASSIFIER_LIBRARY_DIR})

include_directories(${PIPELINE_INCLUDES})

target_link_libraries(${lib_name}
    "deeplocalizer-classifier"
    Pipeline
    BioTrackerLib
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
