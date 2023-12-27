/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Kahrl <kahrl@gmx.net>

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

#include "irrlichttypes_extrabloated.h"
#include <string>
#include <iostream>
#include <optional>
#include <set>
#include "itemgroup.h"
#include "sound.h"
#include "texture_override.h" // TextureOverride
class IGameDef;
class Client;
struct ToolCapabilities;

/*
	Base item definition
*/

enum ItemType
{
	ITEM_NONE,
	ITEM_NODE,
	ITEM_CRAFT,
	ITEM_TOOL,
};

struct ItemDefinition
{
	/*
		Basic item properties
	*/
	ItemType type;
	String name; // "" = hand
	String description; // Shown in tooltip.
	String short_description;

	/*
		Visual properties
	*/
	String inventory_image; // Optional for nodes, mandatory for tools/craftitems
	String inventory_overlay; // Overlay of inventory_image.
	String wield_image; // If empty, inventory_image or mesh (only nodes) is used
	String wield_overlay; // Overlay of wield_image.
	String palette_image; // If specified, the item will be colorized based on this
	video::SColor color; // The fallback color of the node.
	v3f wield_scale;

	/*
		Item stack and interaction properties
	*/
	u16 stack_max;
	bool usable;
	bool liquids_pointable;
	// May be NULL. If non-NULL, deleted by destructor
	ToolCapabilities *tool_capabilities;
	ItemGroupList groups;
	SoundSpec sound_place;
	SoundSpec sound_place_failed;
	SoundSpec sound_use, sound_use_air;
	f32 range;

	// Client shall immediately place this node when player places the item.
	// Server will update the precise end result a moment later.
	// "" = no prediction
	String node_placement_prediction;
	std::optional<u8> place_param2;

	/*
		Some helpful methods
	*/
	ItemDefinition();
	ItemDefinition(const ItemDefinition &def);
	ItemDefinition& operator=(const ItemDefinition &def);
	~ItemDefinition();
	void reset();
	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is, u16 protocol_version);
private:
	void resetInitial();
};

class IItemDefManager
{
public:
	IItemDefManager() = default;

	virtual ~IItemDefManager() = default;

	// Get item definition
	virtual const ItemDefinition& get(const String &name) const=0;
	// Get alias definition
	virtual const String &getAlias(const String &name) const=0;
	// Get set of all defined item names and aliases
	virtual void getAll(std::set<String> &result) const=0;
	// Check if item is known
	virtual bool isKnown(const String &name) const=0;

	virtual void serialize(std::ostream &os, u16 protocol_version)=0;
};

class IWritableItemDefManager : public IItemDefManager
{
public:
	IWritableItemDefManager() = default;

	virtual ~IWritableItemDefManager() = default;

	// Replace the textures of registered nodes with the ones specified in
	// the texture pack's override.txt files
	virtual void applyTextureOverrides(const std::vector<TextureOverride> &overrides)=0;

	// Remove all registered item and node definitions and aliases
	// Then re-add the builtin item definitions
	virtual void clear()=0;
	// Register item definition
	virtual void registerItem(const ItemDefinition &def)=0;
	virtual void unregisterItem(const String &name)=0;
	// Set an alias so that items named <name> will load as <convert_to>.
	// Alias is not set if <name> has already been defined.
	// Alias will be removed if <name> is defined at a later point of time.
	virtual void registerAlias(const String &name,
			const String &convert_to)=0;

	virtual void deSerialize(std::istream &is, u16 protocol_version)=0;
};

IWritableItemDefManager* createItemDefManager();
