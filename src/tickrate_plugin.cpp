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

#include <tickrate_plugin.hpp>
#include <globals.hpp>

#include <stdint.h>

#include <string>
#include <exception>

#include <any_config.hpp>

#include <sourcehook/sourcehook.h>
#include <sourcehook/sh_memory.h>

#include <filesystem.h>
#include <igameeventsystem.h>
#include <inetchannel.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/inetworkserializer.h>
#include <recipientfilter.h>
#include <serversideclient.h>
#include <shareddefs.h>
#include <tier0/commonmacros.h>
#include <usermessages.pb.h>

SH_DECL_HOOK3_void(ICvar, DispatchConCommand, SH_NOATTRIB, 0, ConCommandHandle, const CCommandContext &, const CCommand &);
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t &, ISource2WorldSession *, const char *);
SH_DECL_HOOK8(CNetworkGameServerBase, ConnectClient, SH_NOATTRIB, 0, CServerSideClientBase *, const char *, ns_address *, int, CCLCMsg_SplitPlayerConnect_t *, const char *, const byte *, int, bool);
SH_DECL_HOOK1(CServerSideClientBase, ProcessRespondCvarValue, SH_NOATTRIB, 0, bool, const CCLCMsg_RespondCvarValue_t &);
SH_DECL_HOOK1_void(CServerSideClientBase, PerformDisconnection, SH_NOATTRIB, 0, ENetworkDisconnectionReason);

static TickratePlugin s_aTickratePlugin;
TickratePlugin *g_pTickratePlugin = &s_aTickratePlugin;

const ConcatLineString s_aEmbedConcat =
{
	{
		"\t", // Start message.
		": ", // Padding of key & value.
		"\n", // End.
		"\n\t", // End and next line.
	}
};

const ConcatLineString s_aEmbed2Concat =
{
	{
		"\t\t",
		": ",
		"\n",
		"\n\t\t",
	}
};

PLUGIN_EXPOSE(TickratePlugin, s_aTickratePlugin);

TickratePlugin::TickratePlugin()
 :  Logger(GetName(), [](LoggingChannelID_t nTagChannelID)
    {
    	LoggingSystem_AddTagToChannel(nTagChannelID, s_aTickratePlugin.GetLogTag());
    }, 0, LV_DEFAULT, TICKRATE_LOGGINING_COLOR),
    m_aSVTickrateConVar("sv_tickrate", FCVAR_RELEASE | FCVAR_GAMEDLL, "Server tickrate", TICKRATE_DEFAULT, [](ConVar<int> *pConVar, const CSplitScreenSlot aSlot, const int *pNewValue, const int *pOldValue)
    {
    	if(*pNewValue != *pOldValue)
    	{
    		s_aTickratePlugin.ChangeInternal(*pNewValue);
    	}
    }),
    m_aEnableFrameDetailsConVar("mm_" META_PLUGIN_PREFIX "_enable_frame_details", FCVAR_RELEASE | FCVAR_GAMEDLL, "Enable detail messages of frames", false, true, false, true, true), 
    m_mapConVarCookies(DefLessFunc(const CUtlSymbolLarge)),
    m_mapLanguages(DefLessFunc(const CUtlSymbolLarge))
{
}

bool TickratePlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	MessageFormat("Starting %s plugin...\n", GetName());

	if(!InitGlobals(ismm, error, maxlen))
	{
		return false;
	}

	if(IsChannelEnabled(LS_DETAILED))
	{
		CBufferStringGrowable<1024> sMessage;

		DumpGlobals(s_aEmbedConcat, sMessage);
		Logger::Detailed(sMessage);
	}

	ConVar_Register(FCVAR_RELEASE | FCVAR_GAMEDLL);

	if(!InitProvider(error, maxlen))
	{
		return false;
	}

	if(!LoadProvider(error, maxlen))
	{
		return false;
	}

	if(!ParseLanguages(error, maxlen))
	{
		return false;
	}

	if(!ParseTranslations(error, maxlen))
	{
		return false;
	}

	if(!RegisterGameFactory(error, maxlen))
	{
		return false;
	}

	if(!RegisterTick(error, maxlen))
	{
		return false;
	}

	SH_ADD_HOOK(ICvar, DispatchConCommand, g_pCVar, SH_MEMBER(this, &TickratePlugin::OnDispatchConCommandHook), false);
	SH_ADD_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &TickratePlugin::OnStartupServerHook, true);

	// Register chat commands.
	Tickrate::ChatCommandSystem::Register("tickrate", [&](CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArguments)
	{
		CSingleRecipientFilter aFilter(aSlot);

		int iClient = aSlot.Get();

		Assert(0 <= iClient && iClient < ABSOLUTE_PLAYER_LIMIT);

		const auto &aPlayer = m_aPlayers[iClient];

		const auto &aPhrase = aPlayer.GetCurrentTickratePhrase();

		if(aPhrase.m_pFormat && aPhrase.m_pContent)
		{
			SendTextMessage(&aFilter, HUD_PRINTTALK, 1, aPhrase.m_pContent->Format(*aPhrase.m_pFormat, 1, Get()).Get());
		}
		else
		{
			Logger::Warning("Not found a current tickrate phrase\n");
		}
	});

	if(late)
	{
		auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

		if(pNetServer)
		{
			OnStartupServer(pNetServer, pNetServer->m_GameConfig, NULL);

			for(const auto &pClient : pNetServer->m_Clients)
			{
				if(pClient->IsConnected() && !pClient->IsFakeClient())
				{
					OnConnectClient(pNetServer, pClient, pClient->GetClientName(), &pClient->m_nAddr, -1, NULL, NULL, NULL, 0, pClient->m_bLowViolence);
				}
			}
		}
	}

	// Apply a tickrate.
	{
		auto *pCommandLine = CommandLine();

		if(pCommandLine)
		{
			int iTickrate = pCommandLine->ParmValue("-tickrate", -1);

			if(iTickrate != -1)
			{
				Change(iTickrate);
			}
		}
	}

	MessageFormat("%s started!\n", GetName());

	return true;
}

bool TickratePlugin::Unload(char *error, size_t maxlen)
{
	{
		auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

		if(pNetServer)
		{
			SH_REMOVE_HOOK_MEMFUNC(CNetworkGameServerBase, ConnectClient, pNetServer, this, &TickratePlugin::OnConnectClientHook, true);
		}
	}

	SH_REMOVE_HOOK_MEMFUNC(INetworkServerService, StartupServer, g_pNetworkServerService, this, &TickratePlugin::OnStartupServerHook, true);

	Assert(ClearLanguages());
	Assert(ClearTranslations());

	if(!UnloadProvider(error, maxlen))
	{
		return false;
	}

	if(!UnregisterNetMessages(error, maxlen))
	{
		return false;
	}

	if(!UnregisterTick(error, maxlen))
	{
		return false;
	}

	if(!UnregisterSource2Server(error, maxlen))
	{
		return false;
	}

	if(!UnregisterGameFactory(error, maxlen))
	{
		return false;
	}

	if(!UnregisterGameResource(error, maxlen))
	{
		return false;
	}

	if(!DestoryGlobals(error, maxlen))
	{
		return false;
	}

	ConVar_Unregister();

	// ...

	return true;
}

