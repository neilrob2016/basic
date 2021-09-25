   10 DIM i
   20 PRINT "Number of enviroment variables = ",arrsize($env)
   30 FOR i = 1 TO arrsize($env)
   40     PRINT "   ",i,": ",$env(i)
   50 NEXT 
