/*********************************************************************
 *                                                                   *
 * MODULE NAME :  cnrez.c                AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-06-92                                           *
 *                                                                   *
 * HOW TO RUN THIS PROGRAM:                                          *
 *                                                                   *
 *  Just enter CNREZ on the command line.                            *
 *                                                                   *
 * MODULE DESCRIPTION:                                               *
 *                                                                   *
 *  Only module for CNREZ.EXE, a program that demonstrates the base  *
 *  functionality of a container control. This module creates a      *
 *  standard window with a container in it. It then inserts records  *
 *  into the container and allows the user to switch between views.  *
 *                                                                   *
 *  Drag/Drop, Deltas, Context Menus, Ownerdraw, MiniIcons,          *
 *  record-sharing, sorting are not demonstrated in this sample      *
 *  program.                                                         *
 *                                                                   *
 * NOTES:                                                            *
 *                                                                   *
 *  This program is strictly a sample and should be treated as such. *
 *  There is minimal error-checking and global variables are used.   *
 *  I *hate* doing that but the clarity of a sample program suffers  *
 *  when error-checking code is put in and window words are used to  *
 *  eliminate globals.                                               *
 *                                                                   *
 *  I hope this code proves useful for other PM programmers. The     *
 *  more of us the better!                                           *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-06-92 - Program coded                                         *
 *                                                                   *
 *  Rick Fishman                                                     *
 *  Code Blazers, Inc.                                               *
 *  4113 Apricot                                                     *
 *  Irvine, CA. 92720                                                *
 *  CIS ID: 72251,750                                                *
 *                                                                   *
 *********************************************************************/

/*********************************************************************/
/*------- Include relevant sections of the OS/2 header files --------*/
/*********************************************************************/

#define  INCL_WINFRAMEMGR
#define  INCL_WINMLE
#define  INCL_WINPOINTERS
#define  INCL_WINSTDCNR
#define  INCL_WINSTDDLGS
#define  INCL_WINWINDOWMGR

/**********************************************************************/
/*----------------------------- INCLUDES -----------------------------*/
/**********************************************************************/

#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnrez.h"

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

#define FRAME_FLAGS         (FCF_TASKLIST | FCF_TITLEBAR   | FCF_SYSMENU | \
                             FCF_MINMAX   | FCF_SIZEBORDER | FCF_MENU    | \
                             FCF_SHELLPOSITION)

#define PROGRAM_TITLE       "CNREZ Container Sample"

#define CONTAINER_COLUMNS   5

#define SPLITBAR_OFFSET     150

#define MAX_DATA_LENGTH     25

/**********************************************************************/
/*---------------------------- STRUCTURES ----------------------------*/
/**********************************************************************/

typedef struct _CNRITEM
{
  MINIRECORDCORE    rc;
  HPOINTER          hptrIcon;
  CDATE             date;
  CTIME             time;
  ULONG             ulData;
  PSZ               szData;

} CNRITEM, *PCNRITEM;

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

       INT   main               ( VOID );
static VOID  CustomizeWindow    ( VOID );
static VOID  SetContainerColumns( VOID );
static VOID  InsertRecord       ( VOID );
static VOID  FillInRecord       ( PCNRITEM pci, INT iRecNum );
static VOID  SetContainerView   ( ULONG ulViewType );
static VOID  CnrBeginEdit       ( PCNREDITDATA pced );
static VOID  CnrEndEdit         ( PCNREDITDATA pced );

FNWP wpClient;

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

CHAR szRecName[]  = "Container Item";

CHAR szChildRecName[]  = "Container Child Item";

CHAR szItemData[] = "Item Data";

CHAR szCnrTitle[] = "This is a Container";

CHAR szClass[] = "TestClass";

HAB  hab;

HWND hwndFrame, hwndClient, hwndCnr;

HPOINTER hptr;

/**********************************************************************/
/*------------------------------ MAIN --------------------------------*/
/*                                                                    */
/*  PROGRAM ENTRYPOINT                                                */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: return code                                               */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
INT main( VOID )
{
    HMQ   hmq;
    QMSG  qmsg;
    ULONG flFrame = FRAME_FLAGS;

    hab = WinInitialize( 0 );

    hmq = WinCreateMsgQueue( hab, 0 );

    WinRegisterClass( hab, szClass, wpClient, CS_SIZEREDRAW, 0 );

    hwndFrame = WinCreateStdWindow( HWND_DESKTOP, 0, &flFrame, szClass, NULL,
                                    0, NULLHANDLE, ID_RESOURCES, &hwndClient );

    CustomizeWindow();

    while( WinGetMsg( hab, &qmsg, (HWND) NULL, 0, 0 ) )
        WinDispatchMsg( hab, &qmsg );

    WinDestroyWindow( hwndFrame );

    WinDestroyMsgQueue( hmq );

    WinTerminate( hab );

    return 0;
}

