#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX 16

struct Node {
    char character;
    int freq;
    struct Node* l;
    struct Node* r;
};

struct MinHeap {
    int size;
    struct Node** array;
};

struct Node* newNode(char character, int freq) {
    struct Node* node = (struct Node*)malloc(sizeof(struct Node));
    node->character = character;
    node->freq = freq;
    node->l = node->r = NULL;
    return node;
}

void swapNode(struct Node** a, struct Node** b) {
    struct Node* temp = *a;
    *a = *b;
    *b = temp;
}

void heapify(struct MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;
    if (left < minHeap->size && minHeap->array[left]->freq < minHeap->array[smallest]->freq) {
        smallest = left;
    }
    if (right < minHeap->size && minHeap->array[right]->freq < minHeap->array[smallest]->freq) {
        smallest = right;
    }
    if (smallest != idx) {
        swapNode(&minHeap->array[smallest], &minHeap->array[idx]);
        heapify(minHeap, smallest);
    }
}

struct Node* extractMin(struct MinHeap* minHeap) {
    struct Node* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    --minHeap->size;
    heapify(minHeap, 0);
    return temp;
}

void insertMinHeap(struct MinHeap* minHeap, struct Node* node) {
    ++minHeap->size;
    int i = minHeap->size - 1;
    while (i && node->freq < minHeap->array[(i - 1) / 2]->freq) {
        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    minHeap->array[i] = node;
}

struct Node* buildHuffmanTree(char arr[], int freq[], int unique_size) {
    struct Node *l, *r, *top;
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
    minHeap->size = unique_size;
    minHeap->array = (struct Node**)malloc(unique_size * sizeof(struct Node*));
    for (int i = 0; i < unique_size; ++i) {
        minHeap->array[i] = newNode(arr[i], freq[i]);
    }
    int n = minHeap->size - 1;
    for (int i = (n - 1) / 2; i >= 0; --i) {
        heapify(minHeap, i);
    }
    while (minHeap->size != 1) {
        l = extractMin(minHeap);
        r = extractMin(minHeap);
        top = newNode('$', l->freq + r->freq);
        top->l = l;
        top->r = r;
        insertMinHeap(minHeap, top);
    }
    return extractMin(minHeap);
}

void printCodes(struct Node* root, int codes[], int top, int fd2) {
    if (root->l) {
        codes[top] = 0;
        printCodes(root->l, codes, top + 1, fd2);
    }
    if (root->r) {
        codes[top] = 1;
        printCodes(root->r, codes, top + 1, fd2);
    }
    if (!root->l && !root->r) {
        char data = root->character;
        write(fd2, &data, sizeof(char));
        write(fd2, &top, sizeof(int));
        int dec = 0;
        for (int i = 0; i < top; i++) {
            dec = dec * 2 + codes[i];
        }
        write(fd2, &dec, sizeof(int));
    }
}

void compressFile(int fd1, int fd2) {
    struct stat fileStat;
    fstat(fd1, &fileStat);
    int size = fileStat.st_size;
    unsigned char* buffer = (unsigned char*)malloc(size);
    read(fd1, buffer, size);
    char arr[size];
    int freq[size], unique_size = 0;
    for (int i = 0; i < size; i++) {
        int j;
        for (j = 0; j < unique_size; j++) {
            if (arr[j] == buffer[i]) {
                freq[j]++;
                break;
            }
        }
        if (j == unique_size) {
            arr[unique_size] = buffer[i];
            freq[unique_size++] = 1;
        }
    }
    struct Node* root = buildHuffmanTree(arr, freq, unique_size);
    int codes[MAX];
    printCodes(root, codes, 0, fd2);
    free(buffer);
}

void decompressFile(int fd1, int fd2) {
    int unique_size;
    read(fd1, &unique_size, sizeof(int));
    char arr[unique_size];
    int freq[unique_size];
    for (int i = 0; i < unique_size; i++) {
        read(fd1, &arr[i], sizeof(char));
        read(fd1, &freq[i], sizeof(int));
    }
    struct Node* root = buildHuffmanTree(arr, freq, unique_size);
    int size;
    read(fd1, &size, sizeof(int));
    int codes[size];
    unsigned char code[size];
    int idx = 0;
    while (read(fd1, &code[idx], sizeof(unsigned char)) != 0) {
        idx++;
    }
    idx = 0;
    struct Node* curr = root;
    while (idx < size) {
        if (curr->l == NULL && curr->r == NULL) {
            write(fd2, &(curr->character), sizeof(char));
            curr = root;
        }
        if (code[idx] == 0) {
            curr = curr->l;
        } else {
            curr = curr->r;
        }
        idx++;
    }
}

int main() {
    // Opening input file in read-only mode
    int fd1 = open("sample.txt", O_RDONLY);
    if (fd1 == -1) {
        perror("Open Failed For Input File:\n");
        exit(1);
    }

    // Creating output file in write mode
    int fd2 = open("sample-compressed.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd2 == -1)
    {
        perror("Open Failed For Output File:\n");
        exit(1);
    }

    // Compress the file
    compressFile(fd1, fd2);

    // Close the files
    close(fd1);
    close(fd2);

    // Reopen the compressed file in read-only mode
    fd1 = open("sample-compressed.txt", O_RDONLY);
    if (fd1 == -1) {
        perror("Open Failed For Compressed File:\n");
        exit(1);
    }

    // Create a new file for decompressed output
    fd2 = open("sample-decompressed.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd2 == -1) {
        perror("Open Failed For Decompressed File:\n");
        exit(1);
    }

    // Decompress the file
    decompressFile(fd1, fd2);

    // Close the files
    close(fd1);
    close(fd2);

    // Reopen the decompressed file in read-only mode
    fd1 = open("sample-decompressed.txt", O_RDONLY);
    if (fd1 == -1) {
        perror("Open Failed For Decompressed File:\n");
        exit(1);
    }

    // Create a new file for final output
    fd2 = open("sample-final.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd2 == -1) {
        perror("Open Failed For Final File:\n");
        exit(1);
    }

    // Write the contents of the decompressed file to the final file
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(fd1, buffer, sizeof(buffer))) > 0) {
        write(fd2, buffer, bytesRead);
    }

    // Close the files
    close(fd1);
    close(fd2);
    
    // Reopen the decompressed file in read-only mode
    fd1 = open("sample-final.txt", O_RDONLY);
    if (fd1 == -1) {
        perror("Open Failed For Decompressed File:\n");
        exit(1);
    }
    
    // Close the files
    close(fd1);
    close(fd2);
    return 0;
}
