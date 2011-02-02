project(Plugin_ScriptSerializer)

set(PROJECT_HEADERS
  include/ScriptSerializer.h
  include/ScriptSerializerManager.h
  include/ScriptSerializerMemoryAllocatorConfig.h
  include/ScriptSerializerPlugin.h
  include/ScriptSerializerPreCompiled.h
  include/ScriptSerializerPrerequisites.h
)
set(PROJECT_SOURCES
  src/ScriptSerializer.cpp
  src/ScriptSerializerDll.cpp
  src/ScriptSerializerManager.cpp
  src/ScriptSerializerPlugin.cpp
  src/ScriptSerializerPreCompiled.cpp
)

set(OGRE_INSTALL_DIR "" CACHE STRING "Location where Ogre SDK is installed")
set(OGRE_INCLUDE_DIR "${OGRE_INSTALL_DIR}/include/OGRE")
set(OGRE_LIB_DIR_REL "${OGRE_INSTALL_DIR}/lib/Release")
set(OGRE_LIB_DIR_DBG "${OGRE_INSTALL_DIR}/lib/Debug")

set(OGRE_LIB_REL "${OGRE_LIB_DIR_REL}/OgreMain.lib")
set(OGRE_LIB_DBG "${OGRE_LIB_DIR_DBG}/OgreMain_d.lib")

mark_as_advanced(OGRE_INCLUDE_DIR OGRE_LIB_DIR_REL OGRE_LIB_DIR_DBG CMAKE_INSTALL_PREFIX OGRE_LIB_REL OGRE_LIB_DBG)

include_directories(include)
include_directories("${OGRE_INCLUDE_DIR}")

include(PrecompiledHeader)
use_precompiled_header("" "ScriptSerializerPreCompiled.h" "src/ScriptSerializerPreCompiled.cpp")

add_definitions(-DSCRIPTSERIALIZERDLL_EXPORTS)

add_library(Plugin_ScriptSerializer SHARED ${PROJECT_HEADERS} ${PROJECT_SOURCES} ${PROJECT_PLATFORM_HEADERS} ${PROJECT_PLATFORM_SOURCES})
target_link_libraries(Plugin_ScriptSerializer ${PROJECT_PLATFORM_LIBS})
target_link_libraries(Plugin_ScriptSerializer debug ${OGRE_LIB_DBG})
target_link_libraries(Plugin_ScriptSerializer optimized ${OGRE_LIB_REL})
install_dep(Plugin_ScriptSerializer include ${PROJECT_HEADERS})


set(PROJECT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include" CACHE PATH "" FORCE)
set(PROJECT_LIBRARY_DBG "Plugin_ScriptSerializer_d" CACHE STRING "" FORCE)
set(PROJECT_LIBRARY_REL "Plugin_ScriptSerializer" CACHE STRING "" FORCE)
mark_as_advanced(PROJECT_INCLUDE_DIR PROJECT_LIBRARY_DBG PROJECT_LIBRARY_REL)
