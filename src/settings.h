/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes_bloated.h"
#include "util/string.h"
#include "util/basic_macros.h"
#include <string>
#include <list>
#include <set>
#include <mutex>

class Settings;
struct NoiseParams;

// Global objects
extern Settings *g_settings; // Same as Settings::getLayer(SL_GLOBAL);
extern String g_settings_path;

// Type for a settings changed callback function
typedef void (*SettingsChangedCallback)(const String &name, void *data);

typedef std::vector<
	std::pair<
		SettingsChangedCallback,
		void *
	>
> SettingsCallbackList;

typedef std::unordered_map<String, SettingsCallbackList> SettingsCallbackMap;

enum ValueType {
	VALUETYPE_STRING,
	VALUETYPE_FLAG // Doesn't take any arguments
};

enum SettingsParseEvent {
	SPE_NONE,
	SPE_INVALID,
	SPE_COMMENT,
	SPE_KVPAIR,
	SPE_END,
	SPE_GROUP,
	SPE_MULTILINE,
};

// Describes the global setting layers, SL_GLOBAL is where settings are read from
enum SettingsLayer {
	SL_DEFAULTS,
	SL_GAME,
	SL_GLOBAL,
	SL_TOTAL_COUNT
};

// Implements the hierarchy a settings object may be part of
class SettingsHierarchy {
public:
	/*
	 * A settings object that may be part of another hierarchy can
	 * occupy the index 0 as a fallback. If not set you can use 0 on your own.
	 */
	SettingsHierarchy(Settings *fallback = nullptr);

	DISABLE_CLASS_COPY(SettingsHierarchy)

	Settings *getLayer(int layer) const;

private:
	friend class Settings;
	Settings *getParent(int layer) const;
	void onLayerCreated(int layer, Settings *obj);
	void onLayerRemoved(int layer);

	std::vector<Settings*> layers;
};

struct ValueSpec {
	ValueSpec(ValueType a_type, const char *a_help=NULL)
	{
		type = a_type;
		help = a_help;
	}

	ValueType type;
	const char *help;
};

struct SettingsEntry {
	SettingsEntry() = default;

	SettingsEntry(const String &value_) :
		value(value_)
	{}

	SettingsEntry(Settings *group_) :
		group(group_),
		is_group(true)
	{}

	String value = "";
	Settings *group = nullptr;
	bool is_group = false;
};

typedef std::unordered_map<String, SettingsEntry> SettingEntries;

class Settings {
public:
	/* These functions operate on the global hierarchy! */
	static Settings *createLayer(SettingsLayer sl, const String &end_tag = "");
	static Settings *getLayer(SettingsLayer sl);
	/**/

	Settings(const String &end_tag = "") :
		m_end_tag(end_tag)
	{}
	Settings(const String &end_tag, SettingsHierarchy *h, int settings_layer);
	~Settings();

	Settings & operator += (const Settings &other);
	Settings & operator = (const Settings &other);

	/***********************
	 * Reading and writing *
	 ***********************/

	// Read configuration file.  Returns success.
	bool readConfigFile(const char *filename);
	//Updates configuration file.  Returns success.
	bool updateConfigFile(const char *filename);
	// NOTE: Types of allowed_options are ignored.  Returns success.
	bool parseCommandLine(int argc, char *argv[],
			std::map<String, ValueSpec> &allowed_options);
	bool parseConfigLines(std::istream &is);
	void writeLines(std::ostream &os, u32 tab_depth=0) const;

	/***********
	 * Getters *
	 ***********/

	Settings *getGroup(const String &name) const;
	const String &get(const String &name) const;
	bool getBool(const String &name) const;
	u16 getU16(const String &name) const;
	s16 getS16(const String &name) const;
	u32 getU32(const String &name) const;
	s32 getS32(const String &name) const;
	u64 getU64(const String &name) const;
	float getFloat(const String &name) const;
	float getFloat(const String &name, float min, float max) const;
	v2f getV2F(const String &name) const;
	v3f getV3F(const String &name) const;
	u32 getFlagStr(const String &name, const FlagDesc *flagdesc,
			u32 *flagmask) const;
	bool getNoiseParams(const String &name, NoiseParams &np) const;
	bool getNoiseParamsFromValue(const String &name, NoiseParams &np) const;
	bool getNoiseParamsFromGroup(const String &name, NoiseParams &np) const;

