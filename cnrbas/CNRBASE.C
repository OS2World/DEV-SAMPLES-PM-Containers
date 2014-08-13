/*********************************************************************
 *                                                                   *
 * MODULE NAME :  cnrbase.c              AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-09-92                                           *
 *                                                                   *
 * HOW TO RUN THIS PROGRAM:                                          *
 *                                                                   *
 *  By just entering CNRBASE on the command line, a container will   *
 *  be created that will contain the files in the current directory. *
 *  Any subdirectories will be included and expandable in Tree view. *
 *  The container starts in Tree view and a menu lets you switch     *
 *  between the other supported views.                               *
 *                                                                   *
 *  Optionally enter 'CNRBASE directory' and that directory will be  *
 *  displayed.                                                       *
 *                                                                   *
 * MODULE DESCRIPTION:                                               *
 *                                                                   *
 *  Root module for CNRBASE.EXE, a program that demonstrates the     *
 *  base functionality of a container control. This module contains  *
 *  the client window procedure and all the supporting functions for *
 *  the container messages.                                          *
 *                                                                   *
 *  This sample creates a simple Directory folder that displays      *
 *  icons for each file in a directory. It shows the Details, Name,  *
 *  Icon, and Tree views and lets the user switch between them. It   *
 *  shows simple direct-editing. The main purpose for this sample is *
 *  to demonstrate the process of creating a functional container,   *
 *  not to show its use once created.                                *
 *                                                                   *
 *  The container is populated with records from a secondary thread. *
 *  This is done not to complicate things but to let the user        *
 *  interact with the container before it is completely filled if we *
 *  are traversing a directory with many subdirectories.             *
 *                                                                   *
 *  Drag/Drop, Deltas, Context Menus, Ownerdraw, MiniIcons,          *
 *  record-sharing, sorting are not demonstrated in this sample      *
 *  program.                                                         *
 *                                                                   *
 * OTHER MODULES:                                                    *
 *                                                                   *
 *  create.c -   contains the code used to create and tailor the     *
 *               container. Once the container is created there are  *
 *               no functions called in this module.                 *
 *                                                                   *
 *  populate.c - contains the code for the thread used to fill the   *
 *               container with records.                             *
 *                                                                   *
 *  common.c -   contains common support routines for CNRBASE.EXE.   *
 *                                                                   *
 * NOTES:                                                            *
 *                                                                   *
 *  This program loses some simplicity by using PM programming       *
 *  techniques that most non-beginner programmers use such as the    *
 *  use of multiple threads, Window Words and error checking. These  *
 *  techniques are not necessary for this sample and this sample     *
 *  would be easier to understand if they were not used. I believe   *
 *  the benefits outweigh the drawbacks because this program will    *
 *  now serve as a more useful template. It will also more easily    *
 *  allow for the creation of multiple instances of the sample       *
 *  Directory window, which will become important in any program     *
 *  that builds on this one.                                         *
 *                                                                   *
 *  I hope this code proves useful for other PM programmers. The     *
 *  more of us the better!                                           *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-09-92 - Program coded                                         *
 *                                                                   *
 *  Rick Fishman                                                     *
 *  Code Blazers, Inc.                                               *
 *  4113 Apricot                                                     *
 *  Irvine, CA. 92720                                                *
 *  CIS ID: 72251,750                                                *
 *                                                                   *
 *********************************************************************/

#pragma strings(readonly)   // used for debug version of memory mgmt routines

/*********************************************************************/
/*------- Include relevant sections of the OS/2 header files --------*/
/*********************************************************************/

#define INCL_DOSERRORS
#define INCL_WINDIALOGS
#define INCL_WINERRORS
#define INCL_WINFRAMEMGR
#define INCL_WINMLE
#define INCL_WINSTDCNR
#define INCL_WINSTDDLGS
#define INCL_WINWINDOWMGR

#define GLOBALS_DEFINED        // This module defines the globals in cnrbase.h

/**********************************************************************/
/*----------------------------- INCLUDES -----------------------------*/
/**********************************************************************/

#include <os2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnrbase.h"

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

