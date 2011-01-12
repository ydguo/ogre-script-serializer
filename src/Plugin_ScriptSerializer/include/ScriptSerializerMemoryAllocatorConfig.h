#pragma once
#include "OgreMemoryAllocatedObject.h"

namespace Ogre {
	typedef CategorisedAllocPolicy<Ogre::MEMCATEGORY_SCRIPTING> ScriptSerializerAllocPolicy;
	typedef CategorisedAllocPolicy<Ogre::MEMCATEGORY_SCRIPTING> ScriptSerializerManagerAllocPolicy;

	typedef AllocatedObject<ScriptSerializerAllocPolicy> ScriptSerializerAllocatedObject;
	typedef AllocatedObject<ScriptSerializerManagerAllocPolicy> ScriptSerializerManagerAllocatedObject;

	typedef ScriptSerializerAllocatedObject	ScriptSerializerAlloc;
	typedef ScriptSerializerManagerAllocatedObject	ScriptSerializerManagerAlloc;

}