# A small C program for an optimized image blurring algorithm

## How to use the program
This program is intended to be run on a Linux platform (but could be easily ported to other platforms).

###Prerequisites
* gcc (https://gcc.gnu.org/)
* OpenMP (http://www.openmp.org/)
* pthreads (POSIX threads)
* MPICH (http://www.mpich.org/) - Implementation of the **Message Passing Interface (MPI)** standard.

###Building & Running
Use the provided Makefile 
<br>**Tip:** run `make help` to find relevant information about target and file names

###The input image files:
The format for the input files is **pnm** (http://netpbm.sourceforge.net/doc/pnm.html)<br>
**Tip:** We provide an `in.pnm` to serve for testing the program

####Converting jpeg files to the pnm format
Use the Linux utility jpegtopnm
**example**: `jpegtopnm in.jpg > outExample.pnm`

###Running on the faculty cluster (fep.pub.grid.ro)
Use the provided `script.sh` (that uses the Makefile)
<br>**Tip:** script.sh receives one arugument that's related to the Makefile target.
<br>Currently, it can be:
* **serial-original**
* **serial-improved**
* **openmp**
* **pthreads**
* **mpi**
* **hybrid**

**example:** `qsub -cwd -q ibm-nehalem.q -pe openmpi 3 script.sh mpi`

##Authors:
* Adrian Niculescu (adrianniculescu@yahoo.com) 343C1 <br>
* Nicolae Dragos (dragosnicolae23@gmail.com) 343C1 <br>
* Ciucu Madalina (madalina.ciucu@gmail.com) 343C1

Facultatea de Automatica si Calculatoare (http://acs.pub.ro/)
