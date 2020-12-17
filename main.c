#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <math.h>
#include "show_scores.h"

#define PRODUCER_FNAME "/display"
#define CONSUMER_FNAME "/car"

int calculate_max_tours(double total_km);
void define_session(int argc, char *argv[]);

int car_number[20] = {44, 77, 16, 5, 33, 23, 55, 4, 3, 31, 10, 26, 11, 18, 7, 99, 20, 8, 63, 6}; //Ensemble des nombres utilisés pour les voitures formule 1
struct Session current_session;

int main(int argc, char *argv[]){
    struct Car *race_car;               //Pointeur vers la structure Car
    int shared_mem_id;                  //ID de la mémoire partagée
    pid_t process_id;                   //ID du processus

    define_session(argc, argv);


    //Création du segment de mémoire patagé
    shared_mem_id = shmget(IPC_PRIVATE, sizeof(struct Car) * current_session.total_cars, 0600 | IPC_CREAT);
    if(shared_mem_id == -1){
        write(1, "Error while creating the shared memory segment", sizeof("Error while creating the shared memory segment"));
        exit(-1);
    }


    //On attache la mémoire partagée à la structure Car
    race_car = shmat(shared_mem_id, NULL, 0);
    if(race_car == (void *) (-1)){
        write(1, "Error while attaching the memory segment", sizeof("Eror while attaching the memory segment"));
        exit(-1);
    }


    //On crée un sémaphore pour la mémoire partagée dont le flag correpond au flag 0600 (plus besoin de créér une nouvelle mémoire partagée)
    int sem_shmid = shmget(IPC_PRIVATE, sizeof(sem_t), 0600 | IPC_CREAT);
    if(sem_shmid == -1){
        printf("shmget failed\n");
        exit(EXIT_FAILURE);
    }

    //Attachement du sémaphore au segment de mémoire partagée
    sem_t *cons_sema = shmat(sem_shmid, NULL, 0);
    if(cons_sema == (void *)(-1)){
        printf("shmat failed");
        exit(EXIT_FAILURE);
    }

    sem_shmid = shmget(IPC_PRIVATE, sizeof(sem_t), 0600 | IPC_CREAT);
    if(sem_shmid == -1){
        printf("shmget failed\n");
        exit(EXIT_FAILURE);
    }

    //Attachement du sémaphore au segment de mémoire partagée
    sem_t *prod_sema = shmat(sem_shmid, NULL, 0);
    if(prod_sema == (void *)(-1)){
        printf("shmat failed");
        exit(EXIT_FAILURE);
    }

    //Initialisation d'un nouveau sémaphore sur sema
    sem_init(cons_sema, 1, 1);
    sem_init(prod_sema, 1, 0);


    //Création des processus fils
    int i;
    for( i = 0 ;  i < current_session.total_cars ; i++ ){
        process_id = fork();
        if(process_id == 0){
            break;
        }
    }


    if(process_id == -1){
        //En cas d'erreur du fork
        write(1, "Error while forking", sizeof("Error while forking"));
        exit(-1);
    }
    else if(process_id == 0){
        //Processus enfant
        drive_race_car(&race_car[i], &car_number[i], prod_sema, cons_sema);
        exit(0);
    }
    else{
        //Processus parent
        show_score_table(race_car, prod_sema, cons_sema);

        //Cette boucle permet au programme d'attendre que tous les processus enfants se ferment avant de conrinuer le programme
        for(int j = 0 ; j < current_session.total_cars ; j++){
            wait(NULL);
        }
    }


    //détachement du segment de mémoire partagée
    shmdt(race_car);


    //Déstruction du segment de mémoire partagée
    shmctl(shared_mem_id, IPC_RMID, NULL);

    return 0;
}


void define_session(int argc, char *argv[]){
    double total_km = 0;

    if (argc < 2 || argc > 4){
        write(1, "error: incorrect numbers of arguments", sizeof("error: incorrect numbers of arguments"));
        exit(-1);
    }
    else if (!strcmp(argv[1], "race") && argc == 2){
        write(1, "error: an additional argument is required with 'P1'", sizeof("error: an additional argument is required with 'P1'"));
        exit(-1);
    }
    else if (strcmp(argv[1], "race") && argc == 3){
        write(1, "error: too many arguments", sizeof("error: too many arguments"));
        exit(-1);
    }

    sprintf(current_session.file_name, "%s.txt", argv[1]);

    if (!strcmp(argv[1], "P1") || !strcmp(argv[1], "P2")) {
        current_session.session_time = 5400;
        current_session.total_cars = 20;
        current_session.maximum_tours = 1000;
		current_session.first_lap = 1;
    }
    else if (!strcmp(argv[1], "P3")){
        current_session.session_time = 3600;
        current_session.total_cars = 20;
        current_session.maximum_tours = 1000;
    }
    else if (!strcmp(argv[1], "Q1")){
        current_session.session_time = 1080;
        current_session.total_cars = 20;
        current_session.maximum_tours = 1000;
    }
    else if (!strcmp(argv[1], "Q2")){
        current_session.session_time = 900;
        current_session.total_cars = 15;
        current_session.maximum_tours = 1000;
    }
    else if (!strcmp(argv[1], "Q3")){
        current_session.session_time = 720;
        current_session.total_cars = 10;
        current_session.maximum_tours = 1000;
    }
    else if (!strcmp(argv[1], "race")){
        total_km = atoi(argv[2]);

        if (!total_km){
            write(1, "error: second argument must be an integer", sizeof("error: second argument must be an integer"));
            exit(-1);
        }
        else if (total_km > 7.1 && total_km < 3.2){
            write(1, "error: length of a circuit must be between 3.2 and 7.1 km", sizeof("error: length of a circuit must be between 3.2 and 7.1 km"));
            exit(-1);
        }

        current_session.session_time = 7200;
        current_session.total_cars = 20;
        current_session.maximum_tours = calculate_max_tours(total_km);
    }
    else{
        write(1, "error: invalid first argument, first argument must be in [P1, P2, P3, Q1, Q2, Q3, race]", sizeof("error: invalid first argument, first argument must be in [P1, P2, P3, Q1, Q2, Q3, race]"));
        exit(-1);
    }
}


int calculate_max_tours(double total_km){
    srand(getpid());
    double random_circuit_length = 300 + rand() % 50;

    return (int) round(random_circuit_length / total_km);
}