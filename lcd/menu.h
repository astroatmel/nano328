// This file was auto generated from menu.def and menu.awk
// cat menu.def | awk -f menu.awk | tee menu.h
#define M_ROOT_COUNT                    1
#define M_ROOT_FIRST                    1
#define M_ROOT_LAST                     1
#define M_MAIN_COUNT                    6
#define M_MAIN_FIRST                    2
#define M_MAIN_LAST                     7
#define M_MOZAIC_COUNT                  8
#define M_MOZAIC_FIRST                  8
#define M_MOZAIC_LAST                   15
#define M__MOZ_EXP_COUNT                1
#define M__MOZ_EXP_FIRST                16
#define M__MOZ_EXP_LAST                 16
#define M__MOZ_SPAN1_COUNT              1
#define M__MOZ_SPAN1_FIRST              17
#define M__MOZ_SPAN1_LAST               17
#define M__MOZ_SPAN2_COUNT              1
#define M__MOZ_SPAN2_FIRST              18
#define M__MOZ_SPAN2_LAST               18
#define M__MOZ_SPAN3_COUNT              1
#define M__MOZ_SPAN3_FIRST              19
#define M__MOZ_SPAN3_LAST               19
#define M_TIMELAP_COUNT                 3
#define M_TIMELAP_FIRST                 20
#define M_TIMELAP_LAST                  22
#define M__TIMELAP_EXP_COUNT            1
#define M__TIMELAP_EXP_FIRST            23
#define M__TIMELAP_EXP_LAST             23
#define M__TIMELAP_PERIOD1_COUNT        1
#define M__TIMELAP_PERIOD1_FIRST        24
#define M__TIMELAP_PERIOD1_LAST         24
#define M__TIMELAP_PERIOD2_COUNT        1
#define M__TIMELAP_PERIOD2_FIRST        25
#define M__TIMELAP_PERIOD2_LAST         25
#define M_CGEM_STATUS_COUNT             7
#define M_CGEM_STATUS_FIRST             26
#define M_CGEM_STATUS_LAST              32
#define M__MOZ_RUN_COUNT                2
#define M__MOZ_RUN_FIRST                33
#define M__MOZ_RUN_LAST                 34
#define M__TIMELAP_RUN_COUNT            2
#define M__TIMELAP_RUN_FIRST            35
#define M__TIMELAP_RUN_LAST             36
#define M_CANCEL_COUNT                  1
#define M_CANCEL_FIRST                  37
#define M_CANCEL_LAST                   37
#define M_CANCELING_COUNT               1
#define M_CANCELING_FIRST               38
#define M_CANCELING_LAST                38
#define M_EXECUTE_COUNT                 1
#define M_EXECUTE_FIRST                 39
#define M_EXECUTE_LAST                  39
#define M_PRESET_COUNT                  1
#define M_PRESET_FIRST                  40
#define M_PRESET_LAST                   40

PROGMEM const long edit_increment[] = {
              1000            ,                   //  0
              ONE_DEG_CGEM    ,                   //  1
              ONE_MIN_CGEM    ,                   //  2
              ONE_SEC_CGEM    ,                   //  3
              3600            ,                   //  4
              60              };                  //  5

