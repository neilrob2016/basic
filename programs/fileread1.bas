   10 ' Open a file and read it iline by iline   
   20 ON ERROR GOTO 230
   30 DIM file,fd
   40 DIM cnt,iline
   50 PRINT "Enter file> ";: INPUT file
   60 fd = open(file,"r")
   70 IF NOT fd THEN 
   80     PRINT "Can't open file: ",syserror$($syserror)
   90     STOP 
  100 FI 
  110 ' 
  120 INPUT #fd,iline
  130 WHILE NOT $eof
  140     PRINT cnt,": ",iline
  150     cnt = cnt + 1
  160     INPUT #fd,iline
  170 WEND 
  180 ' 
  190 CLOSE fd
  200 PRINT "*** DONE ***"
  210 STOP 
  220 ' 
  230 PRINT "Error: ",error$($error),": ",syserror$($syserror)
  240 CLOSE fd
