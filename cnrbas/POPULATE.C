/*********************************************************************
 *                                                                   *
 * MODULE NAME :  populate.c             AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-09-92                                           *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  This module is part of CNRBASE.EXE. It performs the function of  *
 *  filling a container window with file icons. This processing is   *
 *  taking place in a secondary thread that was started with         *
 *  _beginthread from the CreateContainer function in create.c.      *
 *                                                                   *
 *  Recursion is used to fill the container with all subdirectories  *
 *  from the base directory.  This is done to demonstrate the Tree   *
 *  view.                                                            *
 *                                                                   *
 *  The reason this is in a separate thread is that, if we are       *
 *  traversing the root directory and its subdirectories, it could   *
 *  take a long time to fill the container.                          *
 *                                                                   *
 *  This thread posts a UM_CONTAINER_FILLED message to the client    *
 *  window when the container is filled.                             *
 *                                                                   *
 * CALLABLE FUNCTIONS:                                               *
 *                                                                   *
 *  VOID PopulateContainer( PVOID pThreadParms );                    *
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

#define  INCL_DOSERRORS
#define  INCL_DOSFILEMGR
#define  INCL_WINERRORS
#define  INCL_WINFRAMEMGR
#define  INCL_WINSTDCNR
#define  INCL_WINWINDOWMGR

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

#define FILES_TO_GET       100         // Nbr of files to search for at a time
#define FF_BUFFSIZE        (sizeof( FILEFINDBUF3 ) * FILES_TO_GET)

/**********************************************************************/
/*---------------------------- STRUCTURES ----------------------------*/
/**********************************************************************/

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

static VOID ProcessDirectory ( HWND hwndCnr, PCNRITEM pciParent, PSZ szDirBase,
                               PSZ szDirectory );
static VOID RecurseSubdirs   ( HWND hwndCnr, PCNRITEM pciParent, PSZ szDir );
static BOOL InsertRecords    ( HWND hwndCnr, PCNRITEM pciParent, PSZ szDir,
                               PFILEFINDBUF3 pffb, ULONG cFiles);
static BOOL FillInRecord     ( PCNRITEM pci, PSZ szDir, PFILEFINDBUF3 pffb );

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

/**********************************************************************/
/*------------------------ PopulateContainer -------------------------*/
/*                                                                    */
/*  THREAD THAT FILLS THE CONTAINER WITH RECORDS.                     */
/*                                                                    */
/*  INPUT: pointer to thread parameters passed by main thread         */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
VOID PopulateContainer( PVOID pThreadParms )
{
    HAB         hab;
    HMQ         hmq = NULLHANDLE;
    HWND        hwndClient = ((PTHREADPARMS) pThreadParms)->hwndClient;
    PINSTANCE   pi = INSTDATA( hwndClient );

    // We must create a message queue so that this thread can do WinSendMsg's.
    // All contact with the container is done with WinSendMsg.

    hab = WinInitialize( 0 );

    if( hab )
        hmq = WinCreateMsgQueue( hab, 0 );
    else
        Msg( "PopulateContainer WinInitialize failed!" );

    if( hmq )
    {
        if( pi )
        {
            // Insert the container records from the specified directory. Using
            // NULL for the parent record tells ProcessDirectory that it is
            // dealing with the top-level directory should recursion cause
            // subdirectories to be expanded. The last parameter is a pointer
            // to a subdirectory used during recursion and can be NULL here.

            ProcessDirectory( WinWindowFromID( hwndClient, CNR_DIRECTORY ),
                              NULL, pi->szDirectory, NULL );
        }
        else
            Msg( "PopulateContainer cant get Inst data. RC(%X)",
                 HWNDERR( hwndClient ) );
    }
    else
        Msg( "PopulateContainer CreateMsgQueue failed! RC(%X)", HABERR( hab ) );

    if( hmq )
        (void) WinDestroyMsgQueue( hmq );

    if( hab )
        (void) WinTerminate( hab );

    free( pThreadParms );

    // Let the primary thread know the container is filled

    WinPostMsg( hwndClient, UM_CONTAINER_FILLED, NULL, NULL );

    _endthread();
}

