   10 ' The mid point circle drawing algorithm
   20 PRINT "Radius> ";
   30 INPUT radius
   40 radius = tonum(radius)
   50 radius2 = radius * radius
   60 xmid = radius + 20
   70 ymid = radius + 10
   80 x = 0
   90 y = -radius
  100 ' 
  110 CLS 
  120 WHILE y <= 0
  130     px1 = xmid + x
  140     px2 = xmid - x
  150     py1 = ymid + y
  160     py2 = ymid - y - 1
  170     PLOT px1,py1,"X"
  180     PLOT px1,py2,"X"
  190     PLOT px2,py1,"X"
  200     PLOT px2,py2,"X"
  210     ' 
  220     ' Find where the halfway to the next potential point is relative to 
  230     ' the circles perimeter
  240     relative = pow(x + 0.5,2) + pow(y + 1,2) - radius2
  250     ' 
  260     ' if < 0 then point is inside the circle, if its 0 then its on the
  270     ' perimeter and if its > 0 then its outside the circle3
  280     IF relative < 0 THEN 
  290         x = x + 1
  300     ELSE IF relative = 0 THEN 
  310         x = x + 1
  320         y = y + 1
  330     ELSE 
  340         y = y + 1
  350     FIALL 
  360 WEND 
  370 GOTO 20
