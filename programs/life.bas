   10 ' Conways game of life. September 2021
   20 ON TERMSIZE GOTO 690
   30 SEED time()
   40 ATTR 0: PAPER 0: PEN 7: CLS 
   50 ' 
   60 ' Set up initial grid state. If $runarg has 2 elements then use them
   70 ' as the grid size else use terminal size. Eg: RUN "30 30"
   80 IF isstr($run_arg) AND elementcnt($run_arg) = 2 THEN 
   90     xs = tonum(element$($run_arg,1))
  100     ys = tonum(element$($run_arg,2))
  110 ELSE 
  120     xs = $term_cols - 1
  130     ys = $term_rows - 1
  140 FI 
  150 DIM grid(2,xs,ys)
  160 gn = 1
  170 FOR x = 1 TO xs
  180     FOR y = 1 TO ys
  190         IF rand() < 0.05 THEN grid(1,x,y) = 1 FI 
  200     NEXT 
  210 NEXT 
  220 ' 
  230 ' Main loop
  240 GOSUB 290: ' Draw
  250 GOSUB 410: ' Calculate
  260 SLEEP 0.1
  270 GOTO 240
  280 ' 
  290 ' Draw the grid
  300 CLS 
  310 FOR x = 1 TO xs
  320     FOR y = 1 TO ys
  330         IF grid(gn,x,y) = 1 THEN 
  340             LOCATE x,y
  350             PRINT "O"
  360         FI 
  370     NEXT 
  380 NEXT 
  390 RETURN 
  400 ' 
  410 ' Calculate new grid
  420 new_gn = gn % 2 + 1
  430 FOR x = 1 TO xs
  440     FOR y = 1 TO ys
  450         prev_x = x - 1
  460         IF prev_x < 1 THEN prev_x = xs FI 
  470         prev_y = y - 1
  480         IF prev_y < 1 THEN prev_y = ys FI 
  490         next_x = x % xs + 1
  500         next_y = y % ys + 1
  510         ' 
  520         ' Do the counts of other cells surrounding this one
  530         cnt = grid(gn,prev_x,prev_y) + grid(gn,x,prev_y) + grid(gn,next_x,prev_y)
  540         cnt = cnt + grid(gn,prev_x,y) + grid(gn,next_x,y)
  550         cnt = cnt + grid(gn,prev_x,next_y) + grid(gn,x,next_y) + grid(gn,next_x,next_y)
  560         IF cnt < 2 THEN 
  570             grid(new_gn,x,y) = 0
  580         ELSE IF cnt = 3 OR (cnt < 4 AND grid(gn,x,y) = 1) THEN 
  590                 grid(new_gn,x,y) = 1
  600             ELSE IF cnt > 3 AND grid(gn,x,y) THEN 
  610                     grid(new_gn,x,y) = 0
  620         FIALL 
  630     NEXT 
  640 NEXT 
  650 gn = new_gn
  660 RETURN 
  670 ' 
  680 ' Reset the simulation to the new screen size
  690 CLEAR 
  700 GOTO 30
