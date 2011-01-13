#pragma once
#include "OgrePlugin.h"

namespace Ogre {
	
	/** Plugin instance for Script Serializer */
	class ScriptSerializerPlugin : public Plugin
	{
	public:
		ScriptSerializerPlugin();

		/// @copydoc Plugin::getName
		const String& getName() const;

		/// @copydoc Plugin::install
		void install();

		/// @copydoc Plugin::initialise
		void initialise();

		/// @copydoc Plugin::shutdown
		void shutdown();

		/// @copydoc Plugin::uninstall
		void uninstall();

	private:
		ScriptSerializerManager* mSerializeManager;

	};
}
