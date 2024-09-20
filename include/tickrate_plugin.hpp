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

#ifndef _INCLUDE_METAMOD_SOURCE_TICKRATE_PLUGIN_HPP_
#	define _INCLUDE_METAMOD_SOURCE_TICKRATE_PLUGIN_HPP_

#	pragma once

#	include <itickrate.hpp>
#	include <tickrate/chat_command_system.hpp>
#	include <tickrate/provider.hpp>
#	include <concat.hpp>

#	include <logger.hpp>
#	include <translations.hpp>

#	include <ISmmPlugin.h>

#	include <bitvec.h>
#	include <const.h>
#	include <igameevents.h>
#	include <igamesystem.h>
#	include <igamesystemfactory.h>
#	include <iloopmode.h>
#	include <iserver.h>
#	include <netmessages.h>
#	include <playerslot.h>
#	include <tier0/bufferstring.h>
#	include <tier0/strtools.h>
#	include <tier1/convar.h>
#	include <tier1/utlvector.h>

#	define TICKRATE_DEFAULT 64

#	define TICKRATE_LOGGINING_COLOR {255, 222, 145, 255}

#	define TICKRATE_BASE_DIR "addons" CORRECT_PATH_SEPARATOR_S META_PLUGIN_PREFIX
#	define TICKRATE_GAME_EVENTS_FILES "resource" CORRECT_PATH_SEPARATOR_S "*.gameevents"
#	define TICKRATE_GAME_TRANSLATIONS_FILES "translations" CORRECT_PATH_SEPARATOR_S "*.phrases.*"
#	define TICKRATE_GAME_TRANSLATIONS_PATH_FILES TICKRATE_BASE_DIR CORRECT_PATH_SEPARATOR_S TICKRATE_GAME_TRANSLATIONS_FILES
#	define TICKRATE_GAME_LANGUAGES_FILES "configs" CORRECT_PATH_SEPARATOR_S "languages.*"
#	define TICKRATE_GAME_LANGUAGES_PATH_FILES TICKRATE_BASE_DIR CORRECT_PATH_SEPARATOR_S TICKRATE_GAME_LANGUAGES_FILES
#	define TICKRATE_BASE_PATHID "GAME"

#	define TICKRATE_EXAMPLE_CHAT_COMMAND "example"

#	define TICKRATE_CLIENT_CVAR_NAME_LANGUAGE "cl_language"

class CBasePlayerController;
class INetworkMessageInternal;