/**********************************************************************/
/*---------------------------- STRUCTURES ----------------------------*/
/**********************************************************************/

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

       INT   main               ( INT iArgc, PSZ szArg[] );
static BOOL  InitClient         ( HWND hwndClient, PSZ szDirectory );
static VOID  SetContainerView   ( HWND hwndClient, ULONG ulViewType );
static VOID  KeepCnrAspectRatio ( HWND hwndClient, ULONG cxNew, ULONG cyNew );
static VOID  CnrBeginEdit       ( HWND hwndClient, PCNREDITDATA pced );
static VOID  CnrEndEdit         ( HWND hwndClient, PCNREDITDATA pced );
static ULONG GetMaxNameSize     ( CHAR chDrive );
static VOID  ContainerFilled    ( HWND hwndClient );
static VOID  UserWantsToClose   ( HWND hwndClient );
static VOID  FreeResources      ( HWND hwndClient );

FNWP wpClient;

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

/**********************************************************************/
/*------------------------------ MAIN --------------------------------*/
/*                                                                    */
/*  PROGRAM ENTRYPOINT                                                */
/*                                                                    */
/*  INPUT: command line                                               */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: return code                                               */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
INT main( INT iArgc, PSZ szArg[] )
{
    BOOL  fSuccess = FALSE;
    HAB   hab;
    HMQ   hmq = NULLHANDLE;
    QMSG  qmsg;
    PSZ   szStartingDir = NULL;
    HWND  hwndFrame = NULLHANDLE;

    // This macro is defined for the debug version of the C Set/2 Memory
    // Management routines. Since the debug version writes to stderr, we
    // send all stderr output to a debuginfo file. Look in MAKEFILE to see how
    // to enable the debug version of those routines.

#ifdef __DEBUG_ALLOC__
    freopen( DEBUG_FILENAME, "w", stderr );
#endif

    // Get the directory to display from the command line if specified.

    if( iArgc > 1 )
        szStartingDir = szArg[ 1 ];

    hab = WinInitialize( 0 );

    if( hab )
        hmq = WinCreateMsgQueue( hab, 0 );
    else
    {
        DosBeep( 1000, 100 );

        (void) fprintf( stderr, "WinInitialize failed!" );
    }

    if( hmq )

        // CS_SIZEREDRAW needed for initial display of the container in the
        // client window. Allocate enough extra bytes for 1 window word.

        fSuccess = WinRegisterClass( hab, DIRECTORY_WINCLASS, wpClient,
                                     CS_SIZEREDRAW, sizeof( PVOID ) );
    else
    {
        DosBeep( 1000, 100 );

        (void) fprintf( stderr, "WinCreateMsgQueue RC(%X)", HABERR( hab ) );
    }

    if( fSuccess )

        // CreateDirectoryWin is in CREATE.C

        hwndFrame = CreateDirectoryWin( szStartingDir );
    else
        Msg( "WinRegisterClass RC(%X)", HABERR( hab ) );

    if( hwndFrame )
        while( WinGetMsg( hab, &qmsg, NULLHANDLE, 0, 0 ) )
            (void) WinDispatchMsg( hab, &qmsg );

    if( hmq )
        (void) WinDestroyMsgQueue( hmq );

    if( hab )
        (void) WinTerminate( hab );

#ifdef __DEBUG_ALLOC__
    _dump_allocated( -1 );
#endif

    return 0;
}

