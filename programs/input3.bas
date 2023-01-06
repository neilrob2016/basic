   10 ' How to deal with interrupts with CINPUT
   20 WHILE $true
   30     secs = 1
   40     REPEAT 
   50         start = time()
   60         CINPUT c,secs
   70         secs = secs - (time() - start)
   80     UNTIL NOT $interrupted OR secs <= 0
   90     IF c THEN 
  100         PRINT "Key pressed: ",c
  110     ELSE 
  120         PRINT "No input"
  130     FI 
  140 WEND 
