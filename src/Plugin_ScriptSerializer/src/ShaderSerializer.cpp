#include "ScriptSerializerPreCompiled.h"
#include "ShaderSerializer.h"
using namespace std;



#ifdef USE_MICROCODE_SHADERCACHE

namespace Ogre {

	ShaderSerializer::ShaderSerializer() {
	}

	ShaderSerializer::~ShaderSerializer() {

	}

	void ShaderSerializer::setCaching(bool cache) {
		GpuProgramManager::getSingleton().setSaveMicrocodesToCache(cache);
	}

	void ShaderSerializer::saveCache(const DataStreamPtr& stream) {
		GpuProgramManager::getSingleton().saveMicrocodeCache(stream);
	}

	void ShaderSerializer::loadCache(const DataStreamPtr& stream) {
		GpuProgramManager::getSingleton().loadMicrocodeCache(stream);
	}

}

#endif	//USE_MICROCODE_SHADERCACHE
