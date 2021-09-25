   10 ' Demonstrates CHOOSE-CHOSEN block    
   20 DIM a
   30 WHILE 1
   40     PRINT ">";: INPUT a
   50     IF isnumstr(a) THEN a = tonum(a) FI 
   60     CHOOSE a
   70         CASE 100 / (50 * 2): PRINT "one": BREAK 
   80         CASE 2: PRINT "two"
   90         CASE 3
  100         CHOOSE a
  110             CASE 2: PRINT "nested two": BREAK 
  120             CASE 3: PRINT "nested three": BREAK 
  130         CHOSEN 
  140         CASE 2 + 2: PRINT "four": BREAK 
  150         CASE "wib" + "ble": PRINT "wibble": BREAK 
  160         CASE "he" + ("l" + "lo"): PRINT "world": BREAK 
  170         CASE 5: PRINT "five": BREAK 
  180         DEFAULT : PRINT "default"
  190     CHOSEN 
  200 WEND 
