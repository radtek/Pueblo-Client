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

	Defines the ChWorld module for the Pueblo system.  This module is
	used to connect to external virtual worlds.

----------------------------------------------------------------------------*/

// $Header: /home/cvs/chaco/modules/client/msw/ChWorld/ChWConn.cpp,v 2.69 1996/09/12 19:10:09 pritham Exp $

#include "headers.h"

#include <fstream.h>
#include <ctype.h>

#include <ChCore.h>
#include <ChReg.h>
#include <ChHtmWnd.h>

#include <ChSound.h>

#include "World.h"
#include "ChWConn.h"
#if !defined(CH_PUEBLO_PLUGIN)
#include "resource.h"
#else
#include "vwrres.h"
#endif


/*----------------------------------------------------------------------------
	Constants
----------------------------------------------------------------------------*/

#define MAX_LINE_LEN		4096			/* If a line is longer than 4k,
												then give up looking for a
												line ending */
#define ESCAPE				0x1b
#define ANSI_START_1		ESCAPE
#define ANSI_START_2		'['
#define ANSI_SEPARATOR		';'
#define ANSI_END			"m\n\r\0x1b"	// One of these ends sequence
#define ANSI_MODE			'm'
#define ANSI_ERASE			'j'
											/* Telnet commands:
												(from RFC 854:  Telnet protocol
																spec.) */
#define TN_EOR				239				// End-Of-Record
#define TN_SE				240				// End of subnegotiation params
#define TN_NOP				241				// not used
#define TN_DATA_MARK		242				// not used
#define TN_BRK				243				// not used
#define TN_IP				244				// not used
#define TN_AO				245				// not used
#define TN_AYT				246				// not used
#define TN_EC				247				// not used
#define TN_EL				248				// not used
#define TN_GA				249				// Go Ahead
#define TN_SB				250				/* Indicates that what follows is
												subnegotiation of the indicated
                                 				option */
#define TN_WILL				251				// I offer to ~, or ack for DO
#define TN_WONT				252				// I will stop ~ing, or nack for DO
#define TN_DO				253				// Please do ~?, or ack for WILL
#define TN_DONT				254				// Stop ~ing!, or nack for WILL
#define TN_IAC				255				// Is A Command character

											// Telnet options:

#define TN_ECHO				1				// echo option
#define TN_SGA				3				// suppress GOAHEAD option
#define TN_STATUS			5				// not used
#define TN_TIMING_MARK		6				// not used
#define TN_TERMINAL_TYPE	24				// terminal type option
#define TN_EOR_OPT			25				// End Of Record option
#define TN_NAWS				31				// not used
#define TN_TSPEED			32				// not used
#define TN_LINEMODE			34				// not used

											// Other Telnet codes:
#define TN_IS				0
#define TN_SEND				1


/*----------------------------------------------------------------------------
	Types
----------------------------------------------------------------------------*/

typedef enum { betweenSequences, foundIAC, foundModifier } TelnetState;


/*----------------------------------------------------------------------------
	Static arrays
----------------------------------------------------------------------------*/

#if defined( DUMP_TELNET_CODES )

static const char*	pstrTelnetLabel[256];

#endif	// defined( CH_DEBUG )


/*----------------------------------------------------------------------------
	ChBufferString class
----------------------------------------------------------------------------*/

ChBufferString::ChBufferString() :
					m_iBufferSize( initBufferStringSize ),
					m_iActualSize( 0 )
{
	m_pstrBuffer = m_string.GetBuffer( m_iBufferSize );
	m_pstrBufferEnd = m_pstrBuffer;
}


ChBufferString::ChBufferString( const string& strData )
{
	m_iActualSize = strData.GetLength();
	m_iBufferSize = m_iActualSize + growBufferStringSize;
	m_pstrBuffer = m_string.GetBuffer( m_iBufferSize );
	m_pstrBufferEnd = m_pstrBuffer + m_iActualSize;
}


ChBufferString::~ChBufferString()
{
}


ChBufferString& ChBufferString::operator +=( char cChar )
{
	if (m_iActualSize >= m_iBufferSize)
	{
		m_string.ReleaseBuffer( m_iActualSize );

											/* We need to reallocate the
												string to make room for the
												new data */

		m_iBufferSize = m_iActualSize + growBufferStringSize;

											// Get new pointers

		m_pstrBuffer = m_string.GetBuffer( m_iBufferSize );
		m_pstrBufferEnd = m_pstrBuffer + m_iActualSize;
	}

	*m_pstrBufferEnd = cChar;
	m_pstrBufferEnd++;
	*m_pstrBufferEnd = 0;
	m_iActualSize++;

	return *this;
}


