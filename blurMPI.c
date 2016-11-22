#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define PI 3.14
#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

//MPI
#define MPI_MASTER 0
#define MPI_RED_WORKER 0
#define MPI_GREEN_WORKER 1
#define MPI_BLUE_WORKER 2

typedef struct image
{
	int width, height;
	int maxval;
	unsigned char **dataR, **dataG, **dataB;
} image;

int num_threads;
int resize_factor;
int gaussian_kernel[3][3] = {{1, 2, 1},
						     {2, 4, 2},
						     {1, 2, 1}};

 void allocate_image_memory_for_color(image *img, char color) {

	int i;

	switch(color) {

		case 'R': {
			img->dataR = (unsigned char **)malloc(img->height * sizeof(unsigned char *));
			for(i = 0; i < img->height; i++) {
				img->dataR[i] = (unsigned char *)malloc(img->width);
			}
			break;
		}
		case 'G': {
			img->dataG = (unsigned char **)malloc(img->height * sizeof(unsigned char *));
			for(i = 0; i < img->height; i++) {
				img->dataG[i] = (unsigned char *)malloc(img->width);
			}
			break;
		}
		case 'B': {
			img->dataB = (unsigned char **)malloc(img->height * sizeof(unsigned char *));
			for(i = 0; i < img->height; i++) {
				img->dataB[i] = (unsigned char *)malloc(img->width);
			}
			break;
		}
	}	
}

void allocate_image_memory(image *img) {

	allocate_image_memory_for_color(img, 'R');
	allocate_image_memory_for_color(img, 'G');
	allocate_image_memory_for_color(img, 'B');
}

void free_image_memory_for_color(image *img, char color) {

	int i;

	switch(color) {

		case 'R': {
			for (i = 0; i < img->height; i++) {
				free(img->dataR[i]);
			}
	
			free(img->dataR);
			break;
		}
		case 'G': {
			for (i = 0; i < img->height; i++) {
				free(img->dataG[i]);
			}
			free(img->dataG);
			break;
		}
		case 'B': {
			for (i = 0; i < img->height; i++) {
				free(img->dataB[i]);
			}
			free(img->dataB);
			break;
		}
	}
}


void free_image_memory(image *img) {

	free_image_memory_for_color(img,'R');
	free_image_memory_for_color(img,'G');
	free_image_memory_for_color(img,'B');

}

void readInput(const char * fileName, image *img) {

	//open the file
	FILE *in = fopen(fileName, "rb");

	//read the type of file
	char str[3];
	fgets(str, 3, in);
	if(str[1] != '6') {
		fprintf(stderr, "Input is not a color image!\n");
		exit(1);
	}

	//read the size of the photo
	fscanf(in, "%d%d", &img->width, &img->height);

	//read the max value
	fscanf(in, "%d", &img->maxval);

	//read the extra new line
	fgetc(in);

	//read the pixels' values
	int i, j;

	allocate_image_memory(img);

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

	//close the file
	fclose(in);
}

void writeData(const char * fileName, image *img) {
	//open the file
	FILE *out = fopen(fileName, "wb");

	//write the type of file
	fprintf(out, "P6\n");

	//write the size of the photo
	fprintf(out, "%d %d\n", img->width, img->height);

	//write the max value
	fprintf(out, "%d\n", img->maxval);

	//write the pixels' values
	int i, j;

	unsigned char *buffer;

	buffer = (unsigned char *)malloc(3 * img->width);

	for(i = 0; i < img->height; i++) {
		for(j = 0; j < img->width; j++) {
			buffer[3 * j] = img->dataR[i][j];
			buffer[3 * j + 1] = img->dataG[i][j];
			buffer[3 * j + 2] = img->dataB[i][j];
		}

		fwrite(buffer, 1, 3 * img->width, out);
	}

	free(buffer);

	free_image_memory(img);

	//close the file
	fclose(out);
}

