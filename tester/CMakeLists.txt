file(GLOB SRCS *.cpp *.h)

add_executable(tester ${SRCS})

# include_directories()
target_include_directories(
    tester
    PRIVATE
    ${PROJECT_ROOT_DIR}/client
)

target_link_libraries(
    tester
    PRIVATE
    rabbitClient
)


install(TARGETS tester RUNTIME DESTINATION BIN)