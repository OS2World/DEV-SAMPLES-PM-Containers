// cnrdtl.c -- Sample program to demonstrate container detail

//--------------------------------------------------------------
//  cnrdtl.c
//
//      Sample program to demonstrate container detail view
//
//  By: Guy Scharf                      (415) 948-9186
//      Software Architects, Inc.       FAX: (415) 948-1620
//      2163 Jardin Drive               
//      Mountain View, CA   94040       
//      CompuServe: 76702,557
//      Internet: 76702.557@compuserve.com
//  (c) Copyright 1992 Software Architects, Inc.  
//
//      All Rights Reserved.
//
//  Software Architects, Inc. develops software products for 
//  independent software vendors using OS/2 and Presentation 
//  Manager.
//--------------------------------------------------------------

#define INCL_PM                         // Basic OS/2 PM
#define INCL_BASE                       // components
#include <OS2.H>

#include <stdlib.h>                     // C runtime library
#include <stddef.h>
#include <string.h>

#include "cnrdtl.h"                     // Container demo defs
#include "cnfunc.h"                     // Container utilities


//  Sample data

typedef struct
{
    SHORT   mm, dd, yy;                 // General date storage
} OURDATE, *POURDATE;

typedef struct
{
    PSZ     pszState;                   // Name of state
    PSZ     pszCapital;                 // Name of city
    ULONG   ulPopulation;               // '80 census population
    OURDATE ourdateAdm;                 // Date admitted
}  STATEREC, *PSTATEREC;

STATEREC statedata[] =
{
    {"Alabama",     "Montgomery",   3893888, {12, 14, 1819}},
    {"Alaska",      "Juneau",        401851, { 1,  3, 1959}},
    {"Arizona",     "Phoenix",      2718425, { 2,  4, 1912}},
    {"Arkansas",    "Little Rock",  2286435, { 6, 15, 1836}},
    {"California",  "Sacramento",  23667565, { 9,  9, 1850}},
    {"Colorado",    "Denver",       2889735, { 8,  1, 1876}},
    {"Connecticut", "Hartford",     3107576, { 1,  9, 1788}},
    {"Delaware",    "Dover",         594317, {12,  7, 1787}},
    {"Florida",     "Tallahassee",  9746342, { 3,  3, 1845}},
    {"Georgia",     "Atlanta",      5463105, { 1,  2, 1788}},
    {"Hawaii",      "Honolulu",      964691, { 8, 21, 1959}}
};

typedef struct                          // Container data record
{
    MINIRECORDCORE  RecordCore;         // MINIRECORDCORE structure
    PSZ         pszCapital;             // Capital city
    ULONG       ulPopulation;           // Population
    CDATE       cdateAdmitted;          // Date admitted
} RECORD, *PRECORD;

COLDESC cdState[] =
{
  {offsetof(RECORD, RecordCore.pszIcon), CFA_STRING | CFA_FIREADONLY |
     CFA_HORZSEPARATOR | CFA_LEFT, IDS_HEAD_STATE, 
     CFA_LEFT, 0, NULL},
  {offsetof(RECORD, pszCapital), CFA_STRING | CFA_FIREADONLY |
     CFA_HORZSEPARATOR | CFA_LEFT, IDS_HEAD_CAP, 
     CFA_LEFT, 0, NULL},
  {offsetof(RECORD, ulPopulation), CFA_ULONG | CFA_FIREADONLY | 
     CFA_HORZSEPARATOR | CFA_RIGHT, IDS_HEAD_POP, 
     CFA_RIGHT, 0, NULL},
  {offsetof(RECORD, cdateAdmitted), CFA_DATE | CFA_FIREADONLY | 
     CFA_HORZSEPARATOR | CFA_RIGHT, IDS_HEAD_ADM, 
     CFA_RIGHT, 0, NULL}
};

//  Prototypes of procedures

static MRESULT EXPENTRY ExampleDlgProc (HWND, MSGID, MPARAM,
                                        MPARAM);
                                        
static MRESULT InitContainer (          // Initialize slider
HWND    hwndDlg,                        // I - Dialog window
USHORT  idContainer,                    // I - Container id
USHORT  cStates,                        // I - Number of states
PSTATEREC pastate);                     // I - State data array

//--------------------------------------------------------------
//                                                                          
//  Main program to drive container example
//                                                                          
//--------------------------------------------------------------

int main (void)
{
    HAB     hab;                        // Handle to anchor blk
    HMQ     hmqMsgQueue;                // Handle to msg queue
#ifndef OS220
    HMODULE hmodContainer;              // Handle to cntr module
#endif

    hab = WinInitialize (0);            // Initialize PM

    hmqMsgQueue = WinCreateMsgQueue (hab, 0);// Create msg queue

#ifndef OS220
    if (DosLoadModule (NULL, 0, CCL_CONTAINER_DLL, 
                       &hmodContainer))
        return FALSE;
#endif

    WinDlgBox (HWND_DESKTOP, HWND_DESKTOP, ExampleDlgProc, 0,
               IDLG_EXAMPLE, NULL);

#ifndef OS220
    DosFreeModule (hmodContainer);
#endif

    WinDestroyMsgQueue (hmqMsgQueue);   // Shutdown
    WinTerminate       (hab);
    return 0;
}