/**********************************************************************/
/*---------------------------- wpClient ------------------------------*/
/*                                                                    */
/*  CLIENT WINDOW PROCEDURE FOR THE DIRECTORY WINDOW.                 */
/*                                                                    */
/*  NOTE: This window is created by CreateDirectoryWin in CREATE.C    */
/*                                                                    */
/*  INPUT: standard window procedure parameters                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: result of message processing                              */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
MRESULT EXPENTRY wpClient( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg )
    {
        case WM_CREATE:

            // If window initialization fails, don't create window

            if( InitClient( hwnd, (PSZ) mp1 ) )
            {
                iWinCount++;

                break;
            }
            else
                return (MRESULT) TRUE;


        case UM_CONTAINER_FILLED:

            // This message is posted to us by the thread that fills the
            // container with records. This indicates that the container is
            // now filled.

            ContainerFilled( hwnd );

            return 0;


        case WM_ERASEBACKGROUND:

            // Paint the client window in the default color

            return (MRESULT) TRUE;


        case WM_SIZE:

            // Size the container with the client window

            if( !WinSetWindowPos( WinWindowFromID( hwnd, CNR_DIRECTORY ),
                                  NULLHANDLE, 0, 0, SHORT1FROMMP( mp2 ),
                                  SHORT2FROMMP( mp2 ), SWP_MOVE | SWP_SIZE ) )
                Msg( "WM_SIZE WinSetWindowPos RC(%X)", HWNDERR( hwnd ) );

            return 0;


        case WM_COMMAND:    // Menu messages

            switch( SHORT1FROMMP( mp1 ) )
            {
                case IDM_TREE:

                    SetContainerView( hwnd, CV_TREE | CV_ICON );

                    return 0;

                case IDM_NAME:

                    SetContainerView( hwnd, CV_NAME | CV_FLOW );

                    return 0;

                case IDM_TEXT:

                    SetContainerView( hwnd, CV_TEXT | CV_FLOW );

                    return 0;

                case IDM_ICON:

                    SetContainerView( hwnd, CV_ICON );

                    return 0;

                case IDM_DETAILS:

                    SetContainerView( hwnd, CV_DETAIL );

                    return 0;
            }

            break;


        case WM_CONTROL:        // These control messages sent by the container

            if( SHORT1FROMMP( mp1 ) == CNR_DIRECTORY )
                switch( SHORT2FROMMP( mp1 ) )
                {
                    case CN_BEGINEDIT:

                        CnrBeginEdit( hwnd, (PCNREDITDATA) mp2 );

                        break;


                    case CN_ENDEDIT:

                        CnrEndEdit( hwnd, (PCNREDITDATA) mp2 );

                        break;
                }

            break;


        case WM_CLOSE:

            // Don't let the WM_QUIT message get posted. *We* will decide
            // when to shut down the message queue.

            UserWantsToClose( hwnd );

            return 0;


        case WM_DESTROY:

            FreeResources( hwnd );

            // If this is the last window open, shut down the message queue
            // which will kill the .EXE. In this program there is only one
            // window so the first WM_DESTROY will always do this.

            if( --iWinCount == 0 )
                (void) WinPostMsg( NULLHANDLE, WM_QUIT, NULL, NULL );

            break;
    }

    return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

/**********************************************************************/
/*--------------------------- InitClient -----------------------------*/
/*                                                                    */
/*  PROCESS WM_CREATE FOR THE CLIENT WINDOW.                          */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         pointer to directory to display in container               */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL InitClient( HWND hwndClient, PSZ szDirectory )
{
    BOOL      fSuccess = TRUE;
    PINSTANCE pi = (PINSTANCE) malloc( sizeof( INSTANCE ) );

    if( pi )
    {
        (void) memset( pi, 0, sizeof( INSTANCE ) );

        if( WinSetWindowPtr( hwndClient, 0, pi ) )
        {
            // CreateContainer is located in CREATE.C

            HWND hwndCnr = CreateContainer( hwndClient, szDirectory );

            if( hwndCnr )

                // Set the initial container view to Tree/Icon view

                SetContainerView( hwndClient, CV_TREE | CV_ICON );
            else
                fSuccess = FALSE;
        }
        else
        {
            fSuccess = FALSE;

            Msg( "InitClient WinSetWindowPtr RC(%X)", HWNDERR( hwndClient ) );
        }
    }
    else
    {
        fSuccess = FALSE;

        Msg( "InitClient out of memory!" );
    }

    return fSuccess;
}

