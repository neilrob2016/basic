   10 ' Draw a spinning cross 
   20 DIM x1,y1,x2,y2
   30 DIM x3,y3,x4,y4
   40 DIM mid_x,mid_y
   50 DIM LEN = 10
   60 DIM ang
   70 DIM col1,col2
   80 ' 
   90 col2 = 4
  100 WHILE 1
  110     ' Allows resizing of terminal while running    
  120     mid_x = $term_cols / 2
  130     mid_y = $term_rows / 2
  140     ' 
  150     x1 = mid_x + sin(ang) * LEN
  160     y1 = mid_y + cos(ang) * LEN
  170     x2 = mid_x + sin(ang + 180) * LEN
  180     y2 = mid_y + cos(ang + 180) * LEN
  190     x3 = mid_x + sin(ang + 90) * LEN
  200     y3 = mid_y + cos(ang + 90) * LEN
  210     x4 = mid_x + sin(ang + 270) * LEN
  220     y4 = mid_y + cos(ang + 270) * LEN
  230     ' 
  240     ATTR 0: PAPER 0: CLS 
  250     PAPER 2
  260     LOCATE 1,1: PRINT "x1 = ",x1,", y1 = ",y1,", x2 = ",x2,", y2 = ",y2
  270     PAPER 3
  280     LOCATE 1,2: PRINT "x3 = ",x3,", y3 = ",y3,", x4 = ",x4,", y4 = ",y4
  290     ' 
  300     PAPER col1: PEN col2
  310     LINE x1,y1,x2,y2,"X"
  320     PAPER col2: PEN col1
  330     LINE x3,y3,x4,y4,"X"
  340     ' 
  350     col1 = (col1 + 1) % 8
  360     col2 = (col2 + 1) % 8
  370     ang = (ang + 10) % 360
  380     ' 
  390     SLEEP 0.2
  400 WEND 
  410 ' 
  420 RETURN 
