#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>

#define PI 3.14
#define RADIUS 10
#define NUM_THREADS 8
#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

typedef struct image
{
	int width, height;
	int maxval;
	int color;
	unsigned char **data, **dataR, **dataG, **dataB;
} image;

typedef struct task
{
	int h, w, start, end, r, rs;
	unsigned char **srcR, **srcG, **srcB;
	unsigned char **dstR, **dstG, **dstB;
} task;

int num_threads;
int resize_factor;
int gaussian_kernel[3][3] = {{1, 2, 1},
						     {2, 4, 2},
						     {1, 2, 1}};
image img, img2;
task params[NUM_THREADS];

void allocate_image_memory(image *img) {
	int i;

	if(img->color) {
		img->dataR = (unsigned char **)malloc(img->height * sizeof(unsigned char *));
		img->dataG = (unsigned char **)malloc(img->height * sizeof(unsigned char *));
		img->dataB = (unsigned char **)malloc(img->height * sizeof(unsigned char *));

		for(i = 0; i < img->height; i++) {
			img->dataR[i] = (unsigned char *)malloc(img->width);
			img->dataG[i] = (unsigned char *)malloc(img->width);
			img->dataB[i] = (unsigned char *)malloc(img->width);
		}
	}
	else {
		img->data= (unsigned char **)malloc(img->height * sizeof(unsigned char *));

		for(i = 0; i < img->height; i++)
			img->data[i] = (unsigned char *)malloc(img->width);
	}
}

void free_image_memory(image *img) {
	int i;

	if(img->color) {
		for(i = 0; i < img->height; i++) {
			free(img->dataR[i]);
			free(img->dataG[i]);
			free(img->dataB[i]);
		}

		free(img->dataR);
		free(img->dataG);
		free(img->dataB);
	}
	else {
		for(i = 0; i < img->height; i++)
			free(img->data[i]);

		free(img->data);
	}
}

void readInput(const char * fileName, image *img) {
	//open the file
	FILE *in = fopen(fileName, "rb");

	//read the type of file
	char str[3];
	fgets(str, 3, in);
	if(str[1] == '5')
		img->color = 0;
	else
		img->color = 1;

	//read the size of the photo
	fscanf(in, "%d%d", &img->width, &img->height);

	//read the max value
	fscanf(in, "%d", &img->maxval);

	//read the extra new line
	fgetc(in);

	//read the pixels' values
	int i, j;

	allocate_image_memory(img);

	if(!img->color)
		//grayscale
		for(i = 0; i < img->height; i++)
			fread(img->data[i], 1, img->width, in);
	else {
		//color
		unsigned char *buffer;

		buffer = (unsigned char *)malloc(3 * img->width);

		for(i = 0; i < img->height; i++) {
			fread(buffer, 1, 3 * img->width, in);

			for(j = 0; j < img->width; j++) {
				img->dataR[i][j] = buffer[3 * j];
				img->dataG[i][j] = buffer[3 * j + 1];
				img->dataB[i][j] = buffer[3 * j + 2];
			}
		}

		free(buffer);
	}

	//close the file
	fclose(in);
}

void writeData(const char * fileName, image *img) {
	//open the file
	FILE *out = fopen(fileName, "wb");

	//write the type of file
	if(!img->color)
		fprintf(out, "P5\n");
	else
		fprintf(out, "P6\n");

	//write the size of the photo
	fprintf(out, "%d %d\n", img->width, img->height);

	//write the max value
	fprintf(out, "%d\n", img->maxval);

	//write the pixels' values
	int i, j;

	if(!img->color)
		//grayscale
		for(i = 0; i < img->height; i++)
			fwrite(img->data[i], 1, img->width, out);
	else {
		//color
		unsigned char *buffer;

		buffer = (unsigned char *)malloc(3 * img->width);

		for(i = 0; i < img->height; i++) {
			for(j = 0; j < img->width; j++){
				buffer[3 * j] = img->dataR[i][j];
				buffer[3 * j + 1] = img->dataG[i][j];
				buffer[3 * j + 2] = img->dataB[i][j];
			}

			fwrite(buffer, 1, 3 * img->width, out);
		}

		free(buffer);
	}

	free_image_memory(img);

	//close the file
	fclose(out);
}

void blur_block(unsigned char **src, unsigned char **dst, int h, int w, int start, int end, int r, int rs) {
	int i, j, iy, ix, x, y, dsq;
    double wght, val, wsum;

	for(i = start; i < end; i++)
        for(j = 0; j < w; j++) {
            
            val = 0;
            wsum = 0;
            
            for(iy = i - rs; iy < i + rs + 1; iy++)
                for(ix = j - rs; ix < j + rs + 1; ix++) {
                    x = min(w - 1, max(0, ix));
                    y = min(h - 1, max(0, iy));
                    
                    dsq = (ix - j) * (ix - j) + (iy - i) * (iy - i);
                    wght = exp(-dsq / (2 * r * r)) / (PI * 2 * r * r);
                    
                    val += src[y][x] * wght;
                    wsum += wght;
                }

            dst[i][j] = floor(val / wsum);            
        }
}

void *blur(void *arg) {
	task *param = (task *)arg;
	
	blur_block(param->srcR, param->dstR, param->h, param->w, param->start, param->end, param->r, param->rs);
	blur_block(param->srcG, param->dstG, param->h, param->w, param->start, param->end, param->r, param->rs);
	blur_block(param->srcB, param->dstB, param->h, param->w, param->start, param->end, param->r, param->rs);
	
	return NULL;
}

int main() {	
	readInput("in.pnm", &img);

	img2.width = img.width;
	img2.height = img.height;
	img2.color = img.color;
	img2.maxval = img.maxval;

	allocate_image_memory(&img2);


	int size, rs, i;
	pthread_t threads[NUM_THREADS];

	size = img.height / NUM_THREADS;
	rs = ceil(RADIUS * 2.57);

	for (i = 0; i < NUM_THREADS; i++) {
		params[i].srcR = img.dataR;
		params[i].srcG = img.dataG;
		params[i].srcB = img.dataB;
		
		params[i].dstR = img2.dataR;
		params[i].dstG = img2.dataG;
		params[i].dstB = img2.dataB;
		
		params[i].h = img.height;
		params[i].w = img.width;
		params[i].r = RADIUS;
		params[i].rs = rs;
		
		params[i].start = i * size;
		params[i].end = (i + 1) * size;
		
		if (i == NUM_THREADS - 1)
			params[i].end = img.height;
		
		if(pthread_create(&threads[i], NULL, blur, &params[i])) {
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
	}
	
	for (i = 0; i < NUM_THREADS; i++)
		if(pthread_join(threads[i], NULL)) {
			fprintf(stderr, "Error joining thread\n");
			return 2;
		}

	free_image_memory(&img);

	writeData("out.pnm", &img2);

	return 0;
}
