#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "tools/alerts.h"
#include <signal.h>
#include <sys/time.h>
#include <limits.h>

// Used for debug print so that even signal handlers cpuld print PID
int pid;


// writing to FIFO must be atomic
#define BUFSIZE PIPE_BUF


#define NAMESIZE 100
#define FIFO_NAME "fifo"


#define TRESHOLD 100000


//============================================================================
//                              SIGALRM HANDLER
//============================================================================

void alarm_handler(int sigio) {
    ERR("Timer expired. FIFO is still not opened, closing...\n")
};


//============================================================================
//                CREATE/CHECK EXISTENCE OF FILE "PID_FIFO"
//============================================================================

char* create_fifo(int client_pid) {
    char *name = (char*)calloc(100, sizeof(char));
    sprintf(name, "%d_", client_pid);
    strcat(name, FIFO_NAME);

    if (mkfifo(name, 0666) == -1) { 
        if (errno != EEXIST) 
            ERR("ERROR: special FIFO cant'be created, "\
                    "client's PID: %d\n", client_pid)

        PRINT("OK: Special FIFO \"%s\" already exists.\n", name) 
        
    }
    else 
        PRINT("Special FIFO \"%s\" created.\n", name) 
    return name;
}




//============================================================================
//                               SERVER CODE
//============================================================================
void do_server(char* file_name) {

    //------------------------------------------------------------------------
    //              MAIN FIFO PROCESSING (OPENING, READING PID)
    //------------------------------------------------------------------------

    // Main FIFO with name FIFO NAME is used to send client the name
    // of input file otherwise it would be impossible to determine 
    // which of the clients has connected to that particular server

    char buf[BUFSIZE];

    // input file, main FIFO, client-specific FIFO respectively
    int fd, main_fifo_fd, client_fd;

    if ((fd = open(file_name, 0)) == -1) 
        ERR("ERROR: cannot open file\n")
    PRINT("File opened\n")

    if ((main_fifo_fd = open(FIFO_NAME, O_RDWR)) == -1) 
        ERR("ERROR: cannot open pipe\n")
    PRINT("Main FIFO opened!\n")

    int bytesnum, client_pid;
    bytesnum = read(main_fifo_fd, &client_pid, sizeof(int));
    
    if (bytesnum != sizeof(int)) 
        ERR("Not enought bytes got reading client's PID, "\
                "%d bytes read\n", bytesnum)
    PRINT("Read main FIFO, client's PID: %d\n", client_pid)



    //------------------------------------------------------------------------
    //              SPECIAL FIFO PROCESSING (CREATING, OPENING)
    //------------------------------------------------------------------------
    
    signal(SIGALRM, alarm_handler);

    char* client_fifo_name = create_fifo(client_pid);
    
    struct itimerval it_val;  /* for setting itimer */

    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = TRESHOLD; 
    it_val.it_interval.tv_sec = 0;
    it_val.it_interval.tv_usec = 0;

    PRINT("Setting timer for 0.1 sec...\n")
    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1)
        ERR("ERROR calling setitimer, errno: %s\n", strerror(errno))

    if ((client_fd = open(client_fifo_name, O_WRONLY)) == -1) 
        ERR("ERROR while opening special FIFO, errno: %s\n", strerror(errno))

    it_val.it_value.tv_usec = 0; 
    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1)
        ERR("ERROR cancelling setitimer, errno: %s\n", strerror(errno))

    PRINT("Connection via special FIFO established\n")
    



    //------------------------------------------------------------------------
    //                      WRITING TO SPECIAL FIFO
    //------------------------------------------------------------------------

    if (write(client_fd, file_name, NAMESIZE) == -1) 
        ERR("ERROR while writing to FIFO (sending name), "\
                "errno: %s\n", strerror(errno))

    while ((bytesnum = read(fd, &buf, BUFSIZE))) {
        if (write(client_fd, &buf, bytesnum) == -1)
            ERR("ERROR while writing to FIFO, errno: %s\n", strerror(errno))
    }
    
    close(main_fifo_fd);
    close(client_fd);
};







