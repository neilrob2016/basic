   10 ' Draws some coloured circles in a ring           
   20 DIM ang1,ang2
   30 DIM x,y
   40 DIM centre_x,centre_y
   50 DIM col
   60 ' 
   70 ATTR 0: PAPER 0: CURSOR "off": CLS 
   80 ON BREAK GOTO 290
   90 col = 1
  100 ' 
  110 WHILE 1
  120     ' 
  130     FOR ang1 = 0 TO 360 - 36 STEP 36
  140         centre_x = 40 + 20 * sin(ang1)
  150         centre_y = 12 + 6 * cos(ang1)
  160         ' 
  170         FOR ang2 = 0 TO 360 STEP 10
  180             x = centre_x + 5 * sin(ang2)
  190             y = centre_y + 5 * cos(ang2)
  200             PAPER col: PLOT x,y
  210             col = col + 1
  220             IF col = 8 THEN col = 1 FI 
  230         NEXT 
  240         ' 
  250     NEXT 
  260     SLEEP 0.1
  270 WEND 
  280 ' 
  290 ATTR 0: CURSOR "on"
