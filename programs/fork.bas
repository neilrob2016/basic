   10 PRINT "Starting. Parent pid = ",$pid," ppid = ",$ppid
   20 FOR i = 1 TO 4
   30     CHOOSE fork()
   40         CASE -1
   50         PRINT "fork() failed": STOP 
   60         CASE 0
   70         PRINT "Child process. Pid = ",$pid,", ppid = ",$ppid: SLEEP 2
   80         DEFAULT 
   90         PRINT "Parent process. Pid still = ",$pid
  100     CHOSEN 
  110 NEXT 
  120 PRINT "Parent pid ",$pid," finished"
