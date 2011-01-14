// This is the main DLL file.

#include "SerializerProfilerPrecompiled.h"
#include "SerializerProfilerPlugin.h"
#include "SerializerProfiler.h"

namespace Ogre {
	
	const String sPluginName = "SerializerProfiler";
	
	SerializerProfilerPlugin::SerializerProfilerPlugin() {
	}

	/// @copydoc Plugin::getName
	const String& SerializerProfilerPlugin::getName() const {
		return sPluginName;
	}

	/// @copydoc Plugin::install
	void SerializerProfilerPlugin::install() {
	}
	
	/// @copydoc Plugin::initialise
	void SerializerProfilerPlugin::initialise() {
		profiler = OGRE_NEW ScriptSerializerProfiler();
	}
	
	/// @copydoc Plugin::shutdown
	void SerializerProfilerPlugin::shutdown() {
		OGRE_DELETE profiler;
	}
	
	/// @copydoc Plugin::uninstall
	void SerializerProfilerPlugin::uninstall() {
	}

}