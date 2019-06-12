#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* This structure will be mapped to shared memory so that both the parent and child process will have access to it
 * Structuring it this way garuntees that access to each variable is atomic  */
typedef struct Petersons_Variables {
	int turn;
	int flag[2]; 
} pv;
int nloop = 50;

/**********************************************************
 * * Function: increment a counter by some amount one by one **
 *  argument: ptr (address of the counter), increment       *
 * * output  : nothing                                       *
 ***********************************************************/
void add_n(int *ptr, int increment){
	int i,j;
	for (i=0; i < increment; i++){
		*ptr = *ptr + 1;
		for (j=0; j < 1000000;j++);
	}
}
/*******************************************************
 * * Function: Sets up the variables in the struct of petersons
 * * Input: pointer to a petersons struc
 * * Output: NA 
 *******************************************************/
void initialize (pv *v) {
	v->flag[0] = 0;
	v->flag[1] = 0;
}
int main(){
	int pid;        /* Process ID                     */

	int *countptr;  /* pointer to the counter         */

	int fd;     /* file descriptor to the file "containing" my counter */
	int zero = 0; /* a dummy variable containing 0 */

	system("rm -f counter");

	/* create a file which will "contain" my shared variable */
	fd = open("counter",O_RDWR | O_CREAT);
	write(fd,&zero,sizeof(int));

	/* map my file to memory */
	countptr = (int *) mmap(NULL, sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);



	if (!countptr) {
		printf("Mapping failed\n");
		exit(1);
	}
	*countptr = 0;

	close(fd);

	/* Peterson Mapping added by Kevin
	 * This is where the struct at the top is mapped.  */
	system("rm -f memfile"); 
	int memd;
	pv *variables;
	memd = open("memfile", O_RDWR | O_CREAT);
	write(memd,&zero,sizeof(pv));

	variables  = (pv *) mmap(NULL, sizeof(pv), PROT_READ | PROT_WRITE,
			MAP_SHARED, memd, 0);	
	if(!variables) {
		printf("Unable to map variables");
		exit(1);
	}
	initialize(variables);
	close(memd);
	pid = fork();
	if (pid == 0) 
	{
		while(1) 
		{
			//Initialization
			variables->flag[0] = 1;
			variables->turn = 1;
			//Entry
			while(variables->flag[1] == 1 && variables->turn == 1);
			//Crit Start
			/* The child increments the counter by two's */
			if (*countptr < nloop)
			{
				add_n(countptr,2);
				printf("Child process -->> counter = %d\n",*countptr);
			} else {
				variables->flag[0] = 0;
				close(memd);
				close(fd);
				break;
			}
			//Crit End

			////Exit 
			variables->flag[0] = 0;
			close(memd);
			close(fd);

		}			
	}
	else {
		while(1)
		{
			//Initialization
			variables->flag[1] = 1;
			variables->turn = 0;
			//Entry
			while(variables->flag[0] == 1 && variables->turn == 0);
			//Crit Start
			/* The parent increments the counter by twenty's */
			if (*countptr < nloop){
				add_n(countptr,20);
				printf("Parent process -->> counter = %d\n",*countptr);
			} else {
				variables->flag[1] = 0;
				close(memd);
				close(fd);
				break;
			}
			//Crit End
			////Exit 
			variables->flag[1] = 0;
			close(memd);
			close(fd);
		}
	}
}










