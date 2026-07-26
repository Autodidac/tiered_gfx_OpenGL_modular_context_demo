add_library(EpochGui STATIC
    include/gui/floating_window.hpp
    include/gui/layout_primitives.hpp
    include/gui/control_layout.hpp
    src/epochgui/floating_window.cpp
    src/epochgui/layout_primitives.cpp
    src/epochgui/control_layout.cpp
)
add_library(epoch_gui ALIAS EpochGui)
add_library(Autodidac::EpochGui ALIAS EpochGui)
target_include_directories(EpochGui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(EpochGui PUBLIC cxx_std_23)
set_target_properties(EpochGui PROPERTIES FOLDER "Epoch/GUI" CXX_STANDARD 23 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
if(MSVC)
    target_compile_options(EpochGui PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()
