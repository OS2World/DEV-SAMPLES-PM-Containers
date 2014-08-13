// cnrdtl.h -- Definitions for cnrdtl demo

//--------------------------------------------------------------
//  Change the following as required for target system
//--------------------------------------------------------------
#define OS220                           // Define target system
//#define OS213                         // OS/2 1.3 + CUA Lib/2

#ifdef OS220
    #define MSGID   ULONG               // OS/2 2.0
#else
    #define MSGID   USHORT              // OS/2 1.3
    #include <fclcnrp.h>                // CUA Library/2
    #define WC_CONTAINER CCL_CONTAINER  // Window class
#endif                               

//  Defines for dialogs, controls
#define IDLG_EXAMPLE        100
#define IDCN_STATEINFO      200
#define IDS_HEAD_STATE      500
#define IDS_HEAD_CAP        501
#define IDS_HEAD_POP        502
#define IDS_HEAD_ADM        503
#define IDS_TITLE           504
