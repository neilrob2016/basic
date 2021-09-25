   10 ' Ignore breaks (control-C). The only downside is that during a sleep 
   20 ' it'll exit the sleep with an error due to unix's signal mechanism   
   30 ON BREAK CONT 
   40 ON ERROR GOSUB 160
   50 DIM i
   60 FOR i = 1 TO 10
   70     PRINT i
   80     IF i = 5 THEN 
   90         PRINT "Disabling ON BREAK"
  100         ON BREAK BREAK 
  110     FI 
  120     SLEEP 1
  130 NEXT 
  140 PRINT "DONE": STOP 
  150 ' 
  160 IF $error = 60 THEN 
  170     PRINT "Sleep interrupted!"
  180     RETURN 
  190 FI 
