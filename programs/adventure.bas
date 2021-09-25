   10 ' 
   20 ' The basics of a simple adventure game.                 
   30 ' This requires at least BASIC 1.1.1 otherwise you will get a DATA      
   40 ' exhausted error.                 
   50 ' 
   60 DIM NUM_ROOMS = 4
   70 DIM NUM_OBJECTS = 3
   80 DIM NUM_DIRS = 8
   90 DIM DESC_END = "*"
  100 DIM INVEN_ROOM = -1
  110 DIM room_text(NUM_ROOMS)
  120 DIM room_dir(NUM_ROOMS,NUM_DIRS)
  130 DIM dir_num_to_name(NUM_DIRS)
  140 DIM dir_num_to_abbrv(NUM_DIRS)
  150 DIM obj_room(NUM_OBJECTS)
  160 DIM TYPE_ITEM = 1
  170 DIM TYPE_NPC = 2
  180 DIM obj_name(NUM_OBJECTS)
  190 DIM obj_desc(NUM_OBJECTS)
  200 DIM obj_type(NUM_OBJECTS)
  210 DIM d,a,i,r
  220 DIM current_room
  230 DIM npc_room
  240 DIM new_room
  250 DIM move_npc
  260 DIM userinput
  270 DIM command
  280 DIM item
  290 ' 
  300 ' Expressions       
  310 DEFEXP randnum = floor(rand() * 10)
  320 ' 
  330 ' Set up direction name maps                
  340 ' 
  350 DATA "north","n","south","s","east","e","west","w"
  360 DATA "up","u","down","d","in","i","out","o"
  370 RESTORE 350
  380 FOR i = 1 TO NUM_DIRS
  390     READ d,a
  400     dir_num_to_name(i) = d
  410     dir_num_to_abbrv(i) = a
  420 NEXT 
  430 ' 
  440 ' ************** ROOM DATA *****************              
  450 ' 
  460 ' Room 1                     
  470 DATA "You are standing in a clearing in a small wood." + chr$(10)
  480 DATA "You are surrounded by large ominous looking trees.",DESC_END
  490 ' Direction ordering is: N,S,E,W,U,D,I,O                    
  500 DATA 2,0,3,0,0,0,0,0
  510 ' 
  520 ' Room 2                     
  530 DATA "You are within the trees. It is very dark here and you can hear" + chr$(10)
  540 DATA "strange sounds coming from up in the trees!",DESC_END
  550 DATA 0,1,0,0,0,0,0,0
  560 ' 
  570 ' Room 3                    
  580 DATA "You are standing under a large oak tree. Sunlight scatters on" + chr$(10)
  590 DATA "the ground around you. Small animals scamper around and birds dart about" + chr$(10)
  600 DATA "above you. There is a hut here you can enter.",DESC_END
  610 DATA 0,0,0,1,0,0,4,0
  620 ' 
  630 ' Room 4   
  640 DATA "You are in a dark, smokey hut. You wonder what fiendish rituals have" + chr$(10)
  650 DATA "happened in this small place",DESC_END
  660 DATA 0,0,0,0,0,0,0,3
  670 ' 
  680 ' Read in room data                    
  690 RESTORE 470
  700 FOR r = 1 TO NUM_ROOMS
  710     ' Read room description                             
  720     READ d
  730     room_text(r) = ""
  740     WHILE d <> DESC_END
  750         room_text(r) = room_text(r) + d
  760         READ d
  770     WEND 
  780     ' Read room directions                             
  790     FOR i = 1 TO NUM_DIRS
  800         READ d
  810         room_dir(r,i) = d
  820     NEXT 
  830 NEXT 
  840 ' 
  850 ' **************** OBJECT DATA ******************               
  860 DATA "Dog",TYPE_NPC,1
  870 DATA "The scruffy dog is a small brown mongrel with a happy face"
  880 ' 
  890 DATA "Squirrel",TYPE_NPC,2
  900 DATA "The squirrel is small and furry and nibbles on some nuts"
  910 ' 
  920 DATA "sword",TYPE_ITEM,2
  930 DATA "The sword is a gleaming finely honed blade."
  940 ' 
  950 ' Read in object data             
  960 RESTORE 860
  970 FOR i = 1 TO NUM_OBJECTS
  980     READ obj_name(i),obj_type(i),obj_room(i)
  990     READ obj_desc(i)
 1000 NEXT 
 1010 ' 
 1020 ' Main loop                           
 1030 ' 
 1040 current_room = 1
 1050 GOSUB 1530
 1060 WHILE 1
 1070     ' Call object routines        
 1080     GOSUB 1770
 1090     PRINT ">";
 1100     INPUT userinput
 1110     userinput = lower$(strip$(userinput))
 1120     IF userinput = "" THEN GOTO 1090 FI 
 1130     command = element$(userinput,1)
 1140     CHOOSE command
 1150         CASE "exit"
 1160         STOP 
 1170         ' 
 1180         CASE "look"
 1190         GOSUB 1530: CONTLOOP 
 1200         CONTLOOP 
 1210         ' 
 1220         CASE "get"
 1230         CASE "take"
 1240         GOSUB 2420: CONTLOOP 
 1250         ' 
 1260         CASE "drop"
 1270         GOSUB 2630: CONTLOOP 
 1280         ' 
 1290         CASE "inven"
 1300         GOSUB 2790: CONTLOOP 
 1310         ' 
 1320         CASE "examine"
 1330         GOSUB 2950: CONTLOOP 
 1340         ' 
 1350         DEFAULT 
 1360         ' See if we have a valid direction                      
 1370         FOR i = 1 TO NUM_DIRS
 1380             IF command = dir_num_to_name(i) OR command = dir_num_to_abbrv(i) THEN 
 1390                 new_room = room_dir(current_room,i)
 1400                 IF new_room = 0 THEN 
 1410                     PRINT "You cannot go that way"
 1420                     GOTO 1090
 1430                 FI 
 1440                 PRINT "You go: ",dir_num_to_name(i)
 1450                 current_room = new_room
 1460                 GOSUB 1560
 1470                 GOTO 1090
 1480             FI 
 1490         NEXT 
 1500         PRINT "Unknown command"
 1510     CHOSEN 
 1520 WEND 
 1530 ' 
 1540 ' Print room description                        
 1550 ' 
 1560 PRINT room_text(current_room)
 1570 PRINT 
 1580 PRINT "You can go: ";
 1590 FOR i = 1 TO NUM_DIRS
 1600     IF room_dir(current_room,i) THEN 
 1610         PRINT dir_num_to_name(i),"  ";
 1620     FI 
 1630 NEXT 
 1640 PRINT 
 1650 PRINT 
 1660 FOR i = 1 TO NUM_OBJECTS
 1670     IF obj_room(i) = current_room THEN 
 1680         PRINT obj_name(i)
 1690     FI 
 1700 NEXT 
 1710 PRINT 
 1720 RETURN 
 1730 ' 
 1740 ' Run object routines. This is only done when the user has input         
 1750 ' something, not on a timer. Which is possible in BASIC but complex.        
 1760 ' 
 1770 FOR i = 1 TO NUM_OBJECTS
 1780     IF obj_room(i) <> INVEN_ROOM THEN 
 1790         CHOOSE lower$(obj_name(i))
 1800             CASE "scruffy dog": GOSUB 1880: BREAK 
 1810             CASE "squirrel": GOSUB 2040: BREAK 
 1820             CASE "sword": GOSUB 2180: BREAK 
 1830         CHOSEN 
 1840     FI 
 1850 NEXT 
 1860 RETURN 
 1870 ' 
 1880 ' Scruffy dog        
 1890 CHOOSE !randnum
 1900     CASE 1
 1910     CASE 0
 1920     PRINT "Scruffy dog barks": RETURN 
 1930     ' 
 1940     CASE 2
 1950     PRINT "Scruffy dog licks his balls": RETURN 
 1960     ' 
 1970     CASE 4
 1980     CASE 5
 1990     CASE 6
 2000     move_npc = i
 2010     GOSUB 2220
 2020     RETURN 
 2030 CHOSEN 
 2040 ' 
 2050 ' Squirrel        
 2060 CHOOSE !randnum
 2070     CASE 0
 2080     PRINT "Squirrel twitches his nose": RETURN 
 2090     ' 
 2100     CASE 1
 2110     PRINT "Squirrel nibbles some nuts": RETURN 
 2120     ' 
 2130     DEFAULT 
 2140     move_npc = i
 2150     GOSUB 2220
 2160     RETURN 
 2170 CHOSEN 
 2180 ' 
 2190 ' Sword does nothing       
 2200 RETURN 
 2210 ' 
 2220 ' Move an NPC      
 2230 npc_room = obj_room(move_npc)
 2240 d = floor(rand() * NUM_DIRS) + 1
 2250 ' Loop until we find a valid direction out of the room        
 2260 WHILE 1
 2270     IF room_dir(npc_room,d) <> 0 THEN 
 2280         new_room = room_dir(npc_room,d)
 2290         IF npc_room = current_room THEN 
 2300             PRINT obj_name(i)," goes ",dir_num_to_name(d)
 2310         ELSE IF new_room = current_room THEN 
 2320                 PRINT obj_name(i)," enters"
 2330             FI 
 2340         FI 
 2350         obj_room(i) = new_room
 2360         RETURN 
 2370     FI 
 2380     d = d + 1
 2390     IF d > NUM_DIRS THEN d = 1 FI 
 2400 WEND 
 2410 ' 
 2420 ' GET command      
 2430 item = lower$(element$(userinput,2))
 2440 IF item = "" THEN 
 2450     PRINT "Take what?"
 2460     RETURN 
 2470 FI 
 2480 FOR i = 1 TO NUM_OBJECTS
 2490     IF obj_room(i) = current_room AND lower$(obj_name(i)) = item THEN 
 2500         IF obj_type(i) = TYPE_ITEM THEN 
 2510             PRINT "You take ",item
 2520             obj_room(i) = INVEN_ROOM
 2530             RETURN 
 2540         ELSE 
 2550             PRINT "You cannot take the ",item
 2560             RETURN 
 2570         FI 
 2580     FI 
 2590 NEXT 
 2600 PRINT "There is no ",item," here"
 2610 RETURN 
 2620 ' 
 2630 ' DROP command      
 2640 item = lower$(element$(userinput,2))
 2650 IF item = "" THEN 
 2660     PRINT "Drop what?"
 2670     RETURN 
 2680 FI 
 2690 FOR i = 1 TO NUM_OBJECTS
 2700     IF obj_room(i) = INVEN_ROOM AND lower$(obj_name(i)) = item THEN 
 2710         PRINT "You drop the ",item
 2720         obj_room(i) = current_room
 2730         RETURN 
 2740     FI 
 2750 NEXT 
 2760 PRINT "You do not have the ",item
 2770 RETURN 
 2780 ' 
 2790 ' INVEN command      
 2800 d = 1
 2810 FOR i = 1 TO NUM_OBJECTS
 2820     IF obj_room(i) = INVEN_ROOM THEN 
 2830         IF d THEN 
 2840             PRINT "You are carrying:"
 2850             d = 0
 2860         FI 
 2870         PRINT "   ",obj_name(i)
 2880     FI 
 2890 NEXT 
 2900 IF d THEN 
 2910     PRINT "You not carrying anything"
 2920 FI 
 2930 RETURN 
 2940 ' 
 2950 ' EXAMINE command     
 2960 item = lower$(element$(userinput,2))
 2970 IF item = "" THEN 
 2980     PRINT "Examine what?"
 2990     RETURN 
 3000 FI 
 3010 FOR i = 1 TO NUM_OBJECTS
 3020     IF (obj_room(i) = current_room OR obj_room(i) = INVEN_ROOM) AND lower$(obj_name(i)) = item THEN 
 3030         PRINT obj_desc(i)
 3040         RETURN 
 3050     FI 
 3060 NEXT 
 3070 PRINT "There is no ",item," here or held by you"
 3080 RETURN 
