#include "ScriptSerializerPreCompiled.h"
#include "ScriptSerializer.h"
#include "OgreScriptCompiler.h"
#include <iostream>
#include <sstream>
using namespace Ogre::ScriptBlock;
using namespace std;

#define serializer_cast static_cast

namespace Ogre {

	const uint32 magicCode = ('O' | 'G' << 8 | 'R' << 16 | 'E' << 24 );

	ScriptSerializer::ScriptSerializer(void) : blockIdCounter(0) {
		stringTable = OGRE_NEW StringTable();
	}

	ScriptSerializer::~ScriptSerializer(void)
	{
		OGRE_DELETE stringTable;
	}

	void ScriptSerializer::serialize(const DataStreamPtr& stream, const AbstractNodeListPtr& ast) {
		blockIdCounter = 0;
		stringTable->clear();

		ScriptHeader header;
		header.magic = magicCode;
		header.stringTableOffset = 0;	// Will be overwritten later
		writeToStream(stream, header);

		SerializeStack s;
		writeStackChildren(s, *ast);

		// Traverse all the trees and their nodes in depth-first order
		while (!s.empty()) {
			BlockEntry* entry = s.top();
			s.pop();

			writeBlock(stream, entry);

			// Add child nodes
			if (entry->blockClass == BC_Node) {
				NodeBlockEntry* nodeEntry = serializer_cast<NodeBlockEntry*>(entry);
				const AbstractNodePtr& node = nodeEntry->node;
				if (node->type == ANT_PROPERTY) {
					PropertyAbstractNode* propertyNode = serializer_cast<PropertyAbstractNode*>(node.get());
					writeStackChildren(s, propertyNode->values);
				}
				else if (node->type == ANT_OBJECT) {
					ObjectAbstractNode* objectNode = serializer_cast<ObjectAbstractNode*>(node.get());
					writeStackChildren(s, objectNode->children, OATT_Children);
					writeStackChildren(s, objectNode->values, OATT_Values);
					writeStackChildren(s, objectNode->overrides, OATT_Overrides);
				}
			}

			OGRE_DELETE entry;
		}

		// Write the string table
		header.stringTableOffset = stream->tell();
		writeStringTable(stream);

		// Re-write the header with the correct string table offset
		stream->seek(0);
		writeToStream(stream, header);
	}

	void ScriptSerializer::writeStackChildren(SerializeStack& s, AbstractNodeList& children, int transitionUserdata) {
		BlockEntryList entries;
		entries.push_back(OGRE_NEW TransitionBlockEntry(TTD_Down, transitionUserdata));
		for(AbstractNodeList::iterator i = children.begin(); i != children.end(); ++i) {
			BlockEntry* entry = OGRE_NEW NodeBlockEntry(*i);
			entries.push_back(entry);
		}
		entries.push_back(OGRE_NEW TransitionBlockEntry(TTD_Up, transitionUserdata));

		// reverse the entries before inserting them to the stack to preserve correct ordering
		std::reverse(entries.begin(), entries.end());
		for(BlockEntryList::iterator it = entries.begin(); it != entries.end(); it++) {
			s.push(*it);
		}
	}

