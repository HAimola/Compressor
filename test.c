#include <time.h>

#define LZ77_IMPLEMENTATION
#include "./compressor.h"

#define MAX_TEST_FILE_SIZE 10 * 1024 * 1024 // 1 MB
#define NUM_SPEED_TESTS 1

// Return value of memcmp when both buffers are equal
#define EQUAL 0

// Counts the error in the feature and speed test phases
static size_t error_count = 0;

// Allocated buffers that store the input and result data
static uint8_t* input_buff;
static uint8_t* result_buff;

typedef struct {
  bool result;
  size_t compressed_size;
  size_t original_file_size;
  double elapsed_seconds;
} TestResult;

// Compares compression algorithm with known results.
// The files with the "_result" suffix were created supposing that 
// the output of the program was at least once correct.
TestResult LZ77_test(const char* test_file){
  // Appeding _result to open the result file
  char result_file[128];
  sprintf(result_file, "%s_result", test_file);
  
  FILE* f = fopen(test_file, "r");
  FILE* f_result = fopen(result_file, "r");
  
  if(f == NULL || f_result == NULL){
    fprintf(stderr, "ERROR %u in LZ77_test\n"
	    "Couldn't read test (%s) or result file when compiling tests.\n%s", errno,
	    test_file, strerror(errno));
    exit(1);
  }

  // Finding the size of the input and result files
  fseek(f, 0, SEEK_END);
  size_t f_size = (size_t)ftell(f);
  rewind(f);

  fseek(f_result, 0, SEEK_END);
  size_t f_result_size = (size_t)ftell(f_result);
  rewind(f_result);
  
  fread(input_buff, f_size, 1, f);
  fread(result_buff, f_result_size, 1, f_result);
  
  clock_t t_start = clock();
  size_t c_size = LZ77_compress_buffer_inplace(input_buff, f_size);
  clock_t t_end = clock();
  
  double elapsed = (double)(t_end - t_start) / (double)CLOCKS_PER_SEC;
  bool pass = memcmp(input_buff, result_buff, f_result_size) == EQUAL ? true : false; 

  TestResult r = {
    .result = pass,
    .compressed_size = c_size,
    .original_file_size = f_size,
    .elapsed_seconds = elapsed
  };

  fclose(f_result);
  fclose(f);
  
  if(!pass) ++error_count;
  return r;
}

void test_feature(const char* test_name, const char* test_file){
  TestResult r = LZ77_test(test_file);

  size_t compression_ratio = ((r.original_file_size - r.compressed_size)*100)/(r.original_file_size);
  printf("[%s]\t %-25s| %zuB -> %zuB (%zu%%)\n", r.result ? "OK" : "FAIL",
	 test_name, r.original_file_size, r.compressed_size, compression_ratio);
}

void test_speed(const char* test_name, const char* test_file){
  double accum = 0;
  TestResult r;
  for(uint32_t i = 0; i < NUM_SPEED_TESTS; ++i){
    r = LZ77_test(test_file);
    accum += r.elapsed_seconds;
  }
  
  printf("[%s]\t %-25s| %.3fMB @ %f KB/s (%fs/iteration)\n",
	 r.result ? "OK" : "FAIL", test_name, (double)r.compressed_size/(1024*1024),
	 (((double)r.original_file_size*NUM_SPEED_TESTS)/(accum*1024)), (accum/NUM_SPEED_TESTS));
}

int main(){
  input_buff = (uint8_t*)malloc(MAX_TEST_FILE_SIZE);
  result_buff = (uint8_t*)malloc(MAX_TEST_FILE_SIZE);

#ifdef TEST_FEATURE
  printf("Feature Test:\n");
  test_feature("Repeated Char", "./tests/char_repetition");
  test_feature("Interrupted Match", "./tests/interrupted");
  test_feature("No match", "./tests/no_match");
  test_feature("Maximum match length", "./tests/maximum_length");

  size_t feature_error_count = error_count;
  error_count = 0;
#endif // TEST_FEATURE

#ifdef TEST_SPEED
  printf("\nSpeed Test:\n");
  test_speed("Alice in Wonderland", "./tests/alice");
  /* test_speed("Random Data (10MB)", "./tests/random_data_small"); */
  /* test_speed("Random Data (100MB)", "./tests/random_data_big"); */
  /* test_speed("Long Repeated Char (10MB)", "./tests/long_repeated_char"); */
#endif // TEST_SPEED
  
#ifdef TEST_FEATURE
  printf("\nEnded feature test with %zu errors.", feature_error_count);
#endif
#ifdef TEST_SPEED
  printf("\nEnded speed test with %d iterations/test with %zu errors.\n", NUM_SPEED_TESTS, error_count);
#endif
  
  free(input_buff);
  free(result_buff);
  return 0;
}
