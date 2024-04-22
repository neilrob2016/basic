   10 ' Child process gets data from user via parent and shared memory 
   20 DIM a@(10)
   30 PRINT "Type something..."
   40 ' 
   50 CHOOSE fork()
   60     CASE -1: STOP 
   70     ' 
   80     CASE 0
   90     WHILE 1
  100         PRINT "Child has: ",a
  110         SLEEP 1
  120     WEND 
  130     ' 
  140     DEFAULT 
  150     WHILE 1
  160         PRINT ">";: INPUT a
  170     WEND 
  180 CHOSEN 
