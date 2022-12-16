   10 ' Demonstrates killing a process   
   20 DIM pid = fork()
   30 ' 
   40 CHOOSE pid
   50     CASE -1: PRINT "ERROR": STOP 
   60     ' 
   70     CASE 0
   80     PRINT "Child pid = ",pid
   90     WHILE 1: WEND 
  100     ' 
  110     DEFAULT 
  120     PRINT "Parent sleeping for 2 seconds..."
  130     SLEEP 2
  140     PRINT "Parent killing child..."
  150     EVAL kill(pid,9)
  160     PRINT "Waitpid = ",waitpid$(pid)
  170 CHOSEN 