	void ScriptSerializer::writeBlock(const DataStreamPtr& stream, BlockEntry* entry) {
		if (entry->blockClass == BC_Transition) {
			TransitionBlockEntry* transitionEntry = serializer_cast<TransitionBlockEntry*>(entry);
			TransitionBlock block;
			block.header.blockID = ++blockIdCounter;
			block.header.blockClass = BC_Transition;
			block.header.blockType = 0;		// Not used
			block.direction = transitionEntry->direction;
			block.userData = transitionEntry->userData;
			writeToStream(stream, block);
		}
		else if (entry->blockClass == BC_Node) {
			NodeBlockEntry* nodeEntry = serializer_cast<NodeBlockEntry*>(entry);
			AbstractNodePtr node = nodeEntry->node;
			if (node->type == ANT_ATOM) {
				AtomAbstractNode* atomNode = serializer_cast<AtomAbstractNode*>(node.get());
				AtomAbstractNodeBlock block;
				block.nodeInfo.header.blockClass = BC_Node;
				block.nodeInfo.header.blockType = ANT_ATOM;
				block.nodeInfo.header.blockID = ++blockIdCounter;
				block.nodeInfo.lineNumber = atomNode->line;
				block.id = atomNode->id;
				block.value = stringTable->registerString(atomNode->value);
				writeToStream(stream, block);
			}
			else if (node->type == ANT_PROPERTY) {
				PropertyAbstractNode* propertyNode = serializer_cast<PropertyAbstractNode*>(node.get());
				PropertyAbstractNodeBlock block;
				block.nodeInfo.header.blockClass = BC_Node;
				block.nodeInfo.header.blockType = ANT_PROPERTY;
				block.nodeInfo.header.blockID = ++blockIdCounter;
				block.nodeInfo.lineNumber = propertyNode->line;
				block.id = propertyNode->id;
				block.name = stringTable->registerString(propertyNode->name);
				writeToStream(stream, block);
			}
			else if (node->type == ANT_OBJECT) {
				ObjectAbstractNode* objectNode = serializer_cast<ObjectAbstractNode*>(node.get());
				ObjectAbstractNodeBlock block;
				block.nodeInfo.header.blockClass = BC_Node;
				block.nodeInfo.header.blockType = ANT_OBJECT;
				block.nodeInfo.header.blockID = ++blockIdCounter;
				block.nodeInfo.lineNumber = objectNode->line;
				block.name = stringTable->registerString(objectNode->name);
				block.cls = stringTable->registerString(objectNode->cls);
				block.id = objectNode->id;
				block.abstract = objectNode->abstract;
				block.bases.count = objectNode->bases.size();
				block.environmentVars.count = objectNode->getVariables().size();
				writeToStream(stream, block);

				// Write out the "bases" list
				for(std::vector<String>::iterator it = objectNode->bases.begin(); it != objectNode->bases.end(); it++) {
					ResourceID id = stringTable->registerString(*it);
					writeToStream(stream, id);
				}

				// Write the environment variables
				for(map<String,String>::type::const_iterator i = objectNode->getVariables().begin(); i != objectNode->getVariables().end(); ++i) {
					ResourceID keyID = stringTable->registerString(i->first);
					ResourceID valueID = stringTable->registerString(i->second);
					writeToStream(stream, keyID);
					writeToStream(stream, valueID);
				}
			}
		}
		else {
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Cannot serialize block of type:" + entry->blockClass, "ScriptSerializer::writeBlock");
		}
	}

	
	AbstractNodeListPtr ScriptSerializer::deserialize(const DataStreamPtr& stream) {
		AbstractNodeListPtr trees = AbstractNodeListPtr(OGRE_NEW_T(AbstractNodeList, MEMCATEGORY_GENERAL)(), SPFM_DELETE_T);
		
		ScriptHeader header;
		readFromStream(stream, header);

		if (header.magic != magicCode) {
			OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Binary file is not in correct format: " + stream->getName(), "ScriptSerializer::deserialize");
		}

		// Seek to the string table 
		stream->seek(header.stringTableOffset);
		stringTable->clear();
		readStringTable(stream);

		// Seek back to the start for reading the node data
		stream->seek(sizeof(header));
		
		typedef std::pair<AbstractNode*, int> ParentEntry;
		typedef std::stack<ParentEntry> ParentStack;

		ParentStack parentStack;
		AbstractNode* previousNode = 0;

		while (true) {
			ScriptBlockHeader blockHeader;
			readFromStream(stream, blockHeader);
			int headerSize = sizeof(blockHeader);
			stream->skip(-headerSize);

			if (blockHeader.blockClass == BC_Transition) {
				TransitionBlock block;
				readFromStream(stream, block);

				if (block.direction == TTD_Down) {
					parentStack.push(ParentEntry(previousNode, block.userData));
				}
				else if (block.direction == TTD_Up) {
					previousNode = parentStack.top().first;
					parentStack.pop();
				}
				else {
					OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "Unsupported direction type", "ScriptSerializer::deserialize");
				}

			} 
			else if (blockHeader.blockClass == BC_Node) {
				AbstractNode* parent = parentStack.top().first;
				AbstractNodePtr asn;

				if (blockHeader.blockType == ANT_ATOM) {
					AtomAbstractNodeBlock block;
					readFromStream(stream, block);

					AtomAbstractNode *impl = OGRE_NEW AtomAbstractNode(parent);
					impl->file = stream->getName();
					impl->line = block.nodeInfo.lineNumber;
					impl->value = stringTable->getString(block.value);
					impl->id = block.id;
					previousNode = impl;
					asn = AbstractNodePtr(impl);
				}
				else if (blockHeader.blockType == ANT_PROPERTY) {
					PropertyAbstractNodeBlock block;
					readFromStream(stream, block);

					PropertyAbstractNode* impl = OGRE_NEW PropertyAbstractNode(parent);
					impl->file = stream->getName();
					impl->line = block.nodeInfo.lineNumber;
					impl->name = stringTable->getString(block.name);
					impl->id = block.id;
					previousNode = impl;
					asn = AbstractNodePtr(impl);
				}
				else if (blockHeader.blockType = ANT_OBJECT) {
					ObjectAbstractNodeBlock block;
					readFromStream(stream, block);

					ObjectAbstractNode* impl = OGRE_NEW ObjectAbstractNode(parent);
					impl->file = stream->getName();
					impl->line = block.nodeInfo.lineNumber;
					impl->name = stringTable->getString(block.name);
					impl->cls = stringTable->getString(block.cls);
					impl->id = block.id;
					impl->abstract = block.abstract;

					uint64 baseCount = block.bases.count;
					uint64 envCount = block.environmentVars.count;

					for (int i = 0; i < baseCount; i++) {
						ResourceID id;
						readFromStream(stream, id);
						String base = stringTable->getString(id);
						impl->bases.push_back(base);
					}

					for (int i = 0; i < envCount; i++) {
						ResourceID keyId, valueId;
						readFromStream(stream, keyId);
						readFromStream(stream, valueId);

						String key = stringTable->getString(keyId);
						String value = stringTable->getString(valueId);

						impl->setVariable(key, value);
					}

					previousNode = impl;
					asn = AbstractNodePtr(impl);
				}

				// Attach the node to the parent's appropriate child list
				if (parent) {
					if (parent->type == ANT_PROPERTY) {
						PropertyAbstractNode* propertyNode = serializer_cast<PropertyAbstractNode*>(parent);
						propertyNode->values.push_back(asn);
					}
					else if (parent->type == ANT_OBJECT) {
						ObjectAbstractNode* objectNode = serializer_cast<ObjectAbstractNode*>(parent);
						int userData = parentStack.top().second;
						switch(userData) {
						case OATT_Children:
							objectNode->children.push_back(asn);
							break;

						case OATT_Overrides:
							objectNode->overrides.push_back(asn);
							break;

						case OATT_Values:
							objectNode->values.push_back(asn);
							break;

						default:
							OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "Unsupported child type", "ScriptSerializer::deserialize");
						}
					}
				} 
				else {
					// This node has no parent and is the root node
					trees->push_back(asn);
				}
			}
			else if (blockHeader.blockClass == BC_StringTable) {
				break;
			}
		}

		return trees;
	}

	void ScriptSerializer::writeStringTable(const DataStreamPtr& stream) {
		StringTableBlock block;
		block.header.blockID = ++blockIdCounter;
		block.header.blockClass = BC_StringTable;
		block.header.blockType = 0;		// Not used
		StringTable::ResourceTable table = stringTable->getTable();
		block.count = table.size();
		writeToStream(stream, block);

		for (StringTable::ResourceTable::iterator it = table.begin(); it != table.end(); it++) {
			String data = it->first;
			ResourceID id = it->second;
			uint32 length = serializer_cast<uint32>(data.length());

			writeToStream(stream, id);
			writeToStream(stream, length);
			stream->write(data.c_str(), length);
		}
	}

	
	void ScriptSerializer::readStringTable(const DataStreamPtr& stream) {
		StringTableBlock block;
		readFromStream(stream, block);
		if (block.header.blockClass != BC_StringTable) {
			OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "Error reading String Table", "ScriptSerializer::readStringTable");
		}

		for (int i = 0; i < block.count; i++) {
			ResourceID id;
			uint32 length;

			readFromStream(stream, id);
			readFromStream(stream, length);

			char *buffer = new char[length + 1];
			stream->read(buffer, length);
			buffer[length] = 0;		// String null terminator
			String value = String(buffer);
			stringTable->setKeyValue(id, value);
			delete buffer;
		}
	}


	ScriptBlock::BlockEntry* ScriptSerializer::readBlock(const DataStreamPtr& stream) {
		
		return 0;
	}

	template<typename T> 
	void ScriptSerializer::writeToStream(const DataStreamPtr& stream, T& t) {
		stream->write(reinterpret_cast<const char*>(&t), sizeof(T));
	}

	template<typename T> 
	void ScriptSerializer::readFromStream(const DataStreamPtr& stream, T& t) {
		stream->read(reinterpret_cast<char*>(&t), sizeof(T));
	}


	StringTable::StringTable() : idCounter(0) {
	}

	ResourceID StringTable::registerString(const String& data) {
		if (table.count(data)) {
			return table[data];
		}

		ResourceID id = ++idCounter;
		table[data] = id;
		reverseLookup[id] = data;
		return id;
	}

	String StringTable::getString(ResourceID id) {
		if (!reverseLookup.count(id)) {
			OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, "Cannot find resource string with specified id", "StringTable::getString");
		}
		return reverseLookup[id];
	}

	void StringTable::clear() {
		table.clear();
		reverseLookup.clear();
		idCounter = 0;
	}

	void StringTable::setKeyValue(ResourceID id, const String& data) {
		table[data] = id;
		reverseLookup[id] = data;
	}

}