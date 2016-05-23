//Maria Joao Mira Paulo e Nuno Ramos
//2º Trabalho SOPE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // For O_* constants
#include <semaphore.h>

#define PARQUE_FILE_NAME "parque.log"
#define FIFO_NAME_LENGTH 10
#define OK 0
#define VEHICLE_IN 0
#define PARK_FULL 1
#define PARK_CLOSED 2
#define VEHICLE_OUT 3
#define PARKING 4
#define LAST_VEHICLE_ID -1
#define STATUS_MAX_LENGTH 20
#define FILE_LINE_MAX_LENGTH 100



//Park_close variable indicates park's state (0 means is open, 1 means is closed)
int park_close;

//total number of car-parking spaces on park
int park_capacity;

//number of unavailable car-parking spaces on park
int unavailable_space;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

int fd_parque_log;

//Direction enums are the four cardinal points of access to the park
typedef enum {NORTH, SOUTH, EAST, WEST} Direction;

typedef struct {
  Direction direction;
  int id;
  float parking_time;
  char fifo_name[FIFO_NAME_LENGTH] ;
  int initial_ticks;

}Vehicle;

void write_to_log_file(Vehicle *vehicle, int state){
  char buffer[FILE_LINE_MAX_LENGTH];
  char status[STATUS_MAX_LENGTH];

  switch(state){
    case 0:
    strcpy(status, "entrada");
    break;
    case 1:
    strcpy(status, "cheio");
    break;
    case 2:
    strcpy(status, "fechado");
    break;
    case 3:
    strcpy(status, "saida");
    break;
    case 4:
    strcpy(status, "estacionamento");
    break;

  }

  sprintf(buffer, "%-8d ; %4d ; %7d ; %s\n",vehicle->initial_ticks, (park_capacity - unavailable_space),vehicle->id, status);

  write(fd_parque_log,buffer,strlen(buffer));

  strcpy(buffer, "");

}

void* vehicle_guide(void* arg){
  Vehicle vehicle= *(Vehicle*) arg;
  void* ret=NULL;
  int state;
  int fd_write;

  fd_write=open(vehicle.fifo_name,O_WRONLY);
  //verify park state
  sleep(1);
  printf("ID VEICULO:%d\n",vehicle.id);

  //Mutex that controlls the number of available spaces to park a car. The mutex lock all the others threads when acess to varibles unavailable_space and park_capacity
  pthread_mutex_lock(&mutex);
  if(unavailable_space<park_capacity && !park_close){
    state=VEHICLE_IN;
    unavailable_space++;
    write(fd_write,&state,sizeof(int));
    pthread_mutex_lock(&file_mutex);
    write_to_log_file(&vehicle, PARKING);
    pthread_mutex_unlock(&file_mutex);
    printf("Entrei no parque!! ID %d capacidade %d, lugares %d\n",vehicle.id, park_capacity, unavailable_space);
    pthread_mutex_unlock(&mutex);
    usleep(vehicle.parking_time*pow(10,3));
    unavailable_space--;
    state=VEHICLE_OUT;
  }
  else if(park_close){
    pthread_mutex_unlock(&mutex);
    printf("Parque Fechado!!!!\n");
    state=PARK_CLOSED;
  }
  else {
    pthread_mutex_unlock(&mutex);
    state=PARK_FULL;
    printf("Parque Cheio!!!!\n");
  }
  printf("ID VEICULO:%d  State: %d\n", vehicle.id, state);
  write(fd_write,&state,sizeof(int));

  pthread_mutex_lock(&file_mutex);
  write_to_log_file(&vehicle, state);
  pthread_mutex_unlock(&file_mutex);

  close(fd_write);

  return ret;
}

