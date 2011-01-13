#pragma once
#include "OgreResourceGroupManager.h"
#include "OgreScriptCompiler.h"

namespace Ogre {

	class ScriptSerializer;

	class ScriptSerializerManager : public ScriptSerializerManagerAlloc, public ResourceGroupListener, public ScriptCompilerListener
	{
	public:
		ScriptSerializerManager();
		~ScriptSerializerManager(void);

		/// Interface ResourceGroupListener
		virtual void scriptParseStarted(const String& scriptName, bool& skipThisScript);
		virtual void scriptParseEnded(const String& scriptName, bool skipped) { }
		virtual void resourceGroupScriptingStarted(const String& groupName, size_t scriptCount);
		virtual void resourceGroupScriptingEnded(const String& groupName) { }
		virtual void resourceGroupLoadStarted(const String& groupName, size_t resourceCount) { }
		virtual void resourceLoadStarted(const ResourcePtr& resource) { }
        virtual void resourceLoadEnded(void) { }
        virtual void worldGeometryStageStarted(const String& description) { }
        virtual void worldGeometryStageEnded(void) { }
        virtual void resourceGroupLoadEnded(const String& groupName) { }


		/// Interface ScriptCompilerListener
		virtual bool postConversion(ScriptCompiler *compiler, const AbstractNodeListPtr&);
		

	private:
		void initializeArchive(const String& archiveName);
		bool isBinaryScript(const String& filename);
		void saveAstToDisk(const String& filename, const AbstractNodeListPtr& ast);
		AbstractNodeListPtr loadAstFromDisk(const String& filename);
		time_t getBinaryTimeStamp(const String& filename);

	private:
		ScriptSerializer* mSerializer;
		ScriptCompiler* mCompiler;
		String mActiveScriptName;
		String mActiveResourceGroup;
		Archive* mCacheArchive;
	};

}