   10 ' Ignore breaks (control-C). The only downside is that during a sleep 
   20 ' it'll exit the sleep with an error due to unix's signal mechanism   
   30 ON BREAK CONT 
   40 ON ERROR GOSUB 170
   50 DIM i
   60 FOR i = 1 TO 10
   70     PRINT i
   80     IF i = 5 THEN 
   90         PRINT "Disabling ON BREAK"
  100         ON BREAK BREAK 
  110     FI 
  120     SLEEP 1
  130     PRINT "Interrupted = ",$interrupted
  140 NEXT 
  150 PRINT "DONE": STOP 
  160 ' 
  170 IF $error = 60 THEN 
  180     PRINT "Sleep interrupted!"
  190     RETURN 
  200 FI 
