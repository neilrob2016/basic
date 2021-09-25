   10 DIM iline
   20 PRINT "Enter command>";
   30 INPUT cmd
   40 fd = popen(cmd,"r")
   50 IF NOT fd THEN 
   60     PRINT "Can't start process: ",syserror$($syserror)
   70     GOTO 20
   80 FI 
   90 cnt = 1
  100 ' 
  110 INPUT #fd,iline
  120 WHILE NOT $eof
  130     PRINT cnt,": ",iline
  140     cnt = cnt + 1
  150     INPUT #fd,iline
  160 WEND 
  170 CLOSE fd
  180 PRINT "*** END ***"
  190 GOTO 20