ChBufferString& ChBufferString::operator +=( const char* pstrText )
{
	int		iLen = strlen( pstrText );

	if (m_iActualSize + iLen >= m_iBufferSize)
	{										/* We need to reallocate the
												string to make room for the
												new data */
		m_string.ReleaseBuffer( m_iActualSize );

											/* We need to reallocate the
												string to make room for the
												new data */

		m_iBufferSize = m_iActualSize + growBufferStringSize;

											// Get new pointers

		m_pstrBuffer = m_string.GetBuffer( m_iBufferSize );
		m_pstrBufferEnd = m_pstrBuffer + m_iActualSize;
	}

	strncpy( m_pstrBufferEnd, pstrText, iLen );
	m_pstrBufferEnd += iLen;
	m_iActualSize += iLen;
	*m_pstrBufferEnd = 0;

	return *this;
}


/*----------------------------------------------------------------------------
	ChAnsiState class
----------------------------------------------------------------------------*/

ChAnsiState::ChAnsiState() :
				m_boolAnsiBold( false ),
				m_boolAnsiUnderline( false ),
				m_boolAnsiItalic( false ),
				m_boolAnsiColor( false ),
				m_luForeColor( constDefColor ),
				m_luBackColor( constDefColor )
{
}


void ChAnsiState::Reset( string& strHTML )
{
	if (m_boolAnsiBold)
	{
		strHTML += "</b>";
		m_boolAnsiBold = false;
	}

	if (m_boolAnsiUnderline)
	{
		strHTML += "</u>";
		m_boolAnsiUnderline = false;
	}

	if (m_boolAnsiItalic)
	{
		strHTML += "</i>";
		m_boolAnsiItalic = false;
	}

	if (m_boolAnsiColor)
	{
		strHTML += "</font>";
		m_boolAnsiColor = false;
	}
											/* ANSI reset sets the terminal to
												white-on-black */
	strHTML += "<body bgcolor=#000000>\n";
	SetBackColor( COLOR_BLACK, strHTML );
	SetForeColor( COLOR_WHITE, strHTML );
}


void ChAnsiState::SetBold( bool boolBold, string& strHTML )
{
	if (boolBold && !m_boolAnsiBold)
	{
		strHTML += "<b>";
		m_boolAnsiBold = true;
	}
	else if (!boolBold && m_boolAnsiBold)
	{
		strHTML += "</b>";
		m_boolAnsiBold = false;
	}
}


void ChAnsiState::SetUnderline( bool boolUnderline, string& strHTML )
{
	if (boolUnderline && !m_boolAnsiUnderline)
	{
		strHTML += "<u>";
		m_boolAnsiUnderline = true;
	}
	else if (!boolUnderline && m_boolAnsiUnderline)
	{
		strHTML += "</u>";
		m_boolAnsiUnderline = false;
	}
}


void ChAnsiState::SetItalic( bool boolItalic, string& strHTML )
{
	if (boolItalic && !m_boolAnsiItalic)
	{
		strHTML += "<i>";
		m_boolAnsiItalic = true;
	}
	else if (!boolItalic && m_boolAnsiItalic)
	{
		strHTML += "</i>";
		m_boolAnsiItalic = false;
	}
}


void ChAnsiState::SetForeColor( chuint32 luColor, string& strHTML )
{
	m_luForeColor = luColor;
	SetColor( m_luForeColor, m_luBackColor, strHTML );
}


void ChAnsiState::SetBackColor( chuint32 luColor, string& strHTML )
{
	m_luBackColor = luColor;
	SetColor( m_luForeColor, m_luBackColor, strHTML );
}


void ChAnsiState::SetColor( chuint32 luForeColor, chuint32 luBackColor,
							string& strHTML )
{
	string		strTemp;

	if (m_boolAnsiColor)
	{
		strHTML += "</font>";
		m_boolAnsiColor = false;
	}

	if ((luForeColor != constDefColor) || (luBackColor != constDefColor))
	{
		string		strForeHTML;
		string		strBackHTML;

		if (luForeColor != constDefColor)
		{
			strForeHTML.Format( " fgcolor=\"#%6.6lX\"", luForeColor );
		}

		if (luBackColor != constDefColor)
		{
			strBackHTML.Format( " bgcolor=\"#%6.6lX\"", luBackColor );
		}

		strHTML += "<font" + strForeHTML + strBackHTML + ">";
		m_boolAnsiColor = true;
	}
}


/*----------------------------------------------------------------------------
	ChWorldConn class
----------------------------------------------------------------------------*/

