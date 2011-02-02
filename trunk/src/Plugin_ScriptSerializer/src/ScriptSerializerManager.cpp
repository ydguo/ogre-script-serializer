#include "ScriptSerializerPreCompiled.h"
#include "ScriptSerializerManager.h"
#include "ScriptSerializer.h"
#include "ShaderSerializer.h"
#include "OgreScriptTranslator.h"
#include "OgreZip.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h>
#include <sstream>

using namespace std;

namespace Ogre {

	/** The filename of the config file */
	const String configFileName = "ScriptCache.cfg";

	ScriptSerializerManager::ScriptSerializerManager() : mCompiler(0)
	{
		initializeConfig(configFileName);
		bool pluginEnabled = initializeArchive(scriptCacheLocation);
		if (pluginEnabled) {
			mCompiler = OGRE_NEW ScriptCompiler();
			initializeShaderCache();
			ResourceGroupManager::getSingleton().addResourceGroupListener(this);
			ScriptCompilerManager::getSingleton().setListener(this);
		}
	}

	ScriptSerializerManager::~ScriptSerializerManager(void)
	{
		if (pluginEnabled) {
			Ogre::ResourceGroupManager::getSingleton().removeResourceGroupListener(this);
			if (this == Ogre::ScriptCompilerManager::getSingleton().getListener()) {
				Ogre::ScriptCompilerManager::getSingleton().setListener(0);
			}
			mCacheArchive->unload();
			OGRE_DELETE mCompiler;
		}
	}

	bool ScriptSerializerManager::initializeArchive(const String& archiveName) {
		// Check if the directory exists
		struct stat dirInfo;
		int status = stat(archiveName.c_str(), &dirInfo);
		if (status) {
			// Directory does not exist. Create one
			int error = mkdir(archiveName.c_str());
			if (error) {
				LogManager::getSingleton().logMessage("WARNING: Failed to create Script Cache directory.  Script cache plugin disabled");
				return false;
			}
		}

		mCacheArchive = ArchiveManager::getSingleton().load(archiveName, "FileSystem");
		
		if (mCacheArchive->isReadOnly()) {
			LogManager::getSingleton().logMessage("WARNING: Failed to write to Script Cache directory.  Script Cache plugin disabled");
			return false;
		}

		return true;
	}
	
	void ScriptSerializerManager::initializeShaderCache() {
#ifdef USE_MICROCODE_SHADERCACHE
		mShaderSerializer = OGRE_NEW ShaderSerializer();

		// Check if we the shaders have previously been cached
		if (mCacheArchive->exists(shaderCacheFilename)) {
			// A previously cached shader file exists.  load it
			DataStreamPtr shaderCache = mCacheArchive->open(shaderCacheFilename);
			mShaderSerializer->loadCache(shaderCache);
			shaderCache->close();
		}

		// Start cache new shader code
		mShaderSerializer->setCaching(true);
#endif
	}

	void ScriptSerializerManager::saveShaderCache() {
#ifdef USE_MICROCODE_SHADERCACHE
		// A previously cached shader file exists.  load it
		DataStreamPtr shaderCache = mCacheArchive->create(shaderCacheFilename);
		mShaderSerializer->saveCache(shaderCache);
		shaderCache->close();
#endif
	}
	
	void ScriptSerializerManager::resourceGroupScriptingStarted(const String& groupName, size_t scriptCount) {
		mActiveResourceGroup = groupName;
	}

	void ScriptSerializerManager::resourceGroupScriptingEnded(const String& groupName) {
		// Scripts for this resource group where just parsed.  save the shader cache to disk
		saveShaderCache();
	}


	void ScriptSerializerManager::scriptParseStarted(const String& scriptName, bool& skipThisScript) {
		// Check if the binary version is being requested for parsing
		String binaryFilename;
		if (isBinaryScript(scriptName)) {
			// The script ends with the binary extension. fetch it from the cache folder
			binaryFilename = scriptName;
		}
		else {
			// Clear compilation error flags, if any.  This script might have been re-parsed after corrections
			invalidScripts.erase(scriptName);

			// This is a text based script.  Check if the compiled version is unavailable
			binaryFilename = scriptName + binaryScriptExtension;
			if (!mCacheArchive->exists(binaryFilename)) {
				// A compiled version of this script doesn't exist in the cache.  Continue with regular text parsing
				skipThisScript = false;
				return;
			}

			// Check if this script was modified it was last compiled
			size_t binaryTimestamp = getBinaryTimeStamp(binaryFilename);
			size_t scriptTimestamp = ResourceGroupManager::getSingleton().resourceModifiedTime(mActiveResourceGroup, scriptName);

			if (scriptTimestamp > binaryTimestamp) {
				LogManager::getSingleton().logMessage("File Changed. Re-parsing file: " + scriptName);
				skipThisScript = false;
				return;
			}
		}

		// Load the compiled AST from the binary script file
		AbstractNodeListPtr ast = loadAstFromDisk(binaryFilename);
		LogManager::getSingleton().logMessage("Processing binary script: " + binaryFilename);
		mCompiler->_compile(ast, mActiveResourceGroup, false, false, false);

		// Skip further parsing of this script since its already been compiled
		skipThisScript = true;
	}
	
