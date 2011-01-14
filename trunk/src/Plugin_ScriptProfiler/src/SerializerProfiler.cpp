#include "SerializerProfilerPreCompiled.h"
#include "SerializerProfiler.h"
#include <sstream>
using namespace std;

namespace Ogre {

	const String serializerLogName = "SerialzierProfiler.log"; 

	ScriptSerializerProfiler::ScriptSerializerProfiler() : scriptCompileStartTime(0), scriptCount(0) {
		LogManager::getSingleton().createLog(serializerLogName);
		ResourceGroupManager::getSingleton().addResourceGroupListener(this);
	}
	
	ScriptSerializerProfiler::~ScriptSerializerProfiler(void) {
		LogManager::getSingleton().getSingleton().destroyLog(serializerLogName);
		Ogre::ResourceGroupManager::getSingleton().removeResourceGroupListener(this);
	}

	void ScriptSerializerProfiler::resourceGroupScriptingStarted(const String& groupName, size_t scriptCount) {
		this->scriptCount = scriptCount;
		scriptCompileStartTime = clock();
	}

	void ScriptSerializerProfiler::resourceGroupScriptingEnded(const String& groupName) {
		stringstream ss;
		double elapsedTime = (clock() - scriptCompileStartTime) / 1000.0;
		ss << "[" << groupName << "] " << scriptCount << " scripts parsed in " << elapsedTime << " seconds.";
		logMessage(ss.str());
	}


	void ScriptSerializerProfiler::logMessage(const String& message) {
		LogManager::getSingleton().getLog(serializerLogName)->logMessage("SERAILIZER LOG: " + message);
	}

}