cmake_minimum_required(VERSION 3.0)

project(kcontrol_events LANGUAGES C)

include(FindALSA)

if (NOT ALSA_FOUND)
    message(FATAL_ERROR "ALSA API requested but no ALSA dev libraries found")
endif()

add_executable(kcontrol_events kcontrol_events.c)
target_link_libraries(kcontrol_events ${ALSA_LIBRARY})

install(TARGETS kcontrol_events RUNTIME DESTINATION bin)