ChWorldConn::ChWorldConn( const ChModuleID& idModule,
							ChSocketHandler pHandler,
							ChWorldMainInfo* pMainInfo, chparam userData ) :
				ChConn( pHandler, userData ),
				m_idModule( idModule ),
				m_pMainInfo( pMainInfo ),
				m_state( stateNotConnected ),
				m_textMode( modeText ),
				m_boolRawMode( false ),
				m_boolPuebloEnhanced( false ),
				m_boolTextReceived( false ),
				m_boolAnsiResetReceived( false ),
				m_flTelnetState( telnetStateEcho ),
				m_parseState( parseStateScanning )
{
	UpdatePreferences();
											// Cache text out object

	m_pTextOutput = GetMainInfo()->GetTextOutput();

	#if defined( DUMP_TELNET_CODES )
	{
		int		iLoop;

	    for (iLoop = 0; iLoop < 256; iLoop++)
	    {
	    	pstrTelnetLabel[iLoop] = 0;
		}

		pstrTelnetLabel[TN_ECHO]			= "ECHO";
		pstrTelnetLabel[TN_SGA]				= "SGA";
		pstrTelnetLabel[TN_STATUS]			= "STATUS";
		pstrTelnetLabel[TN_TIMING_MARK]		= "TIMING_MARK";
		pstrTelnetLabel[TN_TERMINAL_TYPE]	= "TERMINAL_TYPE";
		pstrTelnetLabel[TN_EOR_OPT]			= "EOR_OPT";
		pstrTelnetLabel[TN_NAWS]			= "NAWS";
		pstrTelnetLabel[TN_TSPEED]			= "TSPEED";
		pstrTelnetLabel[TN_LINEMODE]		= "LINEMODE";
		pstrTelnetLabel[TN_EOR]				= "EOR";
		pstrTelnetLabel[TN_SE]				= "SE";
		pstrTelnetLabel[TN_NOP]				= "NOP";
		pstrTelnetLabel[TN_DATA_MARK]		= "DATA_MARK";
		pstrTelnetLabel[TN_BRK]				= "BRK";
		pstrTelnetLabel[TN_IP]				= "IP";
		pstrTelnetLabel[TN_AO]				= "AO";
		pstrTelnetLabel[TN_AYT]				= "AYT";
		pstrTelnetLabel[TN_EC]				= "EC";
		pstrTelnetLabel[TN_EL]				= "EL";
		pstrTelnetLabel[TN_GA]				= "GA";
		pstrTelnetLabel[TN_SB]				= "SB";
		pstrTelnetLabel[TN_WILL]			= "WILL";
		pstrTelnetLabel[TN_WONT]			= "WONT";
		pstrTelnetLabel[TN_DO]				= "DO";
		pstrTelnetLabel[TN_DONT]			= "DONT";
		pstrTelnetLabel[TN_IAC]				= "IAC";
	}
	#endif	// defined( CH_DEBUG )
}


ChWorldConn::~ChWorldConn()
{
	if (IsConnected())
	{
		TermConnection();
	}
}


void ChWorldConn::AsyncGetHostByName( const string& strHost,
										ChSocketAsyncHandler pprocHandler,
										chparam userData )
{
	GetChacoSocket()->AsyncGetHostByName( strHost, pprocHandler,
											userData );
}


void ChWorldConn::Connect( const string& strHost, chint16 sPort )
{
	ASSERT( this );
											/* Set flag to indicate that we've
												not yet received any data */
	SetMode( modeText );
											// Start async connect
	SOCKSconnect( strHost, sPort );
}


void ChWorldConn::Connect( chuint32 luAddress, chint16 sPort )
{
										// luAddress is in host order
	ASSERT( this );
										/* Set flag to indicate that we've
											not yet received any data */
	SetMode( modeText );
										// Start async connect

	SOCKSconnect( luAddress, sPort );
}


void ChWorldConn::ProcessOutput()
{											/* Send incoming text to the text
												output module */
	chuint32	luLen;

	ASSERT( this );

	if ((luLen = GetBytesAvailable()) > 0)
	{
		char*	pstrBuffer;
		int		iLen;
		string	strOut;

		if (luLen > INT_MAX)
		{
			iLen = INT_MAX;
		}
		else
		{
			iLen = (int)luLen;
		}
											/* Read the contents of the socket
												to the end of 'inputbuf' */

		pstrBuffer = strOut.GetBufferSetLength( iLen + 1 );
		read( pstrBuffer, iLen );
		strOut.ReleaseBuffer( iLen );

		ASSERT( strOut.GetLength() == iLen );

		ParseText( strOut, (GetMode() == modeHtml) );

											// Display the text
		OutputBuffer( strOut );

		if (!IsAnyTextReceived())
		{
			SetAnyTextReceived();
			GetMainInfo()->DoAutoLogin();
		}
	}
}