/**********************************************************************/
/*------------------------- CustomizeWindow --------------------------*/
/*                                                                    */
/*  CREATE THE CONTAINER AND SET IT INTO FRAME/CLIENT.                */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID CustomizeWindow()
{
    INT i;

    hwndCnr = WinCreateWindow( hwndClient, WC_CONTAINER, NULL,
                               CCS_MINIRECORDCORE | WS_VISIBLE,
                               0, 0, 0, 0, hwndClient, HWND_BOTTOM,
                               CNR_SAMPLE, NULL, NULL );

    // Set up the Details view columns before we start

    SetContainerColumns();

    WinSetWindowPos( hwndFrame, HWND_TOP, 10, 10, 400, 400,
                     SWP_SIZE | SWP_MOVE | SWP_ZORDER | SWP_ACTIVATE );

    // Get the icon handle that will be used for all container records

    hptr = WinLoadPointer( HWND_DESKTOP, 0, ICO_DOIT );

    // Insert all records into the container

    for( i = 0; i < 10; i++ )
        InsertRecord();

    // Set the initial view of the container

    SetContainerView( CV_ICON );

    // Cause all container records to be painted

    WinSendMsg( hwndCnr, CM_INVALIDATERECORD, NULL, NULL );

    WinSetWindowText( hwndFrame, PROGRAM_TITLE );

    WinShowWindow( hwndFrame, TRUE );
}

/**********************************************************************/
/*----------------------- SetContainerColumns ------------------------*/
/*                                                                    */
/*  SET THE COLUMNS OF THE CONTAINER FOR DETAIL VIEW                  */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID SetContainerColumns()
{
    CNRINFO         cnri;
    PFIELDINFO      pfi, pfiFirst, pfiLastLeftCol;
    FIELDINFOINSERT fii;

    // Allocate memory for the container column data

    pfi = WinSendMsg( hwndCnr, CM_ALLOCDETAILFIELDINFO,
                      MPFROMLONG( CONTAINER_COLUMNS ), NULL );

    // Save the pointer to all column info. It will be needed for the
    // CM_INSERTDETAILFIELDINFO msg

    pfiFirst = pfi;

    // Set up information about each container column

    pfi->flData     = CFA_BITMAPORICON | CFA_HORZSEPARATOR | CFA_CENTER |
                      CFA_SEPARATOR;
    pfi->flTitle    = CFA_CENTER;
    pfi->pTitleData = "Icon";
    pfi->offStruct  = FIELDOFFSET( CNRITEM, hptrIcon );

    pfi             = pfi->pNextFieldInfo;
    pfi->flData     = CFA_STRING | CFA_CENTER | CFA_HORZSEPARATOR;
    pfi->flTitle    = CFA_CENTER;
    pfi->pTitleData = "Data";
    pfi->offStruct  = FIELDOFFSET( CNRITEM, szData );

    // pfiLastLeftCol is the last left column before the splitbar

    pfiLastLeftCol = pfi;

    pfi             = pfi->pNextFieldInfo;
    pfi->flData     = CFA_DATE | CFA_CENTER | CFA_HORZSEPARATOR | CFA_SEPARATOR;
    pfi->flTitle    = CFA_CENTER;
    pfi->pTitleData =  "Date";
    pfi->offStruct  = FIELDOFFSET( CNRITEM, date );

    pfi             = pfi->pNextFieldInfo;
    pfi->flData    = CFA_ULONG | CFA_CENTER | CFA_HORZSEPARATOR | CFA_SEPARATOR;
    pfi->flTitle    = CFA_CENTER;
    pfi->pTitleData = "Ulong";
    pfi->offStruct  = FIELDOFFSET( CNRITEM, ulData );

    pfi             = pfi->pNextFieldInfo;
    pfi->flData     = CFA_TIME | CFA_CENTER | CFA_HORZSEPARATOR;
    pfi->flTitle    = CFA_CENTER;
    pfi->pTitleData = "Time";
    pfi->offStruct  = FIELDOFFSET( CNRITEM, time );

    // Fill in information about the column data we are about to give the
    // container.

    (void) memset( &fii, 0, sizeof( FIELDINFOINSERT ) );

    fii.cb                   = sizeof( FIELDINFOINSERT );
    fii.pFieldInfoOrder      = (PFIELDINFO) CMA_FIRST;
    fii.cFieldInfoInsert     = (SHORT) CONTAINER_COLUMNS;
    fii.fInvalidateFieldInfo = TRUE;

    // Give the container the column information

    WinSendMsg( hwndCnr, CM_INSERTDETAILFIELDINFO, MPFROMP( pfiFirst ),
                MPFROMP( &fii ) );

    // Tell the container about the splitbar

    cnri.cb             = sizeof( CNRINFO );
    cnri.pFieldInfoLast = pfiLastLeftCol;
    cnri.xVertSplitbar  = SPLITBAR_OFFSET;

    WinSendMsg( hwndCnr, CM_SETCNRINFO, MPFROMP( &cnri ),
                MPFROMLONG( CMA_PFIELDINFOLAST | CMA_XVERTSPLITBAR ) );
}

