   10 ' Draw a starfield  
   20 ON BREAK GOTO 590
   30 ' 
   40 DIM NUM_STARS = 10
   50 DIM MAX_RADIUS = 10
   60 DIM ADD_MULT = 1.3
   70 DIM radius(NUM_STARS)
   80 DIM radius_inc(NUM_STARS)
   90 DIM x(NUM_STARS)
  100 DIM xadd(NUM_STARS)
  110 DIM yadd(NUM_STARS)
  120 DIM y(NUM_STARS)
  130 DIM colour(NUM_STARS)
  140 DIM i
  150 ' 
  160 SEED time()
  170 FOR i = 1 TO NUM_STARS
  180     GOSUB 420
  190 NEXT 
  200 ' 
  210 WHILE 1
  220     CURSOR "off"
  230     ATTR 0: PAPER 0: CLS 
  240     ' Draw the circles  
  250     FOR i = 1 TO NUM_STARS
  260         IF radius(i) > MAX_RADIUS OR x(i) < 0 OR x(i) > $term_cols OR y(i) < 0 OR y(i) > $term_rows THEN 
  270             GOSUB 420
  280         FI 
  290         PAPER colour(i)
  300         CIRCLE x(i),y(i),radius(i),1,"."
  310         x(i) = x(i) - radius_inc(i) / 2 + xadd(i)
  320         y(i) = y(i) - radius_inc(i) / 2 + yadd(i)
  330         radius(i) = radius(i) + radius_inc(i)
  340         xadd(i) = xadd(i) * ADD_MULT
  350         yadd(i) = yadd(i) * ADD_MULT
  360     NEXT 
  370     ' 
  380     SLEEP 0.1
  390 WEND 
  400 ' 
  410 ' Set up a star 
  420 x(i) = $term_cols / 4 + rand() * $term_cols / 2
  430 y(i) = $term_rows / 4 + rand() * $term_rows / 2
  440 IF x(i) < $term_cols / 2 THEN 
  450     xadd(i) = -0.1
  460 ELSE 
  470     xadd(i) = 0.1
  480 FI 
  490 IF y(i) < $term_rows / 2 THEN 
  500     yadd(i) = -0.1
  510 ELSE 
  520     yadd(i) = 0.1
  530 FI 
  540 radius(i) = 1
  550 radius_inc(i) = rand() / 2 + 0.01
  560 colour(i) = floor(rand() * 7) + 1
  570 RETURN 
  580 ' 
  590 ATTR 0: LOCATE 1,1: CURSOR "on"