void ChWorldConn::InitConnection()
{
	string		strOut;

	m_state = stateConnected;
											/* Make the default text output
												mode HTML, rather than <pre> */
	strOut = "<html>";

											/* Indicate that we will be using
												preformatted text */
	TurnHtmlOff( strOut );
	OutputBuffer( strOut, false );
}


void ChWorldConn::TermConnection()
{
	if (IsConnected())
	{
		string		strOut;

		m_state = stateNotConnected;
		TurnHtmlOn( strOut );
											/* Indicating that we are finished
												using preformatted text */
		OutputBuffer( strOut );
	}
}


void ChWorldConn::SendWorldCommand( const string& strCommand,
									bool boolUserCommand )
{
	SendLine( strCommand );

	if (boolUserCommand)
	{
		EchoState	echo = GetMainInfo()->GetEchoState();

		if (m_boolEcho && (echoOn == echo))
		{
			string		strCommandOut( strCommand );
			string		strOut;

			TurnHtmlOn( strOut );

			if (m_boolBold)
			{
				strOut += "<b>";
			}

			if (m_boolItalic)
			{
				strOut += "<i>";
			}
											/* Append the users' text to the
												output buffer */

			Display( strOut, strCommandOut );

			if (m_boolItalic)
			{
				strOut += "</i>";
			}

			if (m_boolBold)
			{
				strOut += "</b>";
			}

			strOut += "<br>";

			TurnHtmlOff( strOut );
			OutputBuffer( strOut, false );
		}
		else if (m_boolEcho && (echoOn != echo))
		{									// Just send a line break
			OutputBuffer( "\n" );
		}
	}
}


void ChWorldConn::Display( const string& strText )
{
	string		strOut;

	TurnHtmlOn( strOut );
	strOut += "<b>";

	Display( strOut, strText );
	
	strOut += "</b><br>";
	TurnHtmlOff( strOut );

	OutputBuffer( strOut, false );
}


void ChWorldConn::Display( string& strOut, const string& strText )
{
	string		strTextOut( strText );
	chuint32	luBackColor = m_ansiState.GetBackColor();
	chuint32	luForeColor = GetTextOutput()->GetDefForeColor();
	string		strColor;

	if (luForeColor == luBackColor)
	{
		luForeColor = (~luForeColor) & 0xffffff;
	}
	strColor.Format( "#%06lx", luForeColor );

	strOut += "<font text=\"" + strColor + "\">";

								/* Strip out HTML from the users'
									text and append it to the
									output buffer */

	ChHtmlWnd::EscapeForHTML( strTextOut );
	strOut += strTextOut;

	strOut += "</font>";
}


void ChWorldConn::UpdatePreferences()
{
	ChRegistry		worldPrefsReg( WORLD_PREFS_GROUP );

	worldPrefsReg.ReadBool( WORLD_PREFS_ECHO, m_boolEcho, true );
	worldPrefsReg.ReadBool( WORLD_PREFS_ECHO_BOLD, m_boolBold, true );
	worldPrefsReg.ReadBool( WORLD_PREFS_ECHO_ITALIC, m_boolItalic, false );
}


/*----------------------------------------------------------------------------
	ChWorldConn protected methods
----------------------------------------------------------------------------*/

