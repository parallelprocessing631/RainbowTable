#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include <omp.h>

#define MAX_LENGTH 100

// Structure to represent a password-hash pair
typedef struct {
    char password[50];
    char hash[SHA512_DIGEST_LENGTH * 2 + 1];
} PasswordHashPair;

// Function to reduce the hash to a password
char* reduce(char* hash, char* chars, int chars_len, int length) {
    // Convert hash to int
    int hash_int = 0;
    for (int k = 0; k < SHA512_DIGEST_LENGTH; k++) {
        hash_int += hash_int * 256 + (int) hash[k];
    }

    // Reduce the hash to a password
    static char password[MAX_LENGTH];
    for (int i = 0; i < length; i++) {
        password[i] = chars[hash_int % chars_len];
        hash_int /= chars_len;
    }
    password[length] = '\0';
    return password;
}

// Function to generate a random password
char* random_password(char* chars, int length) {
    static char password[MAX_LENGTH];
    for (int i = 0; i < length; i++) {
        password[i] = chars[rand() % strlen(chars)];
    }
    password[length] = '\0';
    return password;
}

// Function to generate hash for a given password using different algorithms
void generate_hash(const char *password, const char *algorithm, char *hash) {
    if (strcmp(algorithm, "sha1") == 0) {
        // SHA-1 hashing
        SHA_CTX sha1_context;
        SHA1_Init(&sha1_context);
        SHA1_Update(&sha1_context, password, strlen(password));
        SHA1_Final((unsigned char *)hash, &sha1_context);
    } else if (strcmp(algorithm, "sha256") == 0) {
        // SHA-256 hashing
        SHA256_CTX sha256_context;
        SHA256_Init(&sha256_context);
        SHA256_Update(&sha256_context, password, strlen(password));
        SHA256_Final((unsigned char *)hash, &sha256_context);
    } else if (strcmp(algorithm, "sha512") == 0) {
        // SHA-512 hashing
        SHA512_CTX sha512_context;
        SHA512_Init(&sha512_context);
        SHA512_Update(&sha512_context, password, strlen(password));
        SHA512_Final((unsigned char *)hash, &sha512_context);
    } else {
        // Invalid hashing algorithm
        printf("Invalid hashing algorithm\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s <num_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    double itime, ftime, exec_time;
    itime = omp_get_wtime();
    //changed the nuber of threads to be specified on command line 
    int num_threads = atoi(argv[1]);
    omp_set_num_threads(num_threads);

    srand(time(NULL));

    char* chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int chars_len = strlen(chars);

    // int length;
    // printf("Enter the length of the password: ");
    // scanf("%d", &length);

    // char algorithm[10];
    // printf("Enter the hashing algorithm (sha1, sha256, sha512):");
    // scanf("%s", algorithm);

    // int n_chains;
    // printf("Enter the number of chains: ");
    // scanf("%d", &n_chains);

    // int chain_length;
    // printf("Enter the chain length: ");
    // scanf("%d", &chain_length);

    int length = atoi(argv[2]);
    const char *algorithm = argv[3];
    int n_chains = atoi(argv[4]);
    int chain_length = atoi(argv[5]);

    // Create an array to store password-hash pairs
    PasswordHashPair *pairs = (PasswordHashPair *)malloc(n_chains * sizeof(PasswordHashPair));
    if (pairs == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    clock_t start_time, end_time;
    double computation_time;

    // Parallelize the generation of password-hash pairs
    #pragma omp parallel for
    for (int i = 0; i < n_chains; i++) {
        // Use private variables for each thread
        char* p = random_password(chars, length);
        strcpy(pairs[i].password, p);

        for (int j = 0; j < chain_length; j++) {
            // Hashing and reducing
            char hash[SHA512_DIGEST_LENGTH];
            generate_hash(p, algorithm, hash);
            p = reduce(hash, chars, chars_len, length);
        }
        char hash[SHA512_DIGEST_LENGTH];
        generate_hash(p, algorithm, hash);
        #pragma omp critical 

        strcpy(pairs[i].hash, hash);
    }

    // End measuring time
    ftime = omp_get_wtime();

    // Save the sorted password-hash pairs to the file
    FILE *output_file = fopen("rainbow_table.txt", "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n_chains; ++i) {
        fprintf(output_file, "Password: %s, Last Hash: %s\n", pairs[i].password, pairs[i].hash);
    }

    fclose(output_file);
    free(pairs);

    // Print the number of threads used
    printf("Number of Threads: %d\n", num_threads);

     // Print the size of the generated file
    FILE *file = fopen("rainbow_table.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    fclose(file);

    printf("Size of File Generated: %ld bytes\n", size);


    // Calculate and print the computation time
    exec_time = ftime - itime;
    printf("Rainbow table generation time: %.4f seconds\n", exec_time);
   
    return 0;
}
