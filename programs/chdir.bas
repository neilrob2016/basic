   10 IF NOT chdir("/tmp") THEN 
   20     PRINT "ERROR: chdir(): ",syserror$($syserror)
   30     STOP 
   40 FI 
   50 DIM fd = open("test","w")
   60 IF NOT fd THEN 
   70     PRINT "ERROR: open(): ",syserror$($syserror)
   80     STOP 
   90 FI 
  100 PRINT #fd,"hello world"
  110 CLOSE fd
  120 DIR 
