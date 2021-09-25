   10 ' Rotates a rectangle in 2D   
   20 DIM xpnt(4),ypnt(4)
   30 DIM x1,y1,x2,y2
   40 DIM mid_x,mid_y,LEN = 10
   50 DIM i,j,ang
   60 ' 
   70 DATA -10,-5,10,-5,10,5,-10,5
   80 RESTORE 70
   90 FOR i = 1 TO 4
  100     READ xpnt(i),ypnt(i)
  110 NEXT 
  120 ' 
  130 WHILE 1
  140     CLS 
  150     LOCATE 1,1: PRINT "Angle = ",ang," degs"
  160     mid_x = $term_cols / 2
  170     mid_y = $term_rows / 2
  180     FOR i = 1 TO 4
  190         IF i < 4 THEN 
  200             j = i + 1
  210         ELSE 
  220             j = 1
  230         FI 
  240         x1 = xpnt(i) * cos(ang) + ypnt(i) * sin(ang)
  250         y1 = ypnt(i) * cos(ang) - xpnt(i) * sin(ang)
  260         x2 = xpnt(j) * cos(ang) + ypnt(j) * sin(ang)
  270         y2 = ypnt(j) * cos(ang) - xpnt(j) * sin(ang)
  280         LINE mid_x + x1,mid_y + y1,mid_x + x2,mid_y + y2,"X"
  290     NEXT 
  300     ang = (ang + 5) % 360
  310     SLEEP 0.1
  320 WEND 
