#pragma once

#include "OgrePrerequisites.h"

namespace Ogre
{
	// Predefine classes
	class ScriptSerializerProfiler;
	class SerializerProfilerPlugin;

	//-------------------------------------------
	// Windows setttings
	//-------------------------------------------
#if (OGRE_PLATFORM == OGRE_PLATFORM_WIN32) && !defined(OGRE_STATIC_LIB)
#	ifdef SCRIPTPROFILERDLL_EXPORTS
#		define _ScriptProfilerExport __declspec(dllexport)
#	else
#       if defined( __MINGW32__ )
#           define _ScriptProfilerExport
#       else
#    		define _ScriptProfilerExport __declspec(dllimport)
#       endif
#	endif
#else
#	define _ScriptProfilerExport
#endif	// OGRE_WIN32
}

