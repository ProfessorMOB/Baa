cmake_minimum_required(VERSION 3.20)
project(baa
    VERSION 0.1.9.8
    DESCRIPTION "لغة باء - The Baa Programming Language"
    LANGUAGES C
)

# إعداد معيار لغة C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to skip parser compilation for development
option(SKIP_PARSER "Skip compiling the parser components" OFF)

# Find LLVM package - Set the default path for Windows to the known installation
if(WIN32)
    set(LLVM_DIR "C:/Program Files/LLVM/lib/cmake/llvm" CACHE PATH "Directory containing LLVMConfig.cmake")
else()
    set(LLVM_DIR "" CACHE PATH "Directory containing LLVMConfig.cmake")
endif()

# Try to find LLVM
find_package(LLVM CONFIG)

if(NOT LLVM_FOUND)
    # Try common installation paths
    if(WIN32)
        set(LLVM_HINT_PATHS
            "C:/Program Files/LLVM"
            "C:/Program Files/LLVM/lib/cmake/llvm"
            "C:/LLVM"
        )
    elseif(APPLE)
        set(LLVM_HINT_PATHS
            "/usr/local/opt/llvm"
            "/opt/homebrew/opt/llvm"
        )
    else() # Linux
        set(LLVM_HINT_PATHS
            "/usr/lib/llvm"
            "/usr/local/lib/llvm"
        )
    endif()

    # Try to find LLVM in the hint paths
    foreach(HINT_PATH ${LLVM_HINT_PATHS})
        message(STATUS "Checking for LLVM in: ${HINT_PATH}")

        if(EXISTS "${HINT_PATH}/lib/cmake/llvm/LLVMConfig.cmake")
            set(LLVM_DIR "${HINT_PATH}/lib/cmake/llvm")
            find_package(LLVM CONFIG)
            if(LLVM_FOUND)
                break()
            endif()
        elseif(EXISTS "${HINT_PATH}/LLVMConfig.cmake")
            set(LLVM_DIR "${HINT_PATH}")
            find_package(LLVM CONFIG)
            if(LLVM_FOUND)
                break()
            endif()
        endif()
    endforeach()
endif()

if(LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    # Add LLVM definitions and include directories
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})

    # Include LLVM CMake modules
    list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
    include(AddLLVM)

    # Set LLVM_AVAILABLE to indicate LLVM is available
    set(LLVM_AVAILABLE TRUE)
else()
    message(WARNING "LLVM not found - code generation will be disabled")
    set(LLVM_AVAILABLE FALSE)

    # Define empty variables to avoid errors
    set(LLVM_INCLUDE_DIRS "")
    set(LLVM_DEFINITIONS "")
endif()

# دعم اليونيكود وUTF-8
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finput-charset=UTF-8")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")
endif()

# إضافة مجلدات التضمين
include_directories(${CMAKE_SOURCE_DIR}/include)

# إضافة المكونات
add_subdirectory(src/utils)
add_subdirectory(src/types)
add_subdirectory(src/lexer)
add_subdirectory(src/parser)
add_subdirectory(src/operators)
add_subdirectory(src/ast)
add_subdirectory(src/codegen)
add_subdirectory(src/analysis)
add_subdirectory(src/preprocessor)

# تفعيل الاختبارات (معطل مؤقتاً)
# enable_testing()
# add_subdirectory(tests)

# البرنامج الرئيسي
add_executable(baa
    src/main.c
    src/compiler.c
)

target_include_directories(baa
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(baa
    PRIVATE
        baa_utils
        baa_types
        baa_lexer
        baa_preprocessor
        baa_parser
        baa_operators
        baa_ast
        baa_codegen
        baa_flow_analysis
)
