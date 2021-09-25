   10 ' Draw an expanding rectangle  
   20 DIM x,y,w,h
   30 y = 5
   40 w = 1
   50 h = 1
   60 ' 
   70 FOR x = 30 TO 10 STEP -0.5
   80     ATTR 0: CLS : PAPER 5
   90     RECT x,y,w,h,1,"."
  100     RECT x,y,w,h,0,"X"
  110     y = y - 0.5
  120     w = w + 1
  130     h = h + 1
  140     SLEEP 0.1
  150 NEXT 