//============================================================================
//                               CLIENT CODE
//============================================================================


void do_client() {

    //------------------------------------------------------------------------
    //              MAIN FIFO PROCESSING (OPENING, WRITING PID)
    //------------------------------------------------------------------------

    char buf[BUFSIZE];

    int fd, main_fifo_fd, client_fd;
    int bytesnum;


    if ((main_fifo_fd = open(FIFO_NAME, O_WRONLY)) == -1) 
        ERR("ERROR: cannot open pipe\n")

    PRINT("Main FIFO opened!\n")

    
    int client_pid = getpid();
    write(main_fifo_fd, &client_pid, sizeof(int));

    PRINT("Sent PID to main FIFO\n")


    //------------------------------------------------------------------------
    //              SPECIAL FIFO PROCESSING (CREATING, OPENING)
    //------------------------------------------------------------------------

    signal(SIGALRM, alarm_handler);

    char* client_fifo_name = create_fifo(client_pid);
 
    struct itimerval it_val;  /* for setting itimer */

    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = TRESHOLD; 
    it_val.it_interval.tv_sec = 0;
    it_val.it_interval.tv_usec = 0;

    PRINT("Setting timer for 0.1 sec...\n")
    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1)
        ERR("ERROR calling setitimer, errno: %s\n", strerror(errno))

    if ((client_fd = open(client_fifo_name, O_RDONLY)) == -1) 
        ERR("ERROR while opening special FIFO, errno: %s\n", strerror(errno))

    it_val.it_value.tv_usec = 0; 
    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1)
        ERR("ERROR cancelling setitimer, errno: %s\n", strerror(errno))

    PRINT("Connection via special FIFO established\n")




    //------------------------------------------------------------------------
    //                         READING FROM SPECIAL FIFO
    //------------------------------------------------------------------------

    char file_name[NAMESIZE];
    memset(file_name, 0, NAMESIZE);

    if (read(client_fd, file_name, NAMESIZE) == -1) 
        ERR("ERROR reading filename from special FIFO, "\
                "errno %s\n", strerror(errno))


    if (strlen(file_name) > 0) {
        strcat(file_name, ".out");

        if ((fd = creat(file_name, 0666)) == -1) 
            ERR("ERROR: cannot create file \"%s\"", file_name)
        PRINT("Created file, name \"%s\"\n", file_name)
        
        while ((bytesnum = read(client_fd, &buf, BUFSIZE))) {
            if (bytesnum == -1) 
                ERR("ERROR reading from special FIFO, "\
                        "errno %s\n", strerror(errno))
            

            write(STDOUT_FILENO, &buf, bytesnum);
            write(fd, &buf, bytesnum);
        }   
    }
    else 
        ERR("ERROR: hasn't got output file name from specific FIFO\n")

    fsync(fd);
    close(fd);
    close(main_fifo_fd);
    close(client_fd);

    unlink(client_fifo_name); // delete special FIFO
    free(client_fifo_name);     
};




int main(int argc, char *argv[]) {
    // Used for printing DEBUG messages and ERRORs
    pid = getpid();
    char* file_name = calloc(NAMESIZE, sizeof(char));
    int is_server;

    if (argc == 1) 
        is_server = 0;

    else if (argc == 2) {
        is_server = 1;
        strncpy(file_name, argv[1], NAMESIZE);
    }

    else 
        ERR("Usage: %s [file (if server)]\n", argv[0])
    
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        if (errno != EEXIST) 
            ERR("ERROR: main FIFO can't be created")
    }


    if (is_server)
        do_server(file_name);
    else
        do_client();

    PRINT("Successfuly done.\n")

    free(file_name);
    exit(EXIT_SUCCESS);
}
