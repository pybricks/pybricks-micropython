// Includes
#include <stdio.h>
#include <stdlib.h>

// Definitions
#define FW_REG_SIZE_WORDS		(106*1024/4)
#define N_READ_MAX  			FW_REG_SIZE_WORDS

int main(int argc, char **argv)
{
	// Get the name of the binary file
	if (argc < 2) {
		printf("Usage: ./checksum binary\n");
		return EXIT_FAILURE;
	}

	// Open the binary
	FILE *ptr;
	ptr = fopen(argv[1],"rb");
	if (ptr == NULL)
		return EXIT_FAILURE;

	// Determine the size of the binary
	fseek(ptr, 0L, SEEK_END);
	unsigned int sz = (unsigned int) ftell(ptr);
	fseek(ptr, 0L, SEEK_SET);
	printf("Size of the binary: %d bytes\n", sz);

	// Read the binary file as a series of words
	unsigned int n_read = 0;
	unsigned int buffer[N_READ_MAX];
	if (sz > (4*N_READ_MAX))
		return EXIT_FAILURE;
	n_read = (unsigned int) fread(buffer,4,N_READ_MAX,ptr);
	printf("Successfully read: %d words\n", n_read);

	// Close the binary
	fclose(ptr);

	// Calculate the checksum
	unsigned int sum = 0;
	for (unsigned int i = 0; i < n_read; i++)
		sum += buffer[i];
	printf("Binary checksum: %08x\n", sum);

	// Continue calculating assuming all 0xFFFFFFFF
	for (unsigned int i = n_read; i < FW_REG_SIZE_WORDS; i++)
		sum += 0xFFFFFFFF;
	printf("Firmware checksum: %08x\n", sum);

	// Determine the correct checksum
	unsigned int checksum = (unsigned int)(0x100000000ULL - (unsigned long)sum);
	printf("Correction: %08x\n", checksum);
	return checksum;
}

