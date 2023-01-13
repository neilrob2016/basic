   10 ' This shows how to efficiently send plain text to a printer and also
   20 ' demonstrates using popen() in write mode. You can simply use the
   30 ' "print #@,<data>" construct to write to the printer but this is very
   40 ' inefficient as it creates a new print job for each print statement.
   50 ' 
   60 output = ""
   70 PRINT "Enter text to send to printer. Finish with a '.' on its own."
   80 PRINT ">>>";: INPUT iline
   90 IF iline = "." THEN GOTO 150 FI 
  100 output = output + iline + chr$(10)
  110 GOTO 80
  120 ' 
  130 ' Send to printer. Can't just open /dev/lp0 anymore, have to use the
  140 ' lp system process.
  150 fd = popen("lp","w")
  160 IF fd = 0 THEN 
  170     PRINT "Can't start lp process: ",syserror$($syserror)
  180     STOP 
  190 FI 
  200 PRINT #fd,output
  210 CLOSE fd
  220 PRINT "*** SENT ***"
  230 GOTO 60
