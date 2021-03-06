const char *MODEL_NAME                             = "Intel(R) Xeon(R) CPU E5-2630 v3 @ 2.40GHz";
const unsigned int PHYSICAL_CHIPS                  = 2;
const unsigned int CORE_PER_CHIP                   = 8;
const unsigned int SMT_LEVEL                       = 2;
const unsigned int CACHE_PER_CORE                  = 20480;
const unsigned int seq_cores[]                     = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
const unsigned int rr_cores[]                      = {0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15,16,24,17,25,18,26,19,27,20,28,21,29,22,30,23,31};
const unsigned int test_hw_thr_cnts_fine_grain[]   = {1,2,4,8,12,16,24,32};
const unsigned int test_hw_thr_cnts_coarse_grain[] = {1,2,4,8,16,24,32};
