   10 DIM i
   20 DIM str = "1234567890"
   30 FOR i = 1 TO 10
   40     PRINT "LEFT : ",left$(str,i)
   50 NEXT 
   60 FOR i = 1 TO 10
   70     PRINT "RIGHT: ",right$(str,i)
   80 NEXT 
