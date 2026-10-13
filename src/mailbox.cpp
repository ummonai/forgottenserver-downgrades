// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "mailbox.h"

#include "game.h"
#include "depotchest.h"
#include "iologindata.h"

extern Game g_game;

ReturnValue Mailbox::queryAdd(int32_t, const Thing& thing, uint32_t, uint32_t, Creature*) const
{
	const Item* item = thing.getItem();
	if (item && Mailbox::canSend(item)) {
		return RETURNVALUE_NOERROR;
	}
	return RETURNVALUE_NOTPOSSIBLE;
}

ReturnValue Mailbox::queryMaxCount(int32_t, const Thing&, uint32_t count, uint32_t& maxQueryCount, uint32_t) const
{
	maxQueryCount = std::max<uint32_t>(1, count);
	return RETURNVALUE_NOERROR;
}

ReturnValue Mailbox::queryRemove(const Thing&, uint32_t, uint32_t, Creature* /*= nullptr */) const
{
	return RETURNVALUE_NOTPOSSIBLE;
}

Cylinder* Mailbox::queryDestination(int32_t&, const Thing&, Item**, uint32_t&) { return this; }

void Mailbox::addThing(Thing* thing) { return addThing(0, thing); }

void Mailbox::addThing(int32_t, Thing* thing)
{
	Item* item = thing->getItem();
	if (item && Mailbox::canSend(item)) {
		sendItem(item);
	}
}

void Mailbox::updateThing(Thing*, uint16_t, uint32_t)
{
	//
}

void Mailbox::replaceThing(uint32_t, Thing*)
{
	//
}

void Mailbox::removeThing(Thing*, uint32_t)
{
	//
}

void Mailbox::postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t)
{
	getParent()->postAddNotification(thing, oldParent, index, LINK_PARENT);
}

void Mailbox::postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, cylinderlink_t)
{
	getParent()->postRemoveNotification(thing, newParent, index, LINK_PARENT);
}

bool Mailbox::sendItem(Item* item) const
{
	std::string receiver, townName;
	if (!getReceiver(item, receiver, townName)) {
		return false;
	}

	/**No need to continue if its still empty**/
	if (receiver.empty()) {
		return false;
	}

	// 7.72: parcels and letters are delivered to the depot of the town written on the label
	Town* town = g_game.map.towns.getTown(townName);
	if (!town) {
		return false;
	}

	Player* player = g_game.getPlayerByName(receiver);
	if (player) {
		DepotChest* depotChest = player->getDepotChest(town->getID(), true);
		if (g_game.internalMoveItem(item->getParent(), depotChest, INDEX_WHEREEVER, item, item->getItemCount(),
		                            nullptr, FLAG_NOLIMIT) == RETURNVALUE_NOERROR) {
			g_game.transformItem(item, item->getID() + 1);
			player->onReceiveMail();
			return true;
		}
	} else {
		Player tmpPlayer(nullptr);
		if (!IOLoginData::loadPlayerByName(&tmpPlayer, receiver)) {
			return false;
		}

		DepotChest* depotChest = tmpPlayer.getDepotChest(town->getID(), true);
		if (g_game.internalMoveItem(item->getParent(), depotChest, INDEX_WHEREEVER, item, item->getItemCount(),
		                            nullptr, FLAG_NOLIMIT) == RETURNVALUE_NOERROR) {
			g_game.transformItem(item, item->getID() + 1);
			IOLoginData::savePlayer(&tmpPlayer);
			return true;
		}
	}
	return false;
}

bool Mailbox::getReceiver(Item* item, std::string& name, std::string& townName) const
{
	const Container* container = item->getContainer();
	if (container) {
		for (Item* containerItem : container->getItemList()) {
			if (containerItem->getID() == ITEM_LABEL && getReceiver(containerItem, name, townName)) {
				return true;
			}
		}
		return false;
	}

	const std::string& text = item->getText();
	if (text.empty()) {
		return false;
	}

	// first line: receiver, second line: town
	std::istringstream iss(text);
	std::getline(iss, name, '\n');
	std::getline(iss, townName, '\n');
	boost::algorithm::trim(name);
	boost::algorithm::trim(townName);
	return true;
}

bool Mailbox::canSend(const Item* item) { return item->getID() == ITEM_PARCEL || item->getID() == ITEM_LETTER; }