void ChWorldConn::ParseText( string& strText, bool boolNewlinesToBreaks )
{
	string			strWorking;
	const char*		pstrOriginText;
	int				iStartOfLine = 0;
	ChBufferString	strParsedBuffer;

	pstrOriginText = strText;

	while (*pstrOriginText)
	{
		bool	boolMoveToNextChar = true;

		switch (m_parseState)
		{
			case parseStateScanning:
			{
				switch( (unsigned char)*pstrOriginText )
				{
					case ESCAPE:
					{
						m_parseState = parseStateEscape;
						break;
					}

					case TN_IAC:			// 0xff
					{
						m_parseState = parseTelnetIAC;
						break;
					}

					case '\n':
					{
						if (*(pstrOriginText + 1) == '\r')
						{
							pstrOriginText++;
						}

						ParseEndOfLine( strParsedBuffer, iStartOfLine, true );

											// Append a newline or <br>

						if (boolNewlinesToBreaks)
						{
							strParsedBuffer += "<br>";
						}
						else
						{
							strParsedBuffer += '\n';
						}

						iStartOfLine = strParsedBuffer.GetLength();
						break;
					}

					case '\r':
					{
						if (*(pstrOriginText + 1) == '\n')
						{
							pstrOriginText++;
						}

						ParseEndOfLine( strParsedBuffer, iStartOfLine, true );

											// Append a newline or <br>

						if (boolNewlinesToBreaks)
						{
							strParsedBuffer += "<br>";
						}
						else
						{
							strParsedBuffer += '\n';
						}

						iStartOfLine = strParsedBuffer.GetLength();
						break;
					}

					default:
					{
						if (isprint( *pstrOriginText ) || ('\t' == *pstrOriginText))
						{
							strParsedBuffer += *pstrOriginText;
						}
						break;
					}
				}
				break;
			}

			case parseStateEscape:
			{
				switch( *pstrOriginText )
				{
					case '[':
					{
						m_parseState = parseStateAnsi;
						break;
					}

					default:
					{
						strParsedBuffer += ESCAPE;
						strParsedBuffer += *pstrOriginText;

											/* Set the state back to being
												between sequences */

						m_parseState = parseStateScanning;
						break;
					}
				}
				break;
			}

			case parseStateAnsi:
			{
				bool		boolEndOfPacket;

				ASSERT( *pstrOriginText );
											/* Search for the end of the ANSI
												sequence (which may contain
												digits or semi-colons) */

				while ((ANSI_SEPARATOR == *pstrOriginText) ||
						isdigit( *pstrOriginText ))
				{
					m_strAnsiSequence += *pstrOriginText;
					pstrOriginText++;
				}
											/* Figure out if we've hit the end
												of the packet before
												encountering the end of the
												ANSI sequence */

				boolEndOfPacket = (0 == *pstrOriginText);
				if (!boolEndOfPacket)
				{							/* Move to processing the ANSI
												terminating character */

					m_parseState = parseStateAnsiTerm;
				}

				boolMoveToNextChar = false;
				break;
			}

			case parseStateAnsiTerm:
			{
				char	cTerm = *pstrOriginText;
				bool	boolValidSequence = true;

				if (cTerm == '\n' || cTerm == '\r' || cTerm == ESCAPE)
				{
											/* Don't strip off newline or
												ESCAPE terms */
					boolMoveToNextChar = false;
					boolValidSequence = false;
				}
				else if (m_strAnsiSequence.IsEmpty())
				{
					#if defined( DUMP_ANSI_CODES )
					{
						string		strSequence;

						strSequence.Format( "ANSI(unknown: [%c)",
											(char)*pstrOriginText );
						AnsiDisplay( strSequence );
					}
					#endif	// defined( DUMP_ANSI_CODES )

					boolValidSequence = false;
				}

				if (boolValidSequence)
				{
					ProcessAnsiCodes( cTerm, m_strAnsiSequence,
										strParsedBuffer );
				}
											// State goes back to 'scanning'
				m_strAnsiSequence = "";
				m_parseState = parseStateScanning;
				break;
			}

			case parseTelnetIAC:
			{
				m_iTelnetCmd = (unsigned char)*pstrOriginText;

				if ((TN_WILL == m_iTelnetCmd) || (TN_WONT == m_iTelnetCmd) ||
					(TN_DO == m_iTelnetCmd) || (TN_DONT == m_iTelnetCmd))
				{
					m_parseState = parseTelnetCmd;
				}
				else if ((TN_GA == m_iTelnetCmd) || (TN_EOR == m_iTelnetCmd))
				{
											// This is definitely a prompt

					#if defined( DUMP_TELNET_CODES )
					{
						string		strSequence;
						string		strOut;

						strSequence.Format( "rcvd IAC %s\n",
											pstrTelnetLabel[m_iTelnetCmd] );
						TelnetDisplay( strSequence );
					}
					#endif	// defined( DUMP_TELNET_CODES )

											// State goes back to 'scanning'

					m_parseState = parseStateScanning;
				}
				else
				{
					strParsedBuffer += (char)TN_IAC;
					strParsedBuffer += *pstrOriginText;

					m_parseState = parseStateScanning;
				}
				break;
			}

			case parseTelnetCmd:
			{
				DoTelnetCmd( m_iTelnetCmd, (unsigned char)*pstrOriginText );
				m_parseState = parseStateScanning;
				break;
			}

			default:
			{
				TRACE( "Error in parse state!\n" );
				ASSERT( false );
				break;
			}
		}

		if (boolMoveToNextChar && *pstrOriginText)
		{
			pstrOriginText++;
		}
	}

	ParseEndOfLine( strParsedBuffer, iStartOfLine, false );

											// Return the parsed buffer
	strText = strParsedBuffer;
}


