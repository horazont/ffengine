if (NOT TARGET sigc++)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(_SIGC++ sigc++-2.0)

    find_path(SIGC++_MAIN_INCLUDE_DIRS
        NAMES
            sigc++/sigc++.h
        PATHS
            ${_SIGC++_INCLUDEDIR}
            /usr/include
            /usr/local/include
        PATH_SUFFIXES
            sigc++-2.0
    )

    find_path(SIGC++_CONFIG_INCLUDE_DIRS
        NAMES
            sigc++config.h
        PATHS
            ${_SIGC++_INCLUDEDIR}
            ${_SIGC++_LIBDIR}
            /usr/include
            /usr/local/include
            /usr/lib
            /usr/lib64
            /usr/local/lib
        PATH_SUFFIXES
            sigc++-2.0/include
    )

    find_library(SIGC++_LIBRARY
        NAMES
            sigc-2.0
        PATHS
            ${_SIGC++_LIBDIR}
            /usr/lib
            /usr/lib64
            /usr/local/lib
            /usr/local/lib64
    )

    set(SIGC++_FOUND "NO")
    if (SIGC++_LIBRARY)
        set(SIGC++_FOUND "YES")
        add_library(sigc++ SHARED IMPORTED)
        set_property(TARGET sigc++ APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${SIGC++_MAIN_INCLUDE_DIRS} ${SIGC++_CONFIG_INCLUDE_DIRS})
        set_property(TARGET sigc++ PROPERTY INTERFACE_LINK_LIBRARIES "${SIGC++_LIBRARY}")
        set_property(TARGET sigc++ APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
        set_property(TARGET sigc++ PROPERTY IMPORTED_LOCATION_RELEASE "${SIGC++_LIBRARY}")
        set_property(TARGET sigc++ PROPERTY IMPORTED_SONAME_RELEASE "${SIGC++_LIBRARY}")
        message("FOO " ${SIGC++_INCLUDE_DIRS})
    endif (SIGC++_LIBRARY)
endif()
