/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Tickrate
 * Written by Wend4r (Vladimir Ezhikov).
 * ======================================================

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tickrate/provider.hpp>
#include <globals.hpp>

#include <filesystem.h>
#include <tier0/commonmacros.h>
#include <tier0/keyvalues3.h>
#include <tier0/strtools.h>

#include <any_config.hpp>

Tickrate::Provider::Provider()
 :  m_mapLibraries(DefLessFunc(const CUtlSymbolLarge))
{
}

bool Tickrate::Provider::Init(GameData::CBufferStringVector &vecMessages)
{
	// Enigne 2.
	{
		const char szEngineModuleName[] = "engine2";

		if(!m_aEngine2Library.InitFromMemory(g_pEngineServer))
		{
			static const char *s_pszMessageConcat[] = {"Failed to ", "get \"", szEngineModuleName, "\" module"};

			vecMessages.AddToTail({s_pszMessageConcat});
		}

		m_mapLibraries.Insert(GetSymbol(szEngineModuleName), &m_aEngine2Library);
	}

	// File System.
	{
		const char szFileSystemSTDIOModuleName[] = "filesystem_stdio";

		if(!m_aFileSystemSTDIOLibrary.InitFromMemory(g_pFullFileSystem))
		{
			static const char *s_pszMessageConcat[] = {"Failed to ", "get \"", szFileSystemSTDIOModuleName, "\" module"};

			vecMessages.AddToTail({s_pszMessageConcat});
		}

		m_mapLibraries.Insert(GetSymbol(szFileSystemSTDIOModuleName), &m_aFileSystemSTDIOLibrary);
	}

	// Server.
	{
		const char szServerModuleName[] = "server";

		if(!m_aServerLibrary.InitFromMemory(g_pSource2Server))
		{
			static const char *s_pszMessageConcat[] = {"Failed to ", "get \"", szServerModuleName, "\" module"};

			vecMessages.AddToTail({s_pszMessageConcat});
		}

		m_mapLibraries.Insert(GetSymbol(szServerModuleName), &m_aServerLibrary);
	}

	return true;
}

bool Tickrate::Provider::Load(const char *pszBaseDir, const char *pszPathID, GameData::CBufferStringVector &vecMessages)
{
	if(!LoadGameData(pszBaseDir, pszPathID, vecMessages))
	{
		return false;
	}

	// ...

	return true;
}

bool Tickrate::Provider::Destroy(GameData::CBufferStringVector &vecMessages)
{
	m_mapLibraries.PurgeAndDeleteElements();

	return true;
}

const DynLibUtils::CModule *Tickrate::Provider::FindLibrary(const char *pszName) const
{
	auto iFoundIndex = m_mapLibraries.Find(FindSymbol(pszName));

	Assert(IS_VALID_GAMEDATA_INDEX(iFoundIndex, m_mapLibraries));

	return m_mapLibraries.Element(iFoundIndex);
}

CUtlSymbolLarge Tickrate::Provider::GetSymbol(const char *pszText)
{
	return m_aSymbolTable.AddString(pszText);
}

CUtlSymbolLarge Tickrate::Provider::FindSymbol(const char *pszText) const
{
	return m_aSymbolTable.Find(pszText);
}

bool Tickrate::Provider::LoadGameData(const char *pszBaseDir, const char *pszPathID, GameData::CBufferStringVector &vecMessages)
{
	char sBaseConfigDir[MAX_PATH];

	snprintf((char *)sBaseConfigDir, sizeof(sBaseConfigDir), "%s" CORRECT_PATH_SEPARATOR_S "%s", pszBaseDir, TICKRATE_GAMECONFIG_FOLDER_DIR);

	return m_aStorage.Load(this, sBaseConfigDir, pszPathID, vecMessages);
}

