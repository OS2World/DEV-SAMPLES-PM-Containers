// cnfunc.h -- Container utility functions

//--------------------------------------------------------------
//
//  cnfunc.h
//
//      Container Functions
//
//--------------------------------------------------------------

//--------------------------------------------------------------
//  Data structure used to describe field information
//--------------------------------------------------------------

typedef struct                  // Field (column) descriptors
{
    ULONG   offField;           // Offset of field in record
    ULONG   flAttributes;       // Field attributes
    USHORT  idTitle;            // Identifier of column title
    ULONG   flTitle;            // Title Attributes
    USHORT  cxWidth;            // Column width (0 = auto calc)
    PSZ     pszTitle;           // Pointer to column title text
} COLDESC, *PCOLDESC;


//--------------------------------------------------------------
//  Function prototypes
//--------------------------------------------------------------

USHORT CnCreateDetailsView (    // Create details view
HWND    hwndContainer,          // I - Handle to container window
USHORT  cColumns,               // I - Number of colums
COLDESC acd[],                  // IO--> column descriptor
SHORT   sLastLeftColumn,        // I - Last column in left split
LONG    lPctSplitBarPos,        // Percent of container width 
USHORT  idTitle,                // I - container heading id
HMODULE hmod);                  // I - handle to resource file

USHORT CnDestroyDetailsView (   // Destroy details view
HWND    hwndContainer,          // I - Handle to container window
USHORT  cColumns,               // I - Number of colums
COLDESC acd[]);                 // IO--> column descriptor
