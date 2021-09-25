   10 ' 
   20 ' 3D Cube rotation demonstrator         
   30 ' 
   40 ON BREAK GOTO 1170
   50 ' Cube vertexes                   
   60 DATA -10,10,10,10,10,10,10,-10,10,-10,-10,10
   70 DATA -10,10,-10,10,10,-10,10,-10,-10,-10,-10,-10
   80 ' Cube lines                   
   90 DATA 1,2,2,3,3,4,4,1
  100 DATA 5,6,6,7,7,8,8,5
  110 DATA 1,5,2,6,3,7,4,8
  120 ' 
  130 DIM NUM_POINTS = 8
  140 DIM NUM_LINES = 12
  150 DIM sq_x(NUM_POINTS),sq_y(NUM_POINTS),sq_z(NUM_POINTS)
  160 DIM draw_sq_x(NUM_POINTS),draw_sq_y(NUM_POINTS),draw_sq_z(NUM_POINTS)
  170 DIM line_from(NUM_LINES),line_to(NUM_LINES)
  180 DIM x_ang,y_ang,z_ang
  190 DIM x_ang_add,y_ang_add,z_ang_add
  200 DIM x_mid,y_mid
  210 DIM ang,inc
  220 DIM i,c,tmp,mult
  230 DIM p1,p2
  240 DIM ANG_ADD = 0.1
  250 DIM horizon_dist = 200
  260 DIM HORIZON_ADD = 5
  270 ' 
  280 RESTORE 60
  290 FOR i = 1 TO NUM_POINTS
  300     READ sq_x(i),sq_y(i),sq_z(i)
  310 NEXT 
  320 FOR i = 1 TO NUM_LINES
  330     READ line_from(i),line_to(i)
  340 NEXT 
  350 ' 
  360 ' Main loop                   
  370 ' 
  380 WHILE 1
  390     x_mid = $term_cols / 2
  400     y_mid = $term_rows / 2
  410     ' 
  420     ' Get keyboard input                 
  430     ' 
  440     CINPUT c,0.05
  450     CHOOSE upper$(c)
  460         CASE "Z": z_ang_add = z_ang_add - ANG_ADD: BREAK 
  470         CASE "A": z_ang_add = z_ang_add + ANG_ADD: BREAK 
  480         CASE "X": x_ang_add = x_ang_add - ANG_ADD: BREAK 
  490         CASE "C": x_ang_add = x_ang_add + ANG_ADD: BREAK 
  500         CASE "Y": y_ang_add = y_ang_add - ANG_ADD: BREAK 
  510         CASE "U": y_ang_add = y_ang_add + ANG_ADD: BREAK 
  520         CASE "H"
  530         IF horizon_dist > HORIZON_ADD THEN 
  540             horizon_dist = horizon_dist - HORIZON_ADD
  550         FI 
  560         BREAK 
  570         CASE "J": horizon_dist = horizon_dist + 5
  580     CHOSEN 
  590     ' 
  600     ' Increment angles                 
  610     ' 
  620     x_ang = (x_ang + x_ang_add) % 360
  630     y_ang = (y_ang + y_ang_add) % 360
  640     z_ang = (z_ang + z_ang_add) % 360
  650     IF x_ang < 0 THEN x_ang = x_ang + 360 FI 
  660     IF y_ang < 0 THEN y_ang = y_ang + 360 FI 
  670     IF z_ang < 0 THEN z_ang = z_ang + 360 FI 
  680     ' 
  690     ' Rotate 3D                 
  700     ' 
  710     FOR i = 1 TO NUM_POINTS
  720         draw_sq_x(i) = sq_x(i)
  730         draw_sq_y(i) = sq_y(i)
  740         draw_sq_z(i) = sq_z(i)
  750         ' Rotate about Y axis                 
  760         IF y_ang THEN 
  770             tmp = draw_sq_x(i)
  780             draw_sq_x(i) = draw_sq_x(i) * cos(y_ang) + draw_sq_z(i) * sin(y_ang)
  790             draw_sq_z(i) = draw_sq_z(i) * cos(y_ang) - tmp * sin(y_ang)
  800         FI 
  810         ' Rotate about X axis               
  820         IF x_ang THEN 
  830             tmp = draw_sq_y(i)
  840             draw_sq_y(i) = draw_sq_y(i) * cos(x_ang) - draw_sq_z(i) * sin(x_ang)
  850             draw_sq_z(i) = tmp * sin(x_ang) + draw_sq_z(i) * cos(x_ang)
  860         FI 
  870         ' Rotate about Z axis               
  880         IF z_ang THEN 
  890             tmp = draw_sq_x(i)
  900             draw_sq_x(i) = draw_sq_x(i) * cos(z_ang) + draw_sq_y(i) * sin(z_ang)
  910             draw_sq_y(i) = draw_sq_y(i) * cos(z_ang) - tmp * sin(z_ang)
  920         FI 
  930         ' 
  940         ' Add perspective             
  950         ' 
  960         mult = pow(10,draw_sq_z(i) / horizon_dist)
  970         draw_sq_x(i) = draw_sq_x(i) * mult
  980         draw_sq_y(i) = draw_sq_y(i) * mult
  990     NEXT 
 1000     ' 
 1010     ' Draw             
 1020     ' 
 1030     PAPER 0
 1040     CLS 
 1050     LOCATE 1,1: PAPER 2: PRINT "X angle =     ",format$("###.#",x_ang)
 1060     LOCATE 1,2: PAPER 3: PRINT "Y angle =     ",format$("###.#",y_ang)
 1070     LOCATE 1,3: PAPER 6: PRINT "Z angle =     ",format$("###.#",z_ang)
 1080     LOCATE 1,4: PAPER 1: PRINT "Horizon dist = ",format$("####",horizon_dist)
 1090     PAPER 4
 1100     FOR i = 1 TO NUM_LINES
 1110         p1 = line_from(i)
 1120         p2 = line_to(i)
 1130         LINE x_mid + draw_sq_x(p1),y_mid + draw_sq_y(p1),x_mid + draw_sq_x(p2),y_mid + draw_sq_y(p2),"."
 1140     NEXT 
 1150 WEND 
 1160 ' 
 1170 ATTR 0
