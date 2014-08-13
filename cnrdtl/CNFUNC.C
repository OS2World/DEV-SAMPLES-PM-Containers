// cnfunc.c -- Container utility functions

//--------------------------------------------------------------
//
//  cnfunc.c
//
//      Container utility functions
//
//--------------------------------------------------------------

#define INCL_WIN
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include "cnfunc.h"


#define MAXMSGSIZE  255                 // Max size of string


//--------------------------------------------------------------
//  UtilLoadString - load a string from the resource file
//--------------------------------------------------------------

PSZ UtilLoadString (            // Load string from res
USHORT  usStringID,             // I - String identifier
HAB     hab,                    // I - Current HAB
HMODULE hmod)                   // I - Resource module or NULL
{
    PSZ     pszBuf;             // String buffer

    pszBuf = malloc (MAXMSGSIZE);
    if (WinLoadString (hab, hmod, usStringID, MAXMSGSIZE, 
                       pszBuf) == 0)
    {
        free (pszBuf);
        return (NULL);
    }

    return (pszBuf);            // Return -> string
}


//--------------------------------------------------------------
//
//  CnCreateDetailsView
//
//      Create the details view of the container.  This requires
//      setting up the column information.
//
//--------------------------------------------------------------

USHORT CnCreateDetailsView (            // Create details view
HWND    hwndContainer,                  // I - container window
USHORT  cColumns,                       // I - Number of colums
COLDESC acd[],                          // IO->column descriptor
SHORT   sLastLeftColumn,                // I - Last column in 
                                        // left split window
                                        //     -1 means no split
LONG    lPctSplitBarPos,                // Percent of cntr width 
                                        // to set split bar
USHORT  idTitle,                        // I - container hdg id
HMODULE hmod)                           // I -hmod for resources
{
    USHORT      i;                      // Loop counter

    CNRINFO     cnrinfo;                // Cntr info structure
    PFIELDINFO  pFieldInfoHead;         // --> First field
    PFIELDINFO  pFieldInfo;             // --> Current field
    FIELDINFOINSERT FieldInsertInfo;

    PCOLDESC    pcd;                    // Column descriptor

    //----------------------------------------------------------
    //  Get container text information
    //----------------------------------------------------------

    for (pcd = acd, i = 0; i < cColumns; i++, pcd++)
    {
        if (pcd->pszTitle == NULL)
            pcd->pszTitle = UtilLoadString (pcd->idTitle,
                            WinQueryAnchorBlock (hwndContainer),
                                            hmod);
    }


    //----------------------------------------------------------
    //  Initialize the container for a simple details view
    //----------------------------------------------------------

    memset (&cnrinfo, 0, sizeof(CNRINFO));
    cnrinfo.cb = sizeof (CNRINFO);
    
    // Set for Details View, with column titles, with icons and
    // not bitmaps, include a container title, with a separator
    // bar underneath it, and don't let the user change title
    cnrinfo.flWindowAttr = CV_DETAIL            |
                           CA_DETAILSVIEWTITLES |
                           CA_CONTAINERTITLE    |
                           CA_TITLESEPARATOR;
                           
    cnrinfo.pszCnrTitle = UtilLoadString (idTitle,
                            WinQueryAnchorBlock (hwndContainer),
                            hmod);

    WinSendMsg (hwndContainer, CM_SETCNRINFO,
                MPFROMP (&cnrinfo),
                MPFROMLONG (CMA_FLWINDOWATTR | CMA_CNRTITLE));
    

    //----------------------------------------------------------
    //  Get memory for the column information
    //----------------------------------------------------------

    pFieldInfoHead = (PFIELDINFO) PVOIDFROMMR (
                            WinSendMsg (hwndContainer,
                                        CM_ALLOCDETAILFIELDINFO,
                                        MPFROMSHORT (cColumns),
                                        0));

    pFieldInfo = pFieldInfoHead;        // Start at top of list


    //----------------------------------------------------------
    //  Load the field info structures
    //----------------------------------------------------------

    for (pcd = acd, i = 0; i < cColumns; i++, pcd++)
    {
        pFieldInfo->cb          = sizeof (FIELDINFO);
        pFieldInfo->flData      = pcd->flAttributes;
        pFieldInfo->flTitle     = pcd->flTitle; 
        pFieldInfo->pTitleData  = pcd->pszTitle;
        pFieldInfo->offStruct   = pcd->offField;
        pFieldInfo->pUserData   = NULL;
        pFieldInfo->cxWidth     = pcd->cxWidth;

        if (sLastLeftColumn >= 0)
        {
            if (sLastLeftColumn == i)
            {
                SWP     swp;
                WinQueryWindowPos (hwndContainer, &swp);
                cnrinfo.pFieldInfoLast = pFieldInfo;
                cnrinfo.xVertSplitbar  = 
                               (swp.cx * lPctSplitBarPos) / 100;
                WinSendMsg (hwndContainer, CM_SETCNRINFO,
                            MPFROMP (&cnrinfo),
                            MPFROMLONG (CMA_PFIELDINFOLAST | 
                                        CMA_XVERTSPLITBAR));
            }
        }
        pFieldInfo = pFieldInfo->pNextFieldInfo;
    };


    //----------------------------------------------------------
    //  Construct the FIELDINFOINSERT structure that describes
    //  the number of fields to be inserted, where they are to 
    //  be inserted, etc.
    //----------------------------------------------------------

    memset (&FieldInsertInfo, 0, sizeof(FIELDINFOINSERT));
    FieldInsertInfo.cb               = sizeof (FIELDINFOINSERT);
    FieldInsertInfo.pFieldInfoOrder  = (PFIELDINFO) CMA_END;
    FieldInsertInfo.cFieldInfoInsert = cColumns;
    FieldInsertInfo.fInvalidateFieldInfo = TRUE;


    //----------------------------------------------------------
    //  Insert the fields.
    //----------------------------------------------------------

    WinSendMsg (hwndContainer, CM_INSERTDETAILFIELDINFO,
                MPFROMP (pFieldInfoHead),
                MPFROMP (&FieldInsertInfo));
    return 0;

}



