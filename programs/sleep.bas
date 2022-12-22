   10 ' How to have a consistent sleep time even with interrupts.
   20 ' Resize window to test. ON TERMSIZE CONT is default behaviour but
   30 ' put here just as a reminder.
   40 ON TERMSIZE CONT 
   50 secs = 1
   60 WHILE $true
   70     PRINT "Time = ",time()
   80     sleep_time = secs
   90     ' Since the returned time will never be < sleep_time if not
  100     ' interrupted we don't need to check $interrupted
  110     REPEAT 
  120         start = time()
  130         SLEEP sleep_time
  140         IF $interrupted THEN PRINT "Interrupt!" FI 
  150         sleep_time = sleep_time - (time() - start)
  160     UNTIL sleep_time <= 0
  170 WEND 
