/*
Minetest
Copyright (C) 2018 bendeutsch, Ben Deutsch <ben@bendeutsch.de>
Copyright (C) 2021 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

// This file is an edited copy of test_authdatabase.cpp

#include "cmake_config.h"

#include "test.h"

#include <algorithm>
#include <cstdlib>
#include "database/database-sqlite3.h"
#include "filesys.h"

namespace
{
// Anonymous namespace to create classes that are only
// visible to this file
//
// These are helpers that return a *ModStorageDatabase and
// allow us to run the same tests on different databases and
// database acquisition strategies.

class ModStorageDatabaseProvider
{
public:
	virtual ~ModStorageDatabaseProvider() = default;
	virtual ModStorageDatabase *getModStorageDatabase() = 0;
};

class FixedProvider : public ModStorageDatabaseProvider
{
public:
	FixedProvider(ModStorageDatabase *db): m_db(db) {}

	~FixedProvider() = default;

	ModStorageDatabase *getModStorageDatabase() override { return m_db; }

private:
	ModStorageDatabase *m_db;
};

class SQLite3Provider : public ModStorageDatabaseProvider
{
public:
	SQLite3Provider(const String &dir): m_dir(dir) {}

	~SQLite3Provider()
	{
		if (m_db)
			m_db->endSave();
		delete m_db;
	}

	ModStorageDatabase *getModStorageDatabase() override
	{
		if (m_db)
			m_db->endSave();
		delete m_db;
		m_db = new ModStorageDatabaseSQLite3(m_dir);
		m_db->beginSave();
		return m_db;
	}

private:
	String m_dir;
	ModStorageDatabase *m_db = nullptr;
};

}

class TestModStorageDatabase : public TestBase
{
public:
	TestModStorageDatabase() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestModStorageDatabase"; }

	void runTests(IGameDef *gamedef);
	void runTestsForCurrentDB();

	void testRecallFail();
	void testCreate();
	void testRecall();
	void testChange();
	void testRecallChanged();
	void testListMods();
	void testRemove();

private:
	ModStorageDatabaseProvider *mod_storage_provider;
};

static TestModStorageDatabase g_test_instance;

void TestModStorageDatabase::runTests(IGameDef *gamedef)
{
	// fixed directory, for persistence
	thread_local const String test_dir = getTestTempDirectory();

	// Each set of tests is run twice for each database type except dummy:
	// one where we reuse the same ModStorageDatabase object (to test local caching),
	// and one where we create a new ModStorageDatabase object for each call
	// (to test actual persistence).
	// Since the dummy database is only in-memory, it has no persistence to test.

	ModStorageDatabase *mod_storage_db;

	rawstream << "-------- SQLite3 database (same object)" << std::endl;

	mod_storage_db = new ModStorageDatabaseSQLite3(test_dir);
	mod_storage_provider = new FixedProvider(mod_storage_db);

	runTestsForCurrentDB();

	delete mod_storage_db;
	delete mod_storage_provider;

	// reset database
	fs::DeleteSingleFileOrEmptyDirectory(test_dir + DIR_DELIM + "mod_storage.sqlite");

	rawstream << "-------- SQLite3 database (new objects)" << std::endl;

	mod_storage_provider = new SQLite3Provider(test_dir);

	runTestsForCurrentDB();

	delete mod_storage_provider;

}

////////////////////////////////////////////////////////////////////////////////

void TestModStorageDatabase::runTestsForCurrentDB()
{
	TEST(testRecallFail);
	TEST(testCreate);
	TEST(testRecall);
	TEST(testChange);
	TEST(testRecallChanged);
	TEST(testListMods);
	TEST(testRemove);
	TEST(testRecallFail);
}

void TestModStorageDatabase::testRecallFail()
{
	ModStorageDatabase *mod_storage_db = mod_storage_provider->getModStorageDatabase();
	StringMap recalled;
	std::vector<String> recalled_keys;
	mod_storage_db->getModEntries("mod1", &recalled);
	mod_storage_db->getModKeys("mod1", &recalled_keys);
	UASSERT(recalled.empty());
	UASSERT(recalled_keys.empty());
	String key1_value;
	UASSERT(!mod_storage_db->getModEntry("mod1", "key1", &key1_value));
	UASSERT(!mod_storage_db->hasModEntry("mod1", "key1"));
}

void TestModStorageDatabase::testCreate()
{
	ModStorageDatabase *mod_storage_db = mod_storage_provider->getModStorageDatabase();
	UASSERT(mod_storage_db->setModEntry("mod1", "key1", "value1"));
}

void TestModStorageDatabase::testRecall()
{
	ModStorageDatabase *mod_storage_db = mod_storage_provider->getModStorageDatabase();
	StringMap recalled;
	std::vector<String> recalled_keys;
	mod_storage_db->getModEntries("mod1", &recalled);
	mod_storage_db->getModKeys("mod1", &recalled_keys);
	UASSERTCMP(std::size_t, ==, recalled.size(), 1);
	UASSERTCMP(std::size_t, ==, recalled_keys.size(), 1);
	UASSERTCMP(String, ==, recalled["key1"], "value1");
	UASSERTCMP(String, ==, recalled_keys[0], "key1");
	String key1_value;
	UASSERT(mod_storage_db->getModEntry("mod1", "key1", &key1_value));
	UASSERTCMP(String, ==, key1_value, "value1");
	UASSERT(mod_storage_db->hasModEntry("mod1", "key1"));
}

void TestModStorageDatabase::testChange()
{
	ModStorageDatabase *mod_storage_db = mod_storage_provider->getModStorageDatabase();
	UASSERT(mod_storage_db->setModEntry("mod1", "key1", "value2"));
}

void TestModStorageDatabase::testRecallChanged()
{
	ModStorageDatabase *mod_storage_db = mod_storage_provider->getModStorageDatabase();
	StringMap recalled;
	mod_storage_db->getModEntries("mod1", &recalled);
	UASSERTCMP(std::size_t, ==, recalled.size(), 1);
	UASSERTCMP(String, ==, recalled["key1"], "value2");
	String key1_value;
	UASSERT(mod_storage_db->getModEntry("mod1", "key1", &key1_value));
	UASSERTCMP(String, ==, key1_value, "value2");
	UASSERT(mod_storage_db->hasModEntry("mod1", "key1"));
}

void TestModStorageDatabase::testListMods()
{
	ModStorageDatabase *mod_storage_db = mod_storage_provider->getModStorageDatabase();
	UASSERT(mod_storage_db->setModEntry("mod2", "key1", "value1"));
	UASSERT(mod_storage_db->setModEntry("mod2", "key2", "value1"));
	std::vector<String> mod_list;
	mod_storage_db->listMods(&mod_list);
	UASSERTCMP(size_t, ==, mod_list.size(), 2);
	UASSERT(std::find(mod_list.cbegin(), mod_list.cend(), "mod1") != mod_list.cend());
	UASSERT(std::find(mod_list.cbegin(), mod_list.cend(), "mod2") != mod_list.cend());
}

void TestModStorageDatabase::testRemove()
{
	ModStorageDatabase *mod_storage_db = mod_storage_provider->getModStorageDatabase();
	UASSERT(mod_storage_db->removeModEntry("mod1", "key1"));
	UASSERT(!mod_storage_db->removeModEntries("mod1"));
	UASSERT(mod_storage_db->removeModEntries("mod2"));
}