void blur (unsigned char **src, unsigned char **dst, int h, int w, int r) {
    int i, j, iy, ix, x, y, dsq;
    double wght, val, wsum;

    int rs = ceil(r * 2.57);
    
    for(i = 0; i < h; i++)
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

int main(int argc, char *argv[]) {

	int  numtasks, rank, i;
	MPI_Status status;
	char color;
	image imgIn, imgOut;

	// initialize MPI  
	MPI_Init(&argc,&argv);
	
	// get number of tasks 
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

	// get my rank  
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	if (rank == MPI_RED_WORKER) {
		color = 'R';
	} else if (rank == MPI_GREEN_WORKER) {
		color = 'G';
	} else if (rank == MPI_BLUE_WORKER) {
		color = 'B';
	}

	// The Master reads the input image and sends tasks to workers
	if (rank == MPI_MASTER) {
		readInput("in.pnm", &imgIn);
	}

	MPI_Bcast (&imgIn.width, 1, MPI_INT, MPI_MASTER, MPI_COMM_WORLD);
	MPI_Bcast (&imgIn.height, 1, MPI_INT, MPI_MASTER, MPI_COMM_WORLD);
	MPI_Bcast (&imgIn.maxval, 1, MPI_INT, MPI_MASTER, MPI_COMM_WORLD);

	imgOut.width = imgIn.width;
	imgOut.height = imgIn.height;
	imgOut.maxval = imgIn.maxval;

	if (rank == MPI_MASTER) {

		allocate_image_memory(&imgOut);

		for (i = 0; i < imgIn.height; i++) {
			MPI_Send(imgIn.dataG[i], imgIn.width, MPI_UNSIGNED_CHAR, MPI_GREEN_WORKER, 0, MPI_COMM_WORLD);
		}
		for (i = 0; i < imgIn.height; i++) {
			MPI_Send(imgIn.dataB[i], imgIn.width, MPI_UNSIGNED_CHAR, MPI_BLUE_WORKER, 0, MPI_COMM_WORLD);
		}

	} else if (rank == MPI_GREEN_WORKER || rank == MPI_BLUE_WORKER) {

		allocate_image_memory_for_color(&imgIn, color);
		allocate_image_memory_for_color(&imgOut, color);

		if (rank == MPI_GREEN_WORKER) {
			for (i = 0; i < imgIn.height; i++) {
				MPI_Recv(imgIn.dataG[i], imgIn.width, MPI_UNSIGNED_CHAR, MPI_MASTER, 0, MPI_COMM_WORLD, &status);
			}
		} else {
			for (i = 0; i < imgIn.height; i++) {
				MPI_Recv(imgIn.dataB[i], imgIn.width, MPI_UNSIGNED_CHAR, MPI_MASTER, 0, MPI_COMM_WORLD, &status);
			}
		}
	}
	
	// Execute the blur
	if (rank == MPI_RED_WORKER) {
	
		printf("Blurring R\n");
		blur(imgIn.dataR, imgOut.dataR, imgIn.height, imgIn.width, 10);
	
	} else if (rank == MPI_GREEN_WORKER) {
	
		printf("Blurring G\n");
		blur(imgIn.dataG, imgOut.dataG, imgIn.height, imgIn.width, 10);
		
	} else if (rank == MPI_BLUE_WORKER) {
	
		printf("Blurring B\n");
		blur(imgIn.dataB, imgOut.dataB, imgIn.height, imgIn.width, 10);
	}
	
	// Gather data to the Master
	if (rank == MPI_GREEN_WORKER) {

		for (i = 0; i < imgIn.height; i++) {
			MPI_Send(imgOut.dataG[i], imgOut.width, MPI_UNSIGNED_CHAR, MPI_MASTER, 0, MPI_COMM_WORLD);
		}

	} else if (rank == MPI_BLUE_WORKER) {

		for (i = 0; i < imgIn.height; i++) {
			MPI_Send(imgOut.dataB[i], imgOut.width, MPI_UNSIGNED_CHAR, MPI_MASTER, 0, MPI_COMM_WORLD);
		}
	
	} else if (rank == MPI_MASTER) {

		for (i = 0; i < imgIn.height; i++) {
			MPI_Recv(imgOut.dataG[i], imgOut.width, MPI_UNSIGNED_CHAR, MPI_GREEN_WORKER, 0, MPI_COMM_WORLD, &status);
		}

		for (i = 0; i < imgIn.height; i++) {
			MPI_Recv(imgOut.dataB[i], imgOut.width, MPI_UNSIGNED_CHAR, MPI_BLUE_WORKER, 0, MPI_COMM_WORLD, &status);
		}
	}
	
	if (rank == MPI_MASTER) {
		free_image_memory(&imgIn);
		// the Master writes the output
		writeData("out.pnm", &imgOut);
	} else if (rank == MPI_GREEN_WORKER || rank == MPI_BLUE_WORKER) {
		free_image_memory_for_color(&imgIn, color);
		free_image_memory_for_color(&imgOut, color);
	}
	
	MPI_Finalize();

	return 0;
}
