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

#ifndef _INCLUDE_METAMOD_SOURCE_CONCAT_HPP_
#	define _INCLUDE_METAMOD_SOURCE_CONCAT_HPP_

#	pragma once

#	include <stddef.h>

#	include <vector>

#	include <tier0/platform.h>
#	include <tier0/bufferstring.h>
#	include <tier1/utlvector.h>

template<class T>
struct ConcatLine
{
	T m_aStartWith;
	T m_aPadding;
	T m_aEnd;
	T m_aEndAndNextLine;

	std::vector<T> GetKeyValueConcat(const T &aKey) const
	{
		return {m_aStartWith, aKey, m_aPadding, m_aEnd};
	}

	std::vector<T> GetKeyValueConcat(const T &aKey, const T &aValue) const
	{
		return {m_aStartWith, aKey, m_aPadding, aValue, m_aEnd};
	}

	std::vector<T> GetKeyValueConcat(const T &aKey, const std::vector<T> &vecValues) const
	{
		std::vector<T> vecResult = {m_aStartWith, aKey, m_aPadding};

		vecResult.insert(vecResult.cend(), vecValues.cbegin(), vecValues.cend());
		vecResult.push_back(m_aEnd);

		return vecResult;
	}

	std::vector<T> GetKeyValueConcatString(const T &aKey, const T &aValue) const
	{
		return {m_aStartWith, aKey, m_aPadding, "\"", aValue, "\"", m_aEnd};
	}
}; // ConcatLine

using ConcatLineStringBase = ConcatLine<const char *>;

struct ConcatLineString : ConcatLineStringBase
{
	using Base = ConcatLineStringBase;

	ConcatLineString(const Base &aInit);

	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, bool bValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, int iValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, float flValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const;
	const char *AppendToBuffer(CBufferString &sMessage, const char *pszKey, std::vector<const char *> vecValues) const;
	const char *AppendBytesToBuffer(CBufferString &sMessage, const char *pszKey, const byte *pData, size_t nLength) const;
	const char *AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint32 uHandle) const;
	const char *AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint64 uHandle) const;
	const char *AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, const void *pHandle) const;
	const char *AppendPointerToBuffer(CBufferString &sMessage, const char *pszKey, const void *pValue) const;
	const char *AppendStringToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const;

	int AppendToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const;
	int AppendStringToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const;
}; // ConcatLineString

#endif // _INCLUDE_METAMOD_SOURCE_CONCAT_HPP_
