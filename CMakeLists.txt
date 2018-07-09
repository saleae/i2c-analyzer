cmake_minimum_required (VERSION 3.11)
project(i2c_analyzer)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/I2cAnalyzer.cpp
src/I2cAnalyzer.h
src/I2cAnalyzerResults.cpp
src/I2cAnalyzerResults.h
src/I2cAnalyzerSettings.cpp
src/I2cAnalyzerSettings.h
src/I2cSimulationDataGenerator.cpp
src/I2cSimulationDataGenerator.h
)

add_library(i2c_analyzer SHARED ${SOURCES})
target_link_libraries(i2c_analyzer PRIVATE Saleae::AnalyzerSDK)

if(MSVC)
    install(TARGETS i2c_analyzer RUNTIME DESTINATION "Analyzers")
else()
    install(TARGETS i2c_analyzer LIBRARY DESTINATION "Analyzers")
endif()

set_post_build_destination(i2c_analyzer)