/**********************************************************************/
/*------------------------- SetContainerView -------------------------*/
/*                                                                    */
/*  SET THE TYPE OF VIEW FOR THE CONTAINER                            */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         view type to set to                                        */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID SetContainerView( HWND hwndClient, ULONG ulViewType )
{
    PINSTANCE pi = INSTDATA( hwndClient );
    CNRINFO   cnri;

    if( !pi )
    {
        Msg( "Set..View cant get Inst data. RC(%X)", HWNDERR( hwndClient ) );

        return;
    }

    cnri.cb = sizeof( CNRINFO );

    // Set the container window attributes: Set the container view mode. Use a
    // container title. Put a separator between the title and the records
    // beneath it. Make the container title read-only.

    cnri.flWindowAttr = ulViewType | CA_CONTAINERTITLE | CA_TITLESEPARATOR |
                        CA_TITLEREADONLY;

    switch( ulViewType )
    {
        case CV_TREE:
        case (CV_TREE | CV_ICON):

            (void) strcpy( pi->szCnrTitle, "Tree/icon view - " );

            // Use default spacing between levels in the tree view. Also use
            // the default width of a line that shows record relationships.

            cnri.cxTreeIndent = -1;
            cnri.cxTreeLine   = -1;

            cnri.flWindowAttr |=  CA_TREELINE;

            break;

        case CV_ICON:

            (void) strcpy( pi->szCnrTitle, "Icon view - " );

            break;

        case CV_NAME:
        case (CV_NAME | CV_FLOW):

            (void) strcpy( pi->szCnrTitle, "Name/flowed view - " );

            break;

        case CV_DETAIL:

            (void) strcpy( pi->szCnrTitle, "Detail view - " );

            // If we are in DETAIL view, tell the container that we will be
            // using column headings.

            cnri.flWindowAttr |= CA_DETAILSVIEWTITLES;

            break;

        case CV_TEXT:
        case (CV_TEXT | CV_FLOW):

            (void) strcpy( pi->szCnrTitle, "Text/flowed view - " );

            break;
    }

    (void) strcat( pi->szCnrTitle, pi->szDirectory );

    cnri.pszCnrTitle = pi->szCnrTitle;

    // Set the line spacing between rows to be the minimal value so we conserve
    // on container whitespace.

    cnri.cyLineSpacing = 0;

    // Set the container parameters. Note that mp2 specifies which fields of
    // of the CNRINFO structure to use. The CMA_FLWINDOWATTR says to use
    // flWindowAttr to specify which 'Window Attribute' fields to use.

    if( !WinSendDlgItemMsg( hwndClient, CNR_DIRECTORY, CM_SETCNRINFO,
                            MPFROMP( &cnri ),
                            MPFROMLONG( CMA_FLWINDOWATTR | CMA_CNRTITLE |
                                        CMA_LINESPACING ) ) )
        Msg( "SetContainerView CM_SETCNRINFO RC(%X)", HWNDERR( hwndClient ) );

    // The CM_ARRANGE message is applicable only in ICON view. It will arrange
    // the icons according to CUA. Note that this message is unnecessary if
    // the CCS_AUTOPOSITION style is used on the WinCreateWindow call for the
    // container. The problem with using that style is that you have no control
    // over *when* the arranging is done.

    if( ulViewType == CV_ICON )
        if( !WinSendDlgItemMsg( hwndClient, CNR_DIRECTORY, CM_ARRANGE, NULL,
                                NULL ) )
            Msg( "SetContainerView CM_ARRANGE RC(%X)", HWNDERR( hwndClient ) );
}

/**********************************************************************/
/*----------------------- KeepCnrAspectRatio -------------------------*/
/*                                                                    */
/*  KEEP THE CONTAINER FILLING THE CLIENT AREA ON A WM_SIZE MESSAGE.  */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         new client window width,                                   */
/*         new client window height                                   */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID KeepCnrAspectRatio( HWND hwndClient, ULONG cxNew, ULONG cyNew )
{
    if( !WinSetWindowPos( WinWindowFromID( hwndClient, CNR_DIRECTORY ),
                          NULLHANDLE, 0, 0, cxNew, cyNew,
                          SWP_MOVE | SWP_SIZE ) )
        Msg( "Keep..Ratio WinSetWindowPos RC(%X)", HWNDERR( hwndClient ) );
}