/**********************************************************************/
/*------------------------- ProcessDirectory -------------------------*/
/*                                                                    */
/*  POPULATE THE CONTAINER WITH THE CONTENTS OF A DIRECTORY           */
/*                                                                    */
/*  INPUT: container window handle,                                   */
/*         parent container record,                                   */
/*         base directory name with drive qualifier,                  */
/*         directory to display                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID ProcessDirectory( HWND hwndCnr, PCNRITEM pciParent,
                              PSZ szDirBase, PSZ szDirectory )
{
    // Allocate a buffer big enough to hold FILES_TO_GET files. Then allocate
    // a work buffer to hold the full file spec.

    PFILEFINDBUF3 pffb = malloc( FF_BUFFSIZE );
    PSZ           szFileSpec = malloc( CCHMAXPATH + 1 );

    if( pffb && szFileSpec )
    {
        HDIR   hdir = HDIR_SYSTEM;
        ULONG  ulMaxFiles = FILES_TO_GET;
        PCH    pchEndPath;
        APIRET rc;
        PINSTANCE pi = INSTDATA( PARENT( hwndCnr ) );

        // Combine C:\DIR1\DIR2 and DIR3 to make C:\DIR1\DIR2\DIR3\*.*
        // Keep a placeholder so we can strip the trailing '\*.*' after the
        // DosFindFirst has completed.

        (void) strcpy( szFileSpec, szDirBase );

        if( szDirectory )
        {
            (void) strcat( szFileSpec, "\\" );

            (void) strcat( szFileSpec, szDirectory );
        }

        pchEndPath = szFileSpec + strlen( szFileSpec );

        (void) strcat( szFileSpec, "\\*.*" );

        // Get buffer of files up to the maximum FILES_TO_GET files. Get both
        // normal files and directories.

        rc = DosFindFirst( szFileSpec, &hdir, FILE_NORMAL | FILE_DIRECTORY,
                           pffb, FF_BUFFSIZE, &ulMaxFiles, FIL_STANDARD );

        *pchEndPath = 0;

        // Let the user know what directory we're processing unless we're
        // in the process of shutting down

        if( pi && !pi->fShutdown )
            SetWindowTitle( PARENT( hwndCnr ), "%s: Processing %s...",
                            PROGRAM_TITLE, szFileSpec );

        while( !rc )
        {
            // If the main thread wants to shutdown, accommodate it

            if( pi && pi->fShutdown )
                break;

            // Insert the files into the container

            if( InsertRecords( hwndCnr, pciParent, szFileSpec, pffb,
                               ulMaxFiles ) )
            {
                // Recursively insert child records into the container for any
                // subdirectories found under this directory

                RecurseSubdirs( hwndCnr, pciParent, szFileSpec );

                // Get more files if there are any

                rc = DosFindNext( hdir, pffb, FF_BUFFSIZE, &ulMaxFiles );
            }
            else
                rc = 1;
        }

        DosFindClose( hdir );
    }
    else
        Msg( "ProcessDirectory Out of Memory!" );

    if( pffb )
        free( pffb );

    if( szFileSpec )
        free( szFileSpec );
}

/**********************************************************************/
/*-------------------------- RecurseSubdirs --------------------------*/
/*                                                                    */
/*  CALL ProcessDirectory FOR EACH SUBDIRECTORY OF THE BASE DIRECTORY */
/*                                                                    */
/*  INPUT: container window handle,                                   */
/*         parent container record,                                   */
/*         base directory name with drive qualifier                   */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID RecurseSubdirs( HWND hwndCnr, PCNRITEM pciParent, PSZ szDirBase )
{
    USHORT    usWhatRec = CMA_FIRSTCHILD;
    PCNRITEM  pci = pciParent;
    PINSTANCE pi = INSTDATA( PARENT( hwndCnr ) );

    // Note that this function is called AFTER a subdirectory's files have
    // been inserted into the container. We traverse those records to see if
    // any are subdirectories themselves.

    for( ; ; )
    {
        // If the main thread wants to shutdown, accommodate it

        if( pi && pi->fShutdown )
            break;

        // Get the next child record. We start with usWhatRec set to
        // CMA_FIRSTCHILD, then use CMA_NEXT after getting the first child.
        // This enumerates the children of pciParent. The docs don't
        // go into it, but my tests show that using CMA_NEXT after a
        // CMA_FIRSTCHILD will return NULL after the last *child* has been
        // enumerated, rather than the last record in the container. Of course
        // that is how it should work but it's not documented.

        pci = WinSendMsg( hwndCnr, CM_QUERYRECORD, MPFROMP( pci ),
                          MPFROM2SHORT( usWhatRec, CMA_ITEMORDER ) );

        if( (INT) pci == -1 )
        {
            Msg( "RecurseSubdirs CM_QUERYRECORD RC(%X)", HWNDERR( hwndCnr ) );

            break;
        }

        if( !pci )
            break;

        // If we found a subdirectory that isn't '.' or '..', recursively
        // call PopulateContainer for that subdirectory (remember, while we're
        // in this function, the parent's PopulateContainer hasn't completed
        // yet)

        if( (pci->attrFile & FILE_DIRECTORY) && pci->szFileName[0] != '.' )
            ProcessDirectory( hwndCnr, pci, szDirBase, pci->szFileName );

        usWhatRec = CMA_NEXT;
    }
}