//--------------------------------------------------------------
//                                                                          
//  ExampleDlgProc() -- Show state info
//                                                                          
//--------------------------------------------------------------

static MRESULT EXPENTRY ExampleDlgProc (
HWND    hwndDlg,
MSGID   msg,
MPARAM  mp1,
MPARAM  mp2)
{
    switch(msg)
    {
        //------------------------------------------------------
        //  Initialize dialog by defining the details view
        //  in the container, loading records, etc.
        //------------------------------------------------------
        case WM_INITDLG:

            CnCreateDetailsView (WinWindowFromID (hwndDlg, 
                                                IDCN_STATEINFO),
                              sizeof cdState / sizeof (COLDESC),
                              cdState, 
                              0,
                              33,
                              IDS_TITLE,
                              0);
                                                               
            //--------------------------------------------------
            //  Load the container
            //--------------------------------------------------
            InitContainer (hwndDlg, IDCN_STATEINFO,
                           sizeof statedata / sizeof (STATEREC),
                           statedata);
            return 0;

        //------------------------------------------------------
        //  Process pushbuttons.  They both just quit dialog.
        //------------------------------------------------------
        case WM_COMMAND:
            switch (SHORT1FROMMP(mp1))
            {
                                        // Cancel pressed
                                        // Dismiss dialog
                case DID_CANCEL:
                    WinDismissDlg (hwndDlg, FALSE);
                    return 0;

                                        // OK button pressed
                case DID_OK:            // We're done
                    WinDismissDlg (hwndDlg, TRUE);
                    return 0;
            }
            return 0;
        
        //------------------------------------------------------
        //  Recover memory allocated for the container
        //------------------------------------------------------
        case WM_DESTROY:
            CnDestroyDetailsView (WinWindowFromID (hwndDlg, 
                                                IDCN_STATEINFO),
                              sizeof cdState / sizeof (COLDESC),
                              cdState);
            return 0;
                                   
        //------------------------------------------------------
        //  All other messages go to default window procedure
        //------------------------------------------------------
        default:
            return (WinDefDlgProc (hwndDlg, msg, mp1, mp2));
    }
    return FALSE;
}



//--------------------------------------------------------------
// Function: InitContainer
// Outputs:  none                                               
//                                                              
// This function loads the container with all of the state data.
// See the article for a complete discussion of this function.
//--------------------------------------------------------------

static MRESULT InitContainer (          // Initialize slider
HWND    hwndDlg,                        // I - Dialog window
USHORT  idContainer,                    // I - Container id
USHORT  cStates,                        // I - Number of states
PSTATEREC pastate)                      // I - State data array
{
    USHORT      i;                      // Loop counter
    ULONG       cbExtraBytes;           // Extra bytes in record structure */

    PRECORD     precord;                // -> container records
    PRECORD     precordFirst;           // -> first container record

    RECORDINSERT recordInsert;          // Record insertion control

    //----------------------------------------------------------
    //  Allocate memory for all user records
    //----------------------------------------------------------

    cbExtraBytes = (ULONG)(sizeof(RECORD) - 
                                         sizeof(MINIRECORDCORE));
    precord = (PRECORD) WinSendDlgItemMsg (hwndDlg, idContainer,
                                  CM_ALLOCRECORD,
                                  MPFROMLONG (cbExtraBytes),
                                  MPFROMSHORT (cStates));
    precordFirst = precord;
                                      
    //----------------------------------------------------------
    //  Initialize all records
    //----------------------------------------------------------

    for (i = 0; i < cStates; i++, pastate++)
    {
        //------------------------------------------------------
        //   Initialize the container record control structure
        //------------------------------------------------------
        precord->RecordCore.cb        = sizeof (MINIRECORDCORE);

        //------------------------------------------------------
        //  Copy our data to the container record control struct
        //------------------------------------------------------
        precord->RecordCore.pszIcon  = pastate->pszState;
        precord->pszCapital          = pastate->pszCapital;
        precord->ulPopulation        = pastate->ulPopulation;
        precord->cdateAdmitted.month = pastate->ourdateAdm.mm;
        precord->cdateAdmitted.day   = pastate->ourdateAdm.dd;
        precord->cdateAdmitted.year  = pastate->ourdateAdm.yy;

                                        // Move to next record
        precord = (PRECORD) precord->RecordCore.preccNextRecord;
    }

    //----------------------------------------------------------
    //  Construct record insertion control structure
    //----------------------------------------------------------

    memset (&recordInsert, 0, sizeof (RECORDINSERT));
    recordInsert.cb                 = sizeof (RECORDINSERT);
    recordInsert.pRecordParent      = NULL;
    recordInsert.pRecordOrder       = (PRECORDCORE)((ULONG)
                                          MPFROMSHORT(CMA_END));
    recordInsert.zOrder             = (USHORT) CMA_TOP;
    recordInsert.cRecordsInsert     = cStates;
    recordInsert.fInvalidateRecord  = TRUE;

    //----------------------------------------------------------
    //  Insert the records
    //----------------------------------------------------------

    WinSendDlgItemMsg (hwndDlg, idContainer,
                       CM_INSERTRECORD,
                       MPFROMP (precordFirst),
                       MPFROMP (&recordInsert));

    //----------------------------------------------------------
    //  Return to caller
    //----------------------------------------------------------

    return 0;
}


