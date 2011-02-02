set(PROJECT_LIB_DIRECTORY "${PROJECT_BINARY_DIR}/lib${LIB_SUFFIX}" CACHE STRING "Install path for libraries, e.g. 'lib64' on some 64-bit Linux distros.")
mark_as_advanced(PROJECT_LIB_DIRECTORY)

set(OGRE_INSTALL_DIR "" CACHE PATH "Location where Ogre SDK is installed")
option(SERIALIZER_SUPPORT_CG "Build support for serializing Cg scripts" TRUE)
option(SERIALIZER_SUPPORT_HLSL "Build support for serializing HSLS scripts" TRUE)
option(SERIALIZER_SUPPORT_GLSL "Build support for serializing GLSL scripts" TRUE)
option(SERIALIZER_SUPPORT_GLSLES "Build support for serializing GLSLES scripts" FALSE)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug, Release" FORCE)
mark_as_advanced(CMAKE_BUILD_TYPE)


# choose paths to build and install dependencies to
if (NOT PROJECT_RUNTIME_OUTPUT)
  set(PROJECT_RUNTIME_OUTPUT ${PROJECT_BINARY_DIR}/bin)
endif ()
if (WIN32)
  set(PROJECT_RELEASE_PATH "/Release")
  set(PROJECT_DEBUG_PATH "/Debug")
else ()
  set(PROJECT_RELEASE_PATH "")
  set(PROJECT_DEBUG_PATH "")
endif ()

# postfix debug libraries with a _d
set(CMAKE_DEBUG_POSTFIX "_d")

# determine if we are compiling for a 32bit or 64bit system
include(CheckTypeSize)
CHECK_TYPE_SIZE("void*" PROJECT_PTR_SIZE BUILTIN_TYPES_ONLY)
if (PROJECT_PTR_SIZE EQUAL 8)
  set(PROJECT_PLATFORM_X64 TRUE)
else ()
  set(PROJECT_PLATFORM_X64 FALSE)
endif ()

# Add -fPIC flag to x64 GCC builds
if (CMAKE_COMPILER_IS_GNUCXX AND NOT MINGW)
  if (PROJECT_PLATFORM_X64 AND NOT APPLE)
    add_definitions(-fPIC)
  endif ()
endif ()


# by default, install to source directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Install prefix" FORCE)
  endif ()
endif ()

# enforce that CMAKE_BUILD_TYPE is set, otherwise install commands
# will not work for Makefile based generators.
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug, Release" FORCE)
endif ()

# configure and install dependency LIBRARY
# INCLUDE_DIR specifies where header files need to be installed
# Any additional parameters are treated as a list of header files
# to install.
macro(install_dep LIBRARY INCLUDE_DIR)
  set_target_properties(${LIBRARY} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_RUNTIME_OUTPUT}
  )
  # always install runtime libraries (i.e. DLL, so)
  install(TARGETS ${LIBRARY}
    RUNTIME DESTINATION "bin${PROJECT_RELEASE_PATH}" CONFIGURATIONS Release
    LIBRARY DESTINATION "${PROJECT_LIB_DIRECTORY}${PROJECT_RELEASE_PATH}" CONFIGURATIONS Release
    ARCHIVE DESTINATION "${PROJECT_BINARY_DIR}/notwanted" CONFIGURATIONS Release
  )
  install(TARGETS ${LIBRARY}
    RUNTIME DESTINATION "bin${PROJECT_DEBUG_PATH}" CONFIGURATIONS Debug
    LIBRARY DESTINATION "${PROJECT_LIB_DIRECTORY}${PROJECT_DEBUG_PATH}" CONFIGURATIONS Debug
    ARCHIVE DESTINATION "${PROJECT_BINARY_DIR}/notwanted" CONFIGURATIONS Debug
  )
endmacro()


add_subdirectory(Plugin_ScriptSerializer)

if (SERIALIZER_SUPPORT_CG)
	add_subdirectory(Plugin_ScriptSerializerSupport_Cg)
endif ()
if (SERIALIZER_SUPPORT_HLSL)
	#add_subdirectory(Plugin_ScriptSerializerSupport_HLSL)
endif ()
if (SERIALIZER_SUPPORT_GLSL)
	#add_subdirectory(Plugin_ScriptSerializerSupport_GLSL)
endif ()
if (SERIALIZER_SUPPORT_GLSLES)
	#add_subdirectory(Plugin_ScriptSerializerSupport_GLSLES)
endif ()