PROGMEM const char linetxt[]=
              "Camera Driver   "                  //  0
              "Version 2.0     "                  //  1
              "RA:\242R\001          "            //  2
              "DE:\242D\002          "            //  3
              "RA:\242H\001          "            //  4
              "Mozaic Menu...  "                  //  5
              "(GOTO Method)   "                  //  6
              "(Pause Method)  "                  //  7
              "Timelap Menu... "                  //  8
              "                "                  //  9
              "CGEM Status...  "                  //  10
              "Start Mozaic... "                  //  11
              "Set Span to:    "                  //  12
              "1/10 of EDGE HD "                  //  13
              "1/10 of Newton 6"                  //  14
              "Set Exposure:   "                  //  15
              "1 Second        "                  //  16
              "30 Second       "                  //  17
              "60 Second       "                  //  18
              "Exposure Time:  "                  //  19
              "Current:\242s\003 sec "            //  20
              "Total Span:     "                  //  21
              "Cur:\242d\017         "            //  22
              "    Set:\243s\003 sec "            //  23
              "Set:\243d\017         "            //  24
              "Start Timelap..."                  //  25
              "Period:         "                  //  26
              "Current:\242t\004     "            //  27
              "    Set:\243t\004     "            //  28
              "RA:\242x\001          "            //  29
              "DE:\242x\002          "            //  30
              "D1:\242x\031          "            //  31
              "D2:\242x\030          "            //  32
              "Aligned:\242y\012     "            //  33
              "In Goto:\242y\013     "            //  34
              "Tracking:\242q\026    "            //  35
              "Moz Err:\242e\014     "            //  36
              "Pos Err:\242e\015     "            //  37
              "Latency:\242E\016     "            //  38
              "Time-Out:\242e\011    "            //  39
              "Histogram:  (x8)"                  //  40
              "\242h\023             "            //  41
              "Mozaic:\242o\005      "            //  42
              "Time:\242m\020    sec "            //  43
              "Pos Id:\242p\007      "            //  44
              "Timelap progres:"                  //  45
              "Shot no:\242s\032     "            //  46
              "Period:\242T\033      "            //  47
              "Really Cancel ? "                  //  48
              "Cancelling...   "                  //  49
              "Executing...    "                  //  50
              "Done...         ";                 //  51

