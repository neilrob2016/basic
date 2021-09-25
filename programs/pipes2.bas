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
  340             PRINT #p(2),"PING!"
  350             BREAK 
  360             ' 
  370             DEFAULT 
  380             INPUT #p(2),cline
  390             PRINT "Child received: ",cline
  400             PRINT #p(2),"Child got " + cline
  410         CHOSEN 
  420     WEND 
  430     ' 
  440     ' Parent reads from STDIN and pipe    
  450     ' 
  460     DEFAULT 
  470     CLOSE p(2)
  480     DIM r(2),w(1)
  490     PRINT "Parent waiting for input..."
  500     cline = ""
  510     w(1) = -1
  520     ' 
  530     WHILE 1
  540         r(1) = 0
  550         r(2) = p(1)
  560         ' -1 in select() means no timeout    
  570         IF select(r,w,-1) = -1 THEN 
  580             PRINT "ERROR in parent select(): ",syserror$($syserror)
  590             STOP 
  600         FI 
  610         IF r(1) THEN 
  620             ' Read from stdin. Use CINPUT because INPUT would block     
  630             ' until it read a newline    
  640             CINPUT c,0
  650             PRINT c;: ' Echo character    
  660             IF asc(c) = 10 THEN 
  670                 PRINT #p(1),cline
  680                 cline = ""
  690             ELSE 
  700                 cline = cline + c
  710             FI 
  720         FI 
  730         IF r(2) THEN 
  740             ' Read from pipe    
  750             INPUT #p(1),c
  760             PRINT "Parent received: ",c
  770         FI 
  780     WEND 
  790 CHOSEN 
