MPICC=mpicc
MPIEXEC=mpirun
CC=gcc
CFLAGS  = -g -Wall
LFAGS = -lm
OPENMP_FLAGS = -fopenmp
PTHREAD_FLAGS = -lpthread

serial-original: blurSerialOriginal.c 		## Builds original, unmodified serial algorithm
	$(CC) $< -o blurSerialOriginal.out $(CFLAGS) $(LFAGS)
	
serial-improved: blurSerialImproved.c 		## Builds improved serial algorithm
	$(CC) $< -o blurSerialImproved.out $(CFLAGS) $(LFAGS)
	
openmp: blurOpenMP.c 				## Builds OpenMP parallel algorithm
	$(CC) $< -o blurOpenMP.out $(CFLAGS) $(LFAGS)
	
pthreads: blurPthreads.c 			## Builds pThreads parallel algorithm
	$(CC) $< $(PTHREAD_FLAGS) -o blurPthreads.out $(CFLAGS) $(LFAGS)
	
mpi: blurMPI.c 					## Builds MPI parallel algorithm
	$(MPICC) $< -o blurMPI.out $(CFLAGS) $(LFAGS)

hybrid: blurHybrid.c 				## Builds the hybrid parallel algorithm
	$(MPICC) $< -o blurHybrid.out $(OPENMP_FLAGS) $(PTHREAD_FLAGS) $(CFLAGS) $(LFAGS)

.PHONY: run-serial-original run-serial-improved run-mpi run-openmp run-pthreads run-hybrid diff clean help

run-serial-original: blurSerialOriginal.out 	## Run the original, unmodified serial algorithm
	./$<

run-serial-improved: blurSerialImproved.out 	## Run the improved serial algorithm
	./$<
	
run-openmp: blurOpenMP.out 			## Run the OpenMP algorithm
	./$<
	
run-pthreads: blurPthreads.out 			## Run the pThreads algorithm
	./$<
	
run-mpi: blurMPI.out 				## Run the MPI algorithm with 3 tasks
	$(MPIEXEC) -np 3 ./$<
	
run-hybrid: blurHybrid.out 			## Run the hybrid parallel algorithm with 3 MPI tasks
	$(MPIEXEC) -np 3 ./$<
	
diff: out.pnm outRef.pnm		 	## Compare output image with the reference image
	diff out.pnm outRef.pnm
	
clean: 						## Remove executables, objects and output picture
	@rm -f *.out *.o out.pnm

help:						## Show this help.
	@fgrep -h "##" $(MAKEFILE_LIST) | fgrep -v fgrep | sed -e 's/\\$$//' | sed -e 's/##//'

