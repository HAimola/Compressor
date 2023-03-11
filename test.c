#define HUFFMAN_HEADER
#define HUFFMAN_IMPLEMENTATION
#define LZ77_HEADER
#define LZ77_IMPLEMENTATION
#include "./compressor.h"

#define MAX_FEATURE_TEST_FILE_SIZE 512
#define MAX_SPEED_TEST_FILE_SIZE 200000

static size_t error_count = 0;
static uint8_t* data;
static uint8_t* result_data;

bool buffer_cmp(uint8_t* buf1, uint8_t* buf2, size_t length){
  assert(length);
  assert(buf1 && buf2);
  
  for(size_t i = 0; i < length; ++i){
    if(buf1[i] != buf2[i]) return 0;
  }
  return true;
}

// Compares compression algorithm with known and correct results in
// the text files. The files with the "_result" suffix were created
// beforehand.
void test_feature(const char* test_name, const char* test_file){
  char result_file_name[128];
  sprintf(result_file_name, "%s_result", test_file);
  
  FILE* f = fopen(test_file, "r");
  FILE* f_r = fopen(result_file_name, "r");
  
  if(f == NULL || f_r == NULL){
    fprintf(stderr, "ERROR %u: Couldn't read test or result file when compiling tests!\n%s", errno, strerror(errno));
    exit(1);
  }
  
  // Computing f_size
  fseek(f, 0, SEEK_END);
  size_t f_size = (size_t)ftell(f);
  rewind(f);

  fseek(f_r, 0, SEEK_END);
  size_t f_r_size = (size_t)ftell(f_r);
  rewind(f_r);
  
  fread(data, f_size, 1, f);
  fread(result_data, f_r_size, 1, f_r);
  size_t c_size = LZ77_compress_buffer_inplace(data, f_size);
  bool result = buffer_cmp(data, result_data, f_r_size);
  
  printf("[%s]\t %-25s| %zuB -> %zuB (%zu%%)\n", result ? "OK" : "FAIL",
	 test_name, f_size, c_size, (c_size*100)/(f_size));
  if(!result) ++error_count;
}


int main(){
  data = (uint8_t*)malloc(MAX_FEATURE_TEST_FILE_SIZE);
  result_data = (uint8_t*)malloc(MAX_FEATURE_TEST_FILE_SIZE);

  printf("\n----- FEATURE TEST -----\n");
  test_feature("Repeated Char", "./tests/char_repetition");
  test_feature("Interrupted Match", "./tests/interrupted");
  test_feature("No match", "./tests/no_match");
  test_feature("Maximum match length", "./tests/maximum_length"); 
 
  printf("----------------------------------------");
  printf("\nEnded feature test with %zu errors.\n", error_count);
  
  //test_speed("Alice in Wonderland", "./tests/alice");

  
  free(data);
  free(result_data);
  return 0;
}
