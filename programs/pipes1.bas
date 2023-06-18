   10 ' Demonstrates pipe IPC   
   20 DIM a
   30 DIM p(2)
   40 DIM ret = pipe(p)
   50 IF NOT ret THEN 
   60     PRINT "ERROR: pipe(): ",syserror$($syserror)
   70 FI 
   80 ' 
   90 PRINT "p(1) = ",p(1)
  100 PRINT "p(2) = ",p(2)
  110 DIM pid = fork()
  120 CHOOSE pid
  130     CASE -1: PRINT "ERROR": STOP 
  140     ' 
  150     CASE 0
  160     CLOSE p(1)
  170     PRINT "Child pid = ",$pid
  180     SLEEP 2
  190     PRINT "Child writing 'hello' to parent.."
  200     PRINT #p(2),"hello"
  210     PRINT "Child waiting for parent data..."
  220     WHILE NOT canread(p(2))
  230         PRINT "No data from parent..."
  240         SLEEP 0.5
  250     WEND 
  260     INPUT #p(2),a
  270     PRINT "Child got: ",a
  280     STOP 
  290     ' 
  300     DEFAULT 
  310     CLOSE p(2)
  320     PRINT "Parent waiting for child data..."
  330     WHILE NOT canread(p(1))
  340         PRINT "No data from child..."
  350         SLEEP 0.5
  360     WEND 
  370     INPUT #p(1),a
  380     PRINT "Parent got: ",a
  390     SLEEP 2
  400     PRINT "Parent writing 'wibble' to child..."
  410     PRINT #p(1),"wibble"
  420 CHOSEN 
