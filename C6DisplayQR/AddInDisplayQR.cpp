
#include "stdafx.h"

#if defined( __linux__ ) || defined(__APPLE__)
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#endif

#include <wchar.h>
#include <string>
#include "AddInDisplayQR.h"

#ifdef WIN32
#pragma setlocale("ru-RU" )
#endif

static const wchar_t g_kClassNames[] = L"CAddInNative"; //|OtherClass1|OtherClass2";

static const wchar_t *g_PropNames[] = {
	L"PortName"
};

static const wchar_t *g_PropNamesRu[] = {
	L"ИмяПорта"
};

static const wchar_t *g_MethodNames[] = {
	L"OpenPort",
	L"SendQRCode",
	L"ClearDisplay",
	L"ClosePort"
};

static const wchar_t *g_MethodNamesRu[] = {
	L"ОткрытьПорт",
	L"ОтправитьQRКод",
	L"ОчиститьДисплей",
	L"ЗакрытьПорт"
};

static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;

// При взаимодействии с платформой всегда используется только WCHAR_T.
// Есть простое правило: внутри компоненты строки обрабатываются как wchar_t,
// но как только мы передаем строку в 1С или принимаем ее оттуда, то нужен WCHAR_T
// wchar_t на Linux может быть 4 байта, в Windows 2 байта

// Формирует WCHAR_T из стандартного wchar_t (при отправке строки из компоненты в 1С)
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, size_t len = 0);
// Формирует wchar_t из WCHAR_T (при получении строки из 1С в компоненту)
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
// Длина строки, полученной из 1С
uint32_t getLenShortWcharStr(const WCHAR_T* Source);

