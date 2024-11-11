#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_LINE 256
#define SSN_LENGTH 12  // 11 chars + null terminator
#define MAX_INVALID 7  // We only need to track the "mole" SSNs
#define HASH_SIZE 10007  // Prime number for hash table size

typedef struct HashNode { // Linked list node for hash table
    char ssn[SSN_LENGTH]; 
    struct HashNode* next;
} HashNode;

typedef struct { // Invalid SSN structure
    char ssn[SSN_LENGTH];
    int line_number;
    const char* violation_message;
} InvalidSSN;

typedef enum { // Violation types
    INVALID_FORMAT,
    DUPLICATE_SSN,
    STARTS_WITH_9,
    STARTS_WITH_666,
    STARTS_WITH_000,
    MIDDLE_00,
    ENDS_WITH_0000
} ViolationType;

// Hash table functions
unsigned int hash(const char* ssn) {
    unsigned int hash = 0;
    while (*ssn) {
        hash = (hash * 31 + *ssn) % HASH_SIZE;
        ssn++;
    }
    return hash;
}

bool addToHash(HashNode** hash_table, const char* ssn, bool* is_duplicate) { // Returns false if memory allocation fails
    unsigned int index = hash(ssn);
    
    // Check for duplicate
    HashNode* current = hash_table[index];
    while (current != NULL) {
        if (strcmp(current->ssn, ssn) == 0) {
            *is_duplicate = true;
            return false;
        }
        current = current->next;
    }
    
    // Add new node
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    if (new_node == NULL) return false;
    
    strncpy(new_node->ssn, ssn, SSN_LENGTH - 1);
    new_node->ssn[SSN_LENGTH - 1] = '\0';
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
    *is_duplicate = false;
    return true;
}

void freeHashTable(HashNode** hash_table) { // Free all nodes in hash table
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode* current = hash_table[i];
        while (current != NULL) {
            HashNode* temp = current;
            current = current->next;
            free(temp);
        }
    }
}

