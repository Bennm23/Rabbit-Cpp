file(GLOB SRCS *.cpp *.h)

find_package(PkgConfig REQUIRED)

pkg_check_modules(RABBITMQ REQUIRED librabbitmq)

add_library(rabbitClient ${SRCS})

include_directories(${RABBITMQ_INCLUDE_DIRS})
target_link_libraries(
    rabbitClient
    PUBLIC
    ${RABBITMQ_LIBRARIES}
)
