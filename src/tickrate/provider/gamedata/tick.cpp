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

Tickrate::Provider::GameDataStorage::CTick::CTick()
{
	{
		auto &aCallbacks = m_aAddressCallbacks;

		aCallbacks.Insert(m_aGameConfig.GetSymbol("&tick_interval"), [&](const CUtlSymbolLarge &, const DynLibUtils::CMemory &aAddress)
		{
			m_pInterval = aAddress.RCast<decltype(m_pInterval)>();
		});

		aCallbacks.Insert(m_aGameConfig.GetSymbol("&(double)tick_interval"), [&](const CUtlSymbolLarge &, const DynLibUtils::CMemory &aAddress)
		{
			m_pInterval2 = aAddress.RCast<decltype(m_pInterval2)>();
		});

		aCallbacks.Insert(m_aGameConfig.GetSymbol("&server_tick_interval"), [&](const CUtlSymbolLarge &, const DynLibUtils::CMemory &aAddress)
		{
			m_pServerInterval = aAddress.RCast<decltype(m_pServerInterval)>();
		});

		m_aGameConfig.GetAddresses().AddListener(&aCallbacks);
	}
}

bool Tickrate::Provider::GameDataStorage::CTick::Load(IGameData *pRoot, KeyValues3 *pGameConfig, GameData::CBufferStringVector &vecMessages)
{
	return m_aGameConfig.Load(pRoot, pGameConfig, vecMessages);
}

void Tickrate::Provider::GameDataStorage::CTick::Reset()
{
	m_pInterval = nullptr;
	m_pInterval2 = nullptr;
	m_pServerInterval = nullptr;
}

float *Tickrate::Provider::GameDataStorage::CTick::GetIntervalPointer() const
{
	return m_pInterval;
}

double *Tickrate::Provider::GameDataStorage::CTick::GetInterval2Pointer() const
{
	return m_pInterval2;
}

float *Tickrate::Provider::GameDataStorage::CTick::GetServerIntervalPointer() const
{
	return m_pServerInterval;
}
