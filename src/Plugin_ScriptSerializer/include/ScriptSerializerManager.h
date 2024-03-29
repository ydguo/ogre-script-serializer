#pragma once
#include "OgreResourceGroupManager.h"
#include "OgreScriptCompiler.h"
#include <set>
#include <map>

namespace Ogre {

	class ScriptSerializer;
	
	/** 
	 * This class registers and listens to various resource and compilation events and 
	 * handles loading/saving of the binary script from disk.  
	 * A cache folder is created in the working directory where the compiled scripts are dumped 
	 * to disk for later use. Whenever a text bases script is compiled, its AST in memory is saved to disk.
	 * Later when this script needs to be compiled, it would be skipped in favour of its corresponding binary version
	 * which is be directly loaded from disk and sent to the translators.
	 * The text based scripts can also be replaced with the binary version by removing them 
	 * and registering the cache folder in resources.cfg
	 */
	class ScriptSerializerManager : public ScriptSerializerManagerAlloc, public ResourceGroupListener, public ScriptCompilerListener
	{
	public:
		ScriptSerializerManager();
		~ScriptSerializerManager(void);

		/// Interface ResourceGroupListener
		virtual void scriptParseStarted(const String& scriptName, bool& skipThisScript);
		virtual void scriptParseEnded(const String& scriptName, bool skipped) { }
		virtual void resourceGroupScriptingStarted(const String& groupName, size_t scriptCount);
		virtual void resourceGroupScriptingEnded(const String& groupName);
		virtual void resourceGroupLoadStarted(const String& groupName, size_t resourceCount) { }
		virtual void resourceLoadStarted(const ResourcePtr& resource) { }
        virtual void resourceLoadEnded(void) { }
        virtual void worldGeometryStageStarted(const String& description) { }
        virtual void worldGeometryStageEnded(void) { }
        virtual void resourceGroupLoadEnded(const String& groupName) { }


		/// Interface ScriptCompilerListener
		virtual bool postConversion(ScriptCompiler *compiler, const AbstractNodeListPtr&);
		virtual void handleError(ScriptCompiler *compiler, uint32 code, const String &file, int line, const String &msg);
		

	private:
		bool initializeArchive(const String& archiveName);
		void initializeConfig(const String& configFileName);
		void initializeShaderCache();
		void saveShaderCache();
		bool isBinaryScript(const String& filename);
		void saveAstToDisk(const String& filename, size_t scriptTimestamp, const AbstractNodeListPtr& ast);
		AbstractNodeListPtr loadAstFromDisk(const String& filename);
		time_t getBinaryTimeStamp(const String& filename);

	private:
		ScriptCompiler* mCompiler;
		String mActiveResourceGroup;
		Archive* mCacheArchive;
		typedef std::set<String> InvalidScriptList;
		InvalidScriptList invalidScripts;
		String binaryScriptExtension;
		String scriptCacheLocation;
		String shaderCacheFilename;
		bool pluginEnabled;

#ifdef USE_MICROCODE_SHADERCACHE
		ShaderSerializer* mShaderSerializer;
#endif

	};

}