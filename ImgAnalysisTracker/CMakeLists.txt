set(lib_name "ImgAnalysisTracker.tracker")

file(GLOB src_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
file(GLOB hdr_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h *.hpp)
file(GLOB ui_main RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.ui)
file(GLOB src_legacy RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} legacy/*.cpp)
file(GLOB hdr_legacy RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} legacy/*.h *.hpp)

qt5_wrap_ui(UI_HEADERS_LOCAL ${ui_main})

add_library(${lib_name} SHARED
    ${src_main} ${hdr_main} ${src_legacy} ${hdr_legacy} ${UI_HEADERS_LOCAL}
)

target_link_libraries(${lib_name}
    ${CPM_LIBRARIES}
)
