   10 ' Demonstrates pipe IPC using the select() function    
   20 DIM c,cline
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
  160     ' Child reads from its pipe    
  170     ' 
  180     CASE 0
  190     CLOSE p(1)
  200     DIM r(1),w(1)
  210     w(1) = -1: ' Don't care about writing     
  220     PRINT "Child pid = ",$pid
  230     PRINT "Child waiting for input..."
  240     ' 
  250     WHILE 1
  260         r(1) = p(2)
  270         ' Send a ping every 2 seconds    
  280         CHOOSE select(r,w,2)
  290             CASE -1
  300             PRINT "ERROR in child select(): ",syserror$($syserror)
  310             STOP 
  320             ' 
  330             CASE 0
  340             PRINT "Child TX: PING!"
  350             PRINT #p(2),"PING!"
  360             BREAK 
  370             ' 
  380             DEFAULT 
  390             INPUT #p(2),cline
  400             PRINT "Child RX: ",cline
  410             str = cline + " back to you!"
  420             PRINT "Child TX: " + str
  430             PRINT #p(2),str
  440         CHOSEN 
  450     WEND 
  460     ' 
  470     ' Parent reads from STDIN and pipe    
  480     ' 
  490     DEFAULT 
  500     CLOSE p(2)
  510     DIM r(2),w(1)
  520     PRINT "Parent waiting for input..."
  530     cline = ""
  540     w(1) = -1
  550     ' 
  560     WHILE 1
  570         r(1) = 0
  580         r(2) = p(1)
  590         ' -1 in select() means no timeout    
  600         IF select(r,w,-1) = -1 THEN 
  610             PRINT "ERROR in parent select(): ",syserror$($syserror)
  620             STOP 
  630         FI 
  640         IF r(1) THEN 
  650             ' Read from stdin. Use CINPUT because INPUT would block     
  660             ' until it read a newline    
  670             CINPUT c,0
  680             PRINT c;: ' Echo character    
  690             IF asc(c) = 10 THEN 
  700                 PRINT "Parent TX: ",cline
  710                 PRINT #p(1),cline
  720                 cline = ""
  730             ELSE 
  740                 cline = cline + c
  750             FI 
  760         FI 
  770         IF r(2) THEN 
  780             ' Read from pipe    
  790             INPUT #p(1),c
  800             PRINT "Parent RX: ",c
  810         FI 
  820     WEND 
  830 CHOSEN 
