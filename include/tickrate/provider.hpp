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

#ifndef _INCLUDE_METAMOD_SOURCE_TICKRATE_PROVIDER_HPP_
#	define _INCLUDE_METAMOD_SOURCE_TICKRATE_PROVIDER_HPP_

#	pragma once

#	include <stddef.h>
#	include <stdint.h>

#	include <tier0/dbg.h>
#	include <tier0/platform.h>
#	include <tier0/utlscratchmemory.h>
#	include <tier1/utldelegateimpl.h>
#	include <tier1/utlmap.h>
#	include <entity2/entitykeyvalues.h>

#	include <gamedata.hpp> // GameData

#	define TICKRATE_GAMECONFIG_FOLDER_DIR "gamedata"
#	define TICKRATE_GAMECONFIG_GAMERESOURCE_FILENAME "gameresource.games.*"
#	define TICKRATE_GAMECONFIG_GAMESYSTEM_FILENAME "gamesystem.games.*"
#	define TICKRATE_GAMECONFIG_HOSTFRAME_FILENAME "hostframe.games.*"
#	define TICKRATE_GAMECONFIG_SOURCE2SERVER_FILENAME "source2server.games.*"
#	define TICKRATE_GAMECONFIG_TICK_FILENAME "tick.games.*"

class CBaseGameSystemFactory;
class CGameEventManager;
struct CFrame;
namespace Tickrate
{
	class Provider : public IGameData
	{
	public:
		Provider();

	public:
		bool Init(GameData::CBufferStringVector &vecMessages);
		bool Load(const char *pszBaseDir, const char *pszPathID, GameData::CBufferStringVector &vecMessages);
		bool Destroy(GameData::CBufferStringVector &vecMessages);

	protected:
		CUtlSymbolLarge GetSymbol(const char *pszText);
		CUtlSymbolLarge FindSymbol(const char *pszText) const;

	public: // IGameData
		const DynLibUtils::CModule *FindLibrary(const char *pszName) const;

	protected:
		bool LoadGameData(const char *pszBaseDir, const char *pszPathID, GameData::CBufferStringVector &vecMessages);

	public:
		class GameDataStorage
		{
		public:
			bool Load(IGameData *pRoot, const char *pszBaseConfigDir, const char *pszPathID, GameData::CBufferStringVector &vecMessages);

		protected:
			bool LoadGameResource(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
			bool LoadGameSystem(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
			bool LoadHostFrame(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
			bool LoadSource2Server(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
			bool LoadTick(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);

		public:
			class CGameResource
			{
			public:
				CGameResource();

			public:
				bool Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
				void Reset();

			public:
				ptrdiff_t GetEntitySystemOffset() const;

			private:
				GameData::Config::Offsets::ListenerCallbacksCollector m_aOffsetCallbacks;
				GameData::Config m_aGameConfig;

			private: // Offsets.
				ptrdiff_t m_nEntitySystemOffset = -1;
			}; // CGameResource

			class CGameSystem
			{
			public:
				CGameSystem();

			public:
				bool Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
				void Reset();

			public:
				CBaseGameSystemFactory **GetFirstPointer() const;

			private:
				GameData::Config::Addresses::ListenerCallbacksCollector m_aAddressCallbacks;
				GameData::Config m_aGameConfig;

			private: // Addresses.
				CBaseGameSystemFactory **m_ppFirst = nullptr;
			}; // CGameSystem

			class CHostFrame
			{
			public:
				CHostFrame();

			public:
				bool Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
				void Reset();

			public:
				CFrame *GetPointer() const;

			private:
				GameData::Config::Addresses::ListenerCallbacksCollector m_aAddressCallbacks;
				GameData::Config m_aGameConfig;

			private: // Addresses.
				CFrame *m_p = nullptr;
			}; // CHostFrame

			class CSource2Server
			{
			public:
				CSource2Server();

			public:
				bool Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
				void Reset();

			public:
				CGameEventManager **GetGameEventManagerPointer() const;

			private:
				GameData::Config::Addresses::ListenerCallbacksCollector m_aAddressCallbacks;
				GameData::Config m_aGameConfig;

			private: // Addresses.
				CGameEventManager **m_ppGameEventManager = nullptr;
			}; // CSource2Server

			class CTick
			{
			public:
				CTick();

			public:
				bool Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages);
				void Reset();

			public:
				float *GetIntervalPointer() const;
				double *GetInterval2Pointer() const;
				float *GetInterval3DefaultPointer() const;
				float *GetInterval3Pointer() const;

				float *GetPerSecond() const;

			private:
				GameData::Config::Addresses::ListenerCallbacksCollector m_aAddressCallbacks;
				GameData::Config m_aGameConfig;

			private: // Addresses.
				float *m_pInterval = nullptr;
				double *m_pInterval2 = nullptr;
				float *m_pInterval3Default = nullptr;
				float *m_pInterval3 = nullptr;

				float *m_pPerSecond = nullptr;
			}; // CTick

			const CGameResource &GetGameResource() const;
			const CGameSystem &GetGameSystem() const;
			const CHostFrame &GetHostFrame() const;
			const CSource2Server &GetSource2Server() const;
			const CTick &GetTick() const;

		private:
			CGameResource m_aGameResource;
			CGameSystem m_aGameSystem;
			CHostFrame m_aHostFrame;
			CSource2Server m_aSource2Server;
			CTick m_aTick;
		}; // GameDataStorage

		const GameDataStorage &GetGameDataStorage() const;

	private:
		CUtlSymbolTableLarge_CI m_aSymbolTable;
		CUtlMap<CUtlSymbolLarge, DynLibUtils::CModule *> m_mapLibraries;

	private:
		GameDataStorage m_aStorage;

	private:
		DynLibUtils::CModule m_aEngine2Library, 
		                     m_aFileSystemSTDIOLibrary, 
		                     m_aServerLibrary;
	}; // Provider
}; // Tickrate

#endif // _INCLUDE_METAMOD_SOURCE_TICKRATE_PROVIDER_HPP_
