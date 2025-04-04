/**
 * test_read_write_patterns.c
 *
 * Purpose: Verify different handling of read vs. write operations
 * This test creates a struct with distinct read and write patterns
 */

typedef struct {
    int read_only;          // Frequently read, never written
    double write_only;      // Frequently written, rarely read
    float read_write;       // Both read and written
    char rarely_accessed;   // Rarely used at all
    int write_heavy;        // More writes than reads
    double read_heavy;      // More reads than writes
} AccessPatternStruct;

// Function to perform read operations
int read_fields(AccessPatternStruct* data) {
    int result = 0;
    
    // Read read_only field multiple times
    for (int i = 0; i < 10; i++) {
        result += data->read_only;
    }
    
    // Read read_write field a few times
    for (int i = 0; i < 3; i++) {
        result += data->read_write;
    }
    
    // Read write_heavy once
    result += data->write_heavy;
    
    // Read read_heavy multiple times
    for (int i = 0; i < 5; i++) {
        result += data->read_heavy;
    }
    
    // Read write_only once
    result += data->write_only;
    
    // Rarely_accessed not read
    
    return result;
}

// Function to perform write operations
void write_fields(AccessPatternStruct* data, int value) {
    // Never write to read_only
    
    // Write to write_only multiple times
    for (int i = 0; i < 10; i++) {
        data->write_only = value * i;
    }
    
    // Write to read_write a few times
    for (int i = 0; i < 3; i++) {
        data->read_write = value + i;
    }
    
    // Write to write_heavy multiple times
    for (int i = 0; i < 5; i++) {
        data->write_heavy = value * 2 + i;
    }
    
    // Write to read_heavy once
    data->read_heavy = value * 3;
    
    // Rarely write to rarely_accessed
    data->rarely_accessed = value % 128;
}

int main() {
    AccessPatternStruct data = {10, 20.0, 30.0f, 'A', 40, 50.0};
    
    // Write then read
    write_fields(&data, 5);
    int result = read_fields(&data);
    
    // Expected result if behavior is preserved
    return (result == 240) ? 0 : 1;
}

/* Expected transformation:
If the optimizer considers both reads and writes equally:
typedef struct {
    int read_only;        // 10 reads
    double write_only;    // 10 writes + 1 read
    int write_heavy;      // 5 writes + 1 read
    double read_heavy;    // 5 reads + 1 write
    float read_write;     // 3 reads + 3 writes
    char rarely_accessed; // 1 write, 0 reads
} AccessPatternStruct;

If reads are weighted more than writes (or vice versa), the order may differ.
*/