class TickratePlugin final : public ISmmPlugin, public IMetamodListener, public ITickrate, public CBaseGameSystem, 
                             public Tickrate::ChatCommandSystem, public Tickrate::Provider, virtual public Logger, public Translations
{
public:
	TickratePlugin();

public: // ISmmPlugin
	bool Load(PluginId id, ISmmAPI *ismm, char *error = nullptr, size_t maxlen = 0, bool late = true) override;
	bool Unload(char *error, size_t maxlen) override;
	bool Pause(char *error, size_t maxlen) override;
	bool Unpause(char *error, size_t maxlen) override;
	void AllPluginsLoaded() override;

	const char *GetAuthor() override;
	const char *GetName() override;
	const char *GetDescription() override;
	const char *GetURL() override;
	const char *GetLicense() override;
	const char *GetVersion() override;
	const char *GetDate() override;
	const char *GetLogTag() override;

public: // IMetamodListener
	void *OnMetamodQuery(const char *iface, int *ret) override;

public: // ITickrate
	CGameEntitySystem **GetGameEntitySystemPointer() const override;
	CBaseGameSystemFactory **GetFirstGameSystemPointer() const override;
	CFrame *GetHostFramePointer() const override;
	IGameEventManager2 **GetGameEventManagerPointer() const override;
	float *GetTickIntervalPointer() const override;
	double *GetTickInterval2Pointer() const override;
	float *GetTickInterval3DefaultPointer() const override;
	float *GetTickInterval3Pointer() const override;
	float *GetTicksPerSecondPointer() const override;

	class CLanguage : public ITickrate::ILanguage
	{
		friend class TickratePlugin;

	public:
		CLanguage(const CUtlSymbolLarge &sInitName = NULL, const char *pszInitCountryCode = "en");

	public:
		const char *GetName() const override;
		const char *GetCountryCode() const override;

	protected:
		void SetName(const CUtlSymbolLarge &sInitName);
		void SetCountryCode(const char *psz);

	private:
		CUtlSymbolLarge m_sName;
		CUtlString m_sCountryCode;
	}; // CLanguage

	class CPlayerData : public IPlayerData
	{
		friend class TickratePlugin;

	public:
		CPlayerData();

	public:
		const ILanguage *GetLanguage() const override;
		void SetLanguage(const ILanguage *pData) override;
		bool AddLanguageListener(const LanguageHandleCallback_t *pfnCallback) override;
		bool RemoveLanguageListener(const LanguageHandleCallback_t *pfnCallback) override;

	public:
		virtual void OnLanguageReceived(CPlayerSlot aSlot, CLanguage *pData);

	public:
		struct TranslatedPhrase
		{
			const Translations::CPhrase::CFormat *m_pFormat;
			const Translations::CPhrase::CContent *m_pContent;
		};

		void TranslatePhrases(const Translations *pTranslations, const CLanguage &aServerLanguage, CUtlVector<CUtlString> &vecMessages);
		const TranslatedPhrase &GetChangeTickratePhrase() const;
		const TranslatedPhrase &GetCurrentTickratePhrase() const;

	private:
		const ILanguage *m_pLanguage;
		CUtlVector<const LanguageHandleCallback_t *> m_vecLanguageCallbacks;

	private:
		TranslatedPhrase m_aChangeTickratePhrase;
		TranslatedPhrase m_aCurrentTickratePhrase;
	}; // CPlayerData

	const ITickrate::ILanguage *GetServerLanguage() const override;
	const ITickrate::ILanguage *GetLanguageByName(const char *psz) const override;
	IPlayerData *GetPlayerData(const CPlayerSlot &aSlot) override;

public: // Tickrate.
	class CChangedData
	{
	public:
		CChangedData(int nInitOld, int nInitNew);

	public:
		int GetOld() const;
		float GetOldInterval() const;

		int GetNew() const;
		float GetNewInterval() const;
		double GetNewInterval2() const;

		float GetMultiple() const;

	private:
		int m_nOld;
		float m_flOldInterval;

		int m_nNew;
		float m_flNewInterval;
		double m_dblNewInterval;

		float m_flMultiple;
	};

	int Get() override;
	int Set(int nNew) override;
	int Change(int nNew) override;
	int ChangeInternal(int nNew);
	void ChangeHostFrame(CFrame *pHostFrame, const CChangedData &aData);
	void ChangeGlobals(CGlobalVars *pGlobals, const CChangedData &aData);

public: // CBaseGameSystem
	bool Init() override;
	void PostInit() override;
	void Shutdown() override;

	GS_EVENT(GameFrameBoundary);
	GS_EVENT(OutOfGameFrameBoundary);

public: // Utils.
	bool InitProvider(char *error = nullptr, size_t maxlen = 0);
	bool LoadProvider(char *error = nullptr, size_t maxlen = 0);
	bool UnloadProvider(char *error = nullptr, size_t maxlen = 0);

public: // Game Resource.
	bool RegisterGameResource(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterGameResource(char *error = nullptr, size_t maxlen = 0);

public: // Game Factory.
	bool RegisterGameFactory(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterGameFactory(char *error = nullptr, size_t maxlen = 0);

public: // Source 2 Server.
	bool RegisterSource2Server(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterSource2Server(char *error = nullptr, size_t maxlen = 0);

public: // Tick.
	bool RegisterTick(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterTick(char *error = nullptr, size_t maxlen = 0);

public: // Network Messages.
	bool RegisterNetMessages(char *error = nullptr, size_t maxlen = 0);
	bool UnregisterNetMessages(char *error = nullptr, size_t maxlen = 0);

public: // Languages.
	bool ParseLanguages(char *error = nullptr, size_t maxlen = 0);
	bool ParseLanguages(KeyValues3 *pRoot, CUtlVector<CUtlString> &vecMessages);
	bool ClearLanguages(char *error = nullptr, size_t maxlen = 0);

public: // Translations.
	bool ParseTranslations(char *error = nullptr, size_t maxlen = 0);
	bool ClearTranslations(char *error = nullptr, size_t maxlen = 0);

private: // Commands.
	CON_COMMAND_MEMBER_F(TickratePlugin, "mm_" META_PLUGIN_PREFIX "_reload_gamedata", OnReloadGameDataCommand, "Reload gamedata configs", FCVAR_LINKED_CONCOMMAND);

private: // ConVars. See the constructor
	ConVar<int> m_aSVTickrateConVar;
	ConVar<bool> m_aEnableFrameDetailsConVar;

public: // SourceHooks.
	void OnStartupServerHook(const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession, const char *);
	void OnDispatchConCommandHook(ConCommandHandle hCommand, const CCommandContext &aContext, const CCommand &aArgs);
	void OnFillServerInfoHook(CSVCMsg_ServerInfo_t *pServerInfo);
	CServerSideClientBase *OnConnectClientHook(const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);
	bool OnProcessRespondCvarValueHook(const CCLCMsg_RespondCvarValue_t &aMessage);
	void OnDisconectClientHook(ENetworkDisconnectionReason eReason);

public: // Dump ones.
	static void DumpProtobufMessage(const ConcatLineString &aConcat, CBufferString &sOutput, const google::protobuf::Message &aMessage);
	static void DumpGlobalVars(const ConcatLineString &aConcat, CBufferString &sOutput, const CGlobalVarsBase *pGlobals);
	static void DumpGlobalVars(const ConcatLineString &aConcat, const ConcatLineString &aConcat2, CBufferString &sOutput, const CGlobalVars *pGlobals);
	static void DumpHostFrame(const ConcatLineString &aConcat, CBufferString &sOutput, const CFrame *pHostFrame);
	static void DumpEngineLoopState(const ConcatLineString &aConcat, CBufferString &sOutput, const EngineLoopState_t &aMessage);
	static void DumpEntityList(const ConcatLineString &aConcat, CBufferString &sOutput, const CUtlVector<CEntityHandle> &vecEntityList);
	static void DumpEventSimulate(const ConcatLineString &aConcat, const ConcatLineString &aConcat2, CBufferString &sOutput, const EventSimulate_t &aMessage);
	static void DumpEventFrameBoundary(const ConcatLineString &aConcat, CBufferString &sOutput, const EventFrameBoundary_t &aMessage);
	static void DumpServerSideClient(const ConcatLineString &aConcat, CBufferString &sOutput, CServerSideClientBase *pClient);
	static void DumpDisconnectReason(const ConcatLineString &aConcat, CBufferString &sOutput, ENetworkDisconnectionReason eReason);

public: // Utils.
	void SendCvarValueQuery(IRecipientFilter *pFilter, const char *pszName, int iCookie);
	void SendChatMessage(IRecipientFilter *pFilter, int iEntityIndex, bool bIsChat, const char *pszChatMessageFormat, const char *pszParam1 = "", const char *pszParam2 = "", const char *pszParam3 = "", const char *pszParam4 = "");
	void SendTextMessage(IRecipientFilter *pFilter, int iDestination, size_t nParamCount, const char *pszParam, ...);

protected: // Handlers.
	void OnStartupServer(CNetworkGameServerBase *pNetServer, const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession);
	void OnFillServerInfo(CNetworkGameServerBase *pNetServer, CSVCMsg_ServerInfo_t *pServerInfo);
	void OnConnectClient(CNetworkGameServerBase *pNetServer, CServerSideClientBase *pClient, const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence);
	bool OnProcessRespondCvarValue(CServerSideClientBase *pClient, const CCLCMsg_RespondCvarValue_t &aMessage);
	void OnDisconectClient(CServerSideClientBase *pClient, ENetworkDisconnectionReason eReason);

protected: // ConVar symbols.
	CUtlSymbolLarge GetConVarSymbol(const char *pszName);
	CUtlSymbolLarge FindConVarSymbol(const char *pszName) const;

private: // ConVar (hash)map.
	CUtlSymbolTableLarge_CI m_tableConVars;
	CUtlMap<CUtlSymbolLarge, int> m_mapConVarCookies;

protected: // Language symbols.
	CUtlSymbolLarge GetLanguageSymbol(const char *pszName);
	CUtlSymbolLarge FindLanguageSymbol(const char *pszName) const;

private: // Language (hash)map.
	CUtlSymbolTableLarge_CI m_tableLanguages;
	CUtlMap<CUtlSymbolLarge, CLanguage> m_mapLanguages;

private: // Fields.
	IGameSystemFactory *m_pFactory = NULL;

	int m_iTickIntervalPageBits = 0;
	int m_iTickInterval2PageBits = 0;
	int m_iTickInterval3DefaultPageBits = 0;
	int m_iTickInterval3PageBits = 0;
	int m_iTicksPerSecondPageBits = 0;

	INetworkMessageInternal *m_pGetCvarValueMessage = NULL;
	INetworkMessageInternal *m_pSayText2Message = NULL;
	INetworkMessageInternal *m_pTextMsgMessage = NULL;

	CLanguage m_aServerLanguage;
	CUtlVector<CLanguage> m_vecLanguages;

	CPlayerData m_aPlayers[ABSOLUTE_PLAYER_LIMIT];
}; // TickratePlugin

extern TickratePlugin *g_pTickratePlugin;

PLUGIN_GLOBALVARS();

#endif //_INCLUDE_METAMOD_SOURCE_TICKRATE_PLUGIN_HPP_
