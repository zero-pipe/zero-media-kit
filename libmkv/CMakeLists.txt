set(LIBMKV_ROOT "${CMAKE_CURRENT_LIST_DIR}")

file(GLOB LIBMKV_SOURCES "${LIBMKV_ROOT}/src/*.c")

add_library(libmkv STATIC ${LIBMKV_SOURCES})
target_include_directories(libmkv
    PUBLIC "${LIBMKV_ROOT}/include"
    PRIVATE "${LIBMKV_ROOT}/src"
)

if (MSVC)
    target_compile_definitions(libmkv PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(libmkv PRIVATE /wd4267 /wd4101 /wd4828)
endif ()

set_target_properties(libmkv PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)
