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

Tickrate::Provider::GameDataStorage::CSource2Server::CSource2Server()
{
	{
		auto &aCallbacks = m_aAddressCallbacks;

		aCallbacks.Insert(m_aGameConfig.GetSymbol("&s_GameEventManager"), [&](const CUtlSymbolLarge &, const DynLibUtils::CMemory &aAddress)
		{
			m_ppGameEventManager = aAddress.RCast<decltype(m_ppGameEventManager)>();
		});

		m_aGameConfig.GetAddresses().AddListener(&aCallbacks);
	}
}

bool Tickrate::Provider::GameDataStorage::CSource2Server::Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aGameConfig.Load(pRoot, pGameConfig, vecMessages);
}

void Tickrate::Provider::GameDataStorage::CSource2Server::Reset()
{
	m_ppGameEventManager = nullptr;
}

CGameEventManager **Tickrate::Provider::GameDataStorage::CSource2Server::GetGameEventManagerPointer() const
{
	return m_ppGameEventManager;
}
