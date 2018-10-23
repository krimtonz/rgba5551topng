#include <png.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
}pixel_t;

int bigendian;

static struct option cmdoptions[] = {
	{ "infile",required_argument,0,'i' },
	{ "outfile",required_argument,0,'o' },
	{ "width",required_argument,0,'w' },
	{ "height",required_argument,0,'h' },
	{ "bigendian",no_argument,&bigendian,1 },
	{ 0,0,0,0 }
};

int main(int argc, char **argv) {
	size_t width, height, filesize, numpixels;
	uint16_t *pixelData;
	int i,x,y,opt;
	char *infile, *outfile;

	pixel_t *pixels;
	FILE *fp;
	
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep row;

	width = 0;
	height = 0;
	infile = NULL;
	outfile = NULL;
	while (1) {
		int oi = 0;

		opt = getopt_long(argc, argv, "i:o:w:h:b", cmdoptions, &oi);
		if (opt == -1) break;
		switch (opt) {
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'w':
			width = strtol(optarg, NULL, 10);
			break;
		case 'h':
			height = strtol(optarg, NULL, 10);
			break;
		case 'b':
			bigendian = 1;
		default:
			break;
		}

	}
	if (infile == NULL) {
		printf("No input file specified\r\n");
		return 1;
	}
	if (outfile == NULL) {
		printf("No output file specified\r\n");
		return 1;
	}

	if (width <= 0) {
		printf("Invalid Width\r\n");
		return 1;
	}
	if (height <= 0) {
		printf("Invalid Height\r\n");
		return 1;
	}

	fp = fopen(infile,"rb");
	if (fp == NULL) {
		printf("Invalid file\r\n");
		return 1;
	}
	fseek(fp, 0, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (filesize != (width * height * 2)) {
		printf("Invalid file size.  Got %d, expected %d (width * height * 2)\r\n", filesize, (width*height * 2));
		fclose(fp);
		return 1;
	}

	pixelData = malloc(filesize);
	fread(pixelData, 1, filesize, fp);
	fclose(fp);

	numpixels = width * height;
	pixels = malloc(numpixels * sizeof(pixel_t));

	for (i = 0; i < numpixels ; i++) {
		if (bigendian) {
			pixelData[i] = ((pixelData[i] << 8) & 0xFF00) | pixelData[i] >> 8;
		}
		pixels[i].red = (((pixelData[i] >> 11) & 0x1F) * 255 + 15) / 31; 
		pixels[i].green = (((pixelData[i] >> 6) & 0x1F) * 255 + 15) / 31;
		pixels[i].blue = (((pixelData[i] >> 1) & 0x1F) * 255 + 15) / 31;
		pixels[i].alpha = (pixelData[i] & 0x0001) * 255;
	}

	free(pixelData);

	fp = fopen(outfile, "wb");

	if ((png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL) {
		goto finish;
	}
	if ((info_ptr = png_create_info_struct(png_ptr)) == NULL) {
		goto finish;
	}

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);

	row = (png_bytep)malloc(4 * width * sizeof(png_byte));
	
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pixel_t pix = pixels[width * y + x];

			row[(x * 4)] = pix.red;
			row[(x * 4) + 1] = pix.green;
			row[(x * 4) + 2] = pix.blue;
			row[(x * 4) + 3] = pix.alpha;
		}
		png_write_row(png_ptr, row);
	}
	png_write_end(png_ptr, NULL);

finish:
	if (fp != NULL) fclose(fp);
	if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row != NULL) free(row);
	if (pixels != NULL) free(pixels);
	return 0;
}

