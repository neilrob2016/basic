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
  220     INPUT #p(2),a
  230     PRINT "Child got: ",a
  240     STOP 
  250     ' 
  260     DEFAULT 
  270     CLOSE p(2)
  280     PRINT "Parent waiting for child data..."
  290     WHILE NOT canread(p(1))
  300         PRINT "No data from child..."
  310         SLEEP 0.5
  320     WEND 
  330     INPUT #p(1),a
  340     PRINT "Parent got: ",a
  350     SLEEP 2
  360     PRINT "Parent writing 'wibble' to child..."
  370     PRINT #p(1),"wibble"
  380 CHOSEN 
