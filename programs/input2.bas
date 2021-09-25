   10 DIM a,cnt
   20 WHILE 1
   30     CINPUT a,0
   40     IF a <> "" THEN 
   50         PRINT "Got ",asc(a),": ",a
   60         cnt = 0
   70     FI 
   80     cnt = cnt + 1
   90     IF cnt = 100000 THEN 
  100         PRINT "Waiting"
  110         cnt = 0
  120     FI 
  130 WEND 
