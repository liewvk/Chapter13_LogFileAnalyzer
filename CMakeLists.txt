cmake_minimum_required(VERSION 3.20)

project(Chapter13_LogFileAnalyzer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Chapter13_LogFileAnalyzer
    src/main.cpp
)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/data")

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/data/sample_log.txt")
    file(WRITE "${CMAKE_SOURCE_DIR}/data/sample_log.txt"
"[INFO] Application started
[INFO] User login successful
[WARNING] Disk space is getting low
[ERROR] Failed to connect to database
[INFO] Retrying connection
[ERROR] Database connection failed
[WARNING] High memory usage detected
[INFO] Application ended
")
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/data/log_summary.txt")
    file(WRITE "${CMAKE_SOURCE_DIR}/data/log_summary.txt" "")
endif()

target_compile_definitions(Chapter13_LogFileAnalyzer PRIVATE
    LOG_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)