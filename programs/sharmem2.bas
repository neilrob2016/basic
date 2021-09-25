   10 ' Child process gets data from user via parent and shared memory 
   20 DIM a@(10)
   30 ' 
   40 CHOOSE fork()
   50     CASE -1: STOP 
   60     ' 
   70     CASE 0
   80     WHILE 1
   90         PRINT "Child has: ",a
  100         SLEEP 1
  110     WEND 
  120     ' 
  130     DEFAULT 
  140     WHILE 1
  150         PRINT ">";: INPUT a
  160     WEND 
  170 CHOSEN 
