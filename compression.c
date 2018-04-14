/**
  distributed files
  C program for Huffman Coding
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "encryption.h"

//char encode node
typedef struct char_node {
    char data;
    int* arr;
    int size;
} char_node;

//Huffman tree node
typedef struct tree_node {
    char data;
    unsigned int freq;
    struct tree_node* left;
    struct tree_node* right;
}tree_node;

//Min Heap: used to order tree nodes
typedef struct Minheap {
    unsigned int size;
    unsigned int capacity;
    tree_node** array;
} Minheap;

//global variables
int freq[256] = {0};
int character_size = 0;

char_node* cn;
int cn_idx = 0;

int tree_idx = 0;
tree_node* root;

/**
   Minheap functions
*/
Minheap* create_minheap(unsigned int capacity) {
    Minheap* mh = malloc(sizeof(Minheap));
    mh->size = 0;
    mh->capacity = capacity;
    mh->array = malloc(capacity * sizeof(tree_node*));
    return mh;
}

void destroy_minheap(Minheap* minheap) {
    free(minheap->array);
    free(minheap);
}

void swap(tree_node** a, tree_node** b) {
    tree_node* temp = *a;
    *a = *b;
    *b = temp;
}

int compare_min(Minheap* minheap, int idx1, int idx2) {
    if(minheap->array[idx1]->freq < minheap->array[idx2]->freq) {
	return idx1;
    } else {
	return idx2;
    }
}

int left_child(int idx) {
    return idx * 2 + 1;
}

int right_child(int idx) {
    return idx * 2 + 2;
}

int parent(int idx) {
    return (idx-1) / 2;
}

int empty(Minheap* minheap) {
    if(minheap->size <= 0) {
	return 1;
    } else { 
	return 0;
    }
}

int has_child(Minheap* minheap, int idx) {
    if((idx * 2 + 1) < (int) minheap->size) {
	return 1;
    } else {
	return 0;
    }
}

int min_child(Minheap* minheap, int idx) {
    if(right_child(idx) >= (int) minheap->size) {
	return left_child(idx);
    } else {
	return compare_min(minheap, left_child(idx), right_child(idx));
    }
}

void heapifyDown(Minheap* minheap, int idx) {
    if(has_child(minheap, idx) == 1) {
	int child_idx = min_child(minheap, idx);
	if(compare_min(minheap, idx, child_idx) == child_idx) {
	    swap(&minheap->array[idx], &minheap->array[child_idx]);
	    heapifyDown(minheap, child_idx);
	}
    }
}

void heapifyUp(Minheap* minheap, int idx) {
    if(idx == 0) {
	return;
    }
    int parent_idx = parent(idx);
    if(compare_min(minheap, idx, parent_idx) == idx) {
	swap(&minheap->array[idx], &minheap->array[parent_idx]);
	heapifyUp(minheap, parent_idx);
    }
}

tree_node* pop(Minheap* minheap) {
    tree_node* tn = minheap->array[0];
    minheap->array[0] = minheap->array[minheap->size -1];
    minheap->size = minheap->size - 1;
    heapifyDown(minheap, 0);
    return tn;
}

void push(Minheap* minheap, tree_node* tree_node) {
    minheap->size = minheap->size + 1;
    minheap->array[minheap->size - 1] = tree_node;
    heapifyUp(minheap, minheap->size - 1);    
}

void build_heap(Minheap* minheap) {
    for(int i = minheap->size - 1; i >= 0; i--) {
	heapifyDown(minheap, i);
    }
}

/**
  Tree node functions
*/
tree_node* create_tree_node(char data, unsigned int freq) {
    tree_node* node = malloc(sizeof(tree_node));
    node->left = NULL;
    node->right = NULL;
    node->data = data;
    node->freq = freq;
    return node;
}

void destroy_tree_node(tree_node* node) {
    if(node != NULL) {
	destroy_tree_node(node->left);
	destroy_tree_node(node->right);
	free(node);
    }
}

int tree_node_height(tree_node* node) {
    int height = 0;
    if(node->left != NULL || node->right != NULL) {
	int left_h =  node->left? tree_node_height(node->left): 0;
	int right_h = node->right?tree_node_height(node->right):0;
	if(left_h > right_h) {
	    height = left_h + 1;
	} else {
	    height = right_h + 1;
	}
    }
    return height;
}

int is_leaf(tree_node* node) {
    if(node->left == NULL && node->right == NULL) {
	return 1;
    } else {
	return 0;
    }
}