void ChWorldConn::ParseEndOfLine( ChBufferString& strBuffer, int iStartOfLine,
									bool boolEndOfLine )
{
	register int		iBufferLen = strBuffer.GetLength();

	ASSERT( iBufferLen >= iStartOfLine );

	if (iStartOfLine != iBufferLen)
	{
		const char*		pstrBuffer = strBuffer;
		int				iLen = iBufferLen - iStartOfLine;
		
		pstrBuffer += iStartOfLine;
		AddToTextBlock( pstrBuffer, iLen, boolEndOfLine );
	}
}


void ChWorldConn::ProcessAnsiCodes( char cCodeType, const char* pstrCodes,
									ChBufferString& strBuffer )
{
	string		strAnsiHTML;

	cCodeType = (char)tolower( cCodeType );
	if ((cCodeType != ANSI_MODE) && (cCodeType != ANSI_ERASE))
	{
											/* Right now we only support two
												types of ANSI sequence */
		return;
	}

	while (*pstrCodes != 0)
	{
		while (*pstrCodes == ANSI_SEPARATOR)
		{									// Strip out separators
			pstrCodes++;
		}

		if (isdigit( *pstrCodes ))
		{									// There's a code here
			int		iCode = 0;
			string	strAnsiOutput;

			while (isdigit( *pstrCodes ))
			{
				iCode = (iCode * 10) + (int)(*pstrCodes - '0');
				pstrCodes++;
			}

			ProcessAnsiCode( cCodeType, iCode, strAnsiHTML );
		}
	}

	if (strAnsiHTML.GetLength() > 0)
	{
		string		strTemp;
											// Add HTML
		TurnHtmlOn( strTemp );
		strTemp += strAnsiHTML;
		TurnHtmlOff( strTemp );

		strBuffer += strTemp;
	}
}


void ChWorldConn::ProcessAnsiCode( char cCodeType, int iCode,
									string& strOutput )
{
	string		strCodeHTML;

	switch( cCodeType )
	{
		case ANSI_MODE:
		{
			switch( iCode )
			{
				case 0:						// Reset all
				{
					AnsiDisplay( "ANSI(reset)" );

					if (!m_boolAnsiResetReceived)
					{
											/* The first time we get an ANSI
												reset, switch all black objects
												in the file to white */

						GetTextOutput()->RemapColor( COLOR_BLACK,
														COLOR_WHITE );
						m_boolAnsiResetReceived = true;
					}
					m_ansiState.Reset( strCodeHTML );
					break;
				}

				case 1:						// High intensity
				{
					AnsiDisplay( "ANSI(bold)" );
					m_ansiState.SetBold( true, strCodeHTML );
					break;
				}

				case 2:						// Low intensity
				{
					AnsiDisplay( "ANSI(nonbold)" );
					m_ansiState.SetBold( false, strCodeHTML );
					break;
				}

				case 3:						// Italic
				{
					AnsiDisplay( "ANSI(italic)" );
					m_ansiState.SetItalic( true, strCodeHTML );
					break;
				}

				case 4:						// Underline
				{
					AnsiDisplay( "ANSI(underline)" );
					m_ansiState.SetUnderline( true, strCodeHTML );
					break;
				}

				case 30:					// Black foreground
				{
					AnsiDisplay( "ANSI(fg-black)" );
					m_ansiState.SetForeColor( COLOR_BLACK, strCodeHTML );
					break;
				}

				case 31:					// Red foreground
				{
					AnsiDisplay( "ANSI(fg-red)" );
					m_ansiState.SetForeColor( 0xff0000, strCodeHTML );
					break;
				}

				case 32:					// Green foreground
				{
					AnsiDisplay( "ANSI(fg-green)" );
					m_ansiState.SetForeColor( 0x00ff00, strCodeHTML );
					break;
				}

				case 33:					// Yellow foreground
				{
					AnsiDisplay( "ANSI(fg-yellow)" );
					m_ansiState.SetForeColor( 0xffff00, strCodeHTML );
					break;
				}

				case 34:					// Blue foreground
				{
					AnsiDisplay( "ANSI(fg-blue)" );
					m_ansiState.SetForeColor( 0x0000ff, strCodeHTML );
					break;
				}

				case 35:					// Magenta foreground
				{
					AnsiDisplay( "ANSI(fg-magenta)" );
					m_ansiState.SetForeColor( 0xff00ff, strCodeHTML );
					break;
				}

				case 36:					// Cyan foreground
				{
					AnsiDisplay( "ANSI(fg-cyan)" );
					m_ansiState.SetForeColor( 0x00ffff, strCodeHTML );
					break;
				}

				case 37:					// White foreground
				{
					AnsiDisplay( "ANSI(fg-white)" );
					m_ansiState.SetForeColor( COLOR_BRIGHT_WHITE,
												strCodeHTML );
					break;
				}

				case 40:					// Black background
				{
					AnsiDisplay( "ANSI(bg-black)" );
					m_ansiState.SetBackColor( COLOR_BLACK, strCodeHTML );
					break;
				}

				case 41:					// Red background
				{
					AnsiDisplay( "ANSI(bg-red)" );
					m_ansiState.SetBackColor( 0xff0000, strCodeHTML );
					break;
				}

				case 42:					// Green background
				{
					AnsiDisplay( "ANSI(bg-green)" );
					m_ansiState.SetBackColor( 0x00ff00, strCodeHTML );
					break;
				}

				case 43:					// Yellow background
				{
					AnsiDisplay( "ANSI(bg-yellow)" );
					m_ansiState.SetBackColor( 0xffff00, strCodeHTML );
					break;
				}

				case 44:					// Blue background
				{
					AnsiDisplay( "ANSI(bg-blue)" );
					m_ansiState.SetBackColor( 0x0000ff, strCodeHTML );
					break;
				}

				case 45:					// Magenta background
				{
					AnsiDisplay( "ANSI(bg-magenta)" );
					m_ansiState.SetBackColor( 0xff00ff, strCodeHTML );
					break;
				}

				case 46:					// Cyan background
				{
					AnsiDisplay( "ANSI(bg-cyan)" );
					m_ansiState.SetBackColor( 0x00ffff, strCodeHTML );
					break;
				}

				case 47:					// White background
				{
					AnsiDisplay( "ANSI(bg-white)" );
					m_ansiState.SetBackColor( COLOR_BRIGHT_WHITE,
												strCodeHTML );
					break;
				}

				default:
				{
					break;
				}
			}
			break;
		}

		case ANSI_ERASE:
		{
			switch( iCode )
			{
				case 2:						// Clear display
				{
					AnsiDisplay( "ANSI(clear)" );
					break;
				}

				default:
				{
					break;
				}
			}
			break;
		}
	}

	if (strCodeHTML.GetLength() > 0)
	{
		strOutput += strCodeHTML;
	}
}


