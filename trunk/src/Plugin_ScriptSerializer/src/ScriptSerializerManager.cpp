#include "ScriptSerializerPreCompiled.h"
#include "ScriptSerializerManager.h"
#include "ScriptSerializer.h"
#include "OgreScriptTranslator.h"
#include "OgreZip.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <direct.h>
#include <sstream>
using namespace std;

namespace Ogre {

	/** The binary file's extension that would be appended to original filename */
	const String binaryScriptExtension = ".sbin";

	/** The folder to save the compiled scripts in binary format */
	const String scriptCacheFolder = ".scriptCache";

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
	ScriptSerializerManager::ScriptSerializerManager()
	{
		mCompiler = OGRE_NEW ScriptCompiler();
		initializeArchive(".scriptCache");
		Ogre::ResourceGroupManager::getSingleton().addResourceGroupListener(this);
		Ogre::ScriptCompilerManager::getSingleton().setListener(this);

		// Register the binary extension in the proper load order
		Ogre::ScriptCompilerManager::getSingleton().addScriptPattern("*.program" + binaryScriptExtension);
		Ogre::ScriptCompilerManager::getSingleton().addScriptPattern("*.material" + binaryScriptExtension);
		Ogre::ScriptCompilerManager::getSingleton().addScriptPattern("*.particle" + binaryScriptExtension);
		Ogre::ScriptCompilerManager::getSingleton().addScriptPattern("*.compositor" + binaryScriptExtension);
	}


	ScriptSerializerManager::~ScriptSerializerManager(void)
	{
		Ogre::ResourceGroupManager::getSingleton().removeResourceGroupListener(this);
		if (this == Ogre::ScriptCompilerManager::getSingleton().getListener()) {
			Ogre::ScriptCompilerManager::getSingleton().setListener(0);
		}
		mCacheArchive->unload();
		OGRE_DELETE mCompiler;
	}

	void ScriptSerializerManager::initializeArchive(const String& archiveName) {
		// Check if the directory exists
		struct stat dirInfo;
		int status = stat(archiveName.c_str(), &dirInfo);
		if (status) {
			// Directory does not exist. Create one
			mkdir(archiveName.c_str());
		}

		mCacheArchive = ArchiveManager::getSingleton().load(archiveName, "FileSystem");
	}

	void ScriptSerializerManager::resourceGroupScriptingStarted(const String& groupName, size_t scriptCount) {
		mActiveResourceGroup = groupName;
	}


	void ScriptSerializerManager::scriptParseStarted(const String& scriptName, bool& skipThisScript) {
		mActiveScriptName = scriptName;

		// Check if the binary version is being requested for parsing
		String binaryFilename;
		if (isBinaryScript(scriptName)) {
			// The script ends with the binary extension. fetch it from the cache folder
			binaryFilename = scriptName;
		}
		else {
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
		if (!isBinaryScript(mActiveScriptName)) {
			// A text script was just parsed. Save the compiled AST to disk
			String binaryFilename = mActiveScriptName + binaryScriptExtension;
			saveAstToDisk(binaryFilename, ast);
		}

		bool continueParsing = true;
		return continueParsing;
	}
	
	time_t ScriptSerializerManager::getBinaryTimeStamp(const String& filename) {
		DataStreamPtr stream = mCacheArchive->open(filename);
		ScriptBlock::ScriptHeader header;
		stream->read(reinterpret_cast<char*>(&header), sizeof(ScriptBlock::ScriptHeader));
		stream->close();
		return header.lastModifiedTime;
	}

	void ScriptSerializerManager::saveAstToDisk(const String& filename, const AbstractNodeListPtr& ast) {
		// A text script was just parsed. Save the compiled AST to disk
		DataStreamPtr stream = mCacheArchive->create(filename);
		ScriptSerializer* serializer = OGRE_NEW ScriptSerializer();
		size_t scriptTimestamp = ResourceGroupManager::getSingleton().resourceModifiedTime(mActiveResourceGroup, mActiveScriptName);
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

}