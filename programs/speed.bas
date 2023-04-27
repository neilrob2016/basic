   10 ' Gives a rough idea of how fast Basic runs on a given system
   20 ' On my Intel Mac this runs in approx 4.2 secs up to 1.10.0 and
   30 ' approx 3.6 secs from 1.10.1 onwards
   40 iter = 100000000
   50 tm = time()
   60 FOR i = 1 TO iter: NEXT 
   70 diff = time() - tm
   80 PRINT diff," secs, ",round(iter / diff)," iterations per sec"
