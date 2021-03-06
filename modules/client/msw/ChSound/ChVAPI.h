/*----------------------------------------------------------------------------
                        _                              _ _       
        /\             | |                            | (_)      
       /  \   _ __   __| |_ __ ___  _ __ ___   ___  __| |_  __ _ 
      / /\ \ | '_ \ / _` | '__/ _ \| '_ ` _ \ / _ \/ _` | |/ _` |
     / ____ \| | | | (_| | | | (_) | | | | | |  __/ (_| | | (_| |
    /_/    \_\_| |_|\__,_|_|  \___/|_| |_| |_|\___|\__,_|_|\__,_|

    The contents of this file are subject to the Andromedia Public
	License Version 1.0 (the "License"); you may not use this file
	except in compliance with the License. You may obtain a copy of
	the License at http://www.andromedia.com/APL/

    Software distributed under the License is distributed on an
	"AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
	implied. See the License for the specific language governing
	rights and limitations under the License.

    The Original Code is Pueblo client code, released November 4, 1998.

    The Initial Developer of the Original Code is Andromedia Incorporated.
	Portions created by Andromedia are Copyright (C) 1998 Andromedia
	Incorporated.  All Rights Reserved.

	Andromedia Incorporated                         415.365.6700
	818 Mission Street - 2nd Floor                  415.365.6701 fax
	San Francisco, CA 94103

    Contributor(s):
	--------------------------------------------------------------------------
	   Chaco team:  Dan Greening, Glenn Crocker, Jim Doubek,
	                Coyote Lussier, Pritham Shetty.

					Wrote and designed original codebase.

------------------------------------------------------------------------------

	Header file for the ChVAPI object, which manages the interface between
	VAPI and the Sound Module.

----------------------------------------------------------------------------*/


#if !defined( _CHVAPI_H )
#define _CHVAPI_H

#if defined( CH_USE_VAPI )

#include <vapi.h>


/*----------------------------------------------------------------------------
	Constants
----------------------------------------------------------------------------*/

#define SESSION_LIMIT		5


/*----------------------------------------------------------------------------
	ChVAPISession class
----------------------------------------------------------------------------*/

class ChVAPISession
{
	public:
		ChVAPISession();
		~ChVAPISession();

	public:
		inline const string& GetId() { return m_strCallId; }
		inline VAPI_SESSION_HANDLE GetSessionHdl() { return m_hSession; }
		inline chflag32 GetOptions() { return m_flOptions; }

		inline void Set( VAPI_SESSION_HANDLE hSession,
							const string& strCallId,
							chflag32 flOptions )
					{
						m_strCallId = strCallId;
						m_hSession = hSession;
						m_flOptions = flOptions;
					}
		inline void SetSessionHdl( VAPI_SESSION_HANDLE hSession )
					{
						m_hSession = hSession;
					}

	protected:
		string						m_strCallId;
		VAPI_SESSION_HANDLE			m_hSession;
		chflag32					m_flOptions;
};


/*----------------------------------------------------------------------------
	ChVAPISessionMgr class
----------------------------------------------------------------------------*/

class ChVAPISessionMgr
{
	public:
		ChVAPISessionMgr();
		~ChVAPISessionMgr();

	public:
		inline bool AllSessionsInUse()
						{
							return (SESSION_LIMIT <= GetSessionCount());
						}

		bool Set( const string& strCallId, VAPI_SESSION_HANDLE hSession,
					chflag32 flOptions );

		VAPI_SESSION_HANDLE GetSessionHandle( const string& strCallId,
												chflag32* pflOptions = 0 );
		bool Hangup( VAPI_HANDLE hVAPI, const string& strCallId );
		bool ClearSession( const string& strCallId );
		bool ClearSessionHdl( VAPI_SESSION_HANDLE hSession );

		ChVAPISession* Find( const string& strCallId )
						{
							return Find( strCallId, 0 );
						}
		ChVAPISession* Find( VAPI_SESSION_HANDLE hSession )
						{
							return Find( hSession, 0 );
						}

	protected:
		inline bool IsInUse( int iSession )
						{
							return m_boolSessionInUse[iSession];
						}
		inline void SetInUse( int iSession, bool boolInUse = true )
						{
							if (IsInUse( iSession ) && !boolInUse)
							{
								++m_iSessions;
							}
							else if (!IsInUse( iSession ) && boolInUse)
							{
								--m_iSessions;
							}

							m_boolSessionInUse[iSession] = boolInUse;
						}
		inline ChVAPISession* GetSession( int iSession )
						{
							return &m_sessionList[iSession];
						}

		ChVAPISession* Find( const string& strCallId, int* piLoc );
		ChVAPISession* Find( VAPI_SESSION_HANDLE hSession, int* piLoc );

		inline int GetSessionCount() { return m_iSessions; }

		bool Hangup( VAPI_HANDLE hVAPI, VAPI_SESSION_HANDLE hSession );

	protected:
		int				m_iSessions;

		ChVAPISession	m_sessionList[SESSION_LIMIT];
		bool			m_boolSessionInUse[SESSION_LIMIT];
};


/*----------------------------------------------------------------------------
	ChVAPI class
----------------------------------------------------------------------------*/

class ChVAPI
{
	public:
		ChVAPI();
		~ChVAPI();

		static chuint16 GetVAPIPort() { return m_suVAPIPort; }

		static ChVAPI* GetVAPI() { return m_this; }

		static bool MakeCall( const string& strCallId, const string& strHost,
								chuint16 suPort, chflag32 flOptions,
								chflag32 flRemoteOptions );
		static bool Hangup( const string& strCallId );

		static void SetMikeSensitivity( chuint32 luValue );
		static void SetMikeVolume( chuint32 luValue );

		static void UpdatePrefs();

	protected:
		static VAPI_HANDLE GetVAPIHandle() { return m_hVAPI; }
		static const string& GetVAPILocalHost() { return m_strVAPILocalHost; }
		static chuint32 GetMikeVolume() { return m_luMikeVolume; }
		static chuint32 GetMikeSensitivity() { return m_luMikeSensitivity; }
		static bool IsRejectingCalls() { return m_boolRejectCalls; }

		static void SetVAPIPort( chuint16 suPort )
						{
							m_suVAPIPort = suPort;
						}
		static void SetVAPILocalHost( const char* pstrLocalHost )
						{
							m_strVAPILocalHost = pstrLocalHost;
						}

		static VAPI_RETCODE NotifyProc( unsigned short wMessage,
										unsigned long hTransaction,
					    				unsigned long dwVapiParamOne,
					    				unsigned long dwVapiParamTwo,
					    				unsigned long dwUserParam );

	protected:
		static ChVAPI*			m_this;
		static int				m_iUsage;

		static VAPI_HANDLE		m_hVAPI;
		static chuint16			m_suVAPIPort;
		static string			m_strVAPILocalHost;

		static ChVAPISessionMgr	m_vapiSessions;

		static VAPI_USER_INFO	m_userInfo;

											// User preferences...

		static chuint32			m_luMikeVolume;
		static chuint32			m_luMikeSensitivity;
		static bool				m_boolRejectCalls;
};


#endif	// defined( CH_USE_VAPI )
#endif	// !defined( _CHVAPI_H )
