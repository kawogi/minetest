/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include "network/networkprotocol.h"
#include "irrlichttypes.h"

enum ModChannelState : u8
{
	MODCHANNEL_STATE_INIT,
	MODCHANNEL_STATE_READ_WRITE,
	MODCHANNEL_STATE_READ_ONLY,
	MODCHANNEL_STATE_MAX,
};

class ModChannel
{
public:
	ModChannel(const String &name) : m_name(name) {}
	~ModChannel() = default;

	const String &getName() const { return m_name; }
	bool registerConsumer(session_t peer_id);
	bool removeConsumer(session_t peer_id);
	const std::vector<u16> &getChannelPeers() const { return m_client_consumers; }
	bool canWrite() const;
	void setState(ModChannelState state);

private:
	String m_name;
	ModChannelState m_state = MODCHANNEL_STATE_INIT;
	std::vector<u16> m_client_consumers;
};

enum ModChannelSignal : u8
{
	MODCHANNEL_SIGNAL_JOIN_OK,
	MODCHANNEL_SIGNAL_JOIN_FAILURE,
	MODCHANNEL_SIGNAL_LEAVE_OK,
	MODCHANNEL_SIGNAL_LEAVE_FAILURE,
	MODCHANNEL_SIGNAL_CHANNEL_NOT_REGISTERED,
	MODCHANNEL_SIGNAL_SET_STATE,
};

class ModChannelMgr
{
public:
	ModChannelMgr() = default;
	~ModChannelMgr() = default;

	void registerChannel(const String &channel);
	bool setChannelState(const String &channel, ModChannelState state);
	bool joinChannel(const String &channel, session_t peer_id);
	bool leaveChannel(const String &channel, session_t peer_id);
	bool channelRegistered(const String &channel) const;
	ModChannel *getModChannel(const String &channel);
	/**
	 * This function check if a local mod can write on the channel
	 *
	 * @param channel
	 * @return true if write is allowed
	 */
	bool canWriteOnChannel(const String &channel) const;
	void leaveAllChannels(session_t peer_id);
	const std::vector<u16> &getChannelPeers(const String &channel) const;

private:
	bool removeChannel(const String &channel);

	std::unordered_map<String, std::unique_ptr<ModChannel>>
			m_registered_channels;
};
