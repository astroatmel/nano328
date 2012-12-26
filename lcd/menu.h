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
#define M__MOZ_RUN_COUNT                2
#define M__MOZ_RUN_FIRST                16
#define M__MOZ_RUN_LAST                 17
#define M__MOZ_EXP_COUNT                1
#define M__MOZ_EXP_FIRST                18
#define M__MOZ_EXP_LAST                 18
#define M__MOZ_SPAN1_COUNT              1
#define M__MOZ_SPAN1_FIRST              19
#define M__MOZ_SPAN1_LAST               19
#define M__MOZ_SPAN2_COUNT              1
#define M__MOZ_SPAN2_FIRST              20
#define M__MOZ_SPAN2_LAST               20
#define M__MOZ_SPAN3_COUNT              1
#define M__MOZ_SPAN3_FIRST              21
#define M__MOZ_SPAN3_LAST               21
#define M_TIMELAP_COUNT                 3
#define M_TIMELAP_FIRST                 22
#define M_TIMELAP_LAST                  24
#define M__TIMELAP_RUN_COUNT            2
#define M__TIMELAP_RUN_FIRST            25
#define M__TIMELAP_RUN_LAST             26
#define M__TIMELAP_EXP_COUNT            1
#define M__TIMELAP_EXP_FIRST            27
#define M__TIMELAP_EXP_LAST             27
#define M__TIMELAP_PERIOD1_COUNT        1
#define M__TIMELAP_PERIOD1_FIRST        28
#define M__TIMELAP_PERIOD1_LAST         28
#define M__TIMELAP_PERIOD2_COUNT        1
#define M__TIMELAP_PERIOD2_FIRST        29
#define M__TIMELAP_PERIOD2_LAST         29
#define M_CGEM_STATUS_COUNT             6
#define M_CGEM_STATUS_FIRST             30
#define M_CGEM_STATUS_LAST              35
#define M_CANCEL_COUNT                  1
#define M_CANCEL_FIRST                  36
#define M_CANCEL_LAST                   36
#define M_CANCELING_COUNT               1
#define M_CANCELING_FIRST               37
#define M_CANCELING_LAST                37
#define M_EXECUTE_COUNT                 1
#define M_EXECUTE_FIRST                 38
#define M_EXECUTE_LAST                  38
#define M_PRESET_COUNT                  1
#define M_PRESET_FIRST                  39
#define M_PRESET_LAST                   39

PROGMEM const long edit_increment[] = {
              1000            ,                   //  0
              ONE_DEG_CGEM    ,                   //  1
              ONE_MIN_CGEM    ,                   //  2
              ONE_SEC_CGEM    ,                   //  3
              3600            ,                   //  4
              60              };                  //  5

PROGMEM const char linetxt[]=
              "Camera Driver     "                //  0
              "Version 2.0       "                //  1
              "RA:\242R\001          "            //  2
              "DE:\242D\002          "            //  3
              "RA:\242H\001          "            //  4
              "Mozaic Menu...    "                //  5
              "(GOTO Method)     "                //  6
              "(Pause Method)    "                //  7
              "Timelap Menu...   "                //  8
              "                  "                //  9
              "CGEM Status...    "                //  10
              "Start Mozaic...   "                //  11
              "Set Span to:      "                //  12
              "1/10 of EDGE HD   "                //  13
              "1/10 of Newton 6  "                //  14
              "Set Exposure:     "                //  15
              "1 Second          "                //  16
              "30 Second         "                //  17
              "60 Second         "                //  18
              "Exposure Time:    "                //  19
              "Current:\242s\003 sec "            //  20
              "Total Span:       "                //  21
              "Cur:\242d\017         "            //  22
              "Mozaic:\242o\005      "            //  23
              "Time:\242m\020    sec "            //  24
              "Pos Id:\242p\007      "            //  25
              "    Set:\243s\003 sec "            //  26
              "Set:\243d\017         "            //  27
              "Start Timelap...  "                //  28
              "Period:           "                //  29
              "Current:\242t\004     "            //  30
              "Timelap progres:  "                //  31
              "Shot no:\242s\032     "            //  32
              "Period:\242T\033      "            //  33
              "    Set:\243t\004     "            //  34
              "RA:\242x\001          "            //  35
              "DE:\242x\002          "            //  36
              "D1:\242x\031          "            //  37
              "D2:\242x\030          "            //  38
              "Aligned:\242y\012     "            //  39
              "In Goto:\242y\013     "            //  40
              "Moz Err:\242e\014     "            //  41
              "Pos Err:\242e\015     "            //  42
              "Latency:\242E\016     "            //  43
              "Time-Out:\242e\011    "            //  44
              "Histogram:  (x8)  "                //  45
              "\242h\023             "            //  46
              "Really Cancel ?   "                //  47
              "Cancelling...     "                //  48
              "Excuting...       ";               //  49