//--------------------------------------------------------------
//
//  CnDestroyDetailsView
//
//      Destroy the details view of the container.  This
//      requires freeing column information.
//
//--------------------------------------------------------------

USHORT CnDestroyDetailsView (   // Destroy details view
HWND    hwndContainer,          // I - Handle to cntr window
USHORT  cColumns,               // I - Number of colums
COLDESC acd[])                  // IO--> column descriptor
{
    USHORT      i;              // Loop counter
    PCOLDESC    pcd;            // Column descriptor entry
    CNRINFO     cnrinfo;        // Container info structure


    //----------------------------------------------------------
    //  Remove any column titles loaded into memory
    //----------------------------------------------------------
    
    for (pcd = acd, i = 0; i < cColumns; i++, pcd++)
    {
        if (    (pcd->pszTitle != NULL)
             && (pcd->idTitle != 0) )
        {
            free (pcd->pszTitle);
            pcd->pszTitle = NULL;
        }
    }

    
    //----------------------------------------------------------
    //  Remove the column heading from memory
    //----------------------------------------------------------

    WinSendMsg (hwndContainer,
                CM_QUERYCNRINFO,
                MPFROMP(&cnrinfo),
                MPFROMSHORT (sizeof(CNRINFO)));
                
    free (cnrinfo.pszCnrTitle);
    
                
    //----------------------------------------------------------
    //  Remove the column information of the container
    //----------------------------------------------------------

    WinSendMsg (hwndContainer,
                CM_REMOVEDETAILFIELDINFO,
                0,
                MPFROM2SHORT (0, CMA_FREE));


    //----------------------------------------------------------
    //  Remove any records in the container
    //----------------------------------------------------------

    WinSendMsg (hwndContainer,
                CM_REMOVERECORD,
                0,
                MPFROM2SHORT (0, CMA_FREE));

    return 0;
}
