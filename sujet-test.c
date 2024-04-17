#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>

#define MAX_DIVISIONS 3
#define MAX_REGIMENTS 3
#define MAX_COMPANIES 5

#define SHM_KEY 1234
#define SEM_KEY 5678

typedef struct {
    int morts;
    int blesses;
    int ennemis_morts;
    int prisonniers;
} Pertes;

typedef struct {
    int id;
    Pertes pertes;
} Compagnie;

typedef struct {
    int id;
    Compagnie compagnies[MAX_COMPANIES];
} Regiment;

typedef struct {
    int id;
    Regiment regiments[MAX_REGIMENTS];
} Division;

typedef struct {
    Division divisions[MAX_DIVISIONS];
} Armee;

typedef struct {
    Pertes pertes;
} SharedData;

int shm_id;
int sem_id;

void division_process(int division_id);
void regiment_process(int division_id, int regiment_id);
void company_process(int division_id, int regiment_id, int company_id);
void update_shared_data(int division_id, int regiment_id, int company_id, Pertes pertes);

int main() {
    srand(time(NULL));

    // Création du segment de mémoire partagée
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    // Attacher le segment de mémoire partagée
    SharedData *shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialisation des sémaphores
    sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("semget");
        exit(1);
    }

    // Initialisation de la valeur du sémaphore
    semctl(sem_id, 0, SETVAL, 1);

    // Création des processus pour les divisions
    pid_t division_pids[MAX_DIVISIONS];
    for (int i = 0; i < MAX_DIVISIONS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            division_process(i);
            exit(0);
        } else if (pid > 0) {
            division_pids[i] = pid;
        } else {
            perror("fork");
            exit(1);
        }
    }

    // Attente de la fin des processus fils
    for (int i = 0; i < MAX_DIVISIONS; i++) {
        waitpid(division_pids[i], NULL, 0);
    }

    // Détachement du segment de mémoire partagée
    shmdt(shared_data);

    // Suppression du segment de mémoire partagée
    shmctl(shm_id, IPC_RMID, NULL);

    // Suppression des sémaphores
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}

void division_process(int division_id) {
    // Création des processus pour les régiments
    pid_t regiment_pids[MAX_REGIMENTS];
    for (int i = 0; i < MAX_REGIMENTS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            regiment_process(division_id, i);
            exit(0);
        } else if (pid > 0) {
            regiment_pids[i] = pid;
        } else {
            perror("fork");
            exit(1);
        }
    }

    // Attente de la fin des processus fils
    for (int i = 0; i < MAX_REGIMENTS; i++) {
        waitpid(regiment_pids[i], NULL, 0);
    }
}

void regiment_process(int division_id, int regiment_id) {
    // Création des processus pour les compagnies
    pid_t company_pids[MAX_COMPANIES];
    for (int i = 0; i < MAX_COMPANIES; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            company_process(division_id, regiment_id, i);
            exit(0);
        } else if (pid > 0) {
            company_pids[i] = pid;
        } else {
            perror("fork");
            exit(1);
        }
    }

    // Attente de la fin des processus fils
    for (int i = 0; i < MAX_COMPANIES; i++) {
        waitpid(company_pids[i], NULL, 0);
    }
}

void company_process(int division_id, int regiment_id, int company_id) {
    // Accéder aux données partagées et mettre à jour les pertes
    Pertes pertes;
    pertes.morts = rand() % 20;
    pertes.blesses = rand() % 20;
    pertes.ennemis_morts = rand() % 20;
    pertes.prisonniers = rand() % 20;

    update_shared_data(division_id, regiment_id, company_id, pertes);
}

void update_shared_data(int division_id, int regiment_id, int company_id, Pertes pertes) {
    // Verrouiller le sémaphore
    struct sembuf sem_op = {0, -1, 0};
    semop(sem_id, &sem_op, 1);

    // Accéder aux données partagées et mettre à jour les pertes
    SharedData *shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    shared_data->pertes.morts += pertes.morts;
    shared_data->pertes.blesses += pertes.blesses;
    shared_data->pertes.ennemis_morts += pertes.ennemis_morts;
    shared_data->pertes.prisonniers += pertes.prisonniers;

    // Détacher le segment de mémoire partagée
    shmdt(shared_data);

    // Déverrouiller le sémaphore
    sem_op.sem_op = 1;
    semop(sem_id, &sem_op, 1);
}
