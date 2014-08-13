/*********************************************************************
 *                                                                   *
 * MODULE NAME :  cnrbase.h              AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-09-92                                           *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  Common definitions and function prototypes for CNRBASE.EXE       *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-09-92 - File Created                                          *
 *                                                                   *
 *  Rick Fishman                                                     *
 *  Code Blazers, Inc.                                               *
 *  4113 Apricot                                                     *
 *  Irvine, CA. 92720                                                *
 *  CIS ID: 72251,750                                                *
 *                                                                   *
 *********************************************************************/

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

#define ID_FIRST_DIRWINDOW  1         // Id of first Directory frame window

#define ID_RESOURCES        1         // Id of frame wnd resources in .RC file

#define CNR_DIRECTORY       30        // The start of our show

#define IDM_ICON            1101      // Menu options
#define IDM_NAME            1102
#define IDM_TREE            1103
#define IDM_DETAILS         1104
#define IDM_TEXT            1105

#define UM_CONTAINER_FILLED WM_USER   // Posted by Fill thread to primary thread

#define DEBUG_FILENAME      "cnrbase.dbg"

#define DIRECTORY_WINCLASS  "CnrBaseSample"

#define PROGRAM_TITLE       "CNRBASE Container Sample"

#define HABERR( hab )       (ERRORIDERROR( WinGetLastError( hab ) ))

#define ANCHOR( hwnd )      (WinQueryAnchorBlock( hwnd ))

#define HWNDERR( hwnd )      \
            (ERRORIDERROR( WinGetLastError( WinQueryAnchorBlock( hwnd ) ) ))

#define INSTDATA( hwnd )     (WinQueryWindowPtr( hwnd, 0 ))

#define PARENT( hwnd )       (WinQueryWindow( hwnd, QW_PARENT ))

#define GRANDPARENT( hwnd )  (WinQueryWindow( WinQueryWindow(hwnd, QW_PARENT)))

/**********************************************************************/
/*---------------------------- STRUCTURES ----------------------------*/
/**********************************************************************/

typedef struct _CNRITEM               // CONTAINER RECORD STRUCTURE
{
  MINIRECORDCORE rc;                  // Base information
  PSZ            pszFileName;         // Pointer to File name
  HPOINTER       hptrIcon;            // Icon handle
  CDATE          date;                // Date of file
  CTIME          time;                // Time of file
  ULONG          cbFile;              // File size
  CHAR           szFileName[CCHMAXPATH + 1]; // File name
  ULONG          attrFile;            // Attributes of this file

} CNRITEM, *PCNRITEM;

#define EXTRA_RECORD_BYTES (sizeof( CNRITEM ) - sizeof( MINIRECORDCORE ))


typedef struct _INSTANCE              // CONTAINER'S CLIENT WINDOW INSTANCE DATA
{
    BOOL fShutdown;                     // Main thread wants to shut down
    BOOL fContainerFilled;              // Fill thread has completed
    CHAR szCnrTitle[ CCHMAXPATH + 20 ]; // Container title
    CHAR szDirectory[ CCHMAXPATH + 1 ]; // Directory being displayed

} INSTANCE, *PINSTANCE;


typedef struct _THREADPARMS          // PARAMETERS TO PASS TO SECONDARY THREAD
{
    HWND hwndClient;                 // Client window handle

} THREADPARMS, *PTHREADPARMS;

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

// In common.c

VOID SetWindowTitle( HWND hwndClient, PSZ szFormat, ... );
VOID Msg( PSZ szFormat, ... );

// In create.c

HWND CreateDirectoryWin( PSZ szDirectory );
HWND CreateContainer( HWND hwndClient, PSZ szDirectory );

// In populate.c

VOID PopulateContainer( PVOID pThreadParms );

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

#ifdef GLOBALS_DEFINED
#  define DATADEF
#else
#  define DATADEF extern
#endif

DATADEF INT iWinCount;                  // Number of directory windows created

/***********************************************************************
 *                   E N D     O F     S O U R C E                     *
 **********************************************************************/
