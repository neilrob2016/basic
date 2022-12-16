   10 ' Gives a rough idea of how fast Basic runs on a given system
   20 ' On my Intel Mac this runs in 4.2 secs
   25 iter = 100000000
   30 tm = time()
   40 FOR i = 1 TO iter: NEXT 
   50 diff = time() - tm
   60 PRINT diff," secs, ",round(iter / diff)," iterations per sec"