PROGMEM const char main_state_machine[]= {
//             ENTER    UNDO      PREV      NEXT      INCREMENT OFFSET    LINE1     LINE2  
               0,       2,        1,        1,        -1,       -1,       0,        1,         // State: 1    M_ROOT              "Camera Driver"          "Version 2.0"            
              -1,       0,        7,        3,        -1,       -1,       2,        3,         // State: 2    M_MAIN              "RA:\242R\001"           "DE:\242D\002"           
              -1,       0,        2,        4,        -1,       -1,       4,        3,         // State: 3    M_MAIN              "RA:\242H\001"           "DE:\242D\002"           
              -1,       8,        3,        5,        -1,       -1,       5,        6,         // State: 4    M_MAIN              "Mozaic Menu..."         "(GOTO Method)"          
              -1,       8,        4,        6,        -1,       -1,       5,        7,         // State: 5    M_MAIN              "Mozaic Menu..."         "(Pause Method)"         
              -1,       22,       5,        7,        -1,       -1,       8,        9,         // State: 6    M_MAIN              "Timelap Menu..."        ""                       
              -1,       30,       6,        2,        -1,       -1,       10,       9,         // State: 7    M_MAIN              "CGEM Status..."         ""                       
              -1,       16,       15,       9,        -1,       -1,       11,       9,         // State: 8    M_MOZAIC            "Start Mozaic..."        ""                       
              -1,       39,       8,        10,       -1,       -1,       12,       13,        // State: 9    M_MOZAIC            "Set Span to:"           "1/10 of EDGE HD"        
              -1,       39,       9,        11,       -1,       -1,       12,       14,        // State: 10   M_MOZAIC            "Set Span to:"           "1/10 of Newton 6"       
              -1,       39,       10,       12,       -1,       -1,       15,       16,        // State: 11   M_MOZAIC            "Set Exposure:"          "1 Second"               
              -1,       39,       11,       13,       -1,       -1,       15,       17,        // State: 12   M_MOZAIC            "Set Exposure:"          "30 Second"              
              -1,       39,       12,       14,       -1,       -1,       15,       18,        // State: 13   M_MOZAIC            "Set Exposure:"          "60 Second"              
              -1,       18,       13,       15,       -1,       -1,       19,       20,        // State: 14   M_MOZAIC            "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       19,       14,       8,        -1,       -1,       21,       22,        // State: 15   M_MOZAIC            "Total Span:"            "Cur:\242d\017"          
              36,       0,        17,       17,       -1,       -1,       23,       24,        // State: 16   M__MOZ_RUN          "Mozaic:\242o\005"       "Time:\242m\020    sec"  
              36,       0,        16,       16,       -1,       -1,       23,       25,        // State: 17   M__MOZ_RUN          "Mozaic:\242o\005"       "Pos Id:\242p\007"       
              -1,       38,       18,       18,       0,        26,       19,       26,        // State: 18   M__MOZ_EXP          "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       20,       19,       19,       1,        23,       21,       27,        // State: 19   M__MOZ_SPAN1        "Total Span:"            "Set:\243d\017"          
              -1,       21,       20,       20,       2,        26,       21,       27,        // State: 20   M__MOZ_SPAN2        "Total Span:"            "Set:\243d\017"          
              -1,       38,       21,       21,       3,        29,       21,       27,        // State: 21   M__MOZ_SPAN3        "Total Span:"            "Set:\243d\017"          
              -1,       25,       24,       23,       -1,       -1,       28,       9,         // State: 22   M_TIMELAP           "Start Timelap..."       ""                       
              -1,       27,       22,       24,       -1,       -1,       19,       20,        // State: 23   M_TIMELAP           "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       28,       23,       22,       -1,       -1,       29,       30,        // State: 24   M_TIMELAP           "Period:"                "Current:\242t\004"      
              36,       0,        26,       26,       -1,       -1,       31,       32,        // State: 25   M__TIMELAP_RUN      "Timelap progres:"       "Shot no:\242s\032"      
              36,       0,        25,       25,       -1,       -1,       31,       33,        // State: 26   M__TIMELAP_RUN      "Timelap progres:"       "Period:\242T\033"       
              -1,       38,       27,       27,       0,        26,       19,       26,        // State: 27   M__TIMELAP_EXP      "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       29,       28,       28,       4,        25,       29,       34,        // State: 28   M__TIMELAP_PERIOD1  "Period:"                "    Set:\243t\004"      
              -1,       38,       29,       29,       5,        28,       29,       34,        // State: 29   M__TIMELAP_PERIOD2  "Period:"                "    Set:\243t\004"      
              -1,       0,        35,       31,       -1,       -1,       35,       36,        // State: 30   M_CGEM_STATUS       "RA:\242x\001"           "DE:\242x\002"           
              -1,       0,        30,       32,       -1,       -1,       37,       38,        // State: 31   M_CGEM_STATUS       "D1:\242x\031"           "D2:\242x\030"           
              -1,       0,        31,       33,       -1,       -1,       39,       40,        // State: 32   M_CGEM_STATUS       "Aligned:\242y\012"      "In Goto:\242y\013"      
              -1,       0,        32,       34,       -1,       -1,       41,       42,        // State: 33   M_CGEM_STATUS       "Moz Err:\242e\014"      "Pos Err:\242e\015"      
              -1,       0,        33,       35,       -1,       -1,       43,       44,        // State: 34   M_CGEM_STATUS       "Latency:\242E\016"      "Time-Out:\242e\011"     
              -1,       0,        34,       30,       -1,       -1,       45,       46,        // State: 35   M_CGEM_STATUS       "Histogram:  (x8)"       "\242h\023"              
              -1,       37,       36,       36,       -1,       -1,       47,       9,         // State: 36   M_CANCEL            "Really Cancel ?"        ""                       
               0,       0,        37,       37,       -1,       -1,       48,       9,         // State: 37   M_CANCELING         "Cancelling..."          ""                       
               0,       0,        38,       38,       -1,       -1,       49,       9,         // State: 38   M_EXECUTE           "Excuting..."            ""                       
               0,       0,        39,       39,       -1,       -1,       9,        9};        // State: 39   M_PRESET            ""                       ""                       
