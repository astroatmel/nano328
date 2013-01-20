// This file was auto generated from menu.def and menu.awk
// cat menu.def | awk -f menu.awk | tee menu.h
#define M_ROOT_COUNT                    1
#define M_ROOT_FIRST                    1
#define M_ROOT_LAST                     1
#define M_MAIN_COUNT                    9
#define M_MAIN_FIRST                    2
#define M_MAIN_LAST                     10
#define M_MOZAIC_PAUSE_COUNT            10
#define M_MOZAIC_PAUSE_FIRST            11
#define M_MOZAIC_PAUSE_LAST             20
#define M__MOZ_RA_COUNT                 1
#define M__MOZ_RA_FIRST                 21
#define M__MOZ_RA_LAST                  21
#define M__MOZ_DE_COUNT                 1
#define M__MOZ_DE_FIRST                 22
#define M__MOZ_DE_LAST                  22
#define M_MOZAIC_COUNT                  8
#define M_MOZAIC_FIRST                  23
#define M_MOZAIC_LAST                   30
#define M__MOZ_EXP_COUNT                1
#define M__MOZ_EXP_FIRST                31
#define M__MOZ_EXP_LAST                 31
#define M__MOZ_SPAN1_COUNT              1
#define M__MOZ_SPAN1_FIRST              32
#define M__MOZ_SPAN1_LAST               32
#define M__MOZ_SPAN2_COUNT              1
#define M__MOZ_SPAN2_FIRST              33
#define M__MOZ_SPAN2_LAST               33
#define M__MOZ_SPAN3_COUNT              1
#define M__MOZ_SPAN3_FIRST              34
#define M__MOZ_SPAN3_LAST               34
#define M_CATALOG_COUNT                 5
#define M_CATALOG_FIRST                 35
#define M_CATALOG_LAST                  39
#define M_STORE_COUNT                   5
#define M_STORE_FIRST                   40
#define M_STORE_LAST                    44
#define M_RECALL_COUNT                  5
#define M_RECALL_FIRST                  45
#define M_RECALL_LAST                   49
#define M_TIMELAP_COUNT                 3
#define M_TIMELAP_FIRST                 50
#define M_TIMELAP_LAST                  52
#define M__TIMELAP_EXP_COUNT            1
#define M__TIMELAP_EXP_FIRST            53
#define M__TIMELAP_EXP_LAST             53
#define M__TIMELAP_PERIOD1_COUNT        1
#define M__TIMELAP_PERIOD1_FIRST        54
#define M__TIMELAP_PERIOD1_LAST         54
#define M__TIMELAP_PERIOD2_COUNT        1
#define M__TIMELAP_PERIOD2_FIRST        55
#define M__TIMELAP_PERIOD2_LAST         55
#define M_CGEM_STATUS_COUNT             7
#define M_CGEM_STATUS_FIRST             56
#define M_CGEM_STATUS_LAST              62
#define M__MOZ_RUN_COUNT                2
#define M__MOZ_RUN_FIRST                63
#define M__MOZ_RUN_LAST                 64
#define M__TIMELAP_RUN_COUNT            2
#define M__TIMELAP_RUN_FIRST            65
#define M__TIMELAP_RUN_LAST             66
#define M_CANCEL_COUNT                  1
#define M_CANCEL_FIRST                  67
#define M_CANCEL_LAST                   67
#define M_CANCELING_COUNT               1
#define M_CANCELING_FIRST               68
#define M_CANCELING_LAST                68
#define M_GOTO_COUNT                    1
#define M_GOTO_FIRST                    69
#define M_GOTO_LAST                     69
#define M_EXECUTE_COUNT                 1
#define M_EXECUTE_FIRST                 70
#define M_EXECUTE_LAST                  70
#define M_PRESET_COUNT                  1
#define M_PRESET_FIRST                  71
#define M_PRESET_LAST                   71
#define M_DEBUG_COUNT                   3
#define M_DEBUG_FIRST                   72
#define M_DEBUG_LAST                    74