bool TickratePlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool TickratePlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

void TickratePlugin::AllPluginsLoaded()
{
	/**
	 * AMNOTE: This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

const char *TickratePlugin::GetAuthor()        { return META_PLUGIN_AUTHOR; }
const char *TickratePlugin::GetName()          { return META_PLUGIN_NAME; }
const char *TickratePlugin::GetDescription()   { return META_PLUGIN_DESCRIPTION; }
const char *TickratePlugin::GetURL()           { return META_PLUGIN_URL; }
const char *TickratePlugin::GetLicense()       { return META_PLUGIN_LICENSE; }
const char *TickratePlugin::GetVersion()       { return META_PLUGIN_VERSION; }
const char *TickratePlugin::GetDate()          { return META_PLUGIN_DATE; }
const char *TickratePlugin::GetLogTag()        { return META_PLUGIN_LOG_TAG; }

void *TickratePlugin::OnMetamodQuery(const char *iface, int *ret)
{
	if(!strcmp(iface, TICKRATE_INTERFACE_NAME))
	{
		if(ret)
		{
			*ret = META_IFACE_OK;
		}

		return this;
	}

	if(ret)
	{
		*ret = META_IFACE_FAILED;
	}

	return nullptr;
}

CGameEntitySystem **TickratePlugin::GetGameEntitySystemPointer() const
{
	return reinterpret_cast<CGameEntitySystem **>((uintptr_t)g_pGameResourceServiceServer + GetGameDataStorage().GetGameResource().GetEntitySystemOffset());
}

CBaseGameSystemFactory **TickratePlugin::GetFirstGameSystemPointer() const
{
	return GetGameDataStorage().GetGameSystem().GetFirstPointer();
}

IGameEventManager2 **TickratePlugin::GetGameEventManagerPointer() const
{
	return reinterpret_cast<IGameEventManager2 **>(GetGameDataStorage().GetSource2Server().GetGameEventManagerPointer());
}

float *TickratePlugin::GetTickIntervalPointer() const
{
	return GetGameDataStorage().GetTick().GetIntervalPointer();
}

TickratePlugin::CLanguage::CLanguage(const CUtlSymbolLarge &sInitName, const char *pszInitCountryCode)
 :  m_sName(sInitName), 
    m_sCountryCode(pszInitCountryCode)
{
}

const char *TickratePlugin::CLanguage::GetName() const
{
	return m_sName.String();
}

void TickratePlugin::CLanguage::SetName(const CUtlSymbolLarge &s)
{
	m_sName = s;
}

const char *TickratePlugin::CLanguage::GetCountryCode() const
{
	return m_sCountryCode;
}

void TickratePlugin::CLanguage::SetCountryCode(const char *psz)
{
	m_sCountryCode = psz;
}

TickratePlugin::CPlayerData::CPlayerData()
 :  m_pLanguage(nullptr), 
    m_aChangeTickratePhrase({nullptr, nullptr})
{
}

const ITickrate::ILanguage *TickratePlugin::CPlayerData::GetLanguage() const
{
	return m_pLanguage;
}

void TickratePlugin::CPlayerData::SetLanguage(const ILanguage *pData)
{
	m_pLanguage = pData;
}

bool TickratePlugin::CPlayerData::AddLanguageListener(const LanguageHandleCallback_t *pfnCallback)
{
	// Check on exists.
	{
		int iFound = m_vecLanguageCallbacks.Find(pfnCallback);

		Assert(iFound != m_vecLanguageCallbacks.InvalidIndex());
	}

	m_vecLanguageCallbacks.AddToTail(pfnCallback);

	return true;
}

bool TickratePlugin::CPlayerData::RemoveLanguageListener(const LanguageHandleCallback_t *pfnCallback)
{
	return m_vecLanguageCallbacks.FindAndRemove(pfnCallback);
}

void TickratePlugin::CPlayerData::OnLanguageReceived(CPlayerSlot aSlot, CLanguage *pData)
{
	SetLanguage(pData);

	for(const auto &it : m_vecLanguageCallbacks)
	{
		(*it)(aSlot, pData);
	}
}


void TickratePlugin::CPlayerData::TranslatePhrases(const Translations *pTranslations, const CLanguage &aServerLanguage, CUtlVector<CUtlString> &vecMessages)
{
	const struct
	{
		const char *pszName;
		TranslatedPhrase *pTranslated;
	} aPhrases[] =
	{
		{
			"Change tickrate",
			&m_aChangeTickratePhrase,
		},
		{
			"Current tickrate",
			&m_aCurrentTickratePhrase,
		}
	};

	const Translations::CPhrase::CContent *paContent;

	Translations::CPhrase::CFormat aFormat;

	int iFound {};

	const auto *pLanguage = GetLanguage();

	const char *pszServerContryCode = aServerLanguage.GetCountryCode(), 
	           *pszContryCode = pLanguage ? pLanguage->GetCountryCode() : pszServerContryCode;

	for(const auto &aPhrase : aPhrases)
	{
		const char *pszPhraseName = aPhrase.pszName;

		if(pTranslations->FindPhrase(pszPhraseName, iFound))
		{
			const auto &aTranslationsPhrase = pTranslations->GetPhrase(iFound);

			if(!aTranslationsPhrase.Find(pszContryCode, paContent) && !aTranslationsPhrase.Find(pszServerContryCode, paContent))
			{
				CUtlString sMessage;

				sMessage.Format("Not found \"%s\" country code for \"%s\" phrase\n", pszContryCode, pszPhraseName);
				vecMessages.AddToTail(sMessage);

				continue;
			}

			aPhrase.pTranslated->m_pFormat = &aTranslationsPhrase.GetFormat();
		}
		else
		{
			CUtlString sMessage;

			sMessage.Format("Not found \"%s\" phrase\n", pszPhraseName);
			vecMessages.AddToTail(sMessage);

			continue;
		}

		if(!paContent->IsEmpty())
		{
			aPhrase.pTranslated->m_pContent = paContent;
		}
	}
}

const TickratePlugin::CPlayerData::TranslatedPhrase &TickratePlugin::CPlayerData::GetChangeTickratePhrase() const
{
	return m_aChangeTickratePhrase;
}

const TickratePlugin::CPlayerData::TranslatedPhrase &TickratePlugin::CPlayerData::GetCurrentTickratePhrase() const
{
	return m_aCurrentTickratePhrase;
}

const ITickrate::ILanguage *TickratePlugin::GetServerLanguage() const
{
	return &m_aServerLanguage;
}

const ITickrate::ILanguage *TickratePlugin::GetLanguageByName(const char *psz) const
{
	auto iFound = m_mapLanguages.Find(FindLanguageSymbol(psz));

	return m_mapLanguages.IsValidIndex(iFound) ? &m_mapLanguages.Element(iFound) : nullptr;
}

ITickrate::IPlayerData *TickratePlugin::GetPlayerData(const CPlayerSlot &aSlot)
{
	return &m_aPlayers[aSlot.Get()];
}

TickratePlugin::CChangedData::CChangedData(int nInitOld, int nInitNew)
 :  m_nOld(nInitOld), 
    m_flOldInterval(1.0f / nInitOld), 
    m_nNew(nInitNew), 
    m_flNewInterval(1.0f / nInitNew), 
    m_flMultiple(nInitOld / nInitNew)
{
}

int TickratePlugin::CChangedData::GetOld() const
{
	return m_nOld;
}

float TickratePlugin::CChangedData::GetOldInterval() const
{
	return m_flOldInterval;
}

int TickratePlugin::CChangedData::GetNew() const
{
	return m_nNew;
}

float TickratePlugin::CChangedData::GetNewInterval() const
{
	return m_flNewInterval;
}

float TickratePlugin::CChangedData::GetMultiple() const
{
	return m_flMultiple;
}

int TickratePlugin::Get()
{
	float *pTickInterval = GetTickIntervalPointer();

	if(pTickInterval)
	{
		return (int)(1.0f / *pTickInterval);
	}

	WarningFormat("%s: %s\n", __FUNCTION__, "Tick interval is not ready");

	return TICKRATE_DEFAULT;
}

int TickratePlugin::Set(int nNew)
{
	int nOld = Get();

	float *pTickInterval = GetTickIntervalPointer();

	if(!pTickInterval)
	{
		WarningFormat("%s: %s\n", __FUNCTION__, "Tick interval is not ready");

		return nOld;
	}

	const CChangedData aData(nOld, nNew);

	INetworkGameServer *pServer = g_pNetworkServerService->GetIGameServer();

	if(pServer)
	{
		pServer->SetServerTick((int)(pServer->GetServerTick() * aData.GetMultiple()));

		auto *pGlobals = pServer->GetGlobals();

		if(pGlobals)
		{
			ChangeGlobals(pGlobals, aData);
		}
	}

	*pTickInterval = aData.GetNewInterval();

	return nOld;
}

int TickratePlugin::Change(int nNew)
{
	int nOld = Get();

	m_aSVTickrateConVar.SetValue(nNew);

	return nOld;
}

int TickratePlugin::ChangeInternal(int nNew)
{
	int nOld = Set(nNew);

	Logger::MessageFormat("The tickrate are changed from %d to %d\n", nOld, nNew);

	auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

	if(pNetServer)
	{
		for(const auto &pClient : pNetServer->m_Clients)
		{
			if(pClient->IsConnected() && !pClient->IsFakeClient())
			{
				auto aPlayerSlot = pClient->GetPlayerSlot();

				int iClient = aPlayerSlot.Get();

				Assert(0 <= iClient && iClient < ABSOLUTE_PLAYER_LIMIT);

				const auto &aPlayer = m_aPlayers[iClient];

				const auto &aPhrase = aPlayer.GetChangeTickratePhrase();

				if(aPhrase.m_pFormat && aPhrase.m_pContent)
				{
					CSingleRecipientFilter aFilter(aPlayerSlot);

					SendTextMessage(&aFilter, HUD_PRINTTALK, 1, aPhrase.m_pContent->Format(*aPhrase.m_pFormat, 2, nOld, nNew).Get());
				}
			}
		}
	}

	return nOld;
}

void TickratePlugin::ChangeGlobals(CGlobalVars *pGlobals, const CChangedData &aData)
{
	if(IsChannelEnabled(LS_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat, 
		           &aConcat2 = s_aEmbed2Concat;

		CBufferStringGrowable<1024> sMessage;

		sMessage.Format("Global vars:\n");
		DumpGlobalVars(aConcat, aConcat2, sMessage, pGlobals);

		Logger::Detailed(sMessage);
	}

	float flNewInterval = aData.GetNewInterval(), 
	      flMultiple = aData.GetMultiple();

	pGlobals->absoluteframetime = flNewInterval;
	pGlobals->absoluteframestarttimestddev = flNewInterval;

	pGlobals->frametime *= flMultiple;
	pGlobals->curtime *= flMultiple;
	pGlobals->rendertime *= flMultiple;
}

bool TickratePlugin::Init()
{
	if(IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}

	return true;
}

void TickratePlugin::PostInit()
{
	if(IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}
}

void TickratePlugin::Shutdown()
{
	if(IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s\n", __FUNCTION__);
	}
}

GS_EVENT_MEMBER(TickratePlugin, GameFrameBoundary)
{
	if(m_aEnableFrameDetailsConVar.GetValue() && m_aEnableFrameDetailsConVar.GetValue() && IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s:\n", __FUNCTION__);

		{
			const auto &aConcat = s_aEmbedConcat;

			CBufferStringGrowable<1024> sBuffer;

			DumpEventFrameBoundary(aConcat, sBuffer, msg);
			Logger::Detailed(sBuffer);
		}
	}
}

GS_EVENT_MEMBER(TickratePlugin, OutOfGameFrameBoundary)
{
	if(m_aEnableFrameDetailsConVar.GetValue() && IsChannelEnabled(LS_DETAILED))
	{
		Logger::DetailedFormat("%s:\n", __FUNCTION__);

		{
			const auto &aConcat = s_aEmbedConcat;

			CBufferStringGrowable<1024> sBuffer;

			DumpEventFrameBoundary(aConcat, sBuffer, msg);
			Logger::Detailed(sBuffer);
		}
	}
}

bool TickratePlugin::InitProvider(char *error, size_t maxlen)
{
	GameData::CBufferStringVector vecMessages;

	bool bResult = Provider::Init(vecMessages);

	if(vecMessages.Count())
	{
		if(IsChannelEnabled(LS_WARNING))
		{
			auto aWarnings = Logger::CreateWarningsScope();

			FOR_EACH_VEC(vecMessages, i)
			{
				auto &aMessage = vecMessages[i];

				aWarnings.Push(aMessage.Get());
			}

			aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
			{
				Logger::Warning(rgba, sContext);
			});
		}
	}

	if(!bResult)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to initialize provider. See warnings", maxlen);
		}
	}

	return bResult;
}

bool TickratePlugin::LoadProvider(char *error, size_t maxlen)
{
	GameData::CBufferStringVector vecMessages;

	bool bResult = Provider::Load(TICKRATE_BASE_DIR, TICKRATE_BASE_PATHID, vecMessages);

	if(vecMessages.Count())
	{
		if(IsChannelEnabled(LS_WARNING))
		{
			auto aWarnings = Logger::CreateWarningsScope();

			FOR_EACH_VEC(vecMessages, i)
			{
				auto &aMessage = vecMessages[i];

				aWarnings.Push(aMessage.Get());
			}

			aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
			{
				Logger::Warning(rgba, sContext);
			});
		}
	}

	if(!bResult)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to load provider. See warnings", maxlen);
		}
	}

	return bResult;
}

bool TickratePlugin::UnloadProvider(char *error, size_t maxlen)
{
	GameData::CBufferStringVector vecMessages;

	bool bResult = Provider::Destroy(vecMessages);

	if(vecMessages.Count())
	{
		if(IsChannelEnabled(LS_WARNING))
		{
			auto aWarnings = Logger::CreateWarningsScope();

			FOR_EACH_VEC(vecMessages, i)
			{
				auto &aMessage = vecMessages[i];

				aWarnings.Push(aMessage.Get());
			}

			aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
			{
				Logger::Warning(rgba, sContext);
			});
		}
	}

	if(!bResult)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unload provider. See warnings", maxlen);
		}
	}

	return bResult;
}

bool TickratePlugin::RegisterGameResource(char *error, size_t maxlen)
{
	CGameEntitySystem **pGameEntitySystem = GetGameEntitySystemPointer();

	if(!pGameEntitySystem)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to get a game entity system", maxlen);
		}
	}

	Logger::DetailedFormat("RegisterGameEntitySystem\n");

	if(!RegisterGameEntitySystem(*pGameEntitySystem))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a (game) entity system", maxlen);
		}

		return false;
	}

	return true;
}

bool TickratePlugin::UnregisterGameResource(char *error, size_t maxlen)
{
	if(!UnregisterGameEntitySystem())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unregister a (game) entity system", maxlen);
		}

		return false;
	}

	return true;
}

bool TickratePlugin::RegisterGameFactory(char *error, size_t maxlen)
{
	CBaseGameSystemFactory **ppFactory = GetGameDataStorage().GetGameSystem().GetFirstPointer();

	if(!ppFactory)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to get a first game system factory", maxlen);
		}

		return false;
	}

	if(!RegisterFirstGameSystem(ppFactory))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a first game factory", maxlen);
		}

		return false;
	}

	m_pFactory = new CGameSystemStaticFactory<TickratePlugin>(GetName(), this);

	return true;
}

bool TickratePlugin::UnregisterGameFactory(char *error, size_t maxlen)
{
	if(m_pFactory)
	{
		m_pFactory->Shutdown();
		m_pFactory->DestroyGameSystem(this);
	}

	if(!UnregisterFirstGameSystem())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unregister a first game factory", maxlen);
		}

		return false;
	}

	return true;
}

bool TickratePlugin::RegisterSource2Server(char *error, size_t maxlen)
{
	IGameEventManager2 **ppGameEventManager = GetGameEventManagerPointer();

	if(!ppGameEventManager)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to get a game event manager", maxlen);
		}

		return false;
	}

	if(!RegisterGameEventManager(*ppGameEventManager))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a game event manager", maxlen);
		}

		return false;
	}

	return true;
}

bool TickratePlugin::UnregisterSource2Server(char *error, size_t maxlen)
{
	if(!UnregisterGameEventManager())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a game event manager", maxlen);
		}

		return false;
	}

	return true;
}

bool TickratePlugin::RegisterTick(char *error, size_t maxlen)
{
	float *pTickInterval = GetGameDataStorage().GetTick().GetIntervalPointer();

	if(!pTickInterval)
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to get a tick interval", maxlen);
		}

		return false;
	}

	if(SourceHook::GetPageBits(pTickInterval, &m_iTickIntervalPageBits))
	{
		SourceHook::SetMemAccess(pTickInterval, sizeof(pTickInterval), m_iTickIntervalPageBits | SH_MEM_WRITE);
	}

	if(!RegisterTickInterval(pTickInterval))
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to register a game event manager", maxlen);
		}

		return false;
	}

	return true;
}

bool TickratePlugin::UnregisterTick(char *error, size_t maxlen)
{
	float *pTickInterval = GetGameDataStorage().GetTick().GetIntervalPointer();

	if(!pTickInterval)
	{
		return true;
	}

	SourceHook::SetMemAccess(pTickInterval, sizeof(pTickInterval), m_iTickIntervalPageBits);

	return true;
}

bool TickratePlugin::RegisterNetMessages(char *error, size_t maxlen)
{
	const struct
	{
		const char *pszName;
		INetworkMessageInternal **ppInternal;
	} aMessageInitializers[] =
	{
		{
			"CSVCMsg_GetCvarValue",
			&m_pGetCvarValueMessage,
		},
		{
			"CUserMessageSayText2",
			&m_pSayText2Message,
		},
		{
			"CUserMessageTextMsg",
			&m_pTextMsgMessage,
		},
	};

	for(const auto &aMessageInitializer : aMessageInitializers)
	{
		const char *pszMessageName = aMessageInitializer.pszName;

		INetworkMessageInternal *pMessage = g_pNetworkMessages->FindNetworkMessagePartial(pszMessageName);

		if(!pMessage)
		{
			if(error && maxlen)
			{
				snprintf(error, maxlen, "Failed to get \"%s\" message", pszMessageName);
			}

			return false;
		}

		*aMessageInitializer.ppInternal = pMessage;
	}

	return true;
}

bool TickratePlugin::UnregisterNetMessages(char *error, size_t maxlen)
{
	m_pSayText2Message = NULL;

	return true;
}

bool TickratePlugin::ParseLanguages(char *error, size_t maxlen)
{
	const char *pszPathID = TICKRATE_BASE_PATHID, 
	           *pszLanguagesFiles = TICKRATE_GAME_LANGUAGES_PATH_FILES;

	CUtlVector<CUtlString> vecLangugesFiles;
	CUtlVector<CUtlString> vecSubmessages;

	CUtlString sMessage;

	auto aWarnings = Logger::CreateWarningsScope();

	AnyConfig::LoadFromFile_Generic_t aLoadPresets({{&sMessage, NULL, pszPathID}, g_KV3Format_Generic});

	g_pFullFileSystem->FindFileAbsoluteList(vecLangugesFiles, pszLanguagesFiles, pszPathID);

	if(!vecLangugesFiles.Count())
	{
		if(error && maxlen)
		{
			snprintf(error, maxlen, "No found languages by \"%s\" path", pszLanguagesFiles);
		}

		return false;
	}

	for(const auto &sFile : vecLangugesFiles)
	{
		const char *pszFilename = sFile.Get();

		AnyConfig::Anyone aLanguagesConfig;

		aLoadPresets.m_pszFilename = pszFilename;

		if(!aLanguagesConfig.Load(aLoadPresets))
		{
			aWarnings.PushFormat("\"%s\": %s", pszFilename, sMessage.Get());

			continue;
		}

		if(!ParseLanguages(aLanguagesConfig.Get(), vecSubmessages))
		{
			aWarnings.PushFormat("\"%s\"", pszFilename);

			for(const auto &sSubmessage : vecSubmessages)
			{
				aWarnings.PushFormat("\t%s", sSubmessage.Get());
			}

			continue;
		}
	}

	if(aWarnings.Count())
	{
		aWarnings.Send([&](const CUtlString &sMessage)
		{
			Logger::Warning(sMessage);
		});
	}

	return true;
}

bool TickratePlugin::ParseLanguages(KeyValues3 *pRoot, CUtlVector<CUtlString> &vecMessages)
{
	int iMemberCount = pRoot->GetMemberCount();

	if(!iMemberCount)
	{
		vecMessages.AddToTail("No members");

		return true;
	}

	const KeyValues3 *pDefaultData = pRoot->FindMember("default");

	const char *pszServerContryCode = pDefaultData ? pDefaultData->GetString() : "en";

	m_aServerLanguage.SetCountryCode(pszServerContryCode);

	for(KV3MemberId_t n = 0; n < iMemberCount; n++)
	{
		const char *pszMemberName = pRoot->GetMemberName(n);

		auto sMemberSymbol = GetLanguageSymbol(pszMemberName);

		const KeyValues3 *pMember = pRoot->GetMember(n);

		const char *pszMemberValue = pMember->GetString(pszServerContryCode);

		m_mapLanguages.Insert(sMemberSymbol, {sMemberSymbol, pszMemberValue});
	}

	return true;
}

bool TickratePlugin::ClearLanguages(char *error, size_t maxlen)
{
	m_vecLanguages.Purge();

	return true;
}

bool TickratePlugin::ParseTranslations(char *error, size_t maxlen)
{
	const char *pszPathID = TICKRATE_BASE_PATHID, 
	           *pszTranslationsFiles = TICKRATE_GAME_TRANSLATIONS_PATH_FILES;

	CUtlVector<CUtlString> vecTranslationsFiles;

	Translations::CBufferStringVector vecSubmessages;

	CUtlString sMessage;

	auto aWarnings = Logger::CreateWarningsScope();

	AnyConfig::LoadFromFile_Generic_t aLoadPresets({{&sMessage, NULL, pszPathID}, g_KV3Format_Generic});

	g_pFullFileSystem->FindFileAbsoluteList(vecTranslationsFiles, pszTranslationsFiles, pszPathID);

	if(!vecTranslationsFiles.Count())
	{
		if(error && maxlen)
		{
			snprintf(error, maxlen, "No found translations by \"%s\" path", pszTranslationsFiles);
		}

		return false;
	}

	for(const auto &sFile : vecTranslationsFiles)
	{
		const char *pszFilename = sFile.Get();

		AnyConfig::Anyone aTranslationsConfig;

		aLoadPresets.m_pszFilename = pszFilename;

		if(!aTranslationsConfig.Load(aLoadPresets))
		{
			aWarnings.PushFormat("\"%s\": %s", pszFilename, sMessage.Get());

			continue;
		}

		if(!Translations::Parse(aTranslationsConfig.Get(), vecSubmessages))
		{
			aWarnings.PushFormat("\"%s\"", pszFilename);

			for(const auto &sSubmessage : vecSubmessages)
			{
				aWarnings.PushFormat("\t%s", sSubmessage.Get());
			}

			continue;
		}
	}

	if(aWarnings.Count())
	{
		aWarnings.Send([&](const CUtlString &sMessage)
		{
			Logger::Warning(sMessage);
		});
	}

	return true;
}

bool TickratePlugin::ClearTranslations(char *error, size_t maxlen)
{
	Translations::Purge();

	return true;
}

void TickratePlugin::OnReloadGameDataCommand(const CCommandContext &context, const CCommand &args)
{
	char error[256];

	if(!LoadProvider(error, sizeof(error)))
	{
		META_LOG(this, "%s", error);
	}
}

void TickratePlugin::OnDispatchConCommandHook(ConCommandHandle hCommand, const CCommandContext &aContext, const CCommand &aArgs)
{
	if(IsChannelEnabled(LV_DETAILED))
	{
		Logger::DetailedFormat("%s(%d, %d, %s)\n", __FUNCTION__, hCommand.GetIndex(), aContext.GetPlayerSlot().Get(), aArgs.GetCommandString());
	}

	auto aPlayerSlot = aContext.GetPlayerSlot();

	const char *pszArg0 = aArgs.Arg(0);

	static const char szSayCommand[] = "say";

	size_t nSayNullTerminated = sizeof(szSayCommand) - 1;

	if(!V_strncmp(pszArg0, (const char *)szSayCommand, nSayNullTerminated))
	{
		if(!pszArg0[nSayNullTerminated] || !V_strcmp(&pszArg0[nSayNullTerminated], "_team"))
		{
			const char *pszArg1 = aArgs.Arg(1);

			// Skip spaces.
			while(*pszArg1 == ' ')
			{
				pszArg1++;
			}

			bool bIsSilent = *pszArg1 == Tickrate::ChatCommandSystem::GetSilentTrigger();

			if(bIsSilent || *pszArg1 == Tickrate::ChatCommandSystem::GetPublicTrigger())
			{
				pszArg1++; // Skip a command character.

				// Print a chat message before.
				if(!bIsSilent && g_pCVar)
				{
					SH_CALL(g_pCVar, &ICvar::DispatchConCommand)(hCommand, aContext, aArgs);
				}

				// Call the handler.
				{
					size_t nArg1Length = 0;

					// Get a length to a first space.
					while(pszArg1[nArg1Length] && pszArg1[nArg1Length] != ' ')
					{
						nArg1Length++;
					}

					CUtlVector<CUtlString> vecArgs;

					V_SplitString(pszArg1, " ", vecArgs);

					for(auto &sArg : vecArgs)
					{
						sArg.Trim(' ');
					}

					if(IsChannelEnabled(LV_DETAILED))
					{
						const auto &aConcat = s_aEmbedConcat, 
						           &aConcat2 = s_aEmbed2Concat;

						CBufferStringGrowable<1024> sBuffer;

						sBuffer.Format("Handle a chat command:\n");
						aConcat.AppendToBuffer(sBuffer, "Player slot", aPlayerSlot.Get());
						aConcat.AppendToBuffer(sBuffer, "Is silent", bIsSilent);
						aConcat.AppendToBuffer(sBuffer, "Arguments");

						for(const auto &sArg : vecArgs)
						{
							const char *pszMessageConcat[] = {aConcat2.m_aStartWith, "\"", sArg.Get(), "\"", aConcat2.m_aEnd};

							sBuffer.AppendConcat(ARRAYSIZE(pszMessageConcat), pszMessageConcat, NULL);
						}

						Logger::Detailed(sBuffer);
					}

					Tickrate::ChatCommandSystem::Handle(aPlayerSlot, bIsSilent, vecArgs);
				}

				RETURN_META(MRES_SUPERCEDE);
			}
		}
	}

	RETURN_META(MRES_IGNORED);
}

void TickratePlugin::OnStartupServerHook(const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession, const char *)
{
	auto *pNetServer = reinterpret_cast<CNetworkGameServerBase *>(g_pNetworkServerService->GetIGameServer());

	OnStartupServer(pNetServer, config, pWorldSession);

	RETURN_META(MRES_IGNORED);
}

CServerSideClientBase *TickratePlugin::OnConnectClientHook(const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, 
                                                         const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence)
{
	auto *pNetServer = META_IFACEPTR(CNetworkGameServerBase);

	auto *pClient = META_RESULT_ORIG_RET(CServerSideClientBase *);

	OnConnectClient(pNetServer, pClient, pszName, pAddr, socket, pSplitPlayer, pszChallenge, pAuthTicket, nAuthTicketLength, bIsLowViolence);

	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

bool TickratePlugin::OnProcessRespondCvarValueHook(const CCLCMsg_RespondCvarValue_t &aMessage)
{
	auto *pClient = META_IFACEPTR(CServerSideClientBase);

	OnProcessRespondCvarValue(pClient, aMessage);

	RETURN_META_VALUE(MRES_IGNORED, true);
}

void TickratePlugin::OnDisconectClientHook(ENetworkDisconnectionReason eReason)
{
	auto *pClient = META_IFACEPTR(CServerSideClientBase);

	OnDisconectClient(pClient, eReason);

	RETURN_META(MRES_IGNORED);
}

void TickratePlugin::DumpProtobufMessage(const ConcatLineString &aConcat, CBufferString &sOutput, const google::protobuf::Message &aMessage)
{
	CBufferStringGrowable<1024> sProtoOutput;

	sProtoOutput.Insert(0, aMessage.DebugString().c_str());
	sProtoOutput.Replace("\n", aConcat.m_aEndAndNextLine);
	sProtoOutput.SetLength(sProtoOutput.GetTotalNumber() - (V_strlen(aConcat.m_aEndAndNextLine) - 1)); // Strip the last next line, leaving the end.

	const char *pszProtoConcat[] = {aConcat.m_aStartWith, sProtoOutput.Get()};

	sOutput.AppendConcat(ARRAYSIZE(pszProtoConcat), pszProtoConcat, NULL);
}

void TickratePlugin::DumpGlobalVars(const ConcatLineString &aConcat, CBufferString &sOutput, const CGlobalVarsBase *pGlobals)
{
	aConcat.AppendToBuffer(sOutput, "Real time", pGlobals->realtime);
	aConcat.AppendToBuffer(sOutput, "Frame count", pGlobals->framecount);
	aConcat.AppendToBuffer(sOutput, "Absolute frame time", pGlobals->absoluteframetime);
	aConcat.AppendToBuffer(sOutput, "Absolute frame start time STD (Dev)", pGlobals->absoluteframestarttimestddev);
	aConcat.AppendToBuffer(sOutput, "Max clients", pGlobals->maxClients);
	aConcat.AppendToBuffer(sOutput, "Unknown (#1)", pGlobals->unknown1);
	aConcat.AppendToBuffer(sOutput, "Unknown (#2)", pGlobals->unknown2);
	aConcat.AppendToBuffer(sOutput, "Unknown (#3)", pGlobals->unknown3);
	aConcat.AppendToBuffer(sOutput, "Unknown (#4)", pGlobals->unknown4);
	aConcat.AppendToBuffer(sOutput, "Unknown (#5)", pGlobals->unknown5);
	aConcat.AppendToBuffer(sOutput, "Warning function", pGlobals->m_pfnWarningFunc);
	aConcat.AppendToBuffer(sOutput, "Frame time", pGlobals->frametime);
	aConcat.AppendToBuffer(sOutput, "Current time", pGlobals->curtime);
	aConcat.AppendToBuffer(sOutput, "Render time", pGlobals->rendertime);
	aConcat.AppendToBuffer(sOutput, "Unknown (#6)", pGlobals->unknown6);
	aConcat.AppendToBuffer(sOutput, "Unknown (#7)", pGlobals->unknown7);
	aConcat.AppendToBuffer(sOutput, "Is simulation", pGlobals->m_bInSimulation);
	aConcat.AppendToBuffer(sOutput, "Is enable assertions", pGlobals->m_bEnableAssertions);
	aConcat.AppendToBuffer(sOutput, "Tick count", pGlobals->tickcount);
	aConcat.AppendToBuffer(sOutput, "Unknown (#8)", pGlobals->unknown8);
	aConcat.AppendToBuffer(sOutput, "Unknown (#9)", pGlobals->unknown9);
	aConcat.AppendToBuffer(sOutput, "Subtick fraction", pGlobals->m_flSubtickFraction);
}
void TickratePlugin::DumpGlobalVars(const ConcatLineString &aConcat, const ConcatLineString &aConcat2, CBufferString &sOutput, const CGlobalVars *pGlobals)
{
	aConcat.AppendToBuffer(sOutput, "Base");
	DumpGlobalVars(aConcat2, sOutput, reinterpret_cast<const CGlobalVarsBase *>(pGlobals));
	aConcat.AppendStringToBuffer(sOutput, "Map name", pGlobals->mapname.ToCStr());
	aConcat.AppendStringToBuffer(sOutput, "Start spot", pGlobals->startspot.ToCStr());
	aConcat.AppendToBuffer(sOutput, "Map name", static_cast<int>(pGlobals->eLoadType));
	aConcat.AppendToBuffer(sOutput, "Is team play", pGlobals->mp_teamplay);
	aConcat.AppendToBuffer(sOutput, "Max entities", pGlobals->maxEntities);
	aConcat.AppendToBuffer(sOutput, "Server count", pGlobals->serverCount);
}

void TickratePlugin::DumpEngineLoopState(const ConcatLineString &aConcat, CBufferString &sOutput, const EngineLoopState_t &aMessage)
{
	aConcat.AppendHandleToBuffer(sOutput, "Window handle", aMessage.m_hWnd);
	aConcat.AppendHandleToBuffer(sOutput, "Swap chain handle", aMessage.m_hSwapChain);
	aConcat.AppendHandleToBuffer(sOutput, "Input context handle", aMessage.m_hInputContext);
	aConcat.AppendToBuffer(sOutput, "Window width", aMessage.m_nPlatWindowWidth);
	aConcat.AppendToBuffer(sOutput, "Window height", aMessage.m_nPlatWindowHeight);
	aConcat.AppendToBuffer(sOutput, "Render width", aMessage.m_nRenderWidth);
	aConcat.AppendToBuffer(sOutput, "Render height", aMessage.m_nRenderHeight);
}

void TickratePlugin::DumpEntityList(const ConcatLineString &aConcat, CBufferString &sOutput, const CUtlVector<CEntityHandle> &vecEntityList)
{
	for(const auto &it : vecEntityList)
	{
		aConcat.AppendToBuffer(sOutput, it.Get()->GetClassname(), it.GetEntryIndex());
	}
}

void TickratePlugin::DumpEventSimulate(const ConcatLineString &aConcat, const ConcatLineString &aConcat2, CBufferString &sOutput, const EventSimulate_t &aMessage)
{
	aConcat.AppendToBuffer(sOutput, "Loop state");
	DumpEngineLoopState(aConcat2, sOutput, aMessage.m_LoopState);
	aConcat.AppendToBuffer(sOutput, "First tick", aMessage.m_bFirstTick);
	aConcat.AppendToBuffer(sOutput, "Last tick", aMessage.m_bLastTick);
}

void TickratePlugin::DumpEventFrameBoundary(const ConcatLineString &aConcat, CBufferString &sOutput, const EventFrameBoundary_t &aMessage)
{
	aConcat.AppendToBuffer(sOutput, "Frame time", aMessage.m_flFrameTime);
}

void TickratePlugin::DumpServerSideClient(const ConcatLineString &aConcat, CBufferString &sOutput, CServerSideClientBase *pClient)
{
	aConcat.AppendStringToBuffer(sOutput, "Name", pClient->GetClientName());
	aConcat.AppendToBuffer(sOutput, "Player slot", pClient->GetPlayerSlot().Get());
	aConcat.AppendToBuffer(sOutput, "Entity index", pClient->GetEntityIndex().Get());
	aConcat.AppendToBuffer(sOutput, "UserID", pClient->GetUserID().Get());
	aConcat.AppendToBuffer(sOutput, "Signon state", pClient->GetSignonState());
	aConcat.AppendToBuffer(sOutput, "SteamID", pClient->GetClientSteamID().Render());
	aConcat.AppendToBuffer(sOutput, "Is fake", pClient->IsFakeClient());
	aConcat.AppendToBuffer(sOutput, "Address", pClient->GetRemoteAddress()->ToString());
	aConcat.AppendToBuffer(sOutput, "Low violence", pClient->IsLowViolenceClient());
}

void TickratePlugin::DumpDisconnectReason(const ConcatLineString &aConcat, CBufferString &sOutput, ENetworkDisconnectionReason eReason)
{
	aConcat.AppendToBuffer(sOutput, "Disconnect reason", (int)eReason);
}

void TickratePlugin::SendCvarValueQuery(IRecipientFilter *pFilter, const char *pszName, int iCookie)
{
	auto *pGetCvarValueMessage = m_pGetCvarValueMessage;

	if(IsChannelEnabled(LV_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat;

		CBufferStringGrowable<1024> sBuffer;

		sBuffer.Format("Send get cvar message (%s):\n", pGetCvarValueMessage->GetUnscopedName());
		aConcat.AppendStringToBuffer(sBuffer, "Cvar name", pszName);
		aConcat.AppendToBuffer(sBuffer, "Cookie", iCookie);

		Logger::Detailed(sBuffer);
	}

	auto *pMessage = pGetCvarValueMessage->AllocateMessage()->ToPB<CSVCMsg_GetCvarValue>();

	pMessage->set_cvar_name(pszName);
	pMessage->set_cookie(iCookie);

	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pGetCvarValueMessage, pMessage, 0);

	delete pMessage;
}

void TickratePlugin::SendChatMessage(IRecipientFilter *pFilter, int iEntityIndex, bool bIsChat, const char *pszChatMessageFormat, const char *pszParam1, const char *pszParam2, const char *pszParam3, const char *pszParam4)
{
	auto *pSayText2Message = m_pSayText2Message;

	if(IsChannelEnabled(LV_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat;

		CBufferStringGrowable<1024> sBuffer;

		sBuffer.Format("Send chat message (%s):\n", pSayText2Message->GetUnscopedName());
		aConcat.AppendToBuffer(sBuffer, "Entity index", iEntityIndex);
		aConcat.AppendToBuffer(sBuffer, "Is chat", bIsChat);
		aConcat.AppendStringToBuffer(sBuffer, "Chat message", pszChatMessageFormat);

		if(pszParam1 && *pszParam1)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #1", pszParam1);
		}

		if(pszParam2 && *pszParam2)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #2", pszParam2);
		}

		if(pszParam3 && *pszParam3)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #3", pszParam3);
		}

		if(pszParam4 && *pszParam4)
		{
			aConcat.AppendStringToBuffer(sBuffer, "Parameter #4", pszParam4);
		}

		Logger::Detailed(sBuffer);
	}

	auto *pMessage = pSayText2Message->AllocateMessage()->ToPB<CUserMessageSayText2>();

	pMessage->set_entityindex(iEntityIndex);
	pMessage->set_chat(bIsChat);
	pMessage->set_messagename(pszChatMessageFormat);
	pMessage->set_param1(pszParam1);
	pMessage->set_param2(pszParam2);
	pMessage->set_param3(pszParam3);
	pMessage->set_param4(pszParam4);

	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pSayText2Message, pMessage, 0);

	delete pMessage;
}

void TickratePlugin::SendTextMessage(IRecipientFilter *pFilter, int iDestination, size_t nParamCount, const char *pszParam, ...)
{
	auto *pTextMsg = m_pTextMsgMessage;

	if(IsChannelEnabled(LV_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat;

		CBufferStringGrowable<1024> sBuffer;

		sBuffer.Format("Send message (%s):\n", pTextMsg->GetUnscopedName());
		aConcat.AppendToBuffer(sBuffer, "Destination", iDestination);
		aConcat.AppendToBuffer(sBuffer, "Parameter", pszParam);
		Logger::Detailed(sBuffer);
	}

	auto *pMessage = pTextMsg->AllocateMessage()->ToPB<CUserMessageTextMsg>();

	pMessage->set_dest(iDestination);
	pMessage->add_param(pszParam);
	nParamCount--;

	// Parse incoming parameters.
	if(nParamCount)
	{
		va_list aParams;

		va_start(aParams, pszParam);

		for(size_t n = 0; n < nParamCount; n++)
		{
			pMessage->add_param(va_arg(aParams, const char *));
		}

		va_end(aParams);
	}

	g_pGameEventSystem->PostEventAbstract(-1, false, pFilter, pTextMsg, pMessage, 0);

	delete pMessage;
}

void TickratePlugin::OnStartupServer(CNetworkGameServerBase *pNetServer, const GameSessionConfiguration_t &config, ISource2WorldSession *pWorldSession)
{
	SH_ADD_HOOK_MEMFUNC(CNetworkGameServerBase, ConnectClient, pNetServer, this, &TickratePlugin::OnConnectClientHook, true);

	// Initialize & hook game evetns.
	// Initialize network messages.
	{
		char sMessage[256];

		if(!RegisterSource2Server(sMessage, sizeof(sMessage)))
		{
			Logger::WarningFormat("%s\n", sMessage);
		}

		if(!RegisterNetMessages(sMessage, sizeof(sMessage)))
		{
			Logger::WarningFormat("%s\n", sMessage);
		}
	}

	auto *pGlobals = pNetServer->GetGlobals();

	if(IsChannelEnabled(LS_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat, 
		           &aConcat2 = s_aEmbed2Concat;

		CBufferStringGrowable<1024> sMessage;

#ifndef _WIN32
		try
		{
			sMessage.Format("Receive %s message:\n", config.GetTypeName().c_str());
			DumpProtobufMessage(aConcat, sMessage, config);
		}
		catch(const std::exception &aError)
		{
			sMessage.Format("Receive a proto message: %s\n", aError.what());
		}

#endif
		sMessage.AppendFormat("Register globals:\n");
		DumpRegisterGlobals(aConcat, sMessage);

		if(pGlobals)
		{
			sMessage.AppendFormat("Global vars:\n");
			DumpGlobalVars(aConcat, aConcat2, sMessage, pGlobals);
		}

		Logger::Detailed(sMessage);
	}
}

void TickratePlugin::OnConnectClient(CNetworkGameServerBase *pNetServer, CServerSideClientBase *pClient, const char *pszName, ns_address *pAddr, int socket, CCLCMsg_SplitPlayerConnect_t *pSplitPlayer, const char *pszChallenge, const byte *pAuthTicket, int nAuthTicketLength, bool bIsLowViolence)
{
	SH_ADD_HOOK_MEMFUNC(CServerSideClientBase, ProcessRespondCvarValue, pClient, this, &TickratePlugin::OnProcessRespondCvarValueHook, false);
	SH_ADD_HOOK_MEMFUNC(CServerSideClientBase, PerformDisconnection, pClient, this, &TickratePlugin::OnDisconectClientHook, false);

	if(IsChannelEnabled(LS_DETAILED))
	{
		const auto &aConcat = s_aEmbedConcat;

		CBufferStringGrowable<1024> sMessage;

		sMessage.Insert(0, "Connect a client:\n");
		DumpServerSideClient(aConcat, sMessage, pClient);

		if(socket)
		{
			aConcat.AppendHandleToBuffer(sMessage, "Socket", (uint32)socket);
		}

		if(pAuthTicket && nAuthTicketLength)
		{
			aConcat.AppendBytesToBuffer(sMessage, "Auth ticket", pAuthTicket, nAuthTicketLength);
		}

		Logger::Detailed(sMessage);
	}

	// Get "cl_language" cvar value from a client.
	{
		CSingleRecipientFilter aFilter(pClient->GetPlayerSlot());

		const char *pszCvarName = TICKRATE_CLIENT_CVAR_NAME_LANGUAGE;

		int iCookie {};

		{
			auto sConVarSymbol = GetConVarSymbol(pszCvarName);

			auto iFound = m_mapConVarCookies.Find(sConVarSymbol);

			if(m_mapConVarCookies.IsValidIndex(iFound))
			{
				auto &iFoundCookie = m_mapConVarCookies.Element(iFound);

				iFoundCookie++;
				iCookie = iFoundCookie;
			}
			else
			{
				iCookie = 0;
				m_mapConVarCookies.Insert(sConVarSymbol, iCookie);
			}
		}

		SendCvarValueQuery(&aFilter, pszCvarName, iCookie);
	}
}

bool TickratePlugin::OnProcessRespondCvarValue(CServerSideClientBase *pClient, const CCLCMsg_RespondCvarValue_t &aMessage)
{
	auto sFoundSymbol = FindConVarSymbol(aMessage.name().c_str());

	if(!sFoundSymbol.IsValid())
	{
		return false;
	}

	auto iFound = m_mapConVarCookies.Find(sFoundSymbol);

	if(!m_mapConVarCookies.IsValidIndex(iFound))
	{
		return false;
	}

	const auto &itCookie = m_mapConVarCookies.Element(iFound);

	if(itCookie != aMessage.cookie())
	{
		return false;
	}

	auto iLanguageFound = m_mapLanguages.Find(FindLanguageSymbol(aMessage.value().c_str()));

	if(!m_mapLanguages.IsValidIndex(iLanguageFound))
	{
		return false;
	}

	auto aPlayerSlot = pClient->GetPlayerSlot();

	auto &itLanguage = m_mapLanguages.Element(iLanguageFound);

	int iClient = aPlayerSlot.Get();

	Assert(0 <= iClient && iClient < ABSOLUTE_PLAYER_LIMIT);

	auto &aPlayer = m_aPlayers[iClient];

	aPlayer.OnLanguageReceived(aPlayerSlot, &itLanguage);

	{
		CUtlVector<CUtlString> vecMessages;

		auto aWarnings = Logger::CreateWarningsScope();

		aPlayer.TranslatePhrases(this, this->m_aServerLanguage, vecMessages);

		for(const auto &sMessage : vecMessages)
		{
			aWarnings.Push(sMessage.Get());
		}

		aWarnings.SendColor([&](Color rgba, const CUtlString &sContext)
		{
			Logger::Warning(rgba, sContext);
		});
	}

	return true;
}

void TickratePlugin::OnDisconectClient(CServerSideClientBase *pClient, ENetworkDisconnectionReason eReason)
{
	SH_REMOVE_HOOK_MEMFUNC(CServerSideClientBase, ProcessRespondCvarValue, pClient, this, &TickratePlugin::OnProcessRespondCvarValueHook, false);
	SH_REMOVE_HOOK_MEMFUNC(CServerSideClientBase, PerformDisconnection, pClient, this, &TickratePlugin::OnDisconectClientHook, false);

	if(IsChannelEnabled(LS_DETAILED))
	{
		CBufferStringGrowable<1024> sMessage;

		const auto &aConcat = s_aEmbedConcat;

		sMessage.Insert(0, "Disconnect a client:\n");
		DumpServerSideClient(aConcat, sMessage, pClient);
		DumpDisconnectReason(aConcat, sMessage, eReason);

		Logger::Detailed(sMessage);
	}
}

CUtlSymbolLarge TickratePlugin::GetConVarSymbol(const char *pszName)
{
	return m_tableConVars.AddString(pszName);
}

CUtlSymbolLarge TickratePlugin::FindConVarSymbol(const char *pszName) const
{
	return m_tableConVars.Find(pszName);
}

CUtlSymbolLarge TickratePlugin::GetLanguageSymbol(const char *pszName)
{
	return m_tableLanguages.AddString(pszName);
}

CUtlSymbolLarge TickratePlugin::FindLanguageSymbol(const char *pszName) const
{
	return m_tableLanguages.Find(pszName);
}
