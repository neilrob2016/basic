   10 ' Open a file and read it iline by iline   
   20 ON ERROR GOTO 270
   30 DIM file,fd
   40 DIM cnt,iline
   50 PRINT "Enter file> ";: INPUT file
   60 fd = open(file,"r")
   70 IF NOT fd THEN 
   80     IF $syserror THEN 
   90         PRINT "Can't open file: ",syserror$($syserror)
  100     ELSE 
  110         PRINT "Can't resolve path"
  120     FI 
  130     GOTO 50
  140 FI 
  150 ' 
  160 INPUT #fd,iline
  170 WHILE NOT $eof
  180     PRINT cnt,": ",iline
  190     cnt = cnt + 1
  200     INPUT #fd,iline
  210 WEND 
  220 ' 
  230 CLOSE fd
  240 PRINT "--------------"
  250 GOTO 50
  260 ' 
  270 PRINT "Error: ",error$($error),": ",syserror$($syserror)
  280 CLOSE fd
