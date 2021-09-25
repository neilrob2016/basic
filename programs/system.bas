   10 DIM com
   20 WHILE 1
   30     PRINT "Enter command> ";: INPUT com
   40     IF NOT system(com) THEN 
   50         IF $syserror THEN 
   60             PRINT "ERROR: system() failed: ",syserror$($syserror)
   70         ELSE 
   80             PRINT "ERROR: Shell error"
   90         FI 
  100     FI 
  110 WEND 