void ChWorldConn::TelnetSend( int iCommand, int iOption )
{
	char	cSequence[3];

	cSequence[0] = (char)TN_IAC;
	cSequence[1] = (char)iCommand;
	cSequence[2] = (char)iOption;

	SendBlock( cSequence, 3 );

	#if defined( DUMP_TELNET_CODES )
	{
		string		strSequence;

		if (pstrTelnetLabel[iOption])
		{
			strSequence.Format( "sent IAC %s %s\n", pstrTelnetLabel[iCommand],
													pstrTelnetLabel[iOption] );
		}
		else 
		{
			strSequence.Format( "sent IAC %s %d\n", pstrTelnetLabel[iCommand],
													iOption );
		}

		TelnetDisplay( strSequence );
	}
	#endif	// defined( DUMP_TELNET_CODES )
}

void ChWorldConn::TelnetReceive( int iCommand, int iOption )
{
	#if defined( DUMP_TELNET_CODES )
	{
		string		strSequence;

		if (pstrTelnetLabel[iOption])
		{
			strSequence.Format( "recv IAC %s %s\n", pstrTelnetLabel[iCommand],
													pstrTelnetLabel[iOption] );
		}
		else 
		{
			strSequence.Format( "recv IAC %s %d\n", pstrTelnetLabel[iCommand],
													iOption );
		}

		TelnetDisplay( strSequence );
	}
	#endif	// defined( DUMP_TELNET_CODES )
}

