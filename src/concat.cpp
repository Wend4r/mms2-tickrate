
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

#include <concat.hpp>

ConcatLineString::ConcatLineString(const Base &aInit)
 :  Base(aInit)
{
}

const char *ConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey) const
{
	const auto vecConcat = Base::GetKeyValueConcat(pszKey);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *ConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, bool bValue) const
{
	return AppendToBuffer(sMessage, pszKey, bValue ? "true" : "false");
}

const char *ConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, int iValue) const
{
	char sValue[12];

	V_snprintf(sValue, sizeof(sValue), "%i", iValue);

	return AppendToBuffer(sMessage, pszKey, sValue);
}

const char *ConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, float flValue) const
{
	char sValue[21];

	V_snprintf(sValue, sizeof(sValue), "%f", flValue);

	return AppendToBuffer(sMessage, pszKey, sValue);
}

const char *ConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyValueConcat(pszKey, pszValue);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *ConcatLineString::AppendToBuffer(CBufferString &sMessage, const char *pszKey, std::vector<const char *> vecValues) const
{
	const auto vecConcat = Base::GetKeyValueConcat(pszKey, vecValues);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

const char *ConcatLineString::AppendBytesToBuffer(CBufferString &sMessage, const char *pszKey, const byte *pData, size_t nLength) const
{
	std::vector<const char *> vecValues;

	vecValues.reserve(nLength);

	const size_t nDataSetDepthSize = 4;

	CBufferStringGrowable<4> *psDataSet = new CBufferStringGrowable<4>[nLength];

	for(int n = 0; n < nLength; n++)
	{
		auto &sByte = psDataSet[n];

		sByte.Format("%02X ", pData[n]);
		vecValues.push_back(sByte.Get());
	}

	auto *pResult = AppendToBuffer(sMessage, pszKey, vecValues);

	delete[] psDataSet;

	return pResult;
}

const char *ConcatLineString::AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint32 uHandle) const
{
	char sHandle[21];

	V_snprintf(sHandle, sizeof(sHandle), "%u", uHandle);

	return AppendToBuffer(sMessage, pszKey, sHandle);
}

const char *ConcatLineString::AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, uint64 uHandle) const
{
	char sHandle[21];

	V_snprintf(sHandle, sizeof(sHandle), "%llu", uHandle);

	return AppendToBuffer(sMessage, pszKey, sHandle);
}

const char *ConcatLineString::AppendHandleToBuffer(CBufferString &sMessage, const char *pszKey, const void *pHandle) const
{
	return AppendHandleToBuffer(sMessage, pszKey, (uint64)pHandle);
}

const char *ConcatLineString::AppendPointerToBuffer(CBufferString &sMessage, const char *pszKey, const void *pValue) const
{
	char sPointer[22];

	V_snprintf(sPointer, sizeof(sPointer), "%p", pValue);

	return AppendToBuffer(sMessage, pszKey, sPointer);
}

const char *ConcatLineString::AppendStringToBuffer(CBufferString &sMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyValueConcatString(pszKey, pszValue);

	return sMessage.AppendConcat(vecConcat.size(), vecConcat.data(), NULL);
}

int ConcatLineString::AppendToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyValueConcat(pszKey, pszValue);

	return vecMessage.AddMultipleToTail(vecConcat.size(), vecConcat.data());
}

int ConcatLineString::AppendStringToVector(CUtlVector<const char *> vecMessage, const char *pszKey, const char *pszValue) const
{
	const auto vecConcat = Base::GetKeyValueConcatString(pszKey, pszValue);

	return vecMessage.AddMultipleToTail(vecConcat.size(), vecConcat.data());
}
