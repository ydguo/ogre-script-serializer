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

	/** The binary file's extension that would be appended to original filename */
	const String binaryScriptExtension = ".sbin";

	/** The folder to save the compiled scripts in binary format */
	const String scriptCacheFolder = ".scriptCache";

	/** The filename to cache the shader microcodes */
	const String shaderCacheFilename = "ShaderCache";

	ScriptSerializerManager::ScriptSerializerManager()
	{
		mCompiler = OGRE_NEW ScriptCompiler();
		initializeArchive(scriptCacheFolder);
		initializeShaderCache();
		ResourceGroupManager::getSingleton().addResourceGroupListener(this);
		ScriptCompilerManager::getSingleton().setListener(this);

		// Register the binary extension in the proper load order
		ScriptCompilerManager::getSingleton().addScriptPattern("*.program" + binaryScriptExtension);
		ScriptCompilerManager::getSingleton().addScriptPattern("*.material" + binaryScriptExtension);
		ScriptCompilerManager::getSingleton().addScriptPattern("*.particle" + binaryScriptExtension);
		ScriptCompilerManager::getSingleton().addScriptPattern("*.compositor" + binaryScriptExtension);

		
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

	
	void ScriptSerializerManager::logAST(DataStreamPtr log, int tabs, const AbstractNodePtr &node)
	{
		String msg = "";
		for(int i = 0; i < tabs; ++i)
			msg += "\t";

		switch(node->type)
		{
		case ANT_ATOM:
			{
				AtomAbstractNode *atom = reinterpret_cast<AtomAbstractNode*>(node.get());
				msg = msg + atom->value;
			}
			break;
		case ANT_PROPERTY:
			{
				PropertyAbstractNode *prop = reinterpret_cast<PropertyAbstractNode*>(node.get());
				msg = msg + prop->name + " =";
				for(AbstractNodeList::iterator i = prop->values.begin(); i != prop->values.end(); ++i)
				{
					if((*i)->type == ANT_ATOM)
						msg = msg + " " + reinterpret_cast<AtomAbstractNode*>((*i).get())->value;
				}
			}
			break;
		case ANT_OBJECT:
			{
				ObjectAbstractNode *obj = reinterpret_cast<ObjectAbstractNode*>(node.get());
				msg = msg + node->file + " - " + StringConverter::toString(node->line) + " - " + obj->cls + " \"" + obj->name + "\" =";
				for(AbstractNodeList::iterator i = obj->values.begin(); i != obj->values.end(); ++i)
				{
					if((*i)->type == ANT_ATOM)
						msg = msg + " " + reinterpret_cast<AtomAbstractNode*>((*i).get())->value;
				}
			}
			break;
		default:
			msg = msg + "Unacceptable node type: " + StringConverter::toString(node->type);
		}

		msg += "\r\n";
		log->write(msg.c_str(), msg.length());

		if(node->type == ANT_OBJECT)
		{
			ObjectAbstractNode *obj = reinterpret_cast<ObjectAbstractNode*>(node.get());
			for(AbstractNodeList::iterator i = obj->children.begin(); i != obj->children.end(); ++i)
			{
				logAST(log, tabs + 1, *i);
			}
		}
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

}