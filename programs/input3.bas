   10 ' How to deal with interrupts with CINPUT
   20 WHILE $true
   30     secs = 1
   40     REPEAT 
   50         start = time()
   60         CINPUT c,secs
   70         inter = $interrupted: ' Save as reset on next function
   80         secs = secs - (time() - start)
   90     UNTIL NOT inter OR secs <= 0
  100     IF c THEN 
  110         PRINT "Key pressed: ",c
  120     ELSE 
  130         PRINT "No input"
  140     FI 
  150 WEND 
