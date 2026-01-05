cmake_minimum_required(VERSION 3.20)
project(PhotoGuruViewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Concurrent
    Network
    Sql
    WebEngineWidgets
    Test
)

# Find OpenCV (for image processing fallback)
find_package(OpenCV REQUIRED)

# Google Test for unit testing
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest)
    enable_testing()
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ml
    ${OpenCV_INCLUDE_DIRS}
    /opt/homebrew/Cellar/onnxruntime/1.22.2_7/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/common
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/tools/mtmd
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/examples
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/ggml/include
)

# Source files
set(SOURCES
    src/main.cpp
    src/core/ImageLoader.cpp
    src/core/MetadataReader.cpp
    src/core/MetadataWriter.cpp
    src/core/ExifToolDaemon.cpp
    src/core/ThumbnailCache.cpp
    src/core/PhotoDatabase.cpp
    src/core/Logger.cpp
    src/core/GoogleTakeoutParser.cpp
    src/core/GoogleTakeoutImporter.cpp
    src/ui/MainWindow.cpp
    src/ui/ImageViewer.cpp
    src/ui/ThumbnailGrid.cpp
    src/ui/MetadataPanel.cpp
    src/ui/SKPBrowser.cpp
    src/ui/MapView.cpp
    src/ui/TimelineView.cpp
    src/ui/SemanticSearch.cpp
    src/ui/FilterPanel.cpp
    src/ui/AnalysisPanel.cpp
    src/ui/NotificationToast.cpp
    src/ui/NotificationManager.cpp
    src/ml/ONNXInference.cpp
    src/ml/CLIPAnalyzer.cpp
    src/ml/LlamaVLM.cpp
)

set(HEADERS
    src/core/ImageLoader.h
    src/core/MetadataReader.h
    src/core/MetadataWriter.h
    src/core/ThumbnailCache.h
    src/core/PhotoDatabase.h
    src/core/PhotoMetadata.h
    src/core/GoogleTakeoutParser.h
    src/core/GoogleTakeoutImporter.h
    src/ui/MainWindow.h
    src/ui/ImageViewer.h
    src/ui/ThumbnailGrid.h
    src/ui/MetadataPanel.h
    src/ui/MapView.h
    src/ui/TimelineView.h
    src/ui/SemanticSearch.h
    src/ui/FilterPanel.h
    src/ui/AnalysisPanel.h
    src/ui/SKPBrowser.h
    src/ui/NotificationToast.h
    src/ui/NotificationManager.h
    src/ui/DarkTheme.h
    src/ml/ONNXInference.h
    src/ml/CLIPAnalyzer.h
    src/ml/LlamaVLM.h
)

# Resources (icons, stylesheets)
qt6_add_resources(RESOURCES resources/resources.qrc)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${RESOURCES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::WebEngineWidgets
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Sql
    ${OpenCV_LIBS}
    /opt/homebrew/Cellar/onnxruntime/1.22.2_7/lib/libonnxruntime.dylib
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libllama.dylib
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libggml.dylib
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libggml-base.dylib
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libggml-metal.dylib
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libmtmd.dylib
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/common/libcommon.a
)

# Platform-specific settings
if(APPLE)
    # macOS bundle
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "PhotoGuru Viewer"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.photoguru.viewer"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    )
    
    # Add LibRaw on macOS (brew install libraw)
    find_library(LIBRAW_LIBRARY raw REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${LIBRAW_LIBRARY})
    
    # HEIF support (brew install libheif)
    find_library(LIBHEIF_LIBRARY heif)
    if(LIBHEIF_LIBRARY)
        target_link_libraries(${PROJECT_NAME} ${LIBHEIF_LIBRARY})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HEIF_SUPPORT_ENABLED)
    endif()
endif()

if(UNIX AND NOT APPLE)
    # Linux
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBRAW REQUIRED libraw)
    pkg_check_modules(LIBHEIF libheif)
    
    target_include_directories(${PROJECT_NAME} PRIVATE ${LIBRAW_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${LIBRAW_LIBRARIES})
    
    if(LIBHEIF_FOUND)
        target_include_directories(${PROJECT_NAME} PRIVATE ${LIBHEIF_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} ${LIBHEIF_LIBRARIES})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HEIF_SUPPORT_ENABLED)
    endif()
endif()

# Install
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# Copy Python agent alongside executable
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/agent_v2.py
    DESTINATION bin
)

