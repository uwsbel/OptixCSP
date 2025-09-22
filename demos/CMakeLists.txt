# -------------------------------------------------------------------------------
# List of demos
# -------------------------------------------------------------------------------

set(DEMOS
     demo_large_scene
     demo_dynamic_scene
     demo_three_heliostats
     demo_sun_modeling
     demo_parabolic_surface
     demo_read_stinput
     demo_read_mesh
)

message(STATUS "Adding demo programs for OptiX SolTrace ...")
 
# Loop through demos
foreach(PROGRAM ${DEMOS})
    message(STATUS "Adding ${PROGRAM}")


    add_executable(${PROGRAM})
    target_sources(${PROGRAM} PRIVATE ${PROGRAM}.cpp )
    set_property(TARGET ${PROGRAM} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROGRAM}>")

    target_include_directories(${PROGRAM}
        PRIVATE ${PROJECT_SOURCE_DIR}/src
    )

    message(STATUS "include directory:  ${PROJECT_SOURCE_DIR}/src")

    target_link_libraries(${PROGRAM}
        PUBLIC OptixCSP_core)

endforeach()