/**********************************************************************/
/*--------------------------- CnrBeginEdit ---------------------------*/
/*                                                                    */
/*  PROCESS CN_BEGINEDIT NOTIFY MESSAGE.                              */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         pointer to the CNREDITDATA structure                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID CnrBeginEdit( HWND hwndClient, PCNREDITDATA pced )
{
    PFIELDINFO  pfi = pced->pFieldInfo;
    PINSTANCE   pi = INSTDATA( hwndClient );

    if( !pi )
    {
        Msg( "CnrBeginEdit cant get Inst data. RC(%X)", HWNDERR( hwndClient ) );

        return;
    }

    // pfi only available if details view. If we are in details view and
    // the column the user is direct-editing is the file name field, set the
    // text limit of the MLE to the Maximum that the filename can be.
    // If MLM_SETTEXTLIMIT returns a non-zero value, it means that the text
    // length in the MLE is greater than what we are trying to set it to.

    if( !pfi || pfi->offStruct == FIELDOFFSET( CNRITEM, pszFileName ) )
        if( WinSendDlgItemMsg( WinWindowFromID( hwndClient, CNR_DIRECTORY ),
                        CID_MLE, MLM_SETTEXTLIMIT,
                        MPFROMLONG( GetMaxNameSize( pi->szDirectory[ 0 ] ) ),
                        NULL) )
            Msg( "MLM_SETTEXTLIMIT failed. RC(%X)", HWNDERR( hwndClient ) );
}

/**********************************************************************/
/*-------------------------- GetMaxNameSize --------------------------*/
/*                                                                    */
/*  GET THE MAXIMUM SIZE OF A FILE NAME FOR A DRIVE.                  */
/*                                                                    */
/*  INPUT: drive letter                                               */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: max filename size                                         */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/

#define QFSBUFFSIZE 100

static ULONG GetMaxNameSize( CHAR chDrive )
{
    APIRET      rc;
    CHAR        szDrive[ 3 ], achBuf[ QFSBUFFSIZE ];
    PFSQBUFFER2 pfsqb = (PFSQBUFFER2) achBuf;
    ULONG       cbFileName = 0, cbBuf = sizeof( achBuf );
    PSZ         szFSDName;

    szDrive[ 0 ] = chDrive;
    szDrive[ 1 ] = ':';
    szDrive[ 2 ] = 0;

    // Get the file system type for this drive (i.e. HPFS, FAT, etc.)

    rc = DosQueryFSAttach( szDrive, 0, FSAIL_QUERYNAME, (PFSQBUFFER2) achBuf,
                           &cbBuf );

    // Should probably handle ERROR_BUFFER_OVERFLOW more gracefully, but not
    // in this sample program <g>

    if( rc )
        cbFileName = 12;                     // If any errors, assume FAT
    else
    {
        szFSDName = pfsqb->szName + pfsqb->cbName + 1;

        if( !stricmp( "FAT", szFSDName ) )
            cbFileName = 12;
        else
            cbFileName = CCHMAXPATH;         // If not FAT, assume maximum path
    }

    return cbFileName;
}

/**********************************************************************/
/*---------------------------- CnrEndEdit ----------------------------*/
/*                                                                    */
/*  PROCESS CN_ENDEDIT NOTIFY MESSAGE.                                */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         pointer to the CNREDITDATA structure                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID CnrEndEdit( HWND hwndClient, PCNREDITDATA pced )
{
    PINSTANCE   pi = INSTDATA( hwndClient );
    PCNRITEM    pci = (PCNRITEM) pced->pRecord;
    PFIELDINFO  pfi = pced->pFieldInfo;
    HWND        hwndCnr, hwndMLE;
    CHAR        szData[ CCHMAXPATH + 1 ];

    if( !pi )
    {
        Msg( "CnrEndEdit cant get Inst data. RC(%X)", HWNDERR( hwndClient ) );

        return;
    }

    hwndCnr = WinWindowFromID( hwndClient, CNR_DIRECTORY );

    // Get the handle to the MLE that the container uses for direct editing

    hwndMLE = WinWindowFromID( hwndCnr, CID_MLE );

    // pfi only available if details view

    if( !pfi || pfi->offStruct == FIELDOFFSET( CNRITEM, pszFileName ) )
    {
        WinQueryWindowText( hwndMLE, sizeof( szData ), szData );

        // Just a cute little test to make sure this is all working...

        if( !stricmp( szData, "BADREC" ) )
            (void) WinAlarm( HWND_DESKTOP, WA_WARNING );
    }

    // Invalidate the container record that was being direct-edited because
    // the text has probably changed. Note that this should only be done if the
    // text changed. Since this is just a sample program we do it regardless...

    if( !WinSendMsg( hwndCnr, CM_INVALIDATERECORD, MPFROMP( &pci ),
                     MPFROM2SHORT( 1, CMA_TEXTCHANGED ) ) )
        Msg( "CnrEndEdit CM_INVALIDATERECORD RC(%X)", HWNDERR( hwndCnr ) );
}

