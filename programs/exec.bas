   10 ' Example of using exec() function
   20 DIM cline
   30 DIM p(2)
   40 DIM ret = pipe(p)
   50 IF NOT ret THEN 
   60     PRINT "ERROR: pipe(): ",syserror$($syserror)
   70 FI 
   80 ' 
   90 PRINT "p(1) = ",p(1)
  100 PRINT "p(2) = ",p(2)
  110 DIM pid = fork()
  120 ' 
  130 CHOOSE pid
  140     CASE -1: PRINT "ERROR: fork(): ",syserror$($syserror): STOP 
  150     ' 
  160     CASE 0
  170     PRINT "Child pid = ",$pid
  180     EVAL exec("ls -l",p)
  190     STOP 
  200     ' 
  210     DEFAULT 
  220     DIM r(1),w(1)
  230     PRINT "Parent waiting for input..."
  240     cline = ""
  250     w(1) = -1
  260     ' 
  270     WHILE 1
  280         r(1) = p(2)
  290         ' Need a timeout because we don't get notification of a process
  300         ' exit with a socketpair for some reason.
  310         ret = select(r,w,0.5)
  320         IF ret = -1 THEN 
  330             PRINT "ERROR in parent select(): ",syserror$($syserror)
  340             STOP 
  350         FI 
  360         IF NOT ret THEN 
  370             IF element$(checkpid$(pid),2) = "EXIT" THEN 
  380                 PRINT "*** DONE ***"
  390                 STOP 
  400             ELSE 
  410                 CONTLOOP 
  420         FIALL 
  430         INPUT #p(2),iline
  440         PRINT "LINE: ",iline
  450     WEND 
  460 CHOSEN 
