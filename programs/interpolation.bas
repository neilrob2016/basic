   10 ' Some simple integer interpolation
   20 tv_lines = 625
   30 comp_lines = 400
   40 cnt = 0
   50 l2 = 0
   60 FOR l1 = 0 TO tv_lines
   70     cnt = cnt + comp_lines
   80     IF cnt >= tv_lines THEN 
   90         cnt = cnt % tv_lines
  100         l2 = l2 + 1
  110     FI 
  120     PRINT "Scan line = ",l1,", Y pixel = ",l2
  130 NEXT 
