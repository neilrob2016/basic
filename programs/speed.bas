   10 ' Gives a rough idea of how fast Basic runs on a given system
   20 ' On my Intel Mac this runs in 4.2 secs
   30 iter = 100000000
   40 tm = time()
   50 FOR i = 1 TO iter: NEXT 
   60 diff = time() - tm
   70 PRINT diff," secs, ",round(iter / diff)," iterations per sec"
