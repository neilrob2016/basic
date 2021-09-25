   10 ON BREAK GOSUB 140
   20 ON ERROR CONT 
   30 DIM i
   40 FOR i = 1 TO 10
   50     PRINT i
   60     IF i = 5 THEN 
   70         PRINT "Disabling ON BREAK"
   80         ON BREAK BREAK 
   90     FI 
  100     SLEEP 1
  110 NEXT 
  120 PRINT "DONE": STOP 
  130 ' 
  140 PRINT "Break at line ",$break_line
  150 RETURN 
