#pragma once

#include "OgrePrerequisites.h"
#include "ScriptSerializerMemoryAllocatorConfig.h"

namespace Ogre
{
	// Predefine classes
	class ScriptSerializer;
	class ScriptSerializerManager;
	class ScriptSerializerPlugin;
	class ShaderSerializer;

	//-------------------------------------------
	// Windows setttings
	//-------------------------------------------
#if (OGRE_PLATFORM == OGRE_PLATFORM_WIN32) && !defined(OGRE_STATIC_LIB)
#	ifdef SCRIPTSERIALIZERDLL_EXPORTS
#		define _ScriptSerializerExport __declspec(dllexport)
#	else
#       if defined( __MINGW32__ )
#           define _ScriptSerializerExport
#       else
#    		define _ScriptSerializerExport __declspec(dllimport)
#       endif
#	endif
#else
#	define _ScriptSerializerExport
#endif	// OGRE_WIN32

	
#if OGRE_VERSION_MAJOR > 1 || OGRE_VERSION_MINOR >= 8
#	define USE_MICROCODE_SHADERCACHE
#endif

}