/**********************************************************************/
/*--------------------------- InsertRecords --------------------------*/
/*                                                                    */
/*  INSERT DIRECTORY ENTRIES INTO THE CONTAINER.                      */
/*                                                                    */
/*  INPUT: container window handle,                                   */
/*         parent container record,                                   */
/*         directory being displayed,                                 */
/*         buffer containing directory entries,                       */
/*         count of files in directory buffer                         */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL InsertRecords( HWND hwndCnr, PCNRITEM pciParent, PSZ szDirectory,
                           PFILEFINDBUF3 pffb, ULONG cFiles )
{
    BOOL     fSuccess = TRUE;
    PBYTE    pbBuf = (PBYTE) pffb;
    PCNRITEM pci;

    // Allocate memory for cFiles container records. EXTRA_RECORD_BYTES refers
    // to the number of bytes per record over and above the MINIRECORDCORE
    // structure size that we need per record. Take a look at the PCNRITEM
    // struct in CNRBASE.H to see what kind of data we are storing. The good
    // thing is that the container will allocate this for us during the
    // CM_ALLOCRECORD message. When we do a CM_REMOVERECORD during WM_DESTROY
    // processing, we can free all this container memory in 1 shot. Note that
    // CM_ALLOCRECORD allocates a linked list of records and sets the
    // MINIRECORDCORE.cb size field for each record. It also appears to
    // zero out all allocated memory besides this .cb field. It knows to
    // allocate enough memory for MINIRECORDCORE rather than RECORDCORE structs
    // due to using CCS_MINIRECORDCORE on the WinCreateWindow of the container.

    pci = WinSendMsg( hwndCnr, CM_ALLOCRECORD, MPFROMLONG( EXTRA_RECORD_BYTES ),
                      MPFROMLONG( cFiles ) );

    if( pci )
    {
        INT           i;
        PFILEFINDBUF3 pffbFile;
        RECORDINSERT  ri;
        PCNRITEM      pciFirst = pci;
        ULONG         cFilesInserted = cFiles;

        // Insert all files into the container in one shot by filling in each
        // linked list node that the container allocated for us.

        for( i = 0; i < cFiles; i++ )
        {
            // Get next FILEFINDBUF3 structure that points to a found file.

            pffbFile = (PFILEFINDBUF3) pbBuf;

            // Fill in the container record with the file info.

            if( FillInRecord( pci, szDirectory, pffbFile ) )

                // Get the next container record in the linked list that the
                // container allocated for us.

                pci = (PCNRITEM) pci->rc.preccNextRecord;
            else
                cFilesInserted--;

            // Point to the next file in the buffer. This is done by adding
            // an offset value to the current location in the buffer. Since
            // the file name is variable length, this offset points to the
            // end of the current file name and the beginning of the next
            // one.

            pbBuf += pffbFile->oNextEntryOffset;
        }

        // Use the RECORDINSERT structure to tell the container how to
        // insert this batch of records. Here we ask to insert the
        // records at the end of the linked list. The parent record indicates
        // who to stick this batch of records under (if pciParent is NULL, the
        // records are at the top level). (Child records are only displayed in
        // Tree view). The zOrder is used for icon view only and specifies the
        // ZORDER that places one record on top of another. In this case we are
        // placing this batch of records at the top of the ZORDER. Also since
        // fInvalidateRecord is TRUE, we will cause the records to be painted
        // as they are inserted. In a container with a small amount of records
        // you probably want to set this to FALSE and do a CM_INVALIDATERECORD
        // (using 0 for cNumRecord) after all records have been inserted. But
        // here the user needs visual feedback if a large amount of
        // subdirectories are found.

        (void) memset( &ri, 0, sizeof( RECORDINSERT ) );

        ri.cb                 = sizeof( RECORDINSERT );
        ri.pRecordOrder       = (PRECORDCORE) CMA_END;
        ri.pRecordParent      = (PRECORDCORE) pciParent;
        ri.zOrder             = (USHORT) CMA_TOP;
        ri.cRecordsInsert     = cFilesInserted;
        ri.fInvalidateRecord  = TRUE;

        if( !WinSendMsg( hwndCnr, CM_INSERTRECORD, MPFROMP( pciFirst ),
                         MPFROMP( &ri ) ) )
        {
            fSuccess = FALSE;

            Msg( "InsertRecords CM_INSERTRECORD RC(%X)", HWNDERR( hwndCnr ) );
        }
    }
    else
    {
        fSuccess = FALSE;

        Msg( "InsertRecords CM_ALLOCRECORD RC(%X)", HWNDERR( hwndCnr ) );
    }

    return fSuccess;
}

