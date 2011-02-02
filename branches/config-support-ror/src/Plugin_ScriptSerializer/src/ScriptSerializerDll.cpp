#include "ScriptSerializerPreCompiled.h"
#include "ScriptSerializerPlugin.h"
#include "OgreRoot.h"

#ifndef OGRE_STATIC_LIB

namespace Ogre 
{
	ScriptSerializerPlugin* plugin;

	extern "C" void _ScriptSerializerExport dllStartPlugin(void) throw()
	{
		plugin = new ScriptSerializerPlugin();
		Root::getSingleton().installPlugin(plugin);
	}

	extern "C" void _ScriptSerializerExport dllStopPlugin(void)
	{
		Root::getSingleton().uninstallPlugin(plugin);
		delete plugin; plugin = 0;
	}
}
#endif