bool Tickrate::Provider::GameDataStorage::Load(IGameData *pRoot, const char *pszBaseConfigDir, const char *pszPathID, GameData::CBufferStringVector &vecMessages)
{
	const struct
	{
		const char *pszFilename;
		bool (Tickrate::Provider::GameDataStorage::*pfnLoad)(IGameData *, KeyValues3 *, GameData::CBufferStringVector &);
	} aConfigs[] =
	{
		{
			TICKRATE_GAMECONFIG_GAMERESOURCE_FILENAME,
			&GameDataStorage::LoadGameResource
		},
		{
			TICKRATE_GAMECONFIG_GAMESYSTEM_FILENAME,
			&GameDataStorage::LoadGameSystem
		},
		{
			TICKRATE_GAMECONFIG_HOSTFRAME_FILENAME,
			&GameDataStorage::LoadHostFrame
		},
		{
			TICKRATE_GAMECONFIG_SOURCE2SERVER_FILENAME,
			&GameDataStorage::LoadSource2Server
		},
		{
			TICKRATE_GAMECONFIG_TICK_FILENAME,
			&GameDataStorage::LoadTick
		}
	};

	char sConfigFile[MAX_PATH];

	CUtlString sError;

	AnyConfig::LoadFromFile_Generic_t aLoadPresets({{&sError, NULL, pszPathID}, g_KV3Format_Generic});

	for(const auto &aConfig : aConfigs)
	{
		AnyConfig::Anyone aGameConfig;

		snprintf((char *)sConfigFile, sizeof(sConfigFile), "%s" CORRECT_PATH_SEPARATOR_S "%s", pszBaseConfigDir, aConfig.pszFilename);

		CUtlVector<CUtlString> vecConfigFiles;

		g_pFullFileSystem->FindFileAbsoluteList(vecConfigFiles, (const char *)sConfigFile, pszPathID);

		if(vecConfigFiles.Count() < 1)
		{
			const char *pszMessageConcat[] = {"Failed to ", "find \"", sConfigFile, "\" file", ": ", sError.Get()};

			vecMessages.AddToTail({pszMessageConcat});

			continue;
		}

		aLoadPresets.m_pszFilename = vecConfigFiles[0].Get();

		if(!aGameConfig.Load(aLoadPresets)) // Hot.
		{
			const char *pszMessageConcat[] = {"Failed to ", "load \"", sConfigFile, "\" file", ": ", sError.Get()};

			vecMessages.AddToTail({pszMessageConcat});

			continue;
		}

		if(!(this->*(aConfig.pfnLoad))(pRoot, aGameConfig.Get(), vecMessages))
		{
			const char *pszMessageConcat[] = {"Failed to ", "parse \"", sConfigFile, "\" file", ": ", sError.Get()};

			vecMessages.AddToTail({pszMessageConcat});

			continue;
		}

		// ...
	}

	return true;
}

bool Tickrate::Provider::GameDataStorage::LoadGameResource(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aGameResource.Load(pRoot, pGameConfig, vecMessages);
}

bool Tickrate::Provider::GameDataStorage::LoadGameSystem(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aGameSystem.Load(pRoot, pGameConfig, vecMessages);
}

bool Tickrate::Provider::GameDataStorage::LoadHostFrame(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aHostFrame.Load(pRoot, pGameConfig, vecMessages);
}

bool Tickrate::Provider::GameDataStorage::LoadSource2Server(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aSource2Server.Load(pRoot, pGameConfig, vecMessages);
}

bool Tickrate::Provider::GameDataStorage::LoadTick(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aTick.Load(pRoot, pGameConfig, vecMessages);
}

const Tickrate::Provider::GameDataStorage::CGameResource &Tickrate::Provider::GameDataStorage::GetGameResource() const
{
	return m_aGameResource;
}

const Tickrate::Provider::GameDataStorage::CGameSystem &Tickrate::Provider::GameDataStorage::GetGameSystem() const
{
	return m_aGameSystem;
}

const Tickrate::Provider::GameDataStorage::CHostFrame &Tickrate::Provider::GameDataStorage::GetHostFrame() const
{
	return m_aHostFrame;
}

const Tickrate::Provider::GameDataStorage::CSource2Server &Tickrate::Provider::GameDataStorage::GetSource2Server() const
{
	return m_aSource2Server;
}

const Tickrate::Provider::GameDataStorage::CTick &Tickrate::Provider::GameDataStorage::GetTick() const
{
	return m_aTick;
}

const Tickrate::Provider::GameDataStorage &Tickrate::Provider::GetGameDataStorage() const
{
	return m_aStorage;
}
