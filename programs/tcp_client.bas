   10 ' 
   20 ' Simple TCP client. Run tcp_server.bas first.
   30 ' 
   40 DIM sock = connect("localhost:4000","no_wait_nl")
   50 IF NOT sock THEN 
   60     IF $syserror THEN 
   70         PRINT "ERROR: Unable to connect: ",syserror$($syserror)
   80     ELSE 
   90         PRINT "ERROR: Unable to connect: ",reserror$($reserror)
  100     FI 
  110     STOP 
  120 FI 
  130 ' 
  140 DIM r(2) = -1,w(1) = -1
  150 DIM l
  160 ' 
  170 WHILE 1
  180     r(1) = 0: ' STDIN       
  190     r(2) = sock
  200     IF select(r,w,-1) < 1 THEN 
  210         PRINT "ERROR: select(): ",syserror$($syserror)
  220         STOP 
  230     FI 
  240     IF r(1) THEN 
  250         INPUT l
  260         PRINT #sock,l
  270     FI 
  280     IF r(2) THEN 
  290         INPUT #sock,l
  300         IF $eof THEN 
  310             PRINT "*** Connection closed ***"
  320             STOP 
  330         FI 
  340         PRINT l;
  350     FI 
  360 WEND 
