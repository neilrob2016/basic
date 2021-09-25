   10 DIM i
   20 FOR i=1 TO 10
   30     IF i=5 THEN 
   40         PRINT "FIVE!"
   50     ELSE 
   60         PRINT "NOT FIVE!"
   70         IF i=6 THEN 
   80             PRINT "SIX!"
   90         ELSE 
  100             PRINT "NOT SIX!"
  110         FI 
  120         PRINT "STILL NOT FIVE!"
  130     FI 
  140     PRINT i
  150 NEXT 
  160 PRINT "DONE"