char* print_tree(tree_node* root, int* size) {
    if(root == NULL) {
	*size = 0;
	return NULL;
    }
    if(is_leaf(root) == 1) {
	char* result = malloc(3);
	result[0] = '1';
	result[1] = root->data;
	result[2] = '\0';
	*size = 2;
	return result;
    } else {
	int left_len = 0;
	int right_len = 0;
	char* left = print_tree(root->left, &left_len);
	char* right = print_tree(root->right, &right_len);

	char* result = malloc(2+left_len+right_len);
	result[0] = '0';
	if(left != NULL) {
	    for(int i = 0; i < left_len; i++) {
	    	result[1 + i] = left[i];
	    }
	}
	if(right != NULL) {
	    for(int i = 0; i < right_len; i++) {
		result[1 + left_len + i] = right[i];
	    }
	}
	result[1+left_len+right_len] = '\0';
	free(left);
	free(right);
	*size = 1 + left_len + right_len;

	return result;
    }
}

tree_node* construct_tree(char* str) {    
    if(str[tree_idx] == '1') {
	tree_idx++;
	char data = str[tree_idx];
	tree_idx++;
	return create_tree_node(data, 1);
    } else {
	tree_idx++;
	tree_node* left = construct_tree(str);
	tree_node* right = construct_tree(str);
	tree_node* newnode =  create_tree_node('$', left->freq + right->freq);
	newnode->left = left;
	newnode->right = right;
	return newnode;
    }
}


/**
  Huffman encoding functions
*/
Minheap* create_build_heap() {
    //printf("character_size is %d\n", character_size);
    Minheap* minheap = create_minheap(character_size);
    //printf("reach here\n");
    int idx = 0;
    for(int i = 0; i < 256; i++) {
	if(freq[i] != 0) {
	    minheap->array[idx] = create_tree_node((char)i, freq[i]);
	    idx++;
	}
    }
    minheap->size = character_size;
    build_heap(minheap);
    return minheap;
}

tree_node* build_huffman_tree() {
    Minheap* minheap = create_build_heap();
    tree_node* left, *right, *top;
    while(minheap->size != 1) {
	left = pop(minheap);
	right = pop(minheap);
	//top is an internal node
	top = create_tree_node('#',left->freq + right->freq);
	top->left = left;
	top->right = right;
	push(minheap, top);
    }
    tree_node* root = pop(minheap);
    destroy_minheap(minheap);
    return root;
}

void printCodes(tree_node* root, int arr[], int top) {
    if(root->left) {
	arr[top] = 0;
	printCodes(root->left, arr, top+1);
    }
    if(root->right) {
	arr[top] = 1;
	printCodes(root->right, arr, top+1);
    }
    if(is_leaf(root) == 1) {
        cn[cn_idx].data = root->data;
	cn[cn_idx].size = top;
	cn[cn_idx].arr = malloc(sizeof(int) * top);	
	//printf("%c: ", root->data);
	for(int i = 0; i < top; i++) {
	    cn[cn_idx].arr[i] = arr[i];
	    //printf("%d", arr[i]);
	}
	cn_idx++;
	//printf("\n");
    }
}

tree_node* huffman_codes() {
    tree_node* root = build_huffman_tree();
    int arr[tree_node_height(root)];
    int top = 0;
    printCodes(root, arr, top);
    return root;
}

int count_occurrence(char* str) {
    int len = (int)strlen(str);
    for(int i = 0; i < 256; i++) {
	freq[i] = 0;
    }
    for(int i = 0; i < len; i++) {
	int c = (int) str[i];
	if(c < 0) {
	    c = c + 256;
	}
	freq[c]++;
    } 
    //null character '\0'
    //freq[0]++;
    int exist = 0;
    for(int i = 0; i < 256; i++) {
	if(freq[i] != 0) {
	    exist++;
	}
    }
    character_size = exist;
    return exist;
}

char_node* get_char_node(char c) {
    for(int i = 0; i < character_size; i++) {
	if(cn[i].data == c) {
	    return &cn[i];
	}
    }
    return NULL;
}

char* create_bit_array(char* str, int* bit_size) {
    int total_size = 0;
    for(size_t i = 0; i < strlen(str); i++) {
	char_node* temp = get_char_node(str[i]);
	total_size += temp->size;
    }
    *bit_size = total_size / 8;
    if(total_size % 8 != 0) {
	(*bit_size)++;
    }

    char* bit_array = calloc(1, (*bit_size));
    int n = 0;
    for(int i =0; i < (int)strlen(str); i++) {
	char_node* temp = get_char_node(str[i]);
	int char_size = temp->size;
	int* arr = temp->arr;
	for(int j = 0; j < char_size; j++) {
	    int n_i = n/8;
	    int pos = 7 - n%8;
	    unsigned int flag = arr[j];
	    flag = flag << pos;
	    bit_array[n_i] = bit_array[n_i] | flag;
	    n++;
	}
    }
    return bit_array;
}

