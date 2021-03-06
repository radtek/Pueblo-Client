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

----------------------------------------------------------------------------*/

#include "ChMazePr.h"

#if defined( CH_PUEBLO_PLUGIN )
class ChMainInfo;
#endif

/////////////////////////////////////////////////////////////////////////////
// ChMazeRlabPrefPage dialog

class ChMazeRlabPrefPage : public ChMazePrefPage
{
	DECLARE_DYNCREATE(ChMazeRlabPrefPage)

// Construction
public:
	ChMazeRlabPrefPage();
	~ChMazeRlabPrefPage();

	void SetInitialPreferences( int iAsciiTextQuality,
								bool boolCollisionAlarm,        
								bool boolMoveVector,
								float fHeadlight );

#if defined( CH_PUEBLO_PLUGIN )
		void SetMainInfo( ChMainInfo* pMainInfo );
#endif

	virtual void WritePreferences();

	inline int GetAsciiTextQuality()
				{
					return m_iAsciiTextQuality;
				}

	inline int GetCollisionAlarm()
				{
					return m_boolCollisionAlarm;
				}



// Dialog Data
	//{{AFX_DATA(ChMazeRlabPrefPage)
	enum { IDD = IDD_PREF_PAGE_RLAB };
	int		m_iAsciiTextQuality;
	BOOL	m_boolCollisionAlarm;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(ChMazeRlabPrefPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation

protected:
	// Generated message map functions
	//{{AFX_MSG(ChMazeRlabPrefPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
