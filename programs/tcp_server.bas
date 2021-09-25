   10 ' 
   20 ' Example TCP server        
   30 ' 
   40 MAX_CONN = 10
   50 EINTR = 4
   60 DIM sock(MAX_CONN)
   70 DIM listen_sock = listen(4000,20)
   80 DIM r(MAX_CONN + 1),w(1)
   90 DIM i,cnt
  100 DIM tmpsock,iline
  110 ' 
  120 DEFEXP addr = "[" + tostr$(sock(i)) + "] " + getip$(sock(i)) + " (" + ip2host$(getip$(sock(i))) + ") "
  130 ' 
  140 ' Main loop             
  150 ' 
  160 PRINT "Running..."
  170 w(1) = -1
  180 ' 
  190 WHILE 1
  200     ' Set up select read array             
  210     r = -1
  220     r(MAX_CONN + 1) = listen_sock
  230     FOR i = 1 TO MAX_CONN
  240         IF sock(i) THEN 
  250             r(i) = sock(i)
  260         FI 
  270     NEXT 
  280     ' 
  290     CHOOSE select(r,w,-1)
  300         CASE -1
  310         IF $syserror <> EINTR THEN 
  320             PRINT "ERROR in select(): ",syserror$($syserror)
  330             STOP 
  340         FI 
  350         GOTO 190
  360         ' 
  370         CASE 0
  380         GOTO 190: ' timeout should not happen             
  390     CHOSEN 
  400     ' 
  410     ' Check listen socket            
  420     IF r(MAX_CONN + 1) = 1 THEN 
  430         PRINT "jumping to accept"
  440         GOSUB 620
  450     FI 
  460     ' 
  470     ' Check live sockets            
  480     FOR i = 1 TO MAX_CONN
  490         IF r(i) = 1 THEN 
  500             INPUT #sock(i),iline
  510             IF $eof THEN 
  520                 PRINT !addr,": Remote close"
  530                 CLOSE sock(i)
  540                 sock(i) = 0
  550             ELSE 
  560                 PRINT !addr,": Got data: ",iline
  570                 PRINT #sock(i),"You sent: ",iline
  580             FI 
  590         FI 
  600     NEXT 
  610 WEND 
  620 ' 
  630 ' Set up new connection            
  640 ' 
  650 tmpsock = accept(listen_sock)
  660 FOR i = 1 TO MAX_CONN
  670     IF sock(i) = 0 THEN 
  680         sock(i) = tmpsock
  690         PRINT #tmpsock,"*** Welcome to " + sysinfo$("hostname") + "! ***"
  700         PRINT !addr,": New connection"
  710         RETURN 
  720     FI 
  730 NEXT 
  740 PRINT #tmpsock,"We're full!"
  750 CLOSE tmpsock
  760 RETURN 