/**********************************************************************/
/*---------------------------- wpClient ------------------------------*/
/*                                                                    */
/*  CLIENT WINDOW PROCEDURE.                                          */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
MRESULT EXPENTRY wpClient( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg )
    {
        case WM_ERASEBACKGROUND:

            return (MRESULT) TRUE;


        case WM_SIZE:

            WinSetWindowPos( hwndCnr, (HWND) NULL, 0, 0,
                             SHORT1FROMMP( mp2 ), SHORT2FROMMP( mp2 ),
                             SWP_SIZE | SWP_MOVE );

            return 0;


        case WM_DESTROY:

            // Free all memory allocated for the column data

            WinSendMsg( hwndCnr, CM_REMOVEDETAILFIELDINFO, NULL,
                        MPFROM2SHORT( 0, CMA_FREE ) );

            // Free all memory allocated for the container records

            WinSendMsg( hwndCnr, CM_REMOVERECORD, NULL,
                        MPFROM2SHORT( 0, CMA_FREE ) );

            WinDestroyPointer( hptr );

            break;


        case WM_COMMAND:

            switch( SHORT1FROMMP( mp1 ) )
            {
                case IDM_NAME:

                    SetContainerView( CV_NAME | CV_FLOW );

                    return 0;

                case IDM_ICON:

                    SetContainerView( CV_ICON );

                    return 0;

                case IDM_TREE:

                    SetContainerView( CV_TREE | CV_ICON );

                    return 0;

                case IDM_DETAILS:

                    SetContainerView( CV_DETAIL );

                    return 0;

                case IDM_TEXT:

                    SetContainerView( CV_TEXT | CV_FLOW );

                    return 0;
            }

            break;


        case WM_CONTROL:

            if( SHORT1FROMMP( mp1 ) == CNR_SAMPLE )
                switch( SHORT2FROMMP( mp1 ) )
                {
                    case CN_BEGINEDIT:

                        CnrBeginEdit( (PCNREDITDATA) mp2 );

                        break;


                    case CN_ENDEDIT:

                        CnrEndEdit( (PCNREDITDATA) mp2 );

                        break;
                }

            break;
    }

    return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

/**********************************************************************/
/*-------------------------- InsertRecord ----------------------------*/
/*                                                                    */
/*  INSERT RECORD AND ITS CHILD RECORDS INTO THE CONTAINER            */
/*                                                                    */
/*  INPUT: nothing                                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID InsertRecord()
{
    USHORT       usRecs = 1;
    static INT   iParent = 1, iChild;
    ULONG        cbExtra;
    PCNRITEM     pciDad, pciSave, pciChild;
    RECORDINSERT ri;

    // Calculate extra bytes needed for each record besides that needed for the
    // MINIRECORDCORE structure

    cbExtra = sizeof( CNRITEM ) - sizeof( MINIRECORDCORE );

    // Allocate memory for the parent record

    pciDad = WinSendMsg( hwndCnr, CM_ALLOCRECORD, MPFROMLONG( cbExtra ),
                         MPFROMSHORT( usRecs ) );

    // Fill in the parent record data

    FillInRecord( pciDad, iParent++ );

    (void) memset( &ri, 0, sizeof( RECORDINSERT ) );

    ri.cb                 = sizeof( RECORDINSERT );
    ri.pRecordOrder       = (PRECORDCORE) CMA_END;
    ri.pRecordParent      = (PRECORDCORE) NULL;
    ri.zOrder             = (USHORT) CMA_TOP;
    ri.cRecordsInsert     = usRecs;
    ri.fInvalidateRecord  = FALSE;

    // Insert the parent record

    WinSendMsg( hwndCnr, CM_INSERTRECORD, MPFROMP( pciDad ), MPFROMP( &ri ) );

    // We are about to insert the child records. Set the parent record to be
    // the one we just inserted.

    ri.pRecordParent = (PRECORDCORE) pciDad;

    // Insert 10 child records under the parent record.

    usRecs = 10;

    ri.cRecordsInsert = usRecs;

    pciChild = WinSendMsg( hwndCnr, CM_ALLOCRECORD, MPFROMLONG( cbExtra ),
                           MPFROMSHORT( usRecs ) );

    // Save the pointer to the beginning of the linked list

    pciSave = pciChild;

    for( iChild = 100; iChild < 110; iChild++ )
    {
        FillInRecord( pciChild, iChild );

        pciChild->rc.pszIcon = szChildRecName;

        pciChild = (PCNRITEM) pciChild->rc.preccNextRecord;
    }

    WinSendMsg( hwndCnr, CM_INSERTRECORD, MPFROMP( pciSave ), MPFROMP( &ri ) );
}

/**********************************************************************/
/*-------------------------- FillInRecord ----------------------------*/
/*                                                                    */
/*  POPULATE CONTAINER RECORD                                         */
/*                                                                    */
/*  INPUT: pointer to record buffer to fill,                          */
/*         number of record                                           */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID FillInRecord( PCNRITEM pci, INT iRecNum )
{
    pci->rc.cb          = sizeof( MINIRECORDCORE );
    pci->rc.pszIcon     = szRecName;
    pci->rc.hptrIcon    = hptr;

    pci->hptrIcon       = hptr;
    pci->szData         = szItemData;
    pci->date.day       = 11;
    pci->date.month     = 11;
    pci->date.year      = 11;
    pci->time.seconds   = 12;
    pci->time.minutes   = 12;
    pci->time.hours     = 12;
    pci->ulData         = iRecNum;
}