#define MENU_TABLE_WIDTH 8
#define MENU_TABLE_LEN   40
PROGMEM const char menu_state_machine[]= {  0,0,0,0,0,0,0,0,0, // STATE : 0 is not a valid state, it means 'do nothing'  > there is one more element so that the array starts at 1
//            UNDO      ENTER     PREV      NEXT      INCREMENT OFFSET    LINE1     LINE2  
               0,       2,        1,        1,        -1,       -1,       0,        1,         // State: 1    M_ROOT              "Camera Driver"          "Version 2.0"            
              -1,       0,        7,        3,        -1,       -1,       2,        3,         // State: 2    M_MAIN              "RA:\242R\001"           "DE:\242D\002"           
              -1,       0,        2,        4,        -1,       -1,       4,        3,         // State: 3    M_MAIN              "RA:\242H\001"           "DE:\242D\002"           
              -1,       8,        3,        5,        -1,       -1,       5,        6,         // State: 4    M_MAIN              "Mozaic Menu..."         "(GOTO Method)"          
              -1,       8,        4,        6,        -1,       -1,       5,        7,         // State: 5    M_MAIN              "Mozaic Menu..."         "(Pause Method)"         
              -1,       20,       5,        7,        -1,       -1,       8,        9,         // State: 6    M_MAIN              "Timelap Menu..."        ""                       
              -1,       26,       6,        2,        -1,       -1,       10,       9,         // State: 7    M_MAIN              "CGEM Status..."         ""                       
              -1,       33,       15,       9,        -1,       -1,       11,       9,         // State: 8    M_MOZAIC            "Start Mozaic..."        ""                       
              -1,       40,       8,        10,       -1,       -1,       12,       13,        // State: 9    M_MOZAIC            "Set Span to:"           "1/10 of EDGE HD"        
              -1,       40,       9,        11,       -1,       -1,       12,       14,        // State: 10   M_MOZAIC            "Set Span to:"           "1/10 of Newton 6"       
              -1,       40,       10,       12,       -1,       -1,       15,       16,        // State: 11   M_MOZAIC            "Set Exposure:"          "1 Second"               
              -1,       40,       11,       13,       -1,       -1,       15,       17,        // State: 12   M_MOZAIC            "Set Exposure:"          "30 Second"              
              -1,       40,       12,       14,       -1,       -1,       15,       18,        // State: 13   M_MOZAIC            "Set Exposure:"          "60 Second"              
              -1,       16,       13,       15,       -1,       -1,       19,       20,        // State: 14   M_MOZAIC            "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       17,       14,       8,        -1,       -1,       21,       22,        // State: 15   M_MOZAIC            "Total Span:"            "Cur:\242d\017"          
              -1,       39,       16,       16,       0,        26,       19,       23,        // State: 16   M__MOZ_EXP          "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       18,       17,       17,       1,        23,       21,       24,        // State: 17   M__MOZ_SPAN1        "Total Span:"            "Set:\243d\017"          
              -1,       19,       18,       18,       2,        26,       21,       24,        // State: 18   M__MOZ_SPAN2        "Total Span:"            "Set:\243d\017"          
              -1,       39,       19,       19,       3,        29,       21,       24,        // State: 19   M__MOZ_SPAN3        "Total Span:"            "Set:\243d\017"          
              -1,       35,       22,       21,       -1,       -1,       25,       9,         // State: 20   M_TIMELAP           "Start Timelap..."       ""                       
              -1,       23,       20,       22,       -1,       -1,       19,       20,        // State: 21   M_TIMELAP           "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       24,       21,       20,       -1,       -1,       26,       27,        // State: 22   M_TIMELAP           "Period:"                "Current:\242t\004"      
              -1,       39,       23,       23,       0,        26,       19,       23,        // State: 23   M__TIMELAP_EXP      "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       25,       24,       24,       4,        25,       26,       28,        // State: 24   M__TIMELAP_PERIOD1  "Period:"                "    Set:\243t\004"      
              -1,       39,       25,       25,       5,        28,       26,       28,        // State: 25   M__TIMELAP_PERIOD2  "Period:"                "    Set:\243t\004"      
              -1,       0,        32,       27,       -1,       -1,       29,       30,        // State: 26   M_CGEM_STATUS       "RA:\242x\001"           "DE:\242x\002"           
              -1,       0,        26,       28,       -1,       -1,       31,       32,        // State: 27   M_CGEM_STATUS       "D1:\242x\031"           "D2:\242x\030"           
              -1,       0,        27,       29,       -1,       -1,       33,       34,        // State: 28   M_CGEM_STATUS       "Aligned:\242y\012"      "In Goto:\242y\013"      
              -1,       0,        28,       30,       -1,       -1,       35,       9,         // State: 29   M_CGEM_STATUS       "Tracking:\242q\026"     ""                       
              -1,       0,        29,       31,       -1,       -1,       36,       37,        // State: 30   M_CGEM_STATUS       "Moz Err:\242e\014"      "Pos Err:\242e\015"      
              -1,       0,        30,       32,       -1,       -1,       38,       39,        // State: 31   M_CGEM_STATUS       "Latency:\242E\016"      "Time-Out:\242e\011"     
              -1,       0,        31,       26,       -1,       -1,       40,       41,        // State: 32   M_CGEM_STATUS       "Histogram:  (x8)"       "\242h\023"              
              37,       0,        34,       34,       -1,       -1,       42,       43,        // State: 33   M__MOZ_RUN          "Mozaic:\242o\005"       "Time:\242m\020    sec"  
              37,       0,        33,       33,       -1,       -1,       42,       44,        // State: 34   M__MOZ_RUN          "Mozaic:\242o\005"       "Pos Id:\242p\007"       
              37,       0,        36,       36,       -1,       -1,       45,       46,        // State: 35   M__TIMELAP_RUN      "Timelap progres:"       "Shot no:\242s\032"      
              37,       0,        35,       35,       -1,       -1,       45,       47,        // State: 36   M__TIMELAP_RUN      "Timelap progres:"       "Period:\242T\033"       
              -1,       38,       37,       37,       -1,       -1,       48,       9,         // State: 37   M_CANCEL            "Really Cancel ?"        ""                       
               0,       0,        38,       38,       -1,       -1,       49,       9,         // State: 38   M_CANCELING         "Cancelling..."          ""                       
               0,       0,        39,       39,       -1,       -1,       50,       9,         // State: 39   M_EXECUTE           "Executing..."           ""                       
               0,       0,        40,       40,       -1,       -1,       51,       9};        // State: 40   M_PRESET            "Done..."                ""                       
