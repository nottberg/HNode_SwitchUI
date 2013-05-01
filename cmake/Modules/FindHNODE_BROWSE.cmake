include(FindPkgConfig)

pkg_check_modules(HNODE_BROWSE_PKG hnode-browser-1.0)

if (HNODE_BROWSE_PKG_FOUND)
    find_path(HNODE_BROWSE_INCLUDE_DIR  NAMES hnode-browser.h PATH_SUFFIXES hnode-1.0
       PATHS
       ${HNODE_BROWSE_PKG_INCLUDE_DIRS}
       /usr/include/hnode-1.0
       /usr/include
       /usr/local/include/hnode-1.0
       /usr/local/include
    )

    find_library(HNODE_BROWSE_LIBRARIES NAMES hnode_browser
       PATHS
       ${HNODE_BROWSE_PKG_LIBRARY_DIRS}
       /usr/lib
       /usr/local/lib
    )

else (HNODE_BROWSE_PKG_FOUND)
    # Find Glib even if pkg-config is not working (eg. cross compiling to Windows)
    find_library(HNODE_BROWSE_LIBRARIES NAMES hnode_browser)
    string (REGEX REPLACE "/[^/]*$" "" HNODE_BROWSE_LIBRARIES_DIR ${HNODE_BROWSE_LIBRARIES})

    find_path(HNODE_BROWSE_INCLUDE_DIR NAMES hnode-browser.h PATH_SUFFIXES hnode-1.0)

endif (HNODE_BROWSE_PKG_FOUND)

if (HNODE_BROWSE_INCLUDE_DIR AND HNODE_BROWSE_LIBRARIES)
    set(HNODE_BROWSE_INCLUDE_DIRS ${HNODE_BROWSE_INCLUDE_DIR})
endif (HNODE_BROWSE_INCLUDE_DIR AND HNODE_BROWSE_LIBRARIES)

if(HNODE_BROWSE_INCLUDE_DIRS AND HNODE_BROWSE_LIBRARIES)
   set(HNODE_BROWSE_FOUND TRUE CACHE INTERNAL "hnode-1.0 found")
   message(STATUS "Found hnode-1.0: ${HNODE_BROWSE_INCLUDE_DIR}, ${HNODE_BROWSE_LIBRARIES}")
else(HNODE_BROWSE_INCLUDE_DIRS AND HNODE_BROWSE_LIBRARIES)
   set(HNODE_BROWSE_FOUND FALSE CACHE INTERNAL "hnode_browser found")
   message(STATUS "hnode-1.0 not found.")
endif(HNODE_BROWSE_INCLUDE_DIRS AND HNODE_BROWSE_LIBRARIES)

mark_as_advanced(HNODE_BROWSE_INCLUDE_DIR HNODE_BROWSE_INCLUDE_DIRS HNODE_BROWSE_LIBRARIES)