char* decompress_bit_array(tree_node* root, char* bit_array, int str_size) {
    char* str = malloc(str_size+1);
    str[str_size] = '\0';
    int n = 0;
    int idx = 0;
    tree_node* ptr = root;
    while(idx < str_size) {
	if(is_leaf(ptr) == 1) {
	    char c = ptr->data;
	    str[idx] = c;
	    idx++;
	    ptr = root;
	} else {
	    int n_i = n/8;
	    int pos = 7 - n%8;
	    unsigned int flag = 1;
	    flag = flag << pos;
	    char c = bit_array[n_i];
	    int bit = 0;
	    bit = (int) (c >> pos & 0x01);
	    if(bit == 0) {
		ptr = ptr->left;
	    } else {
		ptr = ptr->right;
	    }
	    n++;
	}
    }
    return str;
}

char* compress(char* str, int* compress_size) {
    if(str == NULL) {
	return NULL;
    }
    char* encrypt = encryption(str);
    int str_len = (int)strlen(encrypt);

    character_size = 0;
    cn_idx = 0;
    int size = count_occurrence(encrypt);
    
    cn = malloc(sizeof(char_node) * size);
    root = huffman_codes();
    
    int bit_size = 0;
    char* bit_array = create_bit_array(encrypt, &bit_size);
    //printf("bit array is:%s\n", bit_array);

    int tree_str_size = 0;
    char* tree_str = print_tree(root, &tree_str_size);
    
    int tree_str_len = tree_str_size;
    int str_length = snprintf(NULL, 0, "%d", str_len);
    int tree_length = snprintf(NULL, 0, "%d", tree_str_len);
    int bit_length = snprintf(NULL, 0, "%d", bit_size);
    int start_length = str_length + tree_length + bit_length + 3;

    char* compress = malloc(start_length + tree_str_len + bit_size);
    snprintf(compress, start_length + 1, "%ds%dt%db", str_len, tree_str_len, bit_size);
    
    for(int i = start_length; i < start_length + tree_str_len; i++) {
	compress[i] = tree_str[i - start_length];
    }
    compress[start_length + tree_str_len] = '\0'; 
    for(int i = start_length + tree_str_len; i < start_length + tree_str_len + bit_size; i++) {
	compress[i] = bit_array[i - start_length - tree_str_len];	
    }

    free(bit_array);
    free(tree_str);
    destroy_tree_node(root);
    for(int i =0; i < size; i++) {
	free(cn[i].arr);
    }
    free(cn);
    free(encrypt);
    *compress_size = start_length + tree_str_len + bit_size;
    return compress;
}

char* decompress(char* str, int compress_size) {
    if(str == NULL) {
	return NULL;
    }
    
    char dest[compress_size];
    int j;
    for(j = 0; j < compress_size; j++) {
	dest[j] = str[j];
	if(str[j] == 'b') {
	    break;
	}
    } 
    dest[j+1] = '\0';

    char* str_l = strtok(dest, "s");
    char* rest_s = strtok(NULL, "s");
    char* tree_l = strtok(rest_s, "t");
    char* rest_t = strtok(NULL, "t");
    char* bit_l = strtok(rest_t, "b");
    int str_len, tree_len, bit_len;
    sscanf(str_l, "%d", &str_len);
    sscanf(tree_l, "%d", &tree_len);
    sscanf(bit_l, "%d", &bit_len);
    int start_length = j+1;      

    char* tree_str = malloc(tree_len + 1);
    tree_str[tree_len] = '\0';
    for(int i = start_length; i < start_length + tree_len; i++) {
	tree_str[i-start_length] = str[i];
    }
 

    tree_node* root = construct_tree(tree_str);
    free(tree_str);    
    
    char* bit_array = malloc(bit_len);
    for(int i = start_length + tree_len; i < start_length + tree_len + bit_len; i++) {
        bit_array[i-start_length-tree_len] = str[i];
    }
    char* decompress = decompress_bit_array(root, bit_array, str_len);
    free(bit_array);
    destroy_tree_node(root);
 
    char* decrypt = decryption(decompress);
    free(decompress);
    return decrypt;
}

/*
int main() {
    char* str = "The quick brown fox jumps over a lazy dog. Funny sentence.";
    int str_len = (int)strlen(str);
    int compress_size = 0;
    char* compress_str = compress(str, &compress_size);
    printf("compressed size is %d\n", compress_size);
    printf("uncompressed size is %d\n", str_len);
    printf("compressed about %f\n", compress_size/(double)str_len);

    char* decompress_str = decompress(compress_str, compress_size);
    printf("decompress str is: \"%s\"\n", decompress_str);

    free(compress_str);
    free(decompress_str);
    return 0;
}
*/
