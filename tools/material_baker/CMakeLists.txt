# Material Baker Tool - Procedural texture generator

add_executable(material_baker
    main.cpp
    material_generator.cpp
)

target_include_directories(material_baker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# C++17 for std::optional, etc.
target_compile_features(material_baker PRIVATE cxx_std_17)
