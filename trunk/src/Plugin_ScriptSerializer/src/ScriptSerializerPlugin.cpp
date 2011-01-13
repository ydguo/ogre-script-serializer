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
	}
	
	/// @copydoc Plugin::initialise
	void ScriptSerializerPlugin::initialise() {
		mSerializeManager = OGRE_NEW ScriptSerializerManager();
	}
	
	/// @copydoc Plugin::shutdown
	void ScriptSerializerPlugin::shutdown() {
		OGRE_DELETE mSerializeManager;
	}
	
	/// @copydoc Plugin::uninstall
	void ScriptSerializerPlugin::uninstall() {
	}

}