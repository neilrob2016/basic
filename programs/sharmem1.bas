   10 ' Demonstrates using a shared memory variable 
   20 DIM a@(10)
   30 ' 
   40 CHOOSE fork()
   50     CASE -1: STOP 
   60     ' 
   70     CASE 0
   80     PRINT "Child sleeping..."
   90     SLEEP 1
  100     PRINT "Child read: ",a
  110     a = "got it"
  120     STOP 
  130     ' 
  140     DEFAULT 
  150     PRINT "Parent writing..."
  160     a = "hello child"
  170     SLEEP 3
  180     PRINT "Parent got: ",a
  190 CHOSEN 
