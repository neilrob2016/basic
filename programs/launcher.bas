   10 ' Moves a missile launcher and lauches missiles. Basis of Space Invaders  
   20 ' style game.  
   30 DIM c,i,lx = $term_cols / 2 - 2
   40 DIM MAX_MISSILES = 10
   50 DIM mx(MAX_MISSILES),my(MAX_MISSILES)
   60 ' 
   70 WHILE 1
   80     ' Draw Launcher and missiles   
   90     CLS 
  100     PLOT lx,$term_rows,"XXXXXX"
  110     PLOT lx,$term_rows - 1,"  XX"
  120     PLOT lx,$term_rows - 2,"  XX"
  130     FOR i = 1 TO MAX_MISSILES
  140         IF my(i) THEN 
  150             PLOT mx(i),my(i),"||"
  160             PLOT mx(i),my(i) - 1,"||"
  170         FI 
  180     NEXT 
  190     PLOT 1,1,"Press spacebar..."
  200     ' 
  210     ' Get input then process it  
  220     CINPUT c,0.05
  230     CHOOSE upper$(c)
  240         CASE "Z"
  250         IF lx > 1 THEN lx = lx - 1 FI 
  260         BREAK 
  270         ' 
  280         CASE "X"
  290         IF lx < $term_cols - 5 THEN lx = lx + 1 FI 
  300         BREAK 
  310         ' 
  320         CASE " "
  330         FOR i = 1 TO MAX_MISSILES
  340             IF NOT my(i) THEN 
  350                 mx(i) = lx + 2
  360                 my(i) = $term_rows - 2
  370                 BREAK :' Breaks out of CHOOSE, not FOR  
  380             FI 
  390         NEXT 
  400     CHOSEN 
  410     ' 
  420     ' Move missiles  
  430     FOR i = 1 TO MAX_MISSILES
  440         IF my(i) THEN 
  450             my(i) = my(i) - 1
  460         FI 
  470     NEXT 
  480 WEND 
