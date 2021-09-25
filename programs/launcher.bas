   10 ' Moves a missile launcher and lauches missiles. Basis of Space Invaders  
   20 ' style game.  
   30 DIM c,i,lx = $term_cols / 2 - 2
   40 DIM MAX_MISSILES = 5
   50 DIM mx(MAX_MISSILES),my(MAX_MISSILES)
   60 ' 
   70 WHILE 1
   80     ' Draw Launcher and missiles   
   90     CLS 
  100     LOCATE lx,$term_rows: PRINT "  XX"
  110     LOCATE lx,$term_rows + 1: PRINT "  XX"
  120     LOCATE lx,$term_rows + 2: PRINT "XXXXXX"
  130     FOR i = 1 TO MAX_MISSILES
  140         IF my(i) THEN 
  150             LOCATE mx(i),my(i): PRINT "||"
  160             LOCATE mx(i),my(i) - 1: PRINT "||"
  170         FI 
  180     NEXT 
  190     ' 
  200     ' Get input then process it  
  210     CINPUT c,0.05
  220     CHOOSE upper$(c)
  230         CASE "Z"
  240         IF lx > 1 THEN lx = lx - 1 FI 
  250         BREAK 
  260         ' 
  270         CASE "X"
  280         IF lx < $term_cols - 5 THEN lx = lx + 1 FI 
  290         BREAK 
  300         ' 
  310         CASE " "
  320         FOR i = 1 TO MAX_MISSILES
  330             IF NOT my(i) THEN 
  340                 mx(i) = lx + 2
  350                 my(i) = $term_rows - 3
  360                 BREAK : ' Breaks out of CHOOSE, not FOR  
  370             FI 
  380         NEXT 
  390     CHOSEN 
  400     ' 
  410     ' Move missiles  
  420     FOR i = 1 TO MAX_MISSILES
  430         IF my(i) THEN 
  440             my(i) = my(i) - 1
  450         FI 
  460     NEXT 
  470 WEND 
