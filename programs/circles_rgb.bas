   10 ' Draws some coloured circles in a ring          
   20 ' This version uses full RGB terminal colours        
   30 DIM ang1,ang2
   40 DIM x,y
   50 DIM centre_x,centre_y
   60 DIM r,g,b
   70 DIM radd,gadd,badd
   80 ' 
   90 SEED time()
  100 r = random(255)
  110 g = random(255)
  120 b = random(255)
  130 ' 
  140 ATTR 0: PAPER 0: CURSOR "off": CLS 
  150 ON BREAK GOTO 380
  160 ' 
  170 WHILE 1
  180     FOR ang1 = 0 TO 360 - 36 STEP 36
  190         centre_x = 40 + 20 * sin(ang1)
  200         centre_y = 12 + 6 * cos(ang1)
  210         radd = random(10)
  220         gadd = random(10)
  230         bass = random(10)
  240         ' 
  250         FOR ang2 = 0 TO 360 STEP 10
  260             x = centre_x + 5 * sin(ang2)
  270             y = centre_y + 5 * cos(ang2)
  280             PAPER r,g,b: PLOT x,y
  290             r = (r + radd) % 255
  300             g = (g + gadd) % 255
  310             b = (b + badd) % 255
  320         NEXT 
  330         ' 
  340     NEXT 
  350     SLEEP 0.1
  360 WEND 
  370 ' 
  380 ATTR 0: CURSOR "on"