/**********************************************************************/
/*------------------------- SetContainerView -------------------------*/
/*                                                                    */
/*  SET THE TYPE OF VIEW FOR THE CONTAINER                            */
/*                                                                    */
/*  INPUT: view type to set to                                        */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID SetContainerView( ULONG ulViewType )
{
    CNRINFO cnri;

    cnri.cb = sizeof( CNRINFO );

    cnri.pszCnrTitle = szCnrTitle;

    cnri.flWindowAttr = ulViewType | CA_CONTAINERTITLE | CA_TITLESEPARATOR;

    switch( ulViewType )
    {
        case CV_DETAIL:

            cnri.flWindowAttr |= CA_DETAILSVIEWTITLES;

            break;

        case CV_TREE:
        case (CV_TREE | CV_ICON):

            cnri.cxTreeIndent = -1;
            cnri.cxTreeLine   = -1;

            cnri.flWindowAttr |=  CA_TREELINE;

            break;
    }

    WinSendMsg( hwndCnr, CM_SETCNRINFO, MPFROMP( &cnri ),
                MPFROMLONG( CMA_FLWINDOWATTR | CMA_CNRTITLE ) );

    if( ulViewType == CV_ICON )
        WinSendMsg( hwndCnr, CM_ARRANGE, NULL, NULL );
}

/**********************************************************************/
/*--------------------------- CnrBeginEdit ---------------------------*/
/*                                                                    */
/*  PROCESS CN_BEGINEDIT NOTIFY MESSAGE.                              */
/*                                                                    */
/*  INPUT: pointer to the CNREDITDATA structure                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID CnrBeginEdit( PCNREDITDATA pced )
{
    PFIELDINFO  pfi = pced->pFieldInfo;
    HWND        hwndMLE = WinWindowFromID( hwndCnr, CID_MLE );

    // pfi only available if details view

    if( !pfi || pfi->offStruct == FIELDOFFSET( CNRITEM, szData ) )
        WinSendMsg( hwndMLE, MLM_SETTEXTLIMIT,
                    MPFROMLONG( MAX_DATA_LENGTH ), (MPARAM) NULL );
}

/**********************************************************************/
/*---------------------------- CnrEndEdit ----------------------------*/
/*                                                                    */
/*  PROCESS CN_ENDEDIT NOTIFY MESSAGE.                                */
/*                                                                    */
/*  INPUT: pointer to the CNREDITDATA structure                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID CnrEndEdit( PCNREDITDATA pced )
{
    PCNRITEM        pcnri = (PCNRITEM) pced->pRecord;
    PFIELDINFO      pfi = pced->pFieldInfo;
    HWND            hwndMLE = WinWindowFromID( hwndCnr, CID_MLE );
    CHAR            szData[ MAX_DATA_LENGTH + 1 ];

    // pfi only available if details view

    if( !pfi || pfi->offStruct == FIELDOFFSET( CNRITEM, szData ) )
    {
        WinQueryWindowText( hwndMLE, MAX_DATA_LENGTH, szData );

        if( !stricmp( szData, "BADREC" ) )
            WinAlarm( HWND_DESKTOP, WA_WARNING );
    }

    WinSendMsg( hwndCnr, CM_INVALIDATERECORD, MPFROMP( &pcnri ),
                MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );
}

/*************************************************************************
 *                     E N D     O F     S O U R C E                     *
 *************************************************************************/
