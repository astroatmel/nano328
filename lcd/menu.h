// This file was auto generated from menu.def and menu.awk
// cat menu.def | awk -f menu.awk | tee menu.h
#define M_ROOT_COUNT                    1
#define M_ROOT_FIRST                    1
#define M_ROOT_LAST                     1
#define M_MAIN_COUNT                    6
#define M_MAIN_FIRST                    2
#define M_MAIN_LAST                     7
#define M_MOZAIC_PAUSE_COUNT            10
#define M_MOZAIC_PAUSE_FIRST            8
#define M_MOZAIC_PAUSE_LAST             17
#define M__MOZ_RA_COUNT                 1
#define M__MOZ_RA_FIRST                 18
#define M__MOZ_RA_LAST                  18
#define M__MOZ_DE_COUNT                 1
#define M__MOZ_DE_FIRST                 19
#define M__MOZ_DE_LAST                  19
#define M_MOZAIC_COUNT                  8
#define M_MOZAIC_FIRST                  20
#define M_MOZAIC_LAST                   27
#define M__MOZ_EXP_COUNT                1
#define M__MOZ_EXP_FIRST                28
#define M__MOZ_EXP_LAST                 28
#define M__MOZ_SPAN1_COUNT              1
#define M__MOZ_SPAN1_FIRST              29
#define M__MOZ_SPAN1_LAST               29
#define M__MOZ_SPAN2_COUNT              1
#define M__MOZ_SPAN2_FIRST              30
#define M__MOZ_SPAN2_LAST               30
#define M__MOZ_SPAN3_COUNT              1
#define M__MOZ_SPAN3_FIRST              31
#define M__MOZ_SPAN3_LAST               31
#define M_TIMELAP_COUNT                 3
#define M_TIMELAP_FIRST                 32
#define M_TIMELAP_LAST                  34
#define M__TIMELAP_EXP_COUNT            1
#define M__TIMELAP_EXP_FIRST            35
#define M__TIMELAP_EXP_LAST             35
#define M__TIMELAP_PERIOD1_COUNT        1
#define M__TIMELAP_PERIOD1_FIRST        36
#define M__TIMELAP_PERIOD1_LAST         36
#define M__TIMELAP_PERIOD2_COUNT        1
#define M__TIMELAP_PERIOD2_FIRST        37
#define M__TIMELAP_PERIOD2_LAST         37
#define M_CGEM_STATUS_COUNT             7
#define M_CGEM_STATUS_FIRST             38
#define M_CGEM_STATUS_LAST              44
#define M__MOZ_RUN_COUNT                2
#define M__MOZ_RUN_FIRST                45
#define M__MOZ_RUN_LAST                 46
#define M__TIMELAP_RUN_COUNT            2
#define M__TIMELAP_RUN_FIRST            47
#define M__TIMELAP_RUN_LAST             48
#define M_CANCEL_COUNT                  1
#define M_CANCEL_FIRST                  49
#define M_CANCEL_LAST                   49
#define M_CANCELING_COUNT               1
#define M_CANCELING_FIRST               50
#define M_CANCELING_LAST                50
#define M_EXECUTE_COUNT                 1
#define M_EXECUTE_FIRST                 51
#define M_EXECUTE_LAST                  51
#define M_PRESET_COUNT                  1
#define M_PRESET_FIRST                  52
#define M_PRESET_LAST                   52
#define M_DEBUG_COUNT                   3
#define M_DEBUG_FIRST                   53
#define M_DEBUG_LAST                    55

PROGMEM const long edit_increment[] = {
              1               ,                   //  0
              1000            ,                   //  1
              ONE_DEG_CGEM    ,                   //  2
              ONE_MIN_CGEM    ,                   //  3
              ONE_SEC_CGEM    ,                   //  4
              3600            ,                   //  5
              60              };                  //  6

PROGMEM const char linetxt[]=
              "Cam Driver V2   "                  //  0
              "Ram:\242X\031         "            //  1
              "Mozaic Menu...  "                  //  2
              "(GOTO Method)   "                  //  3
              "(Pause Method)  "                  //  4
              "Timelap Menu... "                  //  5
              "                "                  //  6
              "CGEM Status...  "                  //  7
              "RA:\242R\001          "            //  8
              "DE:\242D\002          "            //  9
              "RA:\242H\001          "            //  10
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
              "Nb Pic:         "                  //  23
              "Current:\242n\044 (RA)"            //  24
              "Current:\242n\045 (DE)"            //  25
              "    Set:\243n\044 (RA)"            //  26
              "    Set:\243n\045 (DE)"            //  27
              "    Set:\243s\003 sec "            //  28
              "Set:\243d\017         "            //  29
              "Start Timelap..."                  //  30
              "Period:         "                  //  31
              "Current:\242t\004     "            //  32
              "    Set:\243t\004     "            //  33
              "RA:\242x\001          "            //  34
              "DE:\242x\002          "            //  35
              "D1:\242x\031          "            //  36
              "D2:\242x\030          "            //  37
              "Aligned:\242y\012     "            //  38
              "In Goto:\242y\013     "            //  39
              "Tracking:\242q\026    "            //  40
              "Moz Err:\242e\014     "            //  41
              "Pos Err:\242e\015     "            //  42
              "Latency:\242E\016     "            //  43
              "Time-Out:\242e\011    "            //  44
              "Histogram:  (x8)"                  //  45
              "\242h\023             "            //  46
              "Mozaic:\242o\005      "            //  47
              "Time:\242m\020    sec "            //  48
              "Pos Id:\242p\007      "            //  49
              "Timelap progres:"                  //  50
              "Shot no:\242s\032     "            //  51
              "Period:\242T\033      "            //  52
              "Really Cancel ? "                  //  53
              "Cancelling...   "                  //  54
              "Executing...    "                  //  55
              "Done...         "                  //  56
              "D3:\242x\031          "            //  57
              "D4:\242x\030          "            //  58
              "D5:\242x\031          "            //  59
              "D6:\242x\030          ";           //  60

