# IBM Developer's Workframe/2 Make File Creation run at 23:50:57 on 09/17/92

# Make File Creation run in directory:
#   D:\P\MAGAZINE\CNRDTL;

.SUFFIXES:

.SUFFIXES: .c .rc

ALL: CNRDTL.EXE \
     CNRDTL.RES

cnrdtl.exe:  \
  CNFUNC.OBJ \
  CNRDTL.OBJ \
  CNRDTL.RES \
  CNRDTL.MAK
   @REM @<<CNRDTL.@0
     /CO /M /NOL /PM:PM +
     CNFUNC.OBJ +
     CNRDTL.OBJ
     cnrdtl.exe
     
     
     ;
<<
   LINK386.EXE @CNRDTL.@0
   RC CNRDTL.RES cnrdtl.exe

{.}.rc.res:
   RC -r .\$*.RC

{.}.c.obj:
   ICC.EXE /Ss /Kbger /Ti /W2 /C .\$*.c

!include CNRDTL.DEP
