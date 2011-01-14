#pragma once

namespace Ogre {
	
	typedef CategorisedAllocPolicy<Ogre::MEMCATEGORY_SCRIPTING> ScriptSerializerAllocPolicy;
	typedef AllocatedObject<ScriptSerializerAllocPolicy> ScriptSerializerAllocatedObject;
	typedef ScriptSerializerAllocatedObject	ScriptSerializerAlloc;

	/** Plugin instance for Script Serializer */
	class ScriptSerializerProfiler : public ScriptSerializerAlloc, public ResourceGroupListener
	{
	public:
		ScriptSerializerProfiler();
		~ScriptSerializerProfiler(void);
		

		/// Interface ResourceGroupListener
		virtual void scriptParseStarted(const String& scriptName, bool& skipThisScript) { }
		virtual void scriptParseEnded(const String& scriptName, bool skipped) { }
		virtual void resourceGroupScriptingStarted(const String& groupName, size_t scriptCount);
		virtual void resourceGroupScriptingEnded(const String& groupName);
		virtual void resourceGroupLoadStarted(const String& groupName, size_t resourceCount) { }
		virtual void resourceLoadStarted(const ResourcePtr& resource) { }
        virtual void resourceLoadEnded(void) { }
        virtual void worldGeometryStageStarted(const String& description) { }
        virtual void worldGeometryStageEnded(void) { }
        virtual void resourceGroupLoadEnded(const String& groupName) { }

	private:
		void loadStats();
		void logMessage(const String& message);

	private:
		clock_t scriptCompileStartTime;
		int scriptCount;
	};
}
