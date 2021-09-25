   10 DIM fd = open("/etc/passwd","r")
   20 DIM offset
   30 DIM actual_offset
   40 DIM a
   50 ' 
   60 WHILE 1
   70     PRINT "Enter offset> ";: INPUT offset
   80     IF NOT isnumstr(offset) THEN 
   90         PRINT "Please enter a number"
  100         GOTO 60
  110     FI 
  120     actual_offset = seek(fd,tonum(offset))
  130     IF actual_offset = -1 THEN 
  140         PRINT "ERROR: seek(): ",syserror$($syserror)
  150         STOP 
  160     FI 
  170     PRINT "Offset = ",actual_offset
  180     INPUT #fd,a: PRINT a
  190 WEND 