#define MENU_TABLE_WIDTH 8
#define MENU_TABLE_LEN   55
PROGMEM const char menu_state_machine[]= {  0,0,0,0,0,0,0,0,0, // STATE : 0 is not a valid state, it means 'do nothing'  > there is one more element so that the array starts at 1
//            UNDO      ENTER     PREV      NEXT      INCREMENT OFFSET    LINE1     LINE2  
               0,       2,        1,        1,        -1,       -1,       0,        1,         // State: 1    M_ROOT              "Cam Driver V2"          "Ram:\242X\031"          
              -1,       20,       7,        3,        -1,       -1,       2,        3,         // State: 2    M_MAIN              "Mozaic Menu..."         "(GOTO Method)"          
              -1,       8,        2,        4,        -1,       -1,       2,        4,         // State: 3    M_MAIN              "Mozaic Menu..."         "(Pause Method)"         
              -1,       32,       3,        5,        -1,       -1,       5,        6,         // State: 4    M_MAIN              "Timelap Menu..."        ""                       
              -1,       38,       4,        6,        -1,       -1,       7,        6,         // State: 5    M_MAIN              "CGEM Status..."         ""                       
              -1,       0,        5,        7,        -1,       -1,       8,        9,         // State: 6    M_MAIN              "RA:\242R\001"           "DE:\242D\002"           
              -1,       0,        6,        2,        -1,       -1,       10,       9,         // State: 7    M_MAIN              "RA:\242H\001"           "DE:\242D\002"           
              -1,       45,       17,       9,        -1,       -1,       11,       4,         // State: 8    M_MOZAIC_PAUSE      "Start Mozaic..."        "(Pause Method)"         
              -1,       52,       8,        10,       -1,       -1,       12,       13,        // State: 9    M_MOZAIC_PAUSE      "Set Span to:"           "1/10 of EDGE HD"        
              -1,       52,       9,        11,       -1,       -1,       12,       14,        // State: 10   M_MOZAIC_PAUSE      "Set Span to:"           "1/10 of Newton 6"       
              -1,       52,       10,       12,       -1,       -1,       15,       16,        // State: 11   M_MOZAIC_PAUSE      "Set Exposure:"          "1 Second"               
              -1,       52,       11,       13,       -1,       -1,       15,       17,        // State: 12   M_MOZAIC_PAUSE      "Set Exposure:"          "30 Second"              
              -1,       52,       12,       14,       -1,       -1,       15,       18,        // State: 13   M_MOZAIC_PAUSE      "Set Exposure:"          "60 Second"              
              -1,       28,       13,       15,       -1,       -1,       19,       20,        // State: 14   M_MOZAIC_PAUSE      "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       29,       14,       16,       -1,       -1,       21,       22,        // State: 15   M_MOZAIC_PAUSE      "Total Span:"            "Cur:\242d\017"          
              -1,       18,       15,       17,       -1,       -1,       23,       24,        // State: 16   M_MOZAIC_PAUSE      "Nb Pic:"                "Current:\242n\044 (RA)" 
              -1,       19,       16,       8,        -1,       -1,       23,       25,        // State: 17   M_MOZAIC_PAUSE      "Nb Pic:"                "Current:\242n\045 (DE)" 
              -1,       51,       18,       18,       0,        26,       23,       26,        // State: 18   M__MOZ_RA           "Nb Pic:"                "    Set:\243n\044 (RA)" 
              -1,       51,       19,       19,       0,        26,       23,       27,        // State: 19   M__MOZ_DE           "Nb Pic:"                "    Set:\243n\045 (DE)" 
              -1,       45,       27,       21,       -1,       -1,       11,       3,         // State: 20   M_MOZAIC            "Start Mozaic..."        "(GOTO Method)"          
              -1,       52,       20,       22,       -1,       -1,       12,       13,        // State: 21   M_MOZAIC            "Set Span to:"           "1/10 of EDGE HD"        
              -1,       52,       21,       23,       -1,       -1,       12,       14,        // State: 22   M_MOZAIC            "Set Span to:"           "1/10 of Newton 6"       
              -1,       52,       22,       24,       -1,       -1,       15,       16,        // State: 23   M_MOZAIC            "Set Exposure:"          "1 Second"               
              -1,       52,       23,       25,       -1,       -1,       15,       17,        // State: 24   M_MOZAIC            "Set Exposure:"          "30 Second"              
              -1,       52,       24,       26,       -1,       -1,       15,       18,        // State: 25   M_MOZAIC            "Set Exposure:"          "60 Second"              
              -1,       28,       25,       27,       -1,       -1,       19,       20,        // State: 26   M_MOZAIC            "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       29,       26,       20,       -1,       -1,       21,       22,        // State: 27   M_MOZAIC            "Total Span:"            "Cur:\242d\017"          
              -1,       51,       28,       28,       1,        26,       19,       28,        // State: 28   M__MOZ_EXP          "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       30,       29,       29,       2,        23,       21,       29,        // State: 29   M__MOZ_SPAN1        "Total Span:"            "Set:\243d\017"          
              -1,       31,       30,       30,       3,        26,       21,       29,        // State: 30   M__MOZ_SPAN2        "Total Span:"            "Set:\243d\017"          
              -1,       51,       31,       31,       4,        29,       21,       29,        // State: 31   M__MOZ_SPAN3        "Total Span:"            "Set:\243d\017"          
              -1,       47,       34,       33,       -1,       -1,       30,       6,         // State: 32   M_TIMELAP           "Start Timelap..."       ""                       
              -1,       35,       32,       34,       -1,       -1,       19,       20,        // State: 33   M_TIMELAP           "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       36,       33,       32,       -1,       -1,       31,       32,        // State: 34   M_TIMELAP           "Period:"                "Current:\242t\004"      
              -1,       51,       35,       35,       1,        26,       19,       28,        // State: 35   M__TIMELAP_EXP      "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       37,       36,       36,       5,        25,       31,       33,        // State: 36   M__TIMELAP_PERIOD1  "Period:"                "    Set:\243t\004"      
              -1,       51,       37,       37,       6,        28,       31,       33,        // State: 37   M__TIMELAP_PERIOD2  "Period:"                "    Set:\243t\004"      
              -1,       0,        44,       39,       -1,       -1,       34,       35,        // State: 38   M_CGEM_STATUS       "RA:\242x\001"           "DE:\242x\002"           
              -1,       0,        38,       40,       -1,       -1,       36,       37,        // State: 39   M_CGEM_STATUS       "D1:\242x\031"           "D2:\242x\030"           
              -1,       0,        39,       41,       -1,       -1,       38,       39,        // State: 40   M_CGEM_STATUS       "Aligned:\242y\012"      "In Goto:\242y\013"      
              -1,       0,        40,       42,       -1,       -1,       40,       6,         // State: 41   M_CGEM_STATUS       "Tracking:\242q\026"     ""                       
              -1,       0,        41,       43,       -1,       -1,       41,       42,        // State: 42   M_CGEM_STATUS       "Moz Err:\242e\014"      "Pos Err:\242e\015"      
              -1,       0,        42,       44,       -1,       -1,       43,       44,        // State: 43   M_CGEM_STATUS       "Latency:\242E\016"      "Time-Out:\242e\011"     
              -1,       0,        43,       38,       -1,       -1,       45,       46,        // State: 44   M_CGEM_STATUS       "Histogram:  (x8)"       "\242h\023"              
              49,       0,        46,       46,       -1,       -1,       47,       48,        // State: 45   M__MOZ_RUN          "Mozaic:\242o\005"       "Time:\242m\020    sec"  
              49,       0,        45,       45,       -1,       -1,       47,       49,        // State: 46   M__MOZ_RUN          "Mozaic:\242o\005"       "Pos Id:\242p\007"       
              49,       0,        48,       48,       -1,       -1,       50,       51,        // State: 47   M__TIMELAP_RUN      "Timelap progres:"       "Shot no:\242s\032"      
              49,       0,        47,       47,       -1,       -1,       50,       52,        // State: 48   M__TIMELAP_RUN      "Timelap progres:"       "Period:\242T\033"       
              -1,       50,       49,       49,       -1,       -1,       53,       6,         // State: 49   M_CANCEL            "Really Cancel ?"        ""                       
               0,       0,        50,       50,       -1,       -1,       54,       6,         // State: 50   M_CANCELING         "Cancelling..."          ""                       
               0,       0,        51,       51,       -1,       -1,       55,       6,         // State: 51   M_EXECUTE           "Executing..."           ""                       
               0,       0,        52,       52,       -1,       -1,       56,       6,         // State: 52   M_PRESET            "Done..."                ""                       
               0,       0,        55,       54,       -1,       -1,       36,       37,        // State: 53   M_DEBUG             "D1:\242x\031"           "D2:\242x\030"           
               0,       0,        53,       55,       -1,       -1,       57,       58,        // State: 54   M_DEBUG             "D3:\242x\031"           "D4:\242x\030"           
               0,       0,        54,       53,       -1,       -1,       59,       60};       // State: 55   M_DEBUG             "D5:\242x\031"           "D6:\242x\030"           
