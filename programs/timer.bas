   10 ' Gives a rough idea of how fast Basic runs on a given system
   20 ' On my Mac this runs in 3.9 secs
   30 tm = time()
   40 FOR i = 1 TO 100000000: NEXT 
   50 PRINT time() - tm," secs"
