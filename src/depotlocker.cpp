// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "depotlocker.h"

#include "inbox.h"
#include "tools.h"

DepotLocker::DepotLocker(uint16_t type) : Container(type), depotId(0) {}

Attr_ReadValue DepotLocker::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if (attr == ATTR_DEPOT_ID) {
		if (!propStream.read<uint16_t>(depotId)) {
			return ATTR_READ_ERROR;
		}
		return ATTR_READ_CONTINUE;
	}
	return Item::readAttr(attr, propStream);
}

ReturnValue DepotLocker::queryAdd(int32_t, const Thing&, uint32_t, uint32_t flags, Creature*) const
{
	// items can not be put into the locker itself, but into the depot chest inside it (the chest asks its top parent)
	if (hasBitSet(FLAG_CHILDISOWNER, flags)) {
		return RETURNVALUE_NOERROR;
	}
	return RETURNVALUE_NOTENOUGHROOM;
}

void DepotLocker::postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t)
{
	if (parent) {
		parent->postAddNotification(thing, oldParent, index, LINK_PARENT);
	}
}

void DepotLocker::postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, cylinderlink_t)
{
	if (parent) {
		parent->postRemoveNotification(thing, newParent, index, LINK_PARENT);
	}
}

void DepotLocker::removeInbox(Inbox* inbox)
{
	auto cit = std::find(itemlist.begin(), itemlist.end(), inbox);
	if (cit == itemlist.end()) {
		return;
	}
	itemlist.erase(cit);
}