bool isDigitsOnly(const char* str, int len) { // Check if string contains only digits
    if (str == NULL) return false;
    for (int i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

bool isValidFormat(const char* ssn) { // Check if SSN has valid format
    if (ssn == NULL || strlen(ssn) != 11) return false;
    if (ssn[3] != '-' || ssn[6] != '-') return false;
    
    return isDigitsOnly(ssn, 3) && 
           isDigitsOnly(ssn + 4, 2) && 
           isDigitsOnly(ssn + 7, 4);
}

int getFirstThree(const char* ssn) { // Extract first three digits
    if (ssn == NULL) return -1;
    char first_three[4] = {0};
    strncpy(first_three, ssn, 3);
    first_three[3] = '\0';
    return atoi(first_three);
}

int getMiddleTwo(const char* ssn) { // Extract middle two digits
    if (ssn == NULL || strlen(ssn) < 6) return -1;
    char middle_two[3] = {0};
    strncpy(middle_two, ssn + 4, 2);
    middle_two[2] = '\0';
    return atoi(middle_two);
}

int getLastFour(const char* ssn) { // Extract last four digits
    if (ssn == NULL || strlen(ssn) < 11) return -1;
    char last_four[5] = {0};
    strncpy(last_four, ssn + 7, 4);
    last_four[4] = '\0';
    return atoi(last_four);
}

ViolationType checkSSNViolation(const char* ssn) { // Check for SSN violations
    if (!isValidFormat(ssn)) {
        return INVALID_FORMAT;
    }
    
    int first_three = getFirstThree(ssn);
    if (first_three == -1) return INVALID_FORMAT;
    
    int middle_two = getMiddleTwo(ssn);
    if (middle_two == -1) return INVALID_FORMAT;
    
    int last_four = getLastFour(ssn);
    if (last_four == -1) return INVALID_FORMAT;
    
    if (first_three == 900 || (first_three >= 901 && first_three <= 999)) {
        return STARTS_WITH_9;
    }
    if (first_three == 666) {
        return STARTS_WITH_666;
    }
    if (first_three == 0) {
        return STARTS_WITH_000;
    }
    if (middle_two == 0) {
        return MIDDLE_00;
    }
    if (last_four == 0) {
        return ENDS_WITH_0000;
    }
    
    return -1;
}

const char* getViolationMessage(ViolationType violation) { // Get violation message
    switch (violation) {
        case INVALID_FORMAT:
            return "Invalid format (should be XXX-XX-XXXX)";
        case DUPLICATE_SSN:
            return "Duplicate SSN";
        case STARTS_WITH_9:
            return "SSN starts with 9";
        case STARTS_WITH_666:
            return "SSN starts with 666";
        case STARTS_WITH_000:
            return "SSN starts with 000";
        case MIDDLE_00:
            return "SSN has 00 in positions 4-5";
        case ENDS_WITH_0000:
            return "SSN ends with 0000";
        default:
            return "Valid SSN";
    }
}

int main() { // Main function
    FILE* file = fopen("ssns.txt", "r");
    if (file == NULL) {
        printf("Error opening file ssns.txt\n");
        return 1;
    }

    // Initialize hash table
    HashNode** hash_table = (HashNode**)calloc(HASH_SIZE, sizeof(HashNode*));
    if (hash_table == NULL) {
        printf("Memory allocation failed\n");
        fclose(file);
        return 1;
    }

    InvalidSSN invalid_ssns[MAX_INVALID];
    int invalid_count = 0;
    char line[MAX_LINE];
    int line_number = 0;
    
    printf("Processing SSNs...\n");
    
    // Process file one line at a time
    while (fgets(line, sizeof(line), file) && invalid_count < MAX_INVALID) {
        line_number++;
        
        // Progress indicator every 10000 lines
        if (line_number % 10000 == 0) {
            printf("Processed %d lines...\r", line_number);
            fflush(stdout);
        }
        
        // Remove newline and carriage return characters
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        
        if (len == 0) continue;  // Skip empty lines
        if (len >= SSN_LENGTH) continue;  // Skip lines that are too long

        // First check for duplicates
        bool is_duplicate = false;
        if (!addToHash(hash_table, line, &is_duplicate)) {
            if (is_duplicate && invalid_count < MAX_INVALID) {
                strncpy(invalid_ssns[invalid_count].ssn, line, SSN_LENGTH - 1);
                invalid_ssns[invalid_count].ssn[SSN_LENGTH - 1] = '\0';
                invalid_ssns[invalid_count].line_number = line_number;
                invalid_ssns[invalid_count].violation_message = getViolationMessage(DUPLICATE_SSN);
                invalid_count++;
                continue;
            }
        }
        
        // Then check other violations
        ViolationType violation = checkSSNViolation(line);
        if (violation != -1 && invalid_count < MAX_INVALID) {
            strncpy(invalid_ssns[invalid_count].ssn, line, SSN_LENGTH - 1);
            invalid_ssns[invalid_count].ssn[SSN_LENGTH - 1] = '\0';
            invalid_ssns[invalid_count].line_number = line_number;
            invalid_ssns[invalid_count].violation_message = getViolationMessage(violation);
            invalid_count++;
        }
    }
    
    printf("\n\nInvalid SSNs found (7 'mole' SSNs):\n");
    printf("------------------------------------------------\n");
    
    for (int i = 0; i < invalid_count; i++) {
        printf("MOLE #%d - Line %d: %s\n", 
               i + 1, 
               invalid_ssns[i].line_number, 
               invalid_ssns[i].ssn);
        printf("Violation: %s\n\n", 
               invalid_ssns[i].violation_message);
    }
    
    printf("------------------------------------------------\n");
    printf("Total invalid SSNs found: %d\n", invalid_count);
    printf("Total lines processed: %d\n", line_number);
    
    // Cleanup
    freeHashTable(hash_table);
    free(hash_table);
    fclose(file);
    return 0;
}