	// return all keys used in this object
	std::vector<String> getNames() const;
	// check if setting exists anywhere in the hierarchy
	bool exists(const String &name) const;
	// check if setting exists in this object ("locally")
	bool existsLocal(const String &name) const;


	/***************************************
	 * Getters that don't throw exceptions *
	 ***************************************/

	bool getGroupNoEx(const String &name, Settings *&val) const;
	bool getNoEx(const String &name, String &val) const;
	bool getFlag(const String &name) const;
	bool getU16NoEx(const String &name, u16 &val) const;
	bool getS16NoEx(const String &name, s16 &val) const;
	bool getU32NoEx(const String &name, u32 &val) const;
	bool getS32NoEx(const String &name, s32 &val) const;
	bool getU64NoEx(const String &name, u64 &val) const;
	bool getFloatNoEx(const String &name, float &val) const;
	bool getV2FNoEx(const String &name, v2f &val) const;
	bool getV3FNoEx(const String &name, v3f &val) const;

	// Like other getters, but handling each flag individualy:
	// 1) Read default flags (or 0)
	// 2) Override using user-defined flags
	bool getFlagStrNoEx(const String &name, u32 &val,
		const FlagDesc *flagdesc) const;


	/***********
	 * Setters *
	 ***********/

	// N.B. Groups not allocated with new must be set to NULL in the settings
	// tree before object destruction.
	bool setEntry(const String &name, const void *entry,
		bool set_group);
	bool set(const String &name, const String &value);
	bool setDefault(const String &name, const String &value);
	bool setGroup(const String &name, const Settings &group);
	bool setBool(const String &name, bool value);
	bool setS16(const String &name, s16 value);
	bool setU16(const String &name, u16 value);
	bool setS32(const String &name, s32 value);
	bool setU64(const String &name, u64 value);
	bool setFloat(const String &name, float value);
	bool setV2F(const String &name, v2f value);
	bool setV3F(const String &name, v3f value);
	bool setFlagStr(const String &name, u32 flags,
		const FlagDesc *flagdesc = nullptr, u32 flagmask = U32_MAX);
	bool setNoiseParams(const String &name, const NoiseParams &np);

	// remove a setting
	bool remove(const String &name);

	/*****************
	 * Miscellaneous *
	 *****************/

	void setDefault(const String &name, const FlagDesc *flagdesc, u32 flags);
	const FlagDesc *getFlagDescFallback(const String &name) const;

	void registerChangedCallback(const String &name,
		SettingsChangedCallback cbf, void *userdata = NULL);
	void deregisterChangedCallback(const String &name,
		SettingsChangedCallback cbf, void *userdata = NULL);

	void removeSecureSettings();

	// Returns the settings layer this object is.
	// If within the global hierarchy you can cast this to enum SettingsLayer
	inline int getLayer() const { return m_settingslayer; }

private:
	/***********************
	 * Reading and writing *
	 ***********************/

	SettingsParseEvent parseConfigObject(const String &line,
		String &name, String &value);
	bool updateConfigObject(std::istream &is, std::ostream &os,
		u32 tab_depth=0);

	static bool checkNameValid(const String &name);
	static bool checkValueValid(const String &value);
	static String getMultiline(std::istream &is, size_t *num_lines=NULL);
	static void printEntry(std::ostream &os, const String &name,
		const SettingsEntry &entry, u32 tab_depth=0);

	/***********
	 * Getters *
	 ***********/
	Settings *getParent() const;

	const SettingsEntry &getEntry(const String &name) const;

	// Allow TestSettings to run sanity checks using private functions.
	friend class TestSettings;
	// For sane mutex locking when iterating
	friend class LuaSettings;

	void updateNoLock(const Settings &other);
	void clearNoLock();
	void clearDefaultsNoLock();

	void doCallbacks(const String &name) const;

	SettingEntries m_settings;
	SettingsCallbackMap m_callbacks;
	String m_end_tag;

	mutable std::mutex m_callback_mutex;

	// All methods that access m_settings/m_defaults directly should lock this.
	mutable std::mutex m_mutex;

	SettingsHierarchy *m_hierarchy = nullptr;
	int m_settingslayer = -1;

	static std::unordered_map<String, const FlagDesc *> s_flags;
};
