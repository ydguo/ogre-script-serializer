#pragma once
#include "OgreScriptCompiler.h"
#include <stack>


#ifdef USE_MICROCODE_SHADERCACHE

namespace Ogre {

	/** Uses the Ogre's Microcode shader caching feature. */
	class ShaderSerializer : public ScriptSerializerAlloc
	{
	public:
		ShaderSerializer();
		~ShaderSerializer();

		void setCaching(bool cache);
		void saveCache(const DataStreamPtr& stream);
		void loadCache(const DataStreamPtr& stream);
	};

}

#endif