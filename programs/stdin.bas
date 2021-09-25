   10 DIM a
   20 WHILE 1
   30     IF canread(0) THEN 
   40         CINPUT a
   50         PRINT "Got: ",a
   60     ELSE 
   70         PRINT "Nothing on STDIN"
   80     FI 
   90     SLEEP 0.5
  100 WEND 
