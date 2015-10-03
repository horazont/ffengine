set(epoxy_FOUND "no")

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_EPOXY QUIET epoxy)

find_path(epoxy_INCLUDE_DIR epoxy/gl.h
          HINTS ${PC_EPOXY_INCLUDEDIR} ${PC_EPOXY_INCLUDE_DIRS}
          PATH_SUFFIXES epoxy)
find_library(epoxy_LIBRARY_NAMES epoxy
             HINTS ${PC_EPOXY_LIBDIR} ${PC_EPOXY_LIBRARY_DIRS})

if (epoxy_LIBRARY_NAMES)
    set(epoxy_FOUND "yes")
    add_library(epoxy SHARED IMPORTED)
    set_property(TARGET epoxy APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${epoxy_INCLUDE_DIR})
    set_property(TARGET epoxy PROPERTY INTERFACE_LINK_LIBRARIES ${epoxy_LIBRARY_NAMES})
    set_property(TARGET epoxy APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
    set_property(TARGET epoxy PROPERTY IMPORTED_LOCATION_RELEASE "${epoxy_LIBRARY_NAMES}")
    set_property(TARGET epoxy PROPERTY IMPORTED_SONAME_RELEASE "${epoxy_LIBRARY_NAMES}")
endif (epoxy_LIBRARY_NAMES)
