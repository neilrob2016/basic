   10 ' This shows how to send plain text to a printer and also demonstrates
   20 ' using popen() in write mode.
   30 ' 
   40 output = ""
   50 PRINT "Enter text to send to printer. Finish with a '.' on its own."
   60 PRINT ">>>";: INPUT iline
   70 IF iline = "." THEN GOTO 130 FI 
   80 output = output + iline + chr$(10)
   90 GOTO 60
  100 ' 
  110 ' Send to printer. Can't just open /dev/lp0 anymore, have to use the
  120 ' lp system process.
  130 fd = popen("lp","w")
  140 IF fd = 0 THEN 
  150     PRINT "Can't start lp process: ",syserror$($syserror)
  160     STOP 
  170 FI 
  180 PRINT #fd,output
  190 CLOSE fd
  200 PRINT "*** SENT ***"
  210 GOTO 40
