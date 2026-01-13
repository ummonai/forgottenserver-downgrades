// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "networkmessage.h"

#include "container.h"

#include <boost/locale.hpp>

std::string NetworkMessage::getString(uint16_t stringLen /* = 0*/)
{
	if (stringLen == 0) {
		stringLen = get<uint16_t>();
	}

	if (!canRead(stringLen)) {
		return {};
	}

	auto it = buffer.data() + info.position;
	info.position += stringLen;

	std::string_view latin1Str{reinterpret_cast<char*>(it), stringLen};
	return boost::locale::conv::to_utf<char>(latin1Str.data(), latin1Str.data() + latin1Str.size(), "ISO-8859-1",
	                                         boost::locale::conv::skip);
}

Position NetworkMessage::getPosition()
{
	Position pos;
	pos.x = get<uint16_t>();
	pos.y = get<uint16_t>();
	pos.z = getByte();
	return pos;
}

void NetworkMessage::addString(std::string_view value)
{
	std::cout << "addString: " << value << std::endl;
	std::string latin1Str = boost::locale::conv::from_utf<char>(value.data(), value.data() + value.size(), "ISO-8859-1",
	                                                            boost::locale::conv::skip);
	size_t stringLen = latin1Str.size();
	if (!canAdd(stringLen + 2) || stringLen > 8192) {
		return;
	}

	add<uint16_t>(stringLen);
	std::memcpy(buffer.data() + info.position, latin1Str.data(), stringLen);
	info.position += stringLen;
	info.length += stringLen;
}

void NetworkMessage::addDouble(double value, uint8_t precision /* = 2*/)
{
	std::cout << "addDouble: " << value << std::endl;
	addByte(precision);
	add<uint32_t>(static_cast<uint32_t>((value * std::pow(static_cast<float>(10), precision)) +
	                                    std::numeric_limits<int32_t>::max()));
}

void NetworkMessage::addBytes(const char* bytes, size_t size)
{
	std::cout << "addBytes: size " << size << std::endl;
	if (!canAdd(size) || size > 8192) {
		return;
	}

	std::memcpy(buffer.data() + info.position, bytes, size);
	info.position += size;
	info.length += size;
}

void NetworkMessage::addPaddingBytes(size_t n)
{
	std::cout << "addPaddingBytes:";
	for(uint32_t i=0; i<n; ++i) {
		std::cout << "33 ";
	}
	std::cout << std::endl;

	if (!canAdd(n)) {
		return;
	}

	std::fill_n(buffer.data() + info.position, n, 0x33);
	info.length += n;
}

void NetworkMessage::addPosition(const Position& pos)
{
	std::cout << "addPosition: " << std::hex << std::setw(4) << std::setfill('0') << static_cast<uint32_t>(pos.x) << " " << static_cast<uint32_t>(pos.y) << std::setw(2) << " " << static_cast<uint32_t>(pos.z) << std::dec << std::endl;
	add<uint16_t>(pos.x);
	add<uint16_t>(pos.y);
	addByte(pos.z);
}

void NetworkMessage::addItem(uint16_t id, uint8_t count)
{
	const ItemType& it = Item::items[id];

	std::cout << "addItem: " << std::hex << std::setw(4) << std::setfill('0') << static_cast<uint32_t>(it.clientId) << std::dec << std::endl;

	add<uint16_t>(it.clientId);

	if (it.stackable) {
		std::cout << "addItem it.stackable true" << std::endl;
		addByte(count);
	}/* else if (it.isSplash() || it.isFluidContainer()) {
		addByte(fluidMap[count & 7]);
	} */
}

void NetworkMessage::addItem(const Item* item)
{
	const ItemType& it = Item::items[item->getID()];

	std::cout << "addItem: " << std::hex << std::setw(4) << std::setfill('0') << static_cast<uint32_t>(it.clientId) << std::dec << std::endl;

	add<uint16_t>(it.clientId);

	/*
	if (it.stackable) {
		addByte(std::min<uint16_t>(0xFF, item->getItemCount()));
	} else if (it.isSplash() || it.isFluidContainer()) {
		addByte(fluidMap[item->getFluidType() & 7]);
	}*/

	if (it.stackable) {
		std::cout << "addItem it.stackable true" << std::endl;
		addByte(std::min<uint16_t>(0xFF, item->getItemCount()));
	} else if (it.isSplash() || it.isFluidContainer()) {
		std::cout << "addItem it.isSplash() || it.isFluidContainer() true" << std::endl;
		addByte(item->getSubType());
	}
}

void NetworkMessage::addItemId(uint16_t itemId) { 
	std::cout << "addItem: " << std::hex << std::setw(4) << std::setfill('0') << static_cast<uint32_t>(itemId) << std::dec << std::endl;
	add<uint16_t>(Item::items[itemId].clientId); 
}
