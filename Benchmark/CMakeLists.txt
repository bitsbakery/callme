add_executable(benchmark
    "main.cpp"
)

target_include_directories(benchmark PRIVATE
    ../Impl
    ../pretty
)

target_link_libraries(benchmark PRIVATE
    pretty
)