PROGMEM const long edit_increment[] = {
              1               ,                   //  0
              1000            ,                   //  1
              ONE_DEG_CGEM    ,                   //  2
              ONE_MIN_CGEM    ,                   //  3
              ONE_SEC_CGEM    ,                   //  4
              3600            ,                   //  5
              60              };                  //  6

PROGMEM const char linetxt[]=
              "Cam Driver V2   "                  //  0   0
              "Ram:\242X\031         "            //  1   2
              "Mozaic Menu...  "                  //  2   0
              "(GOTO Method)   "                  //  3   0
              "(Pause Method)  "                  //  4   0
              "Catalog Goto... "                  //  5   0
              "                "                  //  6   0
              "Store           "                  //  7   0
              "Position...     "                  //  8   0
              "Recall          "                  //  9   0
              "Timelap Menu... "                  //  10   0
              "CGEM Status...  "                  //  11   0
              "RA:\242R\001          "            //  12   2
              "DE:\242D\002          "            //  13   2
              "RA:\242H\001          "            //  14   2
              "Start Mozaic... "                  //  15   0
              "Set Span to:    "                  //  16   0
              "1/10 of EDGE HD "                  //  17   0
              "1/10 of Newton 6"                  //  18   0
              "Set Exposure:   "                  //  19   0
              "1 Second        "                  //  20   0
              "30 Second       "                  //  21   0
              "60 Second       "                  //  22   0
              "Exposure Time:  "                  //  23   0
              "Current:\242s\003 sec "            //  24   2
              "Total Span:     "                  //  25   0
              "Cur:\242d\017         "            //  26   2
              "Nb Pic:         "                  //  27   0
              "Current:\242n\044 (RA)"            //  28   2
              "Current:\242n\045 (DE)"            //  29   2
              "    Set:\243n\044 (RA)"            //  30   2
              "    Set:\243n\045 (DE)"            //  31   2
              "    Set:\243s\003 sec "            //  32   2
              "Set:\243d\017         "            //  33   2
              "Goto:           "                  //  34   0
              "Orion Fire      "                  //  35   0
              "Orion M42       "                  //  36   0
              "M101            "                  //  37   0
              "Triangulum      "                  //  38   0
              "Andromeda       "                  //  39   0
              "Store:          "                  //  40   0
              "Pos 1           "                  //  41   0
              "Pos 2           "                  //  42   0
              "Pos 3           "                  //  43   0
              "Pos 4           "                  //  44   0
              "Pos 5           "                  //  45   0
              "Recall:         "                  //  46   0
              "Start Timelap..."                  //  47   0
              "Period:         "                  //  48   0
              "Current:\242t\004     "            //  49   2
              "    Set:\243t\004     "            //  50   2
              "RA:\242x\001          "            //  51   2
              "DE:\242x\002          "            //  52   2
              "D1:\242x\031          "            //  53   2
              "D2:\242x\030          "            //  54   2
              "Aligned:\242y\012     "            //  55   2
              "In Goto:\242y\013     "            //  56   2
              "Tracking:\242q\026    "            //  57   2
              "Moz Err:\242e\014     "            //  58   2
              "Pos Err:\242e\015     "            //  59   2
              "Latency:\242E\016     "            //  60   2
              "Time-Out:\242e\011    "            //  61   2
              "Histogram:  (x8)"                  //  62   0
              "\242h\023             "            //  63   2
              "Mozaic:\242o\005      "            //  64   2
              "Time:\242m\020    sec "            //  65   2
              "Pos Id:\242p\007      "            //  66   2
              "Timelap progres:"                  //  67   0
              "Shot no:\242s\032     "            //  68   2
              "Period:\242T\033      "            //  69   2
              "Really Cancel ? "                  //  70   0
              "Cancelling...   "                  //  71   0
              "Goto            "                  //  72   0
              "in Progress...  "                  //  73   0
              "Executing...    "                  //  74   0
              "Done...         "                  //  75   0
              "D3:\242x\031          "            //  76   2
              "D4:\242x\030          "            //  77   2
              "D5:\242x\031          "            //  78   2
              "D6:\242x\030          ";           //  79   2