//---------------------------------------------------------------------------//
long GetClassObject(const wchar_t* wsName, IComponentBase** pInterface)
{
    if(!*pInterface)
    {
        *pInterface= new CAddInNative();
        return (long)*pInterface;
    }
    return 0;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
   if(!*pIntf)
      return -1;

   delete *pIntf;
   *pIntf = 0;
   return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
    static WCHAR_T* names = NULL;
    if (!names)
        ::convToShortWchar(&names, g_kClassNames);
    return names;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
//CAddInNative
CAddInNative::CAddInNative()
: m_iMemory(NULL),
	m_iConnect(NULL),
	m_pPortName(NULL),
	m_hComPort(INVALID_HANDLE_VALUE)
{
}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
	if (m_hComPort != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hComPort);
	}
	if (m_pPortName)
	{
		delete[] m_pPortName;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection)
{ 
	m_iConnect = static_cast<IAddInDefBase*>(pConnection);
	return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{ 
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}
//---------------------------------------------------------------------------//
void CAddInNative::Done()
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{ 
	const wchar_t *wsExtension = L"ComUtil";
	size_t iActualSize = ::wcslen(wsExtension) + 1;
	WCHAR_T* dest = NULL;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsExtensionName, (unsigned)iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{ 
	return ePropLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{ 
	long plPropNum = -1;
	wchar_t* propName = 0;

	::convFromShortWchar(&propName, wsPropName);
	plPropNum = findName(g_PropNames, propName, ePropLast);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, ePropLast);

	delete[] propName;

	return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{ 
	if (lPropNum >= ePropLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsPropName = NULL;
	size_t iActualSize = 0;

	switch (lPropAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropName, (unsigned)iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::GetPropVal", NOERROR);
	//::MessageBox(GetForegroundWindow(), L"Hello, миръ", L"C6DisplayQR", MB_OK);
	switch (lPropNum)
	{
	case ePropPortName:
		//TV_VT(pvarPropVal) = VTYPE_PWSTR;
		//TV_WSTR(pvarPropVal) = m_pPortName;
		return retValFromWChar(pvarPropVal, m_pPortName);
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant* varPropVal)
{ 
	wchar_t* wsTmp = NULL;

	switch (lPropNum)
	{
	case ePropPortName:
		if (m_pPortName)
		{
			delete[] m_pPortName;
			m_pPortName = NULL;
		}
		if (TV_VT(varPropVal) == VTYPE_PWSTR)
		{
			::convFromShortWchar(&wsTmp, TV_WSTR(varPropVal));
			m_pPortName = wsTmp;
			//delete[] wsTmp;
		}
		break;
	default:
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum)
{ 
	switch (lPropNum)
	{
	case ePropPortName:
		return true;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
	switch (lPropNum)
	{
	case ePropPortName:
		return true;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods()
{
	return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{ 
	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::FindMethod", NOERROR);
	long plMethodNum = -1;
	wchar_t* name = 0;

	::convFromShortWchar(&name, wsMethodName);

	plMethodNum = findName(g_MethodNames, name, eMethLast);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

	delete[] name;

	return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, 
                            const long lMethodAlias)
{ 
	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::GetMethodName", NOERROR);
	if (lMethodNum >= eMethLast)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsMethodName = NULL;
	size_t iActualSize = 0;

	switch (lMethodAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsMethodName, (unsigned)iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	return wsMethodName;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{ 
	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::GetNParams", NOERROR);
	switch (lMethodNum)
	{
	case eMethOpenPort:
	case eMethClosePort:
	case eMethClearDisplay:
		return 0;
	case eMethSendQRCode:
		return 1;
	default:
		return 0;
	}

	return 0;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
                          tVariant *pvarParamDefValue)
{ 
	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::GetParamDefValue", NOERROR);
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;

	switch (lMethodNum)
	{
	case eMethOpenPort:
	case eMethClosePort:
	case eMethSendQRCode:
	case eMethClearDisplay:
		// There are no parameter values by default 
		break;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum)
{
	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::HasRetVal", NOERROR);
	switch (lMethodNum)
	{
	case eMethOpenPort:
	case eMethClosePort:
	case eMethSendQRCode:
	case eMethClearDisplay:
		return true;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum,
                    tVariant* paParams, const long lSizeArray)
{ 
	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::CallAsProc", 0);
	
	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum,
                tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{ 
	bool ret = false;
	wchar_t* wsParam = NULL;

	//addError(ADDIN_E_NONE, L"ComUtil", L"CAddInNative::CallAsFunc", NOERROR);

	switch (lMethodNum)
	{
	case eMethOpenPort:
		ret = OpenComPort();
		TV_VT(pvarRetValue) = VTYPE_BOOL;
		TV_BOOL(pvarRetValue) = ret;
		ret = true;
		break;

	case eMethClosePort:
		ret = CloseComPort();
		TV_VT(pvarRetValue) = VTYPE_BOOL;
		TV_BOOL(pvarRetValue) = ret;
		ret = true;
		break;

	case eMethSendQRCode:
		if (lSizeArray != 1 || !paParams)
		{
			return false;
		}
		else if (TV_VT(paParams) != VTYPE_PWSTR)
		{
			addError(ADDIN_E_VERY_IMPORTANT, L"ComUtil", L"Parameter type mismatch.", -1);
			return false;
		}
		::convFromShortWchar(&wsParam, TV_WSTR(paParams));
		ret = SendQRComPort(wsParam);
		delete[] wsParam;
		TV_VT(pvarRetValue) = VTYPE_BOOL;
		TV_BOOL(pvarRetValue) = ret;
		ret = true;
		break;

	case eMethClearDisplay:
		ret = ClearDisplay();
		TV_VT(pvarRetValue) = VTYPE_BOOL;
		TV_BOOL(pvarRetValue) = ret;
		ret = true;
		break;

	default:
		return false;
	}

	return ret;
}
//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
#if !defined( __linux__ ) && !defined(__APPLE__)
	_wsetlocale(LC_ALL, loc);
#else
	//We convert in char* char_locale
	//also we establish locale
	//setlocale(LC_ALL, char_locale);
	int size = 0;
    char *mbstr = 0;
    wchar_t *tmpLoc = 0;
    convFromShortWchar(&tmpLoc, loc);
    size = wcstombs(0, tmpLoc, 0)+1;
    mbstr = new char[size];

    if (!mbstr)
    {
        delete[] tmpLoc;
        return;
    }

    memset(mbstr, 0, size);
    size = wcstombs(mbstr, tmpLoc, wcslen(tmpLoc));
    setlocale(LC_ALL, mbstr);
    delete[] tmpLoc;
    delete[] mbstr;
#endif
}
/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem)
{
	// Позволяет выделять блоки памяти, которые будет освобождать сама платформа.
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != NULL;
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source,
	const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T *err = 0;
		WCHAR_T *descr = 0;

		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);
		delete[] err;
		delete[] descr;
	}
}
//
bool CAddInNative::retValFromWChar(tVariant* pvarRetVal, const wchar_t* wsSource)
{
	if (m_iMemory && wsSource)
	{
		size_t iActualSize = wcslen(wsSource) + 1;
		WCHAR_T* wsRetVal = NULL;
		if (m_iMemory->AllocMemory(reinterpret_cast<void**>(&wsRetVal), (unsigned)iActualSize * sizeof(WCHAR_T)))
		{
			::convToShortWchar(&wsRetVal, wsSource, iActualSize);
			TV_VT(pvarRetVal) = VTYPE_PWSTR;
			//TV_WSTR(pvarRetVal) = wsRetVal;
			pvarRetVal->pwstrVal = wsRetVal;
			pvarRetVal->wstrLen = getLenShortWcharStr(wsRetVal);

			return true;
		}
	}

	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name,
	const uint32_t size) const
{
	long ret = -1;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
		{
			ret = i;
			break;
		}
	}
	return ret;
}
//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, size_t len)
{
	if (!len)
		len = ::wcslen(Source) + 1;

	// и вот он, пиздец
	// вместо m_iMemory тут православный new (!)
	if (!*Dest)
		*Dest = new WCHAR_T[len];

	WCHAR_T* tmpShort = *Dest;
	wchar_t* tmpWChar = (wchar_t*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(WCHAR_T));
#ifdef __linux__
	size_t succeed = (size_t)-1;
	size_t f = len * sizeof(wchar_t), t = len * sizeof(WCHAR_T);
	const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
	iconv_t cd = iconv_open("UTF-16LE", fromCode);
	if (cd != (iconv_t)-1)
	{
		succeed = iconv(cd, (char**)&tmpWChar, &f, (char**)&tmpShort, &t);
		iconv_close(cd);
		if (succeed != (size_t)-1)
			return (uint32_t)succeed;
	}
#endif //__linux__
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpShort = (WCHAR_T)*tmpWChar;
	}

	return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
	if (!len)
		len = getLenShortWcharStr(Source) + 1;

	if (!*Dest)
		*Dest = new wchar_t[len];

	wchar_t* tmpWChar = *Dest;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(wchar_t));
#ifdef __linux__
	size_t succeed = (size_t)-1;
	const char* fromCode = sizeof(wchar_t) == 2 ? "UTF-16" : "UTF-32";
	size_t f = len * sizeof(WCHAR_T), t = len * sizeof(wchar_t);
	iconv_t cd = iconv_open("UTF-32LE", fromCode);
	if (cd != (iconv_t)-1)
	{
		succeed = iconv(cd, (char**)&tmpShort, &f, (char**)&tmpWChar, &t);
		iconv_close(cd);
		if (succeed != (size_t)-1)
			return (uint32_t)succeed;
	}
#endif //__linux__
	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpWChar = (wchar_t)*tmpShort;
	}

	return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
	uint32_t res = 0;
	WCHAR_T *tmpShort = (WCHAR_T*)Source;

	while (*tmpShort++)
		++res;

	return res;
}
//---------------------------------------------------------------------------//
bool CAddInNative::OpenComPort(void)
{
	wchar_t* wcPortName = m_pPortName;
	size_t len = ::wcslen(wcPortName);
	if (len> 4)
	{
		wcPortName = new wchar_t[len + 5];
		memcpy(reinterpret_cast<void*>(wcPortName), L"\\\\.\\", 4 * sizeof(wchar_t));
		memcpy(reinterpret_cast<void*>(&wcPortName[4]), m_pPortName, (len + 1) * sizeof(wchar_t));
		//addError(ADDIN_E_NONE, L"ComUtil", wcPortName, NOERROR);
	}
	m_hComPort = ::CreateFile(wcPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (wcPortName != m_pPortName)
	{
		delete[] wcPortName;
	}
	if (m_hComPort != INVALID_HANDLE_VALUE)
	{
		DCB dcbSerialParams;
		memset(&dcbSerialParams, 0, sizeof(dcbSerialParams));
		dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
		if (::GetCommState(m_hComPort, &dcbSerialParams))
		{
			// параметры по умолчанию
			dcbSerialParams.BaudRate = CBR_115200;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			if (::SetCommState(m_hComPort, &dcbSerialParams))
			{
				return true;
			}
		}
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::CloseComPort(void)
{
	if (m_hComPort != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_hComPort);
		m_hComPort = INVALID_HANDLE_VALUE;
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SendQRComPort(const wchar_t* wsQRCode)
{
	bool ret = false;
	size_t size = wcslen(wsQRCode);

	if (size > 0)
	{
		static const BYTE pack_beg[] = { 0x02, 0xF2, 0x02 };
		static const BYTE pack_end[] = { 0x02, 0xF2, 0x03 };
		WORD wBELength = ((size >> 8) & 0xFF) | (size & 0xFF) << 8; // LE to BE convert
		size_t uBuffSize = sizeof(pack_beg) + sizeof(wBELength) + size + sizeof(pack_end);
		BYTE* pBuffer = new BYTE[uBuffSize];
		BYTE* pIntoBuff = pBuffer;
		memcpy(pIntoBuff, pack_beg, sizeof(pack_beg));
		memcpy(pIntoBuff += sizeof(pack_beg), &wBELength, sizeof(wBELength));
		//memcpy(pIntoBuff += sizeof(wBELength), wsQRCode, size);
		wcstombs_s(NULL, reinterpret_cast<char*>(pIntoBuff += sizeof(wBELength)), uBuffSize, wsQRCode, size);
		memcpy(pIntoBuff += size, pack_end, sizeof(pack_end));

		DWORD dwNOBWritten = 0;
		if (::WriteFile(m_hComPort, pBuffer, static_cast<DWORD>(uBuffSize), &dwNOBWritten, NULL) == TRUE)
		{
			if (dwNOBWritten == uBuffSize)
			{
				ret = true;
			}
		}
		delete pBuffer;
	}

	return ret;
}
//---------------------------------------------------------------------------//
bool CAddInNative::ClearDisplay(void)
{
	bool ret = false;
	static const BYTE pack_data[] = { 0x02, 0xF0, 0x03, 0x43, 0x4C, 0x53, 0x03 };
	size_t uDataSize = sizeof(pack_data);
	DWORD dwNOBWritten = 0;
	if (::WriteFile(m_hComPort, pack_data, static_cast<DWORD>(uDataSize), &dwNOBWritten, NULL) == TRUE)
	{
		if (dwNOBWritten == uDataSize)
		{
			ret = true;
		}
	}

	return ret;
}
//---------------------------------------------------------------------------//
