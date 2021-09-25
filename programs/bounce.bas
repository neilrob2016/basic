   10 ' Bouncing ball 
   20 ' 
   30 DIM radius = 5
   40 DIM bx = $term_cols / 2
   45 DIM by = $term_rows / 2
   50 DIM xadd = 1,yadd = 1
   60 ' 
   70 WHILE 1
   80     CLS 
   90     PRINT "X = ",bx,", y = ",by
  100     CIRCLE bx,by,radius,1,"X"
  110     bx = bx + xadd
  120     by = by + yadd
  130     IF bx <= radius OR bx >= $term_cols - radius THEN 
  140         xadd = -xadd
  150     FI 
  160     IF by <= radius OR by >= $term_rows - radius THEN 
  170         yadd = -yadd
  180     FI 
  190     SLEEP 0.1
  200 WEND 
