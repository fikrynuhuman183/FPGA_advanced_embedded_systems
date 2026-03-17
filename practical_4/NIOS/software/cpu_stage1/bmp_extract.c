#include "bmp_extract.h"
#include <stdlib.h>
#include <unistd.h>

/* g_file_buffer removed — buffer is now malloc'd inside bmp_extract() */
Bmp_header g_input_file_header;

int bmp_extract_header(Bmp_data *pic_data) {
	int i;

	if (pic_data->buffer[0] == 'B' && pic_data->buffer[1] == 'M') {
		for (i = 0; i < 52; i++) {
			pic_data->header->BMPRawData[i] = pic_data->buffer[i + 2];
		}

		// Calculate the effective size of the data field
		pic_data->header->BMPDataSize = pic_data->header->BMPHeight * pic_data->header->BMPWidth
				* (pic_data->header->BMPBitsPerPixel / 8);

		pic_data->bitmap = &pic_data->buffer[pic_data->header->BMPDataOffset];
		return 1;
	}
	return 0;
}

void bmp_rearrange_comps(Bmp_data *pic_data) {
	// BMP images start from the bottom left, and its components are ordered
	// B - G - R. The jpeg encoder starts at the top left, and is ordered
	// R - G - B. This function puts all the bytes in their right place.

	int row, column;
	int8_t r, g, b, *row_ptr, *dest;

	// <*dest> starts from the top left of the image and moves downwards.
	// <*row_ptr> starts from the bottom left of the image and moves upwards.
	dest = pic_data->bitmap;

	for (row = pic_data->header->BMPHeight - 1; row >= pic_data->header->BMPHeight / 2; row--) {
		// We loop until <*dest> and <*row_ptr> meet in the middle of the image.

		row_ptr = &(pic_data->bitmap[row * pic_data->header->BMPWidth * 3]);
		for (column = 0; column < pic_data->header->BMPWidth * 3; column += 3) {
			// Not only do we move data from <*dest> to <*row_ptr> and vice-versa,
			// we also need to reshuffle RGB components.
			r = dest[2];
			g = dest[1];
			b = dest[0];

			*dest++ = row_ptr[column + 2];
			*dest++ = row_ptr[column + 1];
			*dest++ = row_ptr[column];

			row_ptr[column] = r;
			row_ptr[column + 1] = g;
			row_ptr[column + 2] = b;
		}
	}
}

int bmp_extract(char *file_name, Bmp_data *pic_data) {
	FILE *bmp_file = NULL;
	int bmp_file_size, bytes_read;

	printf("[bmp] Opening file: %s\n", file_name);
	fflush(stdout);
	bmp_file = fopen(file_name, "rb");
	if(bmp_file == NULL) {
		printf("[bmp] ERROR: fopen returned NULL - file not found: %s\n", file_name);
		printf("[bmp] Check that the file exists in the 'files/' folder of cpu_stage1 project.\n");
		return 1;
	}
	printf("[bmp] File opened successfully.\n");

	// Get the size of the file
	fseek(bmp_file, 0, SEEK_END);
	bmp_file_size = ftell(bmp_file);
	rewind(bmp_file);
	printf("[bmp] File size: %d bytes. Allocating buffer...\n", bmp_file_size);

	pic_data->buffer = (int8_t *)malloc(bmp_file_size);
	if(pic_data->buffer == NULL) {
		printf("[bmp] ERROR: malloc(%d) failed!\n", bmp_file_size);
		fclose(bmp_file);
		return 3;
	}
	printf("[bmp] Buffer allocated at 0x%08x. Reading file (slow over JTAG)...\n",
	       (unsigned int)pic_data->buffer);

	// Read the whole file into the buffer
	bytes_read = fread((void *)pic_data->buffer, sizeof(int8_t), bmp_file_size, bmp_file);
	fclose(bmp_file);
	printf("[bmp] fread done: %d of %d bytes read.\n", bytes_read, bmp_file_size);

	if(bytes_read != bmp_file_size)
	{
		printf("Error occurred while reading the file\n\n");
		free(pic_data->buffer);
		pic_data->buffer = NULL;
		return 2;
	}

	pic_data->header = &g_input_file_header;

	printf("[bmp] Parsing header...\n");
	bmp_extract_header(pic_data);
	printf("[bmp] Rearranging components (BGR->RGB, flip rows)...\n");
	bmp_rearrange_comps(pic_data);
	printf("[bmp] Done.\n");

	printf("Size of the file: %d\n", (int)pic_data->header->BMPFileSize);
	printf("Size of the height: %d\n", (int)pic_data->header->BMPHeight);
	printf("Size of the width: %d\n", (int)pic_data->header->BMPWidth);
	printf("Size of the offset: %d\n", (int)pic_data->header->BMPDataOffset);
	printf("Data pointer: 0x%08x\n\n", (unsigned int)pic_data->bitmap);

	return 0;
}
