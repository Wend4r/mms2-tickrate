/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source {project}
 * Written by {name of author} ({fullname}).
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

#include <globals.hpp>
#include <concat.hpp>

#include <logger.hpp>

#include <ISmmPlugin.h>

#include <entity2/entitysystem.h>
#include <igameeventsystem.h>
#include <igamesystemfactory.h>
#include <iserver.h>
#include <tier0/dbg.h>
#include <tier0/strtools.h>

IGameEventSystem *g_pGameEventSystem = NULL;
CEntitySystem *g_pEntitySystem = NULL;
CGameEntitySystem *g_pGameEntitySystem = NULL;
CBaseGameSystemFactory **CBaseGameSystemFactory::sm_pFirst = NULL;
IGameEventManager2 *g_pGameEventManager = NULL;

bool InitGlobals(SourceMM::ISmmAPI *ismm, char *error, size_t maxlen)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pEngineServer, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);

	return true;
}

bool RegisterGameEntitySystem(CGameEntitySystem *pGameEntitySystem)
{
	g_pEntitySystem = reinterpret_cast<CEntitySystem *>(pGameEntitySystem);
	g_pGameEntitySystem = pGameEntitySystem;

	return true;
}

bool UnregisterGameEntitySystem()
{
	g_pEntitySystem = NULL;
	g_pGameEntitySystem = NULL;

	return true;
}

bool RegisterFirstGameSystem(CBaseGameSystemFactory **ppFirstGameSystem)
{
	CBaseGameSystemFactory::sm_pFirst = ppFirstGameSystem;

	return true;
}

bool UnregisterFirstGameSystem()
{
	CBaseGameSystemFactory::sm_pFirst = NULL;

	return true;
}

bool RegisterGameEventManager(IGameEventManager2 *pGameEventManager)
{
	g_pGameEventManager = pGameEventManager;

	return true;
}

bool UnregisterGameEventManager()
{
	g_pGameEventManager = NULL;

	return true;
}

void DumpGlobals(const ConcatLineString &aConcat, CBufferString &sOutput)
{
	GLOBALS_APPEND_VARIABLE(g_pEngineServer);
	GLOBALS_APPEND_VARIABLE(g_pGameResourceServiceServer);
	GLOBALS_APPEND_VARIABLE(g_pGameEventSystem);
	GLOBALS_APPEND_VARIABLE(g_pNetworkMessages);
	GLOBALS_APPEND_VARIABLE(g_pCVar);
	GLOBALS_APPEND_VARIABLE(g_pFullFileSystem);
	GLOBALS_APPEND_VARIABLE(g_pSource2Server);
	GLOBALS_APPEND_VARIABLE(g_pNetworkServerService);

	DumpRegisterGlobals(aConcat, sOutput);
}

void DumpRegisterGlobals(const ConcatLineString &aConcat, CBufferString &sOutput)
{
	GLOBALS_APPEND_VARIABLE(g_pEntitySystem);
	GLOBALS_APPEND_VARIABLE(g_pGameEntitySystem);
	GLOBALS_APPEND_VARIABLE(CBaseGameSystemFactory::sm_pFirst);
	GLOBALS_APPEND_VARIABLE(g_pGameEventManager);
}

bool DestoryGlobals(char *error, size_t maxlen)
{
	g_pEngineServer = NULL;
	g_pGameResourceServiceServer = NULL;
	g_pGameEventSystem = NULL;
	g_pNetworkMessages = NULL;
	g_pCVar = NULL;
	g_pFullFileSystem = NULL;
	g_pSource2Server = NULL;
	g_pNetworkServerService = NULL;

	if(!UnregisterGameEntitySystem())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unregister a (game) entity system", maxlen);
		}

		return false;
	}

	if(!UnregisterFirstGameSystem())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unregister a first game system", maxlen);
		}

		return false;
	}

	if(!UnregisterFirstGameSystem())
	{
		if(error && maxlen)
		{
			strncpy(error, "Failed to unregister a game event manager", maxlen);
		}

		return false;
	}

	return true;
}

// AMNOTE: Should only be called within the active game loop (i e map should be loaded and active) 
// otherwise that'll be nullptr!
CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();

	if(!server)
	{
		AssertMsg(server, "Server is not ready");

		return NULL;
	}

	return server->GetGlobals();
}

CGameEntitySystem *GameEntitySystem()
{
	return g_pGameEntitySystem;
}

CEntityInstance* CEntityHandle::Get() const
{
	return GameEntitySystem()->GetEntityInstance( *this );
}