/**********************************************************************/
/*-------------------------- FillInRecord ----------------------------*/
/*                                                                    */
/*  POPULATE CONTAINER RECORD WITH FILE INFORMATION                   */
/*                                                                    */
/*  INPUT: pointer to record buffer to fill,                          */
/*         directory path of file,                                    */
/*         pointer to FILEFINDBUF3 that describes the file            */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL FillInRecord( PCNRITEM pci, PSZ szDirectory, PFILEFINDBUF3 pffb )
{
    BOOL     fSuccess = TRUE;
    CHAR     szFullFileName[ CCHMAXPATH + 1 ];
    HPOINTER hptr;

    // Copy the file name into the storage allocated by the container.

    (void) memset( pci->szFileName, 0, sizeof( pci->szFileName ) );
    (void) memcpy( pci->szFileName, pffb->achName, pffb->cchName );

    // Get the fully qualified path for this file

    (void) memset( szFullFileName, 0, sizeof( szFullFileName ) );

    (void) strcpy( szFullFileName, szDirectory );

    szFullFileName[ strlen( szFullFileName ) ] = '\\';

    (void) strcat( szFullFileName, pci->szFileName );

    // Let PM get the icon for us by using WinLoadFileIcon. Note that a
    // WinFreeFileIcon for this icon is not necessary because we are getting
    // a shared copy by using FALSE as the last parameter. If you do a
    // WinFreeFileIcon on this hptr you will get a PMERR_INVALID_PROCESS_ID.

    hptr = WinLoadFileIcon( szFullFileName, FALSE );

    // When debug info is enabled, the file that debug info gets written to
    // is open and has 0 bytes. I think this is why WinLoadFileIcon fails for
    // this file. In any case, we don't want an error message to come up for
    // that file since it only happens when we specifically enable debug info.

    if( !hptr && stricmp( pci->szFileName, DEBUG_FILENAME ) != 0 )
        Msg( "WinLoadFileIcon failed for %s. No icon will be displayed for "
             "this file.", pci->szFileName );

    // Set up the file name pointer to point to the file name. This is
    // crucial because we instructed the container in the
    // SetContainerColumns function (CREATE.C) to use pszFileName as a pointer
    // to the file name (for reasons explained in that function).

    pci->pszFileName    = pci->szFileName;

    // Fill in all fields of the container record.

    pci->hptrIcon       = hptr;
    pci->date.day       = pffb->fdateLastWrite.day;
    pci->date.month     = pffb->fdateLastWrite.month;
    pci->date.year      = pffb->fdateLastWrite.year;
    pci->time.seconds   = pffb->ftimeLastWrite.twosecs;
    pci->time.minutes   = pffb->ftimeLastWrite.minutes;
    pci->time.hours     = pffb->ftimeLastWrite.hours;
    pci->cbFile         = pffb->cbFile;
    pci->attrFile       = pffb->attrFile;

    // Fill in all fields of the MINIRECORDCORE structure. Note that the .cb
    // field of the MINIRECORDCORE struct was filled in by CM_ALLOCRECORD.

    pci->rc.pszIcon     = pci->pszFileName;
    pci->rc.hptrIcon    = hptr;

    return fSuccess;
}

/*************************************************************************
 *                     E N D     O F     S O U R C E                     *
 *************************************************************************/
