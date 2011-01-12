// This is the main DLL file.

#include "ScriptSerializerPreCompiled.h"
#include "ScriptSerializerPlugin.h"
#include "ScriptSerializerManager.h"

namespace Ogre {
	
	const String sPluginName = "ScriptSerializer";
	
	ScriptSerializerPlugin::ScriptSerializerPlugin() {
	}

	/// @copydoc Plugin::getName
	const String& ScriptSerializerPlugin::getName() const {
		return sPluginName;
	}

	/// @copydoc Plugin::install
	void ScriptSerializerPlugin::install() {
		LogManager::getSingleton().logMessage("Installing Script Serializer Plugin");
	}
	
	/// @copydoc Plugin::initialise
	void ScriptSerializerPlugin::initialise() {
		LogManager::getSingleton().logMessage("Initializing Script Serializer Plugin");
		mSerializeManager = OGRE_NEW ScriptSerializerManager();
	}
	
	/// @copydoc Plugin::shutdown
	void ScriptSerializerPlugin::shutdown() {
		LogManager::getSingleton().logMessage("Shutting down Script Serializer Plugin");
	}
	
	/// @copydoc Plugin::uninstall
	void ScriptSerializerPlugin::uninstall() {
		LogManager::getSingleton().logMessage("Uninstalling Script Serializer Plugin");
	}

}