//Function that the thread with tid_n executes when is created
void* func_north(void* arg){
  void* ret = NULL;
  int fd_read;
  Vehicle vehicle;
  int read_ret;
  pthread_t tid_n;

  mkfifo("fifoN", 0660);

  printf("Vou abrir o fifo\n");

  fd_read = open("fifoN", O_RDONLY | O_NONBLOCK);
  //open on write mode to avoid busy waiting
  open("fifoN", O_WRONLY);

  printf("Já abri os fifos\n");

    while(1){
      read_ret = read(fd_read, &vehicle, sizeof(Vehicle));
      if(vehicle.id == LAST_VEHICLE_ID)
        break;
      else  if(read_ret > 0){
        printf("PARQUE NORTE ID : %d\n", vehicle.id);
        if(pthread_create(&tid_n,NULL,vehicle_guide,&vehicle) != OK)
        perror("Func_North::Error on creating thread\n");
      }
    }

  printf("Norte: vou acabar\n");

  close(fd_read);

  unlink("fifoN");

  return ret;

}

//Function that the thread with tid_s executes when is created
void* func_south(void* arg){
  void* ret = NULL;
  int fd_read;
  Vehicle vehicle;
  int read_ret;
  pthread_t tid_n;


  mkfifo("fifoS", 0660);

  printf("Vou abrir o fifo\n");

  fd_read = open("fifoS", O_RDONLY | O_NONBLOCK);
  //open on write mode to avoid busy waiting
  open("fifoS", O_WRONLY);

  printf("Já abri os fifos\n");

  while(1){
    read_ret = read(fd_read, &vehicle, sizeof(Vehicle));
    if(vehicle.id == LAST_VEHICLE_ID)
      break;
    else  if(read_ret > 0){
      printf("PARQUE SUL ID : %d\n", vehicle.id);
      if(pthread_create(&tid_n,NULL,vehicle_guide,&vehicle) != OK)
      perror("Func_South::Error on creating thread\n");
    }
  }

  printf("Sul: vou acabar\n");

  close(fd_read);

  unlink("fifoS");

  return ret;
}

//Function that the thread with tid_e executes when is created
void* func_east(void* arg){
  void* ret = NULL;
  int fd_read;
  Vehicle vehicle;
  int read_ret;
  pthread_t tid_n;


  mkfifo("fifoE", 0660);

  printf("Vou abrir o fifo\n");

  fd_read = open("fifoE", O_RDONLY | O_NONBLOCK);
  //open on write mode to avoid busy waiting
  open("fifoE", O_WRONLY);

  printf("Já abri os fifos\n");

  while(1){
    read_ret = read(fd_read, &vehicle, sizeof(Vehicle));
    if(vehicle.id == -1)
      break;
    else  if(read_ret > 0){
      printf("PARQUE ESTE ID : %d\n", vehicle.id);
      if(pthread_create(&tid_n,NULL,vehicle_guide,&vehicle) != OK)
      perror("Func_East::Error on creating thread\n");
    }
  }

  printf("Este: vou acabar\n");

  close(fd_read);

  unlink("fifoE");

  return ret;
}

//Function that the thread with tid_w executes when is created
void* func_west(void* arg){
  void* ret = NULL;
  int fd_read;
  Vehicle vehicle;
  int read_ret;
  pthread_t tid_n;

  mkfifo("fifoW", 0660);

  printf("Vou abrir o fifo\n");

  fd_read = open("fifoW", O_RDONLY | O_NONBLOCK);
  //open on write mode to avoid busy waiting
  open("fifoW", O_WRONLY);

  printf("Já abri os fifos\n");

  while(1){
    read_ret = read(fd_read, &vehicle, sizeof(Vehicle));
    if(vehicle.id == -1)
      break;
    else  if(read_ret > 0){
      printf("PARQUE OESTE ID : %d\n", vehicle.id);
      if(pthread_create(&tid_n,NULL,vehicle_guide,&vehicle) != OK)
      perror("Func_West::Error on creating thread\n");
    }
  }

  printf("Oeste: vou acabar\n");

  close(fd_read);

  unlink("fifoW");

  return ret;
}