# Configure exiftool requirement
message(STATUS "Note: ExifTool is required at runtime. Install with:")
message(STATUS "  macOS: brew install exiftool")
message(STATUS "  Linux: sudo apt install libimage-exiftool-perl")

# ============================================================================
# Unit Tests
# ============================================================================
if(BUILD_TESTS)
    # Test sources
    set(TEST_SOURCES
        tests/main.cpp
        tests/test_metadata_reader.cpp
        tests/test_metadata_writer.cpp
        tests/test_exiftool_daemon.cpp
        tests/test_photo_database.cpp
        tests/test_image_loader.cpp
        tests/test_thumbnail_cache.cpp
        tests/test_filter_criteria.cpp
        tests/test_analysis_panel.cpp
        tests/test_analysis_panel_buttons.cpp
        tests/test_analysis_panel_checkboxes.cpp
        tests/test_analysis_panel_actions.cpp
        tests/test_semantic_search.cpp
        tests/test_skp_browser.cpp
        tests/test_metadata_panel.cpp
        tests/test_filter_panel.cpp
        tests/test_onnx_basic.cpp
        tests/test_clip_analyzer.cpp
        tests/test_llama_vlm.cpp
        tests/test_image_viewer.cpp
        tests/test_thumbnail_grid.cpp
        tests/test_main_window.cpp
        tests/test_map_view.cpp
        tests/test_timeline_view.cpp
        # Integration tests (use real AI models)
        tests/integration/test_clip_integration.cpp
        tests/integration/test_vlm_integration.cpp
    )
    
    # Core sources needed for tests (not the full app)
    set(TEST_CORE_SOURCES
        src/core/ImageLoader.cpp
        src/core/MetadataReader.cpp
        src/core/MetadataWriter.cpp
        src/core/ExifToolDaemon.cpp
        src/core/Logger.cpp
        src/core/GoogleTakeoutParser.cpp
        src/core/GoogleTakeoutImporter.cpp
        src/ui/MainWindow.cpp
        src/ui/MapView.cpp
        src/ui/TimelineView.cpp
        src/core/PhotoDatabase.cpp
        src/core/ThumbnailCache.cpp
        src/ui/FilterPanel.cpp
        src/ui/AnalysisPanel.cpp
        src/ui/SemanticSearch.cpp
        src/ui/SKPBrowser.cpp
        src/ui/MetadataPanel.cpp
        src/ui/ImageViewer.cpp
        src/ui/ThumbnailGrid.cpp
        src/ui/NotificationToast.cpp
        src/ui/NotificationManager.cpp
        src/ml/ONNXInference.cpp
        src/ml/CLIPAnalyzer.cpp
        src/ml/LlamaVLM.cpp
    )
    
    # Create test executable
    add_executable(PhotoGuruTests ${TEST_SOURCES} ${TEST_CORE_SOURCES})
    
    # Link test executable with libraries
    target_link_libraries(PhotoGuruTests
        gtest
        gtest_main
        Qt6::Core
        Qt6::Network
        Qt6::WebEngineWidgets
        Qt6::Test
        ${OpenCV_LIBS}
        Qt6::Concurrent
        Qt6::Sql
        /opt/homebrew/Cellar/onnxruntime/1.22.2_7/lib/libonnxruntime.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libllama.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libggml.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libggml-base.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libggml-metal.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/bin/libmtmd.dylib
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/llama.cpp/build/common/libcommon.a
        Qt6::Test
        ${OpenCV_LIBS}
    )
    
    # Platform-specific libraries for tests
    if(APPLE)
        find_library(LIBRAW_LIBRARY raw REQUIRED)
        target_link_libraries(PhotoGuruTests ${LIBRAW_LIBRARY})
        
        find_library(LIBHEIF_LIBRARY heif)
        if(LIBHEIF_LIBRARY)
            target_link_libraries(PhotoGuruTests ${LIBHEIF_LIBRARY})
            target_compile_definitions(PhotoGuruTests PRIVATE HEIF_SUPPORT_ENABLED)
        endif()
    endif()
    
    # Include directories for tests
    target_include_directories(PhotoGuruTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ml
        ${OpenCV_INCLUDE_DIRS}
    )
    
    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(PhotoGuruTests)
    
    # Custom test target
    add_custom_target(run_tests
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/PhotoGuruTests
        DEPENDS PhotoGuruTests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running unit tests..."
    )
    
    message(STATUS "Unit tests enabled. Run with: make test or make run_tests")
endif()
