//
// Created by antoo on 11-Nov-20.
//

#include "show_scores.h"

struct Session current_session;
struct BestTimes best_times;
struct BestTimes empty_best_times;
struct Car race_copy[20];

int sort_cars_by_time(const void *a, const void *b){
    const struct Car *position_1 = (struct Car *) a;
    const struct Car *position_2 = (struct Car *) b;

    if (position_1->bestLap == 0){
        return 1;
    }
    else if (position_2->bestLap == 0){
        return -1;
    }
    else if (position_1->bestLap > position_2->bestLap){
        return 1;
    }
    else if (position_1->bestLap < position_2->bestLap){
        return -1;
    }
    else {
        return 0;
    }
}

int sort_cars_by_lap(const void *a, const void *b){
    const struct Car *position_1 = (struct Car *) a;
    const struct Car *position_2 = (struct Car *) b;

    if (!position_1->lap || position_1->crashed){
        return 1;
    }
    else if (!position_2->lap || position_2->crashed){
        return -1;
    }
    else if (position_1->lap < position_2->lap){
        return 1;
    }
    else if (position_1->lap > position_2->lap){
        return -1;
    }
    else {
        return 0;
    }
}

void set_best_times(){
    best_times = empty_best_times;
    for(int i = 0 ; i < current_session.total_cars ; i++){
        if (race_copy[i].bestLap && (best_times.best_lap == 0 || best_times.best_lap > race_copy[i].bestLap)){
            best_times.best_lap_index = race_copy[i].idCar;
            best_times.best_lap = race_copy[i].bestLap;
        }
        if (race_copy[i].bestS1 && (best_times.best_s1 == 0 || best_times.best_s1 > race_copy[i].bestS1)){
            best_times.best_s1_index = race_copy[i].idCar;
            best_times.best_s1 = race_copy[i].bestS1;
        }
        if (race_copy[i].bestS2 && (best_times.best_s2 == 0 || best_times.best_s2 > race_copy[i].bestS2)){
            best_times.best_s2_index = race_copy[i].idCar;
            best_times.best_s2 = race_copy[i].bestS2;
        }
        if (race_copy[i].bestS3 && (best_times.best_s3 == 0 || best_times.best_s3 > race_copy[i].bestS3)){
            best_times.best_s3_index = race_copy[i].idCar;
            best_times.best_s3 = race_copy[i].bestS3;
        }
    }

}

void build_table(){
    set_best_times();
    system("clear");
    printf("|\tPos.\t|%5s\t|%10s\t|%10s\t|%10s\t|%10s\t|%7s\t|%5s\t|%5s\t|%10s\t|\n\n", "Nr", "S1", "S2", "S3", "Lap Time", "Lap Nr", "Pit", "CRASH", "FINISHED");

    for(int i = 0 ; i < current_session.total_cars ; i++){
        struct Car single_car = race_copy[i];

        printf("|\t%d\t|%5d\t|%10.3f\t|%10.3f\t|%10.3f\t|%10.3f\t|%7d\t|%5d\t|%5d\t|%10d\t|\n", i + 1, single_car.idCar, single_car.s1, single_car.s2, single_car.s3, single_car.totalLap, single_car.lap, single_car.pit, single_car.crashed, single_car.finished);
    }
    printf("\n Best Sector 1 : %d [%.3f]\t", best_times.best_s1_index, best_times.best_s1);
    printf("Best Sector 2 : %d [%.3f]\t", best_times.best_s2_index, best_times.best_s2);
    printf("Best Sector 3 : %d [%.3f]\t", best_times.best_s3_index, best_times.best_s3);
    printf("Best Lap : %d [%.3f]\t\n", best_times.best_lap_index, best_times.best_lap);
    usleep(300);

}

void build_final_table(){
    //system("clear");
    printf("|\tPos.\t|%5s\t|%10s\t|\n\n", "Nr", "BEST LAP");

    for(int i = 0 ; i < current_session.total_cars ; i++){
        struct Car single_car = race_copy[i];

        printf("|\t%d\t|%5d\t|%10.3f\t|\n", i + 1, single_car.idCar, single_car.bestLap);
    }
    sleep(5);
}


void show_score_table(struct Car *race_cars, sem_t *prod_sema, sem_t *cons_sema){
    while (1){
		
        int game_is_finished = 1;
        sem_wait(prod_sema);
        memcpy(race_copy, race_cars, sizeof(struct Car) * current_session.total_cars);
        sem_post(cons_sema);

        if (current_session.first_lap == 0){
            current_session.first_lap++;
            print_previous_ranking(race_copy, current_session);
            continue;
        }

        if (current_session.maximum_tours < 500){
            qsort(race_copy, current_session.total_cars, sizeof(struct Car), sort_cars_by_lap);
        }
        else {
            qsort(race_copy, current_session.total_cars, sizeof(struct Car), sort_cars_by_time);
        }

        build_table();

        for (int i = 0 ; i < current_session.total_cars ; i++){
            if (!race_copy[i].finished && !race_copy[i].crashed){
                game_is_finished = 0;
            }
        }
        if (game_is_finished){
            build_final_table();
			writeClassement(race_copy, current_session);
            break;
        }
    }
}