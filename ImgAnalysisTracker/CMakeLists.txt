set(lib_name "ImgAnalysisTracker")

file(GLOB src_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file(GLOB hdr_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h *.hpp)
file(GLOB ui_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.ui)

list(REMOVE_ITEM src_main ${CMAKE_CURRENT_SOURCE_DIR}/BioTrackerInterface.cpp)

qt5_wrap_ui(UI_HEADERS_LOCAL ${ui_main})

add_library(${lib_name} STATIC
    ${src_main} ${hdr_main} ${src_legacy} ${hdr_legacy} ${UI_HEADERS_LOCAL}
)

target_link_libraries(${lib_name}
    ${CPM_LIBRARIES}
)

add_library("${lib_name}.tracker" SHARED
    BioTrackerInterface.cpp)

target_link_libraries("${lib_name}.tracker"
    ${lib_name}
)

