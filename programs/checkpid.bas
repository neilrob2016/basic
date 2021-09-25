   10 DIM checkstr
   20 DIM pid = fork()
   30 ' 
   40 CHOOSE pid
   50     CASE -1: PRINT "ERROR": STOP 
   60     ' 
   70     CASE 0
   80     PRINT "Child pid = ",pid,", sleeping..."
   90     SLEEP 4
  100     EXIT 123
  110     ' 
  120     DEFAULT 
  130     PRINT "Parent waiting on child pid ",pid
  140     WHILE 1
  150         checkstr = element$(checkpid(pid),2)
  160         IF checkstr = "ERROR" THEN 
  170             PRINT "ERROR: ",syserror$($syserror)
  180             STOP 
  190         FI 
  200         IF checkstr = "NOCHILD" THEN STOP FI 
  210         IF checkstr = "NOEXIT" THEN 
  220             PRINT "CHILD ALIVE"
  230         ELSE 
  240             PRINT checkstr
  250             STOP 
  260         FI 
  270         SLEEP 1
  280     WEND 
  290 CHOSEN 
