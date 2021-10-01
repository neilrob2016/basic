   10 ' Conways game of life. September 2021
   20 SEED time()
   30 ATTR 0: PAPER 0: PEN 7: CLS 
   40 ' 
   50 ' Set up initial grid state. If $runarg has 2 elements then use them
   60 ' as the grid size else use terminal size. Eg: RUN "30 30"
   70 IF isstr($run_arg) AND elementcnt($run_arg) = 2 THEN 
   80     xs = tonum(element$($run_arg,1))
   90     ys = tonum(element$($run_arg,2))
  100 ELSE 
  110     xs = $term_cols - 1
  120     ys = $term_rows - 1
  130 FI 
  140 DIM grid(2,xs,ys)
  150 gn = 1
  160 FOR x = 1 TO xs
  170     FOR y = 1 TO ys
  180         IF rand() < 0.05 THEN grid(1,x,y) = 1 FI 
  190     NEXT 
  200 NEXT 
  210 ' 
  220 ' Main loop
  230 GOSUB 280: ' Draw
  240 GOSUB 400: ' Calculate
  250 SLEEP 0.1
  260 GOTO 230
  270 ' 
  280 ' Draw the grid
  290 CLS 
  300 FOR x = 1 TO xs
  310     FOR y = 1 TO ys
  320         IF grid(gn,x,y) = 1 THEN 
  330             LOCATE x,y
  340             PRINT "O"
  350         FI 
  360     NEXT 
  370 NEXT 
  380 RETURN 
  390 ' 
  400 ' Calculate new grid
  410 new_gn = gn % 2 + 1
  420 FOR x = 1 TO xs
  430     FOR y = 1 TO ys
  440         prev_x = x - 1
  450         IF prev_x < 1 THEN prev_x = xs FI 
  460         prev_y = y - 1
  470         IF prev_y < 1 THEN prev_y = ys FI 
  480         next_x = x % xs + 1
  490         next_y = y % ys + 1
  500         ' 
  510         ' Do the counts of other cells surrounding this one
  520         cnt = grid(gn,prev_x,prev_y) + grid(gn,x,prev_y) + grid(gn,next_x,prev_y)
  530         cnt = cnt + grid(gn,prev_x,y) + grid(gn,next_x,y)
  540         cnt = cnt + grid(gn,prev_x,next_y) + grid(gn,x,next_y) + grid(gn,next_x,next_y)
  550         IF cnt < 2 THEN 
  560             grid(new_gn,x,y) = 0
  570         ELSE IF cnt = 3 OR (cnt < 4 AND grid(gn,x,y) = 1) THEN 
  580                 grid(new_gn,x,y) = 1
  590             ELSE IF cnt > 3 AND grid(gn,x,y) THEN 
  600                     grid(new_gn,x,y) = 0
  610         FIALL 
  620     NEXT 
  630 NEXT 
  640 gn = new_gn
  650 RETURN 
