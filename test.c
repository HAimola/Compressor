#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* #define HUFFMAN_HEADER */
/* #define HUFFMAN_IMPLEMENTATION */
#define LZ77_IMPLEMENTATION
#include "./compressor.h"

#define MAX_FEATURE_TEST_FILE_SIZE 512
#define MAX_SPEED_TEST_FILE_SIZE 110 * 1024 * 1024 // 1 MB
#define NUM_SPEED_TESTS 10

static size_t error_count = 0;
static uint8_t* feature_test_buff;
static uint8_t* speed_test_buff;
static uint8_t* result_buff;

bool buffer_cmp(uint8_t* buf1, uint8_t* buf2, size_t length){
  assert(length);
  assert(buf1 && buf2);
  
  for(size_t i = 0; i < length; ++i){
    if(buf1[i] != buf2[i]) return false;
  }
  return true;
}

typedef struct {
  bool result;
  size_t compressed_size;
  size_t file_size;
  double elapsed_seconds;
} TestResult;


// Compares compression algorithm with known and correct results in
// the text files. The files with the "_result" suffix were created
// beforehand.
TestResult LZ77_test(const char* test_file, uint8_t* buff){
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
  
  // Computing f_size
  fseek(f, 0, SEEK_END);
  size_t f_size = (size_t)ftell(f);
  rewind(f);

  fseek(f_result, 0, SEEK_END);
  size_t f_result_size = (size_t)ftell(f_result);
  rewind(f_result);
  
  fread(buff, f_size, 1, f);
  fread(result_buff, f_result_size, 1, f_result);
  
  clock_t t_start = clock();
  size_t c_size = LZ77_compress_buffer_inplace(buff, f_size);
  clock_t t_end = clock();

  //for(size_t i = 0; i < c_size; ++i) putc(buff[i], stdout);
  
  double elapsed = (double)(t_end - t_start) / (double)CLOCKS_PER_SEC;
  bool pass = buffer_cmp(buff, result_buff, f_result_size);

  TestResult r = {
    .result = pass,
    .compressed_size = c_size,
    .file_size = f_size,
    .elapsed_seconds = elapsed
  };

  fclose(f_result);
  fclose(f);
  
  if(!pass) ++error_count;
  return r;
}

void test_feature(const char* test_name, const char* test_file){
  TestResult r = LZ77_test(test_file, feature_test_buff);

  size_t compression_ratio = ((r.file_size - r.compressed_size)*100)/(r.file_size);
  printf("[%s]\t %-25s| %zuB -> %zuB (%zu%%)\n", r.result ? "OK" : "FAIL",
	 test_name, r.file_size, r.compressed_size, compression_ratio);

  if(!r.result){
    char log_filepath[128];
    sprintf(log_filepath, "%s_error_log", test_file);
    FILE *log_f = fopen(log_filepath, "w");
    
    fprintf(log_f, "[LZ77 TEST ERROR LOG] Test %s, filepath %s\n", test_name, test_file);
    fprintf(log_f, "RESULT: \n");
    for(size_t i = 0; i < r.compressed_size; ++i) putc(feature_test_buff[i], log_f);
    fprintf(log_f, "\n");
    fclose(log_f);
  }
}

void test_speed(const char* test_name, const char* test_file){
  double accum = 0;
  TestResult r;
  for(uint32_t i = 0; i < NUM_SPEED_TESTS; ++i){
    r = LZ77_test(test_file, speed_test_buff);
    accum += r.elapsed_seconds;
  }
  
  printf("[%s]\t %-25s| %.3fMB @ %f KB/s (%fs/iteration)\n",
	 r.result ? "OK" : "FAIL", test_name, (double)r.compressed_size/(1024*1024),
	 (((double)r.file_size*NUM_SPEED_TESTS)/(accum*1024)), (accum/NUM_SPEED_TESTS));
}


int main(){
  feature_test_buff = (uint8_t*)malloc(MAX_FEATURE_TEST_FILE_SIZE);
  speed_test_buff = (uint8_t*)malloc(MAX_SPEED_TEST_FILE_SIZE);
  result_buff = (uint8_t*)malloc(MAX_SPEED_TEST_FILE_SIZE);

  printf("Feature Test:\n");
  test_feature("Repeated Char", "./tests/char_repetition");
  test_feature("Interrupted Match", "./tests/interrupted");
  test_feature("No match", "./tests/no_match");
  test_feature("Maximum match length", "./tests/maximum_length");

  size_t feature_error_count = error_count;
  error_count = 0;

  printf("\nSpeed Test:\n");
  test_speed("Alice in Wonderland", "./tests/alice");
  /* test_speed("Random Data (10MB)", "./tests/random_data_small"); */
  /* test_speed("Random Data (100MB)", "./tests/random_data_big"); */
  /* test_speed("Long Repeated Char (10MB)", "./tests/long_repeated_char"); */
  
  printf("\nEnded feature test with %zu errors.", feature_error_count);
  printf("\nEnded speed test with %d iterations/test with %zu errors.\n", NUM_SPEED_TESTS, error_count);
 
  free(feature_test_buff);
  free(speed_test_buff);
  free(result_buff);
  return 0;
}