	bool ScriptSerializerManager::postConversion(ScriptCompiler *compiler, const AbstractNodeListPtr& ast) {
		String scriptName = ast->front()->file;

		// Cache this AST only if there were no compilation errors
		bool isValid = (invalidScripts.count(scriptName) == 0);
		if (isValid) {
			if (!isBinaryScript(scriptName)) {
				// A text script was just parsed. Save the compiled AST to disk
				String binaryFilename = scriptName + binaryScriptExtension;
				size_t scriptTimestamp = ResourceGroupManager::getSingleton().resourceModifiedTime(mActiveResourceGroup, scriptName);
				saveAstToDisk(binaryFilename, scriptTimestamp, ast);
			}
		}

		bool continueParsing = true;
		return continueParsing;
	}

	
	void ScriptSerializerManager::handleError(ScriptCompiler *compiler, uint32 code, const String &file, int line, const String &msg) {
		invalidScripts.insert(file);
	}

	time_t ScriptSerializerManager::getBinaryTimeStamp(const String& filename) {
		DataStreamPtr stream = mCacheArchive->open(filename);
		ScriptBlock::ScriptHeader header;
		stream->read(reinterpret_cast<char*>(&header), sizeof(ScriptBlock::ScriptHeader));
		stream->close();
		return header.lastModifiedTime;
	}

	void ScriptSerializerManager::saveAstToDisk(const String& filename, size_t scriptTimestamp, const AbstractNodeListPtr& ast) {
		// A text script was just parsed. Save the compiled AST to disk
		DataStreamPtr stream = mCacheArchive->create(filename);
		ScriptSerializer* serializer = OGRE_NEW ScriptSerializer();
		serializer->serialize(stream, ast, scriptTimestamp);
		OGRE_DELETE serializer;
		stream->close();
	}

	AbstractNodeListPtr ScriptSerializerManager::loadAstFromDisk(const String& filename) {
		DataStreamPtr stream = mCacheArchive->open(filename);
		ScriptSerializer* serializer = OGRE_NEW ScriptSerializer();
		AbstractNodeListPtr ast = serializer->deserialize(stream);
		OGRE_DELETE serializer;
		stream->close();
		return ast;
	}

	bool ScriptSerializerManager::isBinaryScript(const String& filename) {
		size_t extensionIndex = filename.find_last_of(".");
		if (extensionIndex != string::npos) {
			String extension = filename.substr(extensionIndex);
			return (extension == binaryScriptExtension);
		}
		return false;
	}

	
	void ScriptSerializerManager::initializeConfig(const String& configFileName) {
		ConfigFile configFile;
		Archive* workingDirectory =  ArchiveManager::getSingleton().load(".", "FileSystem");
		if (workingDirectory->exists(configFileName)) {
			DataStreamPtr configStream = workingDirectory->open(configFileName);
			configFile.load(configStream);
			configStream->close();
		}
		else {
			stringstream message;
			message << "WARNING: Cannot find Script Cache config file:" << configFileName << ". Using default values";
			LogManager::getSingleton().logMessage(message.str());
		}

		binaryScriptExtension = configFile.getSetting("extension", "ScriptCache", ".sbin");
		scriptCacheLocation = configFile.getSetting("location", "ScriptCache", ".scriptCache");
		shaderCacheFilename = configFile.getSetting("filename", "ShaderCache", "ShaderCache");
		String searchExtensions = configFile.getSetting("searchExtensions", "ScriptCache", "program material particle compositor os pu");

		istringstream extensions(searchExtensions);
		String extension;
		while (extensions >> extension) {
			stringstream pattern;
			pattern << "*." << extension << binaryScriptExtension;
			ScriptCompilerManager::getSingleton().addScriptPattern(pattern.str());
		}
	}
}