void opening_park(){

  pthread_t tid_n, tid_s, tid_e, tid_w;

  //Creating the thread controller on the north pole of the park
  if(pthread_create(&tid_n,NULL,func_north,NULL) != OK)
  perror("Parque::Error on creating thread\n");
  //Creating the thread controller on the south pole of the park
  if(pthread_create(&tid_s,NULL,func_south,NULL) != OK)
  perror("Parque::Error on creating thread\n");
  //Creating the thread controller on the east pole of the park
  if(pthread_create(&tid_e,NULL,func_east,NULL) != OK)
  perror("Parque::Error on creating thread\n");
  //Creating the thread controller on the west pole of the park
  if(pthread_create(&tid_w,NULL,func_west,NULL) != OK)
  perror("Parque::Error on creating thread\n");
}

int main(int argc, char* argv[]){

  int number_of_spots=atoi(argv[1]);
  int time_open=atoi(argv[2]);
  //Initializing the park with the number of spots
  park_capacity = number_of_spots;
  unavailable_space = 0;
  pthread_t tid_n, tid_s, tid_e, tid_w;

  //Initialize the last vehicle to inform the park is closed. last_vehicle id is equal to -1
  Vehicle last_vehicle;
  last_vehicle.id = -1;
  last_vehicle.parking_time = 0;
  strcpy(last_vehicle.fifo_name, "over");


  char name[]="/sem";
  sem_t *semaphore = sem_open(name, O_CREAT ,0660,1);

  //name of program, park's number of spots and park's time open
  if(argc != 3){
    perror("Invalid number of arguments.\n\n");
  }

  //The park is open
  park_close = 0;

  //opening the file "parque.log" to write vehicles information
  fd_parque_log = open(PARQUE_FILE_NAME, O_WRONLY | O_CREAT  , 0600);

  char buffer[] = "t(ticks) ; nlug ; id_viat ; observ\n";

  write(fd_parque_log,buffer,strlen(buffer));

  opening_park();
  sleep(time_open);
  printf("Vou acabar\n");

  //The park is closed
  park_close = 1;

  int fd_north = open("fifoN", O_WRONLY);
  int fd_south = open("fifoS", O_WRONLY);
  int fd_east = open("fifoE", O_WRONLY);
  int fd_west = open("fifoW", O_WRONLY);


  sem_wait(semaphore);

  //Send to north controller a vehicle that tells the park is closed (last_vehicle has id -1)
  write(fd_north, &last_vehicle, sizeof(Vehicle));

  //Send to south controller a vehicle that tells the park is closed (last_vehicle has id -1)
  write(fd_south, &last_vehicle, sizeof(Vehicle));

  //Send to east controller a vehicle that tells the park is closed (last_vehicle has id -1)
  write(fd_east, &last_vehicle, sizeof(Vehicle));

  //Send to west controller a vehicle that tells the park is closed (last_vehicle has id -1)
  write(fd_west, &last_vehicle, sizeof(Vehicle));

  sem_post(semaphore);
  sem_close(semaphore);

  close(fd_north);
  close(fd_south);
  close(fd_east);
  close(fd_west);

  //pthread_join() function waits for the north thread to terminate
  if(pthread_join(tid_n, NULL) != OK)
  perror("Parque::Error on joinning thread\n");
  //pthread_join() function waits for the south thread to terminate
  if(pthread_join(tid_s, NULL) != OK)
  perror("Parque::Error on joinning thread\n");
  //pthread_join() function waits for the east thread to terminate
  if(pthread_join(tid_e, NULL) != OK)
  perror("Parque::Error on join thread\n");
  //pthread_join() function waits for the west thread to terminate
  if(pthread_join(tid_w, NULL) != OK)
  perror("Parque::Error on join thread\n");

  //Wait for all the vehicles till the park is empty, then end the program

//  while(unavailable_space!=0){}

  sem_unlink(name);

  pthread_exit(NULL);
}