/**********************************************************************/
/*------------------------- ContainerFilled --------------------------*/
/*                                                                    */
/*  THE FILL THREAD HAS COMPLETED.                                    */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID ContainerFilled( HWND hwndClient )
{
    PINSTANCE pi = INSTDATA( hwndClient );

    if( !pi )
    {
        Msg( "ContainerFilled cant get Inst data. RC(%X)", HWNDERR(hwndClient));

        return;
    }

    // If the user closed the window while the Fill thread was executing, we
    // want to shut down this window now.

    if( pi->fShutdown )
        WinDestroyWindow( PARENT( hwndClient ) );
    else
    {
        // Set a flag so the window will know the Fill thread has finished

        pi->fContainerFilled = TRUE;

        // Set the titlebar to the program title. We do this because while
        // the container was being filled, the titlebar text was changed
        // to indicate progress.

        SetWindowTitle( hwndClient, PROGRAM_TITLE );
    }
}

/**********************************************************************/
/*------------------------- UserWantsToClose -------------------------*/
/*                                                                    */
/*  PROCESS THE WM_CLOSE MESSAGE.                                     */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID UserWantsToClose( HWND hwndClient )
{
    PINSTANCE pi = INSTDATA( hwndClient );

    if( !pi )
    {
        Msg( "UserWantsToClose cant get Inst data. RC(%X)",HWNDERR(hwndClient));

        return;
    }

    // If the Fill thread has completed, destroy the frame window.
    // If the Fill thread has not completed, set a flag that will cause it to
    // terminate, then the destroy will occur under the UM_CONTAINER_FILLED
    // message.

    if( pi->fContainerFilled )
        WinDestroyWindow( PARENT( hwndClient ) );
    else
    {
        // Indicate in the titlebar that the window is in the process of
        // closing.

        SetWindowTitle( hwndClient, "%s: CLOSING...", PROGRAM_TITLE );

        // Set a flag that will tell the Fill thread to shut down

        pi->fShutdown = TRUE;
    }
}

/**********************************************************************/
/*-------------------------- FreeResources ---------------------------*/
/*                                                                    */
/*  FREE PROGRAM RESOURCES.                                           */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID FreeResources( HWND hwndClient )
{
    PINSTANCE pi = INSTDATA( hwndClient );

    if( !pi )
    {
        Msg( "FreeResources cant get Inst data. RC(%X)", HWNDERR( hwndClient ));

        return;
    }

    // Free the memory that was allocated with CM_ALLOCDETAILFIELDINFO. The
    // zero in the first SHORT of mp2 says to free memory for all columns

    if( -1 == (INT) WinSendDlgItemMsg( hwndClient, CNR_DIRECTORY,
                                       CM_REMOVEDETAILFIELDINFO, NULL,
                                       MPFROM2SHORT( 0, CMA_FREE ) ) )
        Msg( "CM_REMOVEDETAILFIELDINFO failed! RC(%X)", HWNDERR( hwndClient ) );

    // Free the memory allocated by the CM_INSERTRECORD messages. The zero
    // in the first SHORT of mp2 says to free memory for all records

    if( -1 == (INT) WinSendDlgItemMsg( hwndClient, CNR_DIRECTORY,
                                       CM_REMOVERECORD, NULL,
                                       MPFROM2SHORT( 0, CMA_FREE ) ) )
        Msg( "CM_REMOVERECORD failed! RC(%X)", HWNDERR( hwndClient ) );

    free( pi );
}

/*************************************************************************
 *                     E N D     O F     S O U R C E                     *
 *************************************************************************/
