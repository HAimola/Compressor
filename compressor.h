/*
  Library for data compression utilities
  License information at the end of the file.
  
  This library includes 2 main methods of compressing information:
     * Huffman coding
     * LZ77 algorithm
     @@@ TODO: DEFLATE algorithn
     
  I tried my best to follow these closest to the standard - if available - as
  possible.

 */

#ifdef HUFFMAN_HEADER

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

typedef struct Node Node;

Node Huffman_tree_create_from_file(const char* filepath);
void Huffman_save_encoded_file(const char* filepath, Node* root);
char Huffman_get_value_from_str(Node* root, const char* code);
char Huffman_get_value_from_binary(Node* root, size_t code);

void Huffman_get_code_string(Node* root, char key, char* code);
size_t Huffman_get_code_binary(Node* root, char key, size_t* code);
void Huffman_print_code(Node* root, char* current_code, size_t curr_position);

#endif // HUFFMAN_HEADER
#ifdef HUFFMAN_IMPLEMENTATION

struct Node{
  char value;
  size_t frequency;
  Node* left;
  Node* right;
};

// @@@ OPTIMIZATION: Implement better sorting algorithm
void bubble_sort(Node* node_arr, size_t length, bool reversed){
  for(size_t i = 0; i < length; ++i){
    for(size_t j = i+1; j < length; ++j){
      if((node_arr[j].frequency < node_arr[i].frequency) ^ reversed){
	Node tmp = node_arr[j];
	node_arr[j] = node_arr[i];
	node_arr[i] = tmp;
      }
    } 
  }
}

// Wrapper function around the sorting algorithm of choicep
// Currently, only bubblesort is implemented because I'm lazy
void sort_node_array(Node* arr, size_t length, bool reversed){
  bubble_sort(arr, length, reversed);
}

#define debug_freq_print(n)
//#define debug_freq_print(n) Huffman_print_frequencies(n)
/* void Huffman_print_frequencies(Node n){ */
/*   if(n.left == NULL && n.right == NULL) { */
/*     printf("[FREQ]%c: %zu\n", n.value, n.frequency); */
/*     return; */
/*   } */
/*   Huffman_print_frequencies(*n.left); */
/*   Huffman_print_frequencies(*n.right); */
/* } */

// @@@ OPTIMIZATION: Profile performance of stack-based tree generation
// There are 256 possible symbols for a byte
#define SYM_COUNT 256
static size_t frequency_buckets[SYM_COUNT];

// The nodes array is going to store all nodes while the tree is built
// There is space for 257 because every time we create a new parent node,
// we need to remove the children nodes from the sorting array, but at the same
// time we need to keep the references to the children somewhere in memory.
// The solution I found to avoid mallocs is to push children nodes to the end of the
// "node stack" and sort the array only up to the point before where the children are
static Node nodes[SYM_COUNT + 1];

Node Huffman_create_tree_from_file(const char* filepath){
  FILE* f = fopen(filepath, "r");
  if(f == NULL){
    fprintf(stderr, "ERROR %u: Couldn't read file specified!\n%s", errno, strerror(errno));
  }

  uint8_t buff[512];
  int64_t bytes_read = (int64_t)fread(buff, sizeof(uint8_t), 512, f);

  // Getting frequencies of each symbol in text file
  while(bytes_read != 0){
    for(uint32_t i = 0; i < bytes_read; ++i){
      ++frequency_buckets[buff[i]];
    }
    bytes_read = (int64_t)fread(buff, sizeof(uint8_t), 512, f);
  }
  
  size_t length = 0;

  // Generating a node without children for every symbol in the text
  for(uint32_t i = 0; i < SYM_COUNT; ++i){
    size_t freq = frequency_buckets[i];
    if(freq == 0) continue;

    Node n = {
      .value = (char)i,
      .frequency = freq,
      .left = NULL,
      .right = NULL,
    };
     
    nodes[length] = n;
    ++length;
  }

  sort_node_array(nodes, length, true);
  uint32_t node_stack_pointer = SYM_COUNT;

  // Generate a tree with every node with a frequency > 0
  for(size_t i = length-1; i > 0; --i){
    
    nodes[node_stack_pointer--] = nodes[length-1];
    nodes[node_stack_pointer--] = nodes[length-2];

    // Push nodes to the end of the "node stack"
    Node* right = &nodes[node_stack_pointer+1];
    Node* left = &nodes[node_stack_pointer+2];
    
    Node new = {
      .value = 0,
      .frequency = right->frequency + left->frequency,
      .right = right,
      .left = left
    };
    nodes[length-2] = new;
    --length; // each iteration deletes two children but creates a new parent

    sort_node_array(nodes, length, true);
  }
    
  debug_freq_print(nodes[0]);

  if(fclose(f) == EOF){
    fprintf(stderr, "ERROR %u: Couldn't write to file specified\n%s", errno, strerror(errno));
  }

  return nodes[0];
}