#define MENU_TABLE_WIDTH 8
#define MENU_TABLE_LEN   74
PROGMEM const char menu_state_machine[]= {  0,0,0,0,0,0,0,0,0, // STATE : 0 is not a valid state, it means 'do nothing'  > there is one more element so that the array starts at 1
//            UNDO      ENTER     PREV      NEXT      INCREMENT OFFSET    LINE1     LINE2  
               0,       2,        1,        1,        -1,       -1,       0,        1,         // State: 1    M_ROOT              "Cam Driver V2"          "Ram:\242X\031"          
              -1,       23,       10,       3,        -1,       -1,       2,        3,         // State: 2    M_MAIN              "Mozaic Menu..."         "(GOTO Method)"          
              -1,       11,       2,        4,        -1,       -1,       2,        4,         // State: 3    M_MAIN              "Mozaic Menu..."         "(Pause Method)"         
              -1,       35,       3,        5,        -1,       -1,       5,        6,         // State: 4    M_MAIN              "Catalog Goto..."        ""                       
              -1,       40,       4,        6,        -1,       -1,       7,        8,         // State: 5    M_MAIN              "Store"                  "Position..."            
              -1,       45,       5,        7,        -1,       -1,       9,        8,         // State: 6    M_MAIN              "Recall"                 "Position..."            
              -1,       50,       6,        8,        -1,       -1,       10,       6,         // State: 7    M_MAIN              "Timelap Menu..."        ""                       
              -1,       56,       7,        9,        -1,       -1,       11,       6,         // State: 8    M_MAIN              "CGEM Status..."         ""                       
              -1,       0,        8,        10,       -1,       -1,       12,       13,        // State: 9    M_MAIN              "RA:\242R\001"           "DE:\242D\002"           
              -1,       0,        9,        2,        -1,       -1,       14,       13,        // State: 10   M_MAIN              "RA:\242H\001"           "DE:\242D\002"           
              -1,       63,       20,       12,       -1,       -1,       15,       4,         // State: 11   M_MOZAIC_PAUSE      "Start Mozaic..."        "(Pause Method)"         
              -1,       71,       11,       13,       -1,       -1,       16,       17,        // State: 12   M_MOZAIC_PAUSE      "Set Span to:"           "1/10 of EDGE HD"        
              -1,       71,       12,       14,       -1,       -1,       16,       18,        // State: 13   M_MOZAIC_PAUSE      "Set Span to:"           "1/10 of Newton 6"       
              -1,       71,       13,       15,       -1,       -1,       19,       20,        // State: 14   M_MOZAIC_PAUSE      "Set Exposure:"          "1 Second"               
              -1,       71,       14,       16,       -1,       -1,       19,       21,        // State: 15   M_MOZAIC_PAUSE      "Set Exposure:"          "30 Second"              
              -1,       71,       15,       17,       -1,       -1,       19,       22,        // State: 16   M_MOZAIC_PAUSE      "Set Exposure:"          "60 Second"              
              -1,       31,       16,       18,       -1,       -1,       23,       24,        // State: 17   M_MOZAIC_PAUSE      "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       32,       17,       19,       -1,       -1,       25,       26,        // State: 18   M_MOZAIC_PAUSE      "Total Span:"            "Cur:\242d\017"          
              -1,       21,       18,       20,       -1,       -1,       27,       28,        // State: 19   M_MOZAIC_PAUSE      "Nb Pic:"                "Current:\242n\044 (RA)" 
              -1,       22,       19,       11,       -1,       -1,       27,       29,        // State: 20   M_MOZAIC_PAUSE      "Nb Pic:"                "Current:\242n\045 (DE)" 
              -1,       70,       21,       21,       0,        26,       27,       30,        // State: 21   M__MOZ_RA           "Nb Pic:"                "    Set:\243n\044 (RA)" 
              -1,       70,       22,       22,       0,        26,       27,       31,        // State: 22   M__MOZ_DE           "Nb Pic:"                "    Set:\243n\045 (DE)" 
              -1,       63,       30,       24,       -1,       -1,       15,       3,         // State: 23   M_MOZAIC            "Start Mozaic..."        "(GOTO Method)"          
              -1,       71,       23,       25,       -1,       -1,       16,       17,        // State: 24   M_MOZAIC            "Set Span to:"           "1/10 of EDGE HD"        
              -1,       71,       24,       26,       -1,       -1,       16,       18,        // State: 25   M_MOZAIC            "Set Span to:"           "1/10 of Newton 6"       
              -1,       71,       25,       27,       -1,       -1,       19,       20,        // State: 26   M_MOZAIC            "Set Exposure:"          "1 Second"               
              -1,       71,       26,       28,       -1,       -1,       19,       21,        // State: 27   M_MOZAIC            "Set Exposure:"          "30 Second"              
              -1,       71,       27,       29,       -1,       -1,       19,       22,        // State: 28   M_MOZAIC            "Set Exposure:"          "60 Second"              
              -1,       31,       28,       30,       -1,       -1,       23,       24,        // State: 29   M_MOZAIC            "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       32,       29,       23,       -1,       -1,       25,       26,        // State: 30   M_MOZAIC            "Total Span:"            "Cur:\242d\017"          
              -1,       70,       31,       31,       1,        26,       23,       32,        // State: 31   M__MOZ_EXP          "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       33,       32,       32,       2,        23,       25,       33,        // State: 32   M__MOZ_SPAN1        "Total Span:"            "Set:\243d\017"          
              -1,       34,       33,       33,       3,        26,       25,       33,        // State: 33   M__MOZ_SPAN2        "Total Span:"            "Set:\243d\017"          
              -1,       70,       34,       34,       4,        29,       25,       33,        // State: 34   M__MOZ_SPAN3        "Total Span:"            "Set:\243d\017"          
              -1,       69,       39,       36,       -1,       -1,       34,       35,        // State: 35   M_CATALOG           "Goto:"                  "Orion Fire"             
              -1,       69,       35,       37,       -1,       -1,       34,       36,        // State: 36   M_CATALOG           "Goto:"                  "Orion M42"              
              -1,       69,       36,       38,       -1,       -1,       34,       37,        // State: 37   M_CATALOG           "Goto:"                  "M101"                   
              -1,       69,       37,       39,       -1,       -1,       34,       38,        // State: 38   M_CATALOG           "Goto:"                  "Triangulum"             
              -1,       69,       38,       35,       -1,       -1,       34,       39,        // State: 39   M_CATALOG           "Goto:"                  "Andromeda"              
              -1,       70,       44,       41,       -1,       -1,       40,       41,        // State: 40   M_STORE             "Store:"                 "Pos 1"                  
              -1,       70,       40,       42,       -1,       -1,       40,       42,        // State: 41   M_STORE             "Store:"                 "Pos 2"                  
              -1,       70,       41,       43,       -1,       -1,       40,       43,        // State: 42   M_STORE             "Store:"                 "Pos 3"                  
              -1,       70,       42,       44,       -1,       -1,       40,       44,        // State: 43   M_STORE             "Store:"                 "Pos 4"                  
              -1,       70,       43,       40,       -1,       -1,       40,       45,        // State: 44   M_STORE             "Store:"                 "Pos 5"                  
              -1,       70,       49,       46,       -1,       -1,       46,       41,        // State: 45   M_RECALL            "Recall:"                "Pos 1"                  
              -1,       70,       45,       47,       -1,       -1,       46,       42,        // State: 46   M_RECALL            "Recall:"                "Pos 2"                  
              -1,       70,       46,       48,       -1,       -1,       46,       43,        // State: 47   M_RECALL            "Recall:"                "Pos 3"                  
              -1,       70,       47,       49,       -1,       -1,       46,       44,        // State: 48   M_RECALL            "Recall:"                "Pos 4"                  
              -1,       70,       48,       45,       -1,       -1,       46,       45,        // State: 49   M_RECALL            "Recall:"                "Pos 5"                  
              -1,       65,       52,       51,       -1,       -1,       47,       6,         // State: 50   M_TIMELAP           "Start Timelap..."       ""                       
              -1,       53,       50,       52,       -1,       -1,       23,       24,        // State: 51   M_TIMELAP           "Exposure Time:"         "Current:\242s\003 sec"  
              -1,       54,       51,       50,       -1,       -1,       48,       49,        // State: 52   M_TIMELAP           "Period:"                "Current:\242t\004"      
              -1,       70,       53,       53,       1,        26,       23,       32,        // State: 53   M__TIMELAP_EXP      "Exposure Time:"         "    Set:\243s\003 sec"  
              -1,       55,       54,       54,       5,        25,       48,       50,        // State: 54   M__TIMELAP_PERIOD1  "Period:"                "    Set:\243t\004"      
              -1,       70,       55,       55,       6,        28,       48,       50,        // State: 55   M__TIMELAP_PERIOD2  "Period:"                "    Set:\243t\004"      
              -1,       0,        62,       57,       -1,       -1,       51,       52,        // State: 56   M_CGEM_STATUS       "RA:\242x\001"           "DE:\242x\002"           
              -1,       0,        56,       58,       -1,       -1,       53,       54,        // State: 57   M_CGEM_STATUS       "D1:\242x\031"           "D2:\242x\030"           
              -1,       0,        57,       59,       -1,       -1,       55,       56,        // State: 58   M_CGEM_STATUS       "Aligned:\242y\012"      "In Goto:\242y\013"      
              -1,       0,        58,       60,       -1,       -1,       57,       6,         // State: 59   M_CGEM_STATUS       "Tracking:\242q\026"     ""                       
              -1,       0,        59,       61,       -1,       -1,       58,       59,        // State: 60   M_CGEM_STATUS       "Moz Err:\242e\014"      "Pos Err:\242e\015"      
              -1,       0,        60,       62,       -1,       -1,       60,       61,        // State: 61   M_CGEM_STATUS       "Latency:\242E\016"      "Time-Out:\242e\011"     
              -1,       0,        61,       56,       -1,       -1,       62,       63,        // State: 62   M_CGEM_STATUS       "Histogram:  (x8)"       "\242h\023"              
              67,       0,        64,       64,       -1,       -1,       64,       65,        // State: 63   M__MOZ_RUN          "Mozaic:\242o\005"       "Time:\242m\020    sec"  
              67,       0,        63,       63,       -1,       -1,       64,       66,        // State: 64   M__MOZ_RUN          "Mozaic:\242o\005"       "Pos Id:\242p\007"       
              67,       0,        66,       66,       -1,       -1,       67,       68,        // State: 65   M__TIMELAP_RUN      "Timelap progres:"       "Shot no:\242s\032"      
              67,       0,        65,       65,       -1,       -1,       67,       69,        // State: 66   M__TIMELAP_RUN      "Timelap progres:"       "Period:\242T\033"       
              -1,       68,       67,       67,       -1,       -1,       70,       6,         // State: 67   M_CANCEL            "Really Cancel ?"        ""                       
               0,       0,        68,       68,       -1,       -1,       71,       6,         // State: 68   M_CANCELING         "Cancelling..."          ""                       
               0,       0,        69,       69,       -1,       -1,       72,       73,        // State: 69   M_GOTO              "Goto"                   "in Progress..."         
               0,       0,        70,       70,       -1,       -1,       74,       6,         // State: 70   M_EXECUTE           "Executing..."           ""                       
               0,       0,        71,       71,       -1,       -1,       75,       6,         // State: 71   M_PRESET            "Done..."                ""                       
               0,       0,        74,       73,       -1,       -1,       53,       54,        // State: 72   M_DEBUG             "D1:\242x\031"           "D2:\242x\030"           
               0,       0,        72,       74,       -1,       -1,       76,       77,        // State: 73   M_DEBUG             "D3:\242x\031"           "D4:\242x\030"           
               0,       0,        73,       72,       -1,       -1,       78,       79};       // State: 74   M_DEBUG             "D5:\242x\031"           "D6:\242x\030"           
