#include "SerializerProfilerPreCompiled.h"
#include "SerializerProfilerPlugin.h"
#include "OgreRoot.h"

#ifndef OGRE_STATIC_LIB

namespace Ogre 
{
	SerializerProfilerPlugin* plugin;

	extern "C" void _ScriptProfilerExport dllStartPlugin(void) throw()
	{
		plugin = new SerializerProfilerPlugin();
		Root::getSingleton().installPlugin(plugin);
	}

	extern "C" void _ScriptProfilerExport dllStopPlugin(void)
	{
		Root::getSingleton().uninstallPlugin(plugin);
		delete plugin;
	}
}
#endif