// Prints the code for every node with the pre-order method
void Huffman_print_code(Node* root, char* curr_code, size_t curr_position){
  if(root->left != NULL){
    curr_code[curr_position] = '1';
    Huffman_print_code(root->left, curr_code, curr_position + 1);
  }
  if(root->right != NULL){
    curr_code[curr_position] = '0';
    Huffman_print_code(root->right, curr_code, curr_position + 1);
  }
  if(!(root->left) && !(root->right)){
    curr_code[curr_position] = '\0';
    printf("[%c] = %s\n", root->value, curr_code);
  }
}

// @@@ LOGIC: Remove the need for trailing zero chars and correct length strings
// Returns the symbol that corresponds to a given code in the tree
// Code is a null terminated, correct length string 
char Huffman_get_value_from_str(Node* r, const char* code){
  if(code == NULL){
    fprintf(stderr, "ERROR: Code given is not valid pointer to string!");
    exit(1);
  }
  if(r == NULL){
    fprintf(stderr, "ERROR: Root node given is Null and not traversable!");
  }
  
  Node root = *r;
  
  size_t str_length = strlen(code);
  for(size_t i = 0; i < str_length; ++i){
    if(code[i] == '0') root = *root.right;
    if(code[i] == '1') root = *root.left;
  }
  return root.value;
}

// Returns the symbol that corresponds to a given code in the tree
char Huffman_get_value_from_binary(Node* r, size_t code){
  if(r == NULL){
    fprintf(stderr, "ERROR: Root node given is Null and not traversable!");
  }
  
  size_t offset = 0;
  Node root = *r;
  
  while(root.right != NULL && root.left != NULL){
    if(((code >> offset) & 1) == 0) root = *root.right;
    if(((code >> offset) & 1) == 1) root = *root.left;
    ++offset;
  }
  return root.value;
}

/* void Huffman_get_code_string(Node* root, char key, char* code){ */
/*     assert(false && "ERROR: Huffman_get_code_string not implemented."); */
/* } */

/* size_t Huffman_get_code_binary(Node* root, char key, size_t* code){ */
/*     assert(false && "ERROR: Huffman_get_code_binary not implemented."); */
/* } */

/* void Huffman_save_encoded_file(const char* filepath, Node* root){ */
/*   assert(false && "ERROR: Huffman_save_encoded_file not implemented."); */
/* } */

#endif // HUFFMAN_IMPLEMENTATION
#ifdef LZ77_HEADER

#define LZ77_DEFAULT_CONFIG {				\
    .window_size = 4096,				\
      .minimum_match_length = 3,			\
      .run_length_code = '\000',			\
      .is_big_endian = 0,				\
      }							\

struct LZ77_config_t{
  size_t window_size;
  size_t minimum_match_length;
  char run_length_code;
  bool is_big_endian;
};
struct LZ77_config_t LZ77_config = LZ77_DEFAULT_CONFIG;

#define MIN_INT(a, b) (((a) > (b)) ? (b) : (a))

static inline void LZ77_compression_config(size_t window_size, size_t minimum_match_length,
					   char run_length_code, bool is_big_endian);
size_t LZ77_compress_buffer(void* restrict src, void* restrict dst, size_t buff_size);
size_t LZ77_compress_buffer_inplace(void* src, size_t buff_size);
size_t LZ77_compress_file_into_memory(const char* src_filepath, void* dst);
size_t LZ77_compress_and_save_file(const char* src_filepath, const char* dst_filepath);  

#endif // LZ77_HEADER
#ifdef LZ77_IMPLEMENTATION

#ifdef DEBUG
#define print_debug() printf("off: %zu len: %zu\n", match_offset, match_length);	\
  printf("mstart: %p  mend: %p msrc: %p rem:%zu\n", (void *)move_start,	\
	 (void*)data_endp, (void*)move_source, move_remaining_data);	\
  printf("data: %p\n", data);						\
  printf("f_size: %zu comp: %zu\n\n", buff_size, compressed_size);	\

void print_data(uint8_t* data, uint8_t* endp){
  size_t length = (size_t)(endp-data);
  for(size_t i = 0; i < length; ++i){
    putc(data[i], stdout);
  }
  putc('\n', stdout);
}
#else
#define print_debug()

#endif // DEBUG

static inline void LZ77_compression_config(size_t window_size, size_t minimum_match_length,
					   char run_length_code, bool is_big_endian){
 
  LZ77_config.window_size = window_size;
  LZ77_config.minimum_match_length = minimum_match_length;
  LZ77_config.run_length_code = run_length_code;
  LZ77_config.is_big_endian = is_big_endian;
}

enum JumpType{
  ShortJump = 0,
  LongJump = 1,
};

size_t LZ77_compress_buffer_inplace(void* src, size_t buff_size){
  if(src == NULL){
    fprintf(stderr, "ERROR: Couldn't read input buffers, NULL pointer!");
    exit(1);
  }

  if(buff_size == 0) {
    fprintf(stderr, "ERROR: Buffer size cannot be zero!");
    exit(1);
  } 
  
  uint8_t* data = (uint8_t*) src;
  size_t data_endp = (size_t)data + buff_size; 
  size_t compressed_size = buff_size;
  
  size_t window_start = 0;
  size_t window_end = MIN_INT(buff_size, LZ77_config.window_size);

  enum JumpType jump_code = ShortJump;
  while(window_start < window_end){

    size_t match_offset = 0;
    size_t match_length = 0;    

    for(size_t cursor = window_start+1; cursor < window_end; ++cursor){
      if(match_length == 15) {
	if(match_offset < 255) jump_code = ShortJump;
	else {      
	  jump_code = LongJump;
	  break;
	}
      }
    
      if(data[window_start + match_length] == data[cursor]){
	if(match_length == 0) match_offset = (cursor - window_start);
	if(match_length == 255) break;
	++match_length;
      } else if (match_length > 0) break;
    }
    
    if(match_length > LZ77_config.minimum_match_length){
   
      size_t write_offset = match_offset + window_start;
      data[write_offset + 0] = jump_code;
      
      if(jump_code == ShortJump){
	data[write_offset + 1] = (uint8_t)(match_offset & 0xFF);
	data[write_offset + 2] = (uint8_t)(match_length & 0xFF);
      }
      if(jump_code == LongJump){
	data[write_offset + 1] = (uint8_t)((match_offset & 0xFFF) >> 8);
	data[write_offset + 2] = (uint8_t)((((match_offset & 0xFFF) >> 16) << 4) | (match_length & 0xF));
      }

      // These encode the position of the memmove. An example of a match would be:
      // Eurotrain is a train
      //     ￪          ￪----| match-length
      // window_start
      //     |----------| match_offset
      uint8_t* move_start = (uint8_t*)(data + match_offset + window_start + 3);
      uint8_t* move_source = (uint8_t*)((size_t) move_start + match_length - 3);
      size_t move_remaining_data = (size_t)(data_endp - (size_t)move_source);

      print_debug();

      if(move_remaining_data > 0)
	memmove(move_start, move_source, move_remaining_data);

      compressed_size -= (match_length - 3);      
      data_endp -= (match_length - 3);
      window_end = compressed_size;

      //print_data(data, data_endp);      
    } else{
      ++window_start;
      window_end = MIN_INT(window_end + 1, compressed_size);
    }
  }
  
  return compressed_size;
}

size_t LZ77_compress_and_save_file(const char* src_filepath, const char* dst_filepath){
  FILE* f = fopen(src_filepath, "r+");
  if(f == NULL){
    fprintf(stderr, "ERROR %u: Couldn't open source file!\n%s", errno, strerror(errno));
    exit(1);
  }
  
  fseek(f, 0, SEEK_END);
  size_t f_size = (size_t)ftell(f);
  rewind(f);

  // @@@ TODO: Implement chunking
  uint8_t* data = (uint8_t*)malloc((size_t)f_size);
  fread(data, 1, f_size, f);
  
  size_t compressed_size = LZ77_compress_buffer_inplace(data, f_size);
  
  FILE* f_output = fopen(dst_filepath, "w+");
  if(f_output == NULL){
    fprintf(stderr, "ERROR %u: Couldn't write to destination file!\n%s", errno, strerror(errno));
  }
  
  fwrite(data, compressed_size, 1, f_output);
  free(data);
  
  return compressed_size;
}

#endif // LZ77_IMPLEMENTATION
// LICENSE
//  This is free and unencumbered software released into the public domain.
//
//  Anyone is free to copy, modify, publish, use, compile, sell, or
//  distribute this software, either in source code form or as a compiled
//  binary, for any purpose, commercial or non-commercial, and by any
//  means.
//
//  In jurisdictions that recognize copyright laws, the author or authors
//  of this software dedicate any and all copyright interest in the
//  software to the public domain. We make this dedication for the benefit
//  of the public at large and to the detriment of our heirs and
//  successors. We intend this dedication to be an overt act of
//  relinquishment in perpetuity of all present and future rights to this
//  software under copyright law.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
//  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.
//
//  For more information, please refer to <http://unlicense.org/>