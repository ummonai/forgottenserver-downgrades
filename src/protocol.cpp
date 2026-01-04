// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "protocol.h"

#include "outputmessage.h"
#include "rsa.h"
#include "xtea.h"

namespace {

void XTEA_encrypt(OutputMessage& msg, const xtea::round_keys& key)
{
	// The message must be a multiple of 8
	size_t paddingBytes = msg.getLength() % 8u;
	if (paddingBytes != 0) {
		msg.addPaddingBytes(8 - paddingBytes);
	}

	uint8_t* buffer = msg.getOutputBuffer();
	xtea::encrypt(buffer, msg.getLength(), key);
}

bool XTEA_decrypt(NetworkMessage& msg, const xtea::round_keys& key)
{
	std::cout << "XTEA_decrypt" << std::endl;
	std::cout << "msg.getLength(): " << msg.getLength() << std::endl;

	if (((msg.getLength() - 2) & 7) != 0) {
	//if (((msg.getLength() - 6) & 7) != 0) {
	//if (((msg.getLength() - 2) & 7) != 0) {
		return false;
	}

	uint8_t* buffer = msg.getRemainingBuffer();
	xtea::decrypt(buffer, msg.getLength() - 2, key);
	//xtea::decrypt(buffer, msg.getLength() - 2, key);
	//xtea::decrypt(buffer, msg.getLength() - 4, key);

	uint16_t innerLength = msg.get<uint16_t>();
	std::cout << "innerLength: " << innerLength << std::endl;
	//if (innerLength + 8 > msg.getLength()) {
	//if (innerLength > msg.getLength() - 4) {
	if (innerLength + 4 > msg.getLength()) {
		return false;
	}

	msg.setLength(innerLength);
	return true;
}

} // namespace

void Protocol::onSendMessage(const OutputMessage_ptr& msg)
{
	std::cout << "Protocol::onSendMessage" << std::endl;
	if (!rawMessages) {
		msg->writeMessageLength();

		// print unencrypted
		std::cout << "Protocol::onSendMessage packet: ";
		for(uint32_t i=0; i<msg->getLength(); ++i)
		{
			std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(*(msg->getBuffer() + i)) << " ";
		}
		std::cout << std::dec << std::endl;
		// --

		if (encryptionEnabled) {
			XTEA_encrypt(*msg, key);
			msg->addCryptoHeader(checksumMode, sequenceNumber);
		}
	}
}

void Protocol::onRecvMessage(NetworkMessage& msg)
{
	std::cout << "Protocol::onRecvMessage" << std::endl;
	if (encryptionEnabled && !XTEA_decrypt(msg, key)) {
		std::cout << "Protocol::onRecvMessage decryption disabled or failed" << std::endl;
		return;
	}

	// print decrypted
	std::cout << "Protocol::onRecvMessage packet: ";
	for(uint32_t i=0; i<msg.getLength(); ++i)
	{
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(*(msg.getBuffer() + i)) << " ";
	}
	std::cout << std::dec << std::endl;
	// --

	parsePacket(msg);
}

OutputMessage_ptr Protocol::getOutputBuffer(int32_t size)
{
	// dispatcher thread
	if (!outputBuffer) {
		outputBuffer = OutputMessagePool::getOutputMessage();
	} else if ((outputBuffer->getLength() + size) > NetworkMessage::MAX_PROTOCOL_BODY_LENGTH) {
		send(outputBuffer);
		outputBuffer = OutputMessagePool::getOutputMessage();
	}
	return outputBuffer;
}

bool Protocol::RSA_decrypt(NetworkMessage& msg)
{
	if (msg.getRemainingBufferLength() < RSA_BUFFER_LENGTH) {
		std::cout << "Protocol::RSA_decrypt - not enough data in packet" << std::endl;
		return false;
	}

	tfs::rsa::decrypt(msg.getRemainingBuffer(), RSA_BUFFER_LENGTH);
	std::cout << "Protocol::RSA_decrypt - post decrypt, should be 0 terminated" << std::endl;
	return msg.getByte() == 0;
}

Connection::Address Protocol::getIP() const
{
	if (auto connection = getConnection()) {
		return connection->getIP();
	}

	return {};
}
