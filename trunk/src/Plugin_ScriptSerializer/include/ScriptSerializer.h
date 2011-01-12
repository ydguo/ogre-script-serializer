#pragma once
#include "OgreScriptCompiler.h"
#include <stack>

namespace Ogre {

	namespace ScriptBlock {
		class StringTable;
		struct BlockEntry;
		typedef std::list<BlockEntry*> BlockEntryList;
		typedef std::stack<BlockEntry*> SerializeStack;
	}
	

	class ScriptSerializer : public ScriptSerializerAlloc
	{
	public:
		ScriptSerializer(void);
		~ScriptSerializer(void);

		void serialize(const DataStreamPtr& stream, const AbstractNodeListPtr& ast);
		AbstractNodeListPtr deserialize(const DataStreamPtr& stream);


	private:
		void writeBlock(const DataStreamPtr& stream, ScriptBlock::BlockEntry* entry);
		void writeStackChildren(ScriptBlock::SerializeStack& s, AbstractNodeList& children, int transitionUserdata = 0);
		void writeStringTable(const DataStreamPtr& stream);

		ScriptBlock::BlockEntry* readBlock(const DataStreamPtr& stream);
		void readStringTable(const DataStreamPtr& stream);

		template<typename T> 
		void writeToStream(const DataStreamPtr& stream, T& t);

		template<typename T> 
		void readFromStream(const DataStreamPtr& stream, T& t);

	private:
		uint32 blockIdCounter;
		ScriptBlock::StringTable* stringTable;
	};


	namespace ScriptBlock {

		enum ScriptBlockType {
			BC_Node			= 0x01,		// Block type containing the node data
			BC_Transition	= 0x02,		// Block type containing the tree transition data
			BC_StringTable	= 0x03
		};

		enum TreeTransitionDirection {
			TTD_Up		= 0x01,
			TTD_Down	= 0x02
		};
		
		typedef	uint32 ResourceID;

		struct ScriptHeader {
			uint32 magic;
			uint64 stringTableOffset;
		};

		struct ScriptBlockHeader {
			uint8 blockClass;
			uint32 blockType;
			uint32 blockID;
		};

		struct TransitionBlock {
			ScriptBlockHeader header;
			uint32 direction;
			uint32 userData;
		};

		struct AbstractNodeBlock {
			ScriptBlockHeader header;
			uint32 lineNumber;
		};

		struct AtomAbstractNodeBlock {
			AbstractNodeBlock nodeInfo;
			uint32 id;
			ResourceID value;
		};

		struct PropertyAbstractNodeBlock {
			AbstractNodeBlock nodeInfo;
			ResourceID name;
			uint32 id;
		};
		
		struct StringTableBlock {
			ScriptBlockHeader header;
			uint64 count;
		};

		struct ResourceArrayHeader {
			uint64 count;
		};

		struct ObjectAbstractNodeBlock {
			AbstractNodeBlock nodeInfo;
			ResourceID name;
			ResourceID cls;
			uint32 id;
			bool abstract;
			ResourceArrayHeader bases;
			ResourceArrayHeader environmentVars;
		};

		enum ObjectASNTransitionType {
			OATT_Children	= 0x01,
			OATT_Values		= 0x02,
			OATT_Overrides	= 0x03
		};

		struct BlockEntry : public ScriptCompilerAlloc {
			int blockClass;
		};

		struct NodeBlockEntry : BlockEntry {
			NodeBlockEntry(const AbstractNodePtr& node) : node(node) { 
				blockClass = BC_Node;
			}

			const AbstractNodePtr& node;
		};

		struct TransitionBlockEntry : BlockEntry {
			TransitionBlockEntry(int direction, int userData) : direction(direction), userData(userData) {
				blockClass = BC_Transition;
			}
			uint32 direction;
			uint32 userData;
		};

		class StringTable : public ScriptCompilerAlloc {
		public:
			StringTable();
			ResourceID registerString(const String& data);
			void setKeyValue(ResourceID id, const String& data);
			String getString(ResourceID id);
			void clear();

			typedef std::map<ResourceID, String> ReverseLookupTable;
			typedef std::map<String, ResourceID> ResourceTable;
			ResourceTable& getTable() { return table; }

		private:
			ReverseLookupTable reverseLookup;
			ResourceTable table;
			int idCounter;
		};

	}
}
