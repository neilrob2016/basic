   10 DIM pid = fork()
   20 ' 
   30 CHOOSE pid
   40     CASE -1: PRINT "ERROR": STOP 
   50     ' 
   60     CASE 0
   70     PRINT "Child pid = ",pid,", sleeping..."
   80     SLEEP 2
   90     EXIT 123
  100     ' 
  110     DEFAULT 
  120     PRINT "Parent waiting on child pid ",pid
  130     PRINT waitpid$(pid): ' Could use waitpid$(-1) to wait for any child 
  140 CHOSEN 
