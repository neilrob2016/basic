   10 ' 
   20 ' Example TCP server        
   30 ' 
   40 PORT = 4000
   50 MAX_CONN = 10
   60 EINTR = 4
   70 DIM sock(MAX_CONN)
   80 DIM listen_sock = listen(PORT,20)
   90 DIM r(MAX_CONN + 1),w(1)
  100 DIM i,cnt
  110 DIM tmpsock,iline
  120 ' 
  130 DEFEXP addr = "[" + tostr$(sock(i)) + "] " + getip$(sock(i)) + " (" + ip2host$(getip$(sock(i))) + ") "
  140 ' 
  150 ' Main loop             
  160 ' 
  170 PRINT "Running on port ",PORT,"..."
  180 w(1) = -1
  190 ' 
  200 WHILE 1
  210     ' Set up select read array             
  220     r = -1
  230     r(MAX_CONN + 1) = listen_sock
  240     FOR i = 1 TO MAX_CONN
  250         IF sock(i) THEN 
  260             r(i) = sock(i)
  270         FI 
  280     NEXT 
  290     ' 
  300     CHOOSE select(r,w,-1)
  310         CASE -1
  320         IF $syserror <> EINTR THEN 
  330             PRINT "ERROR in select(): ",syserror$($syserror)
  340             STOP 
  350         FI 
  360         GOTO 200
  370         ' 
  380         CASE 0
  390         GOTO 200: ' timeout should not happen             
  400     CHOSEN 
  410     ' 
  420     ' Check listen socket            
  430     IF r(MAX_CONN + 1) = 1 THEN 
  440         PRINT "jumping to accept"
  450         GOSUB 640
  460     FI 
  470     ' 
  480     ' Check live sockets            
  490     FOR i = 1 TO MAX_CONN
  500         IF r(i) = 1 THEN 
  510             INPUT #sock(i),iline
  520             IF $eof THEN 
  530                 PRINT !addr,": Remote close"
  540                 CLOSE sock(i)
  550                 sock(i) = 0
  560             ELSE 
  570                 PRINT !addr,": Got data: ",iline
  580                 PRINT #sock(i),"You sent: ",iline
  590                 PRINT #sock(i),">";
  600             FI 
  610         FI 
  620     NEXT 
  630 WEND 
  640 ' 
  650 ' Set up new connection            
  660 ' 
  670 tmpsock = accept(listen_sock)
  680 FOR i = 1 TO MAX_CONN
  690     IF sock(i) = 0 THEN 
  700         sock(i) = tmpsock
  710         PRINT #tmpsock,"*** Welcome to " + sysinfo$("hostname") + "! ***"
  720         PRINT #tmpsock,">";
  730         PRINT !addr,": New connection"
  740         RETURN 
  750     FI 
  760 NEXT 
  770 PRINT #tmpsock,"We're full!"
  780 CLOSE tmpsock
  790 RETURN 
