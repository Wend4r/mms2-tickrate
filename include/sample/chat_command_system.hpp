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

#ifndef _INCLUDE_METAMOD_SOURCE_SAMPLE_CHAT_COMMAND_HPP_
#	define _INCLUDE_METAMOD_SOURCE_SAMPLE_CHAT_COMMAND_HPP_

#	pragma once

#	include <functional>

#	include <playerslot.h>
#	include <tier0/utlstring.h>
#	include <tier1/utlmap.h>
#	include <tier1/utlsymbollarge.h>
#	include <tier1/utlvector.h>

#	include <logger.hpp>

#	define SAMPLE_CHAT_COMMAND_SYSTEM_LOGGINING_COLOR {0, 127, 255, 191}

namespace Sample
{
	class ChatCommandSystem : virtual public Logger
	{
	public:
		ChatCommandSystem();

		using Callback_t = std::function<void (CPlayerSlot, bool, const CUtlVector<CUtlString> &)>;

	public:
		const char *GetName();

	public:
		bool Register(const char *pszName, const Callback_t &fnCallback);
		bool Unregister(const char *pszName);
		void UnregisterAll();

	public:
		static char GetPublicTrigger();
		static char GetSilentTrigger();

	public:
		bool Handle(CPlayerSlot aSlot, bool bIsSilent, const CUtlVector<CUtlString> &vecArgs);

	protected:
		CUtlSymbolLarge GetSymbol(const char *pszText);
		CUtlSymbolLarge FindSymbol(const char *pszText) const;

	private:
		CUtlSymbolTableLarge_CI m_aSymbolTable;
		CUtlMap<CUtlSymbolLarge, Callback_t> m_mapCallbacks;
	}; // ChatCommand
}; // Sample

#endif // _INCLUDE_METAMOD_SOURCE_SAMPLE_CHAT_COMMAND_HPP_
