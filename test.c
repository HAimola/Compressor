#include <time.h>

#define HUFFMAN_HEADER
#define HUFFMAN_IMPLEMENTATION
#define LZ77_HEADER
#define LZ77_IMPLEMENTATION
#include "./compressor.h"

#define MAX_FEATURE_TEST_FILE_SIZE 512
#define MAX_SPEED_TEST_FILE_SIZE 1 * 1024 * 1024 // 1 MB
#define NUM_SPEED_TESTS 100

static size_t error_count = 0;
static uint8_t* feature_test_data;
static uint8_t* speed_test_data;

bool buffer_cmp(uint8_t* buf1, uint8_t* buf2, size_t length){
  assert(length);
  assert(buf1 && buf2);
  
  for(size_t i = 0; i < length; ++i){
    if(buf1[i] != buf2[i]) return 0;
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
TestResult test(const char* test_file, uint8_t* data){
  char result_file_name[128];
  sprintf(result_file_name, "%s_result", test_file);
  
  FILE* f = fopen(test_file, "r");
  FILE* f_result = fopen(result_file_name, "r");
  
  if(f == NULL || f_result == NULL){
    fprintf(stderr, "ERROR %u: Couldn't read test or result file when compiling tests!\n%s", errno, strerror(errno));
    exit(1);
  }
  
  // Computing f_size
  fseek(f, 0, SEEK_END);
  size_t f_size = (size_t)ftell(f);
  rewind(f);

  fseek(f_result, 0, SEEK_END);
  size_t f_result_size = (size_t)ftell(f_result);
  rewind(f_result);

  uint8_t* result_data = (uint8_t*)malloc(f_result_size);
  
  fread(data, f_size, 1, f);
  fread(result_data, f_result_size, 1, f_result);
  
  clock_t t_start = clock();
  size_t c_size = LZ77_compress_buffer_inplace(data, f_size);
  clock_t t_end = clock();
  
  double elapsed = (double)(t_end - t_start) / (double)CLOCKS_PER_SEC;
  bool pass = buffer_cmp(data, result_data, f_result_size);

  TestResult r = {
    .result = pass,
    .compressed_size = c_size,
    .file_size = f_size,
    .elapsed_seconds = elapsed
  };
  
  if(!pass) ++error_count;
  
  free(result_data);
  return r;
}

void test_feature(const char* test_name, const char* test_file){
  TestResult r = test(test_file, feature_test_data);

  size_t compression_ratio = (r.compressed_size*100)/(r.file_size);
  printf("[%s]\t %-25s| %zuB -> %zuB (%zu%%)\n", r.result ? "OK" : "FAIL",
	 test_name, r.file_size, r.compressed_size, compression_ratio);

}

void test_speed(const char* test_name, const char* test_file){
  double accum = 0;
  TestResult r;
  for(uint32_t i = 0; i < NUM_SPEED_TESTS; ++i){
    r = test(test_file, speed_test_data);
    accum += r.elapsed_seconds;
  }
  
  bool using_kb = true;
  size_t f_size = 0;
  if(r.file_size > 1024 * 1024){
    using_kb = false;
    f_size = r.file_size / (1024*1024);
  } else f_size = r.file_size/1024;
  
  printf("[%s]\t %-25s| %zu%s @ %f KB/s (%fs/iteration)\n", r.result ? "OK" : "FAIL", test_name,
	 f_size, using_kb ? "KB" : "MB",
	 (((double)r.file_size*NUM_SPEED_TESTS)/(accum*1024)), (accum/NUM_SPEED_TESTS));
}


int main(){
  feature_test_data = (uint8_t*)malloc(MAX_FEATURE_TEST_FILE_SIZE);
  speed_test_data = (uint8_t*)malloc(MAX_SPEED_TEST_FILE_SIZE);
  
  printf("\n------------- FEATURE TEST -------------\n");
  test_feature("Repeated Char", "./tests/char_repetition");
  test_feature("Interrupted Match", "./tests/interrupted");
  test_feature("No match", "./tests/no_match");
  test_feature("Maximum match length", "./tests/maximum_length"); 
 
  printf("----------------------------------------");
  size_t feature_error_count = error_count;
  error_count = 0;
  
  printf("\n-------------- SPEED TEST --------------\n");
  test_speed("Alice in Wonderland", "./tests/alice");
  //test_speed("Random Data (10MB)", "./tests/random_data_small");
  //test_speed("Random Data (100MB)", "./tests/random_data_big");
  printf("----------------------------------------");
  
  printf("\nEnded feature test with %zu errors.\n", feature_error_count);
  printf("\nEnded speed test with %d iterations/test with %zu errors.\n", NUM_SPEED_TESTS, error_count);
  
  free(feature_test_data);
  free(speed_test_data);
  return 0;
}