void ChWorldConn::DoTelnetCmd( int iCommand, int iOption )
{
	/*
		From RFC 854:

		Telnet protocols always start with the IAC (255) character.  Following
		this character may be a number of bytes, but we only pay attention to
		the following:

			WILL		251
			WON'T		252
			DO			253
			DON'T		254

		These are commands, sent from one end to the other.  They say that the
		sender either WILL or WON'T do something, or that the receiver should
		either DO or DON'T do something.  The third byte indicates what should
		be done.  We currently pay attention to the following:

			ECHO			1
			TERMINAL_TYPE	24

		Others are ignored.
	*/

	switch( iCommand )
	{
		case TN_WILL:
		{
			TelnetReceive( TN_WILL, iOption );

			switch( iOption )
			{
				case TN_ECHO:
				{
                    if (m_flTelnetState & telnetStateEcho)
                    {
                    						// Stop local echo, and acknowledge

						GetMainInfo()->SetEchoState( echoTelnetOff );
                        m_flTelnetState &= ~telnetStateEcho;

                        TelnetSend( TN_DO, TN_ECHO );
                    }
                    else
                    {
						// We already said DO ECHO, so ignore WILL ECHO
                    }
					break;
				}

                case TN_EOR_OPT:
				{
					if (!(m_flTelnetState & telnetStateEOR))
					{
						m_flTelnetState |= telnetStateEOR;
						TelnetSend( TN_DO, TN_EOR_OPT );
					}
					else
					{
						// We already said DO EOR_OPT, so ignore WILL EOR_OPT
					}
				}

				default:
				{							// Don't accept other WILL offers

					TelnetSend( TN_DONT, iOption );
					break;
				}
			}
			break;
		}

		case TN_WONT:
		{
			TelnetReceive( TN_WONT, iOption );

			switch( iOption )
			{
				case TN_ECHO:
				{
                    if (m_flTelnetState & telnetStateEcho)
                    {
						// We're already echoing, so ignore WONT ECHO
                    }
                    else
                    {						// Resume local echo, and acknowledge

						GetMainInfo()->SetEchoState( echoOn );
                        m_flTelnetState |= telnetStateEcho;

                        TelnetSend( TN_DONT, TN_ECHO );
                    }
					break;
				}

                case TN_EOR_OPT:
				{
					if (!(m_flTelnetState & telnetStateEOR))
					{
                        // We're in DONT EOR_OPT state, ignore WONT EOR_OPT
					}
					else
					{						// Acknowledge

						m_flTelnetState &= ~telnetStateEOR;
						TelnetSend( TN_DONT, TN_EOR_OPT );
					}
				}

				default:
				{
					break;
				}
			}
			break;
		}

		case TN_DO:
		{
			TelnetReceive( TN_DO, iOption );
											// Refuse all DO requests
			TelnetSend( TN_WONT, iOption );
			break;
		}

		case TN_DONT:
		{
											/* Ignore all DONT requests (we're
												already in the DONT state) */
			TelnetReceive( TN_DONT, iOption );
			break;
		}

		default:
		{
			break;
		}
	}
}


void ChWorldConn::TranslateNewlinesToBreaks( string& strOut )
{
	int		iIndex;

	iIndex = strOut.Find( '\n' );
	if (-1 != iIndex)
	{
		string		strReturn;

		do
		{
			int		iCopyChars = iIndex;

			if ((iIndex > 0) && ('\r' == strOut[iIndex - 1]))
			{
				iCopyChars--;
			}

			strReturn += strOut.Left( iCopyChars ) + "<br>";
			strOut = strOut.Mid( iIndex + 1 );

			iIndex = strOut.Find( '\n' );

		} while (-1 != iIndex);
											// Append whatever's left
		strReturn += strOut;
											// Now return the processed string
		strOut = strReturn;
	}
}


void ChWorldConn::AddToTextBlock( const char* pstrData, int iDataLen,
									bool boolEndOfLine )
{
	int				iOriginalLen = m_strCurrentLine.GetLength();
	int				iLen = iOriginalLen;
	char*			pstrCurrentLine;

	iLen += iDataLen + 1;
	pstrCurrentLine = m_strCurrentLine.GetBuffer( iLen );
	pstrCurrentLine += iOriginalLen;

	strncpy( pstrCurrentLine, pstrData, iDataLen );
	pstrCurrentLine += iDataLen;
	*pstrCurrentLine = 0;
	m_strCurrentLine.ReleaseBuffer();
											/* Process buffer on end-of-line or
												on grossly large lines */

	if (boolEndOfLine || (m_strCurrentLine.GetLength() > MAX_LINE_LEN))
	{
		OnTextLine( m_strCurrentLine );
		m_strCurrentLine = "";
	}
}


void ChWorldConn::OnTextLine( const string& strLine )
{
	ChWorldMainInfo*	pInfo = GetMainInfo();
	string				strWorking( strLine );

	if (!IsPuebloEnhanced())
	{										/* If we're not Pueblo Enhanced, we
												keep hoping the world will tell
												us that they are */
		ChVersion	ver;

		if (pInfo->LookForPuebloEnhanced( strWorking, ver ))
		{
											// Yay!  They are!  Happy happy!
			SetPuebloEnhanced( true, ver );
		}
	}

	if (pInfo->WantTextLines())
	{
		if (IsPuebloEnhanced())
		{
			ChHtmlWnd::RemoveEntities( strWorking, strWorking );
		}
											// Process TinTin actions
		pInfo->OnTextLine( strWorking );
	}
}


void ChWorldConn::SetPuebloEnhanced( bool boolEnhanced,
										const ChVersion& versEnhanced )
{
	m_boolPuebloEnhanced = boolEnhanced;

	if (m_boolPuebloEnhanced)
	{
		SetPuebloEnhancedVersion( versEnhanced );
	}
}


// Local Variables: ***
// tab-width:4 ***
// End: ***
