############################################################
# CMakeLists for VAL
#
# Copyright 2020. All Rights Reserved.
#
# Created: September 30, 2020
# Authors: Toki Migimatsu
############################################################

# if(NOT TARGET VAL::VAL)
#     init_git_submodule(VAL.git)
#     add_subdirectory(VAL.git)
#     set_target_properties(VAL PROPERTIES
#         LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
#     )
# endif()

# function(lib_install_subdirectory_VAL)
#     install(TARGETS VAL)
# endfunction()

if(NOT TARGET VAL::VAL)
    init_git_submodule(VAL.git)
    add_subdirectory(VAL.git)
    set_target_properties(VAL PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
endif()

# Function to install the VAL library
function(lib_install_subdirectory_VAL)
    # Install the VAL library
    install(TARGETS VAL
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Install the VAL headers (adjust the path as needed)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../external/VAL/VAL.git/libraries/VAL/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VAL)

    # Export the VAL targets to a CMake file for find_package()
    # Only export once - do not export again if it's already done
    if(NOT TARGET VAL::VAL)
        export(EXPORT VALTargets FILE ${CMAKE_BINARY_DIR}/VALTargets.cmake NAMESPACE VAL::)
    endif()

    # Install the VAL CMake configuration files
    install(FILES ${CMAKE_BINARY_DIR}/VALConfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VAL)
    # install(EXPORT VALTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/VAL)
endfunction()

# Call the function to install VAL
# lib_install_subdirectory_VAL()
