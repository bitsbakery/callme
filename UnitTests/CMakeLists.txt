add_executable(unit_tests
    "main.cpp"
    "delegateTests.cpp"
    "APIErrorsTests.cpp"
    "eventTests.cpp"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unit_tests PRIVATE 
        "-Wno-unknown-pragmas"
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(unit_tests PRIVATE
        "/wd4068"#unknown-pragma
    )
endif()

target_include_directories(unit_tests PRIVATE
    ../Impl
)

add_test(
  NAME unit_tests
  COMMAND unit_tests
)