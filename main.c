#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#define PATIENT_NUMBER 250

// DEFINING DEFAULT VALUES
int REGISTRATION_SIZE = 10;
int RESTROOM_SIZE = 10;
int CAFE_NUMBER = 10;
int GP_NUMBER = 10;
int PHARMACY_NUMBER = 10;
int BLOOD_LAB_NUMBER = 10;
int OR_NUMBER = 10;
int SURGEON_NUMBER = 30;
int NURSE_NUMBER = 30;

//OPERATION LIMITS
int SURGEON_LIMIT = 5;
int NURSE_LIMIT = 5;
int HOSPITAL_WALLET = 0;

//WAIT TIMES
int WAIT_TIME = 100;
int REGISTRATION_TIME = 100;
int GP_TIME = 200;
int PHARMACY_TIME = 100;
int BLOOD_LAB_TIME = 200;
int SURGERY_TIME = 500;
int CAFE_TIME = 100;
int RESTROOM_TIME = 100;

//COSTS
int REGISTRATION_COST = 100;
int PHARMACY_COST = 200; // Calculated randomly between 1 and given value.
int BLOOD_LAB_COST = 200;
int SURGERY_OR_COST = 200;
int SURGERY_SURGEON_COST = 100;
int SURGERY_NURSE_COST = 50;
int CAFE_COST = 200; // Calculated randomly between 1 and given value.

//Patient spesifaction increase rate
int HUNGER_INCREASE_RATE = 10;
int RESTROOM_INCREASE_RATE = 10;

//Patient struct

struct Patient
{
    int id;
    int hunger;
    int restroom;
    int medicine;
    int diseasetype;
};
//Create semaphores
sem_t GPsemaphore;
sem_t RESTROOMsemaphore;
sem_t REGISTERsemaphore;
sem_t CAFEsemaphore;
sem_t PHARMACYsemaphore;
sem_t BLOOD_LABsemaphore;

//Create mutexes

pthread_mutex_t HOSPITAL_WALLET_mutex;
pthread_mutex_t SURGERY_mutex;

//Returns given millisecon as microsecond
int millisecToMicrosec(int microsec)
{
    return microsec * 1000;
}
//Returns random value between 1 and given parameter
int randomMax(int max)
{
    return rand() % (max) + 1;
}
//Returns randomly 0 or 1
int randomZeroOne()
{
    return rand() % 2;
}

//Wait function as millisecond
void randomMilisecWait(int maxTime)
{
    usleep(millisecToMicrosec(randomMax(maxTime)));
}

// Waits up to randomly pre specified Wait time
void waitForNextTry()
{
    usleep(millisecToMicrosec(randomMax(WAIT_TIME)));
}
//Increases patient hunger and restroom meter
void increaseHungerRestroom(struct Patient *patient)
{
    int randomRestroomValue = randomMax(RESTROOM_INCREASE_RATE);
    int randomHungerValue = randomMax(HUNGER_INCREASE_RATE);

    patient->restroom += randomRestroomValue;
    patient->hunger += randomHungerValue;
}
//Checks whether patient is hungry. If patient is hungry returns 1 else 0
int isHungry(struct Patient *patient)
{
    if (patient->hunger >= 100)
        return 1;

    return 0;
}
//Checks whether patient needs restroom. If patient needs restroom returns 1 else 0
int needRestroom(struct Patient *patient)
{
    if (patient->restroom >= 100)
        return 1;

    return 0;
}

//If restroom is available patient goes to restroom else waits.
void goToRestroom(struct Patient *patient)
{
    while (0 != sem_trywait(&RESTROOMsemaphore))
    {
        printf("Restroom is full patient %d is waiting\n", patient->id);
        waitForNextTry();
    }
    randomMilisecWait(RESTROOM_TIME);
    sem_post(&RESTROOMsemaphore);
    printf("Patient %d went to restroom\n", patient->id);
    patient->restroom = 0;
}
//If cafe is available patient goes to cafe else waits.
void goToCafe(struct Patient *patient)
{
    while (0 != sem_trywait(&CAFEsemaphore))
    {

        printf("CAFE is full patient %d is waiting\n", patient->id);
        waitForNextTry();
    }
    randomMilisecWait(CAFE_TIME);
    sem_post(&CAFEsemaphore);
    printf("Patient %d went to cafe\n", patient->id);
    pthread_mutex_lock(&HOSPITAL_WALLET_mutex);
    HOSPITAL_WALLET += randomMax(CAFE_COST);
    pthread_mutex_unlock(&HOSPITAL_WALLET_mutex);
    patient->hunger = 0;
}
//Patient tries to register if succesfull pays the price and returns 1 else 0
int goToRegister()

{
    int result = sem_trywait(&REGISTERsemaphore);

    if (0 == result)
    {
        pthread_mutex_lock(&HOSPITAL_WALLET_mutex);
        HOSPITAL_WALLET += REGISTRATION_COST;
        pthread_mutex_unlock(&HOSPITAL_WALLET_mutex);

        randomMilisecWait(REGISTRATION_TIME);
        //int value;
        //sem_getvalue(&GPsemaphore, &value);
        //printf("Semaphore's value is: %d\n",value);

        sem_post(&REGISTERsemaphore);

        return 1;
    }

    return 0;
}
//Patient tries to go to pharmacy if succesfull pays the price and returns 1 else 0
int goToPharmacy()

{
    int result = sem_trywait(&PHARMACYsemaphore);

    if (0 == result)
    {
        pthread_mutex_lock(&HOSPITAL_WALLET_mutex);
        HOSPITAL_WALLET += randomMax(PHARMACY_COST);
        pthread_mutex_unlock(&HOSPITAL_WALLET_mutex);

        randomMilisecWait(PHARMACY_TIME);
        //int value;
        //sem_getvalue(&GPsemaphore, &value);
        //printf("Semaphore's value is: %d\n",value);

        sem_post(&PHARMACYsemaphore);

        return 1;
    }

    return 0;
}
//Patient tries to go to Blood Lab if succesfull pays the price and returns 1 else 0
int goToBloodLab()

{
    int result = sem_trywait(&PHARMACYsemaphore);

    if (0 == result)
    {
        pthread_mutex_lock(&HOSPITAL_WALLET_mutex);
        HOSPITAL_WALLET += BLOOD_LAB_COST;
        pthread_mutex_unlock(&HOSPITAL_WALLET_mutex);

        randomMilisecWait(BLOOD_LAB_TIME);
        //int value;
        //sem_getvalue(&GPsemaphore, &value);
        //printf("Semaphore's value is: %d\n",value);

        sem_post(&PHARMACYsemaphore);

        return 1;
    }

    return 0;
}
//Patient trys to get surgery if there is enough OR,Nurse,Surgeon available operation occurs and pays the price returns 1 else 0
int goToSurgery(struct Patient *patient)

{
    int succesfull = 0;

    pthread_mutex_lock(&SURGERY_mutex);

    int neededSurgeon = randomMax(SURGEON_LIMIT);
    int neededNurse = randomMax(NURSE_LIMIT);

    if (OR_NUMBER == 0)
    {
        printf("There is no available surgery room for patient %d.\n", patient->id);
    }
    else if (SURGEON_NUMBER < neededSurgeon)
    {
        printf("There is not enough surgeon for patient %d.\n", patient->id);
    }
    else if (NURSE_NUMBER < neededNurse)
    {
        printf("There is not enough nurse for patient %d.\n", patient->id);
    }
    else
    {
        printf("There is enough and available OR,Nurse,Surgeon Patient %d is going to surgery.\n", patient->id);
        SURGEON_NUMBER -= neededSurgeon;
        NURSE_NUMBER -= neededNurse;
        OR_NUMBER -= 1;
        succesfull = 1;
    }

    pthread_mutex_unlock(&SURGERY_mutex);

    if (succesfull == 1)
    {
        randomMilisecWait(SURGERY_TIME);

        pthread_mutex_lock(&SURGERY_mutex);
        SURGEON_NUMBER += neededSurgeon;
        NURSE_NUMBER += neededNurse;
        OR_NUMBER += 1;

        pthread_mutex_unlock(&SURGERY_mutex);

        pthread_mutex_lock(&HOSPITAL_WALLET_mutex);
        int totalcost = SURGERY_OR_COST + (neededSurgeon * SURGERY_SURGEON_COST) + (neededNurse * SURGERY_NURSE_COST);
        HOSPITAL_WALLET += totalcost;
        pthread_mutex_unlock(&HOSPITAL_WALLET_mutex);
    }

    return succesfull;
}
//Patient tries to go to GP if succesfull returns 1 else 0
int goToGP()
{
    int result = sem_trywait(&GPsemaphore);
    if (0 == result)
    {
        randomMilisecWait(GP_TIME);
        //int value;
        //sem_getvalue(&GPsemaphore, &value);
        //printf("Semaphore's value is: %d\n",value);

        sem_post(&GPsemaphore);

        return 1;
    }

    return 0;
}
//Creates patient randomly
void createRandomisePatient(struct Patient *patient, int patient_id)
{
    patient->id = patient_id + 1;
    patient->hunger = randomMax(100);
    patient->restroom = randomMax(100);
    patient->medicine = randomZeroOne();
    // 1- only medicine 2-blood test 3- surgery
    patient->diseasetype = randomMax(3);
}

//Checks if patient if hungry or needs restroom then if any of it needed patients goes cafe or restroom
void checkHungerRestroom(struct Patient *patient)
{

    if (1 == isHungry(patient))
    {
        printf("Patient %d is hungry going to cafe.\n", patient->id);
        goToCafe(patient);
    }

    if (1 == needRestroom(patient))
    {
        printf("Patient %d needs to go to restroom.\n", patient->id);
        goToRestroom(patient);
    }
}

//Start individual patient
void *startPatient(void *param)
{
    int patient_id;

    patient_id = (int)param;

    struct Patient patient;

    createRandomisePatient(&patient, patient_id);

    printf("Patient %d arrived to the hospital\n", patient.id);

    /* Print patient spesifications
    
    printf("id=%d\n",patient.id);
    printf("hunger=%d\n",patient.hunger);
    printf("restroom=%d\n",patient.restroom);
    printf("medicine=%d\n",patient.medicine);

    // 1- only medicine 2-blood test 3- surgery

    printf("typeof patient=%d\n",patient.diseasetype);
    */

    //Register section for every patient

    printf("Trying to go to Register Patient %d\n", patient.id);
    while (1 != goToRegister())
    {
        printf("Register is full will try again Patient %d\n", patient.id);
        waitForNextTry();
        increaseHungerRestroom(&patient);

        checkHungerRestroom(&patient);

        printf("Trying to Register again Patient %d\n", patient.id);
    }

    printf("Patient %d registered succesfully.\n", patient.id);

    //First GP section for every patient

    printf("Trying to go to GP Patient %d\n", patient.id);
    while (1 != goToGP())
    {
        printf("GP is full will try again Patient %d\n", patient.id);
        waitForNextTry();
        increaseHungerRestroom(&patient);

        checkHungerRestroom(&patient);

        printf("Trying to go to GP again Patient %d\n", patient.id);
    }

    printf("GP examined patient %d succesfully.\n", patient.id);

    // 1- only medicine 2-blood test 3- surgery
    //Patient gets operations for each disease type

    if (patient.diseasetype == 1)
    {

        if (patient.medicine == 1)
        {
            printf("Trying to go to Pharmacy Patient %d\n", patient.id);
            while (1 != goToPharmacy(&patient))
            {
                printf("Pharmacy is full will try again Patient %d\n", patient.id);

                waitForNextTry();

                increaseHungerRestroom(&patient);

                checkHungerRestroom(&patient);

                printf("Trying to Pharmacy again Patient %d\n", patient.id);
            }

            printf("Patient %d bought in pharmacy succesfully.\n", patient.id);
        }
    }
    else if (patient.diseasetype == 2)
    {

        printf("Trying to go to Blood Lab Patient %d\n", patient.id);
        while (1 != goToBloodLab(&patient))
        {
            printf("Blood Lab is full will try again Patient %d\n", patient.id);

            waitForNextTry();

            increaseHungerRestroom(&patient);

            checkHungerRestroom(&patient);

            printf("Trying to go to Blood Lab again Patient %d\n", patient.id);
        }

        printf("Patient %d gave blood succesfully.\n", patient.id);

        printf("Patient %d going for GP again after Blood Lab if need pharmacy.\n", patient.id);

        printf("Trying to go to GP Patient %d\n", patient.id);
        while (1 != goToGP())
        {
            printf("GP is full will try again Patient %d\n", patient.id);
            waitForNextTry();
            increaseHungerRestroom(&patient);

            checkHungerRestroom(&patient);

            printf("Trying to go to GP again Patient %d\n", patient.id);
        }

        printf("GP examined patient %d succesfully.\n", patient.id);

        if (patient.medicine == 1)
        {
            printf("Patient %d needs medicine after Blood Lab.\n", patient.id);

            printf("Trying to go to Pharmacy Patient %d\n", patient.id);
            while (1 != goToPharmacy(&patient))
            {
                printf("Pharmacy is full will try again Patient %d\n", patient.id);

                waitForNextTry();

                increaseHungerRestroom(&patient);

                checkHungerRestroom(&patient);

                printf("Trying to Pharmacy again Patient %d\n", patient.id);
            }

            printf("Patient %d bought in pharmacy succesfully.\n", patient.id);
        }
    }
    else
    {
        printf("Patient %d will try to go to surgery.\n", patient.id);
        while (1 != goToSurgery(&patient))

        {
            printf("Surgery not completed will try in some time for Patient %d.\n", patient.id);
            waitForNextTry();
        }
        printf("Surgery completed succesfully for Patient %d.\n", patient.id);

        printf("Trying to go to GP after surgery Patient %d\n", patient.id);
        while (1 != goToGP())
        {
            printf("GP is full will try again Patient %d\n", patient.id);
            waitForNextTry();
            increaseHungerRestroom(&patient);

            checkHungerRestroom(&patient);

            printf("Trying to go to GP again Patient %d\n", patient.id);
        }

        printf("GP examined patient %d succesfully.\n", patient.id);

        if (patient.medicine == 1)
        {
            printf("Patient %d needs medicine after Surgery,\n", patient.id);

            printf("Trying to go to Pharmacy Patient %d\n", patient.id);
            while (1 != goToPharmacy(&patient))
            {
                printf("Pharmacy is full will try again Patient %d\n", patient.id);

                waitForNextTry();

                increaseHungerRestroom(&patient);

                checkHungerRestroom(&patient);

                printf("Trying to Pharmacy again Patient %d\n", patient.id);
            }

            printf("Patient %d bought in pharmacy succesfully.\n", patient.id);
        }
    }
    //Patient lefts from hospital
    printf("Patient %d left the hospital.\n", patient.id);
}

int main()
{ //Init semaphores
    sem_init(&GPsemaphore, 0, GP_NUMBER);
    sem_init(&RESTROOMsemaphore, 0, RESTROOM_SIZE);
    sem_init(&REGISTERsemaphore, 0, REGISTRATION_SIZE);
    sem_init(&CAFEsemaphore, 0, CAFE_NUMBER);
    sem_init(&PHARMACYsemaphore, 0, PHARMACY_NUMBER);
    sem_init(&BLOOD_LABsemaphore, 0, BLOOD_LAB_NUMBER);

    //Init random
    srand(time(NULL));
    //Init patient variable
    int i = 0;
    //Init thread
    pthread_t *threads;
    threads = (pthread_t *)malloc(PATIENT_NUMBER * sizeof(pthread_t));

    //Start hospital simualation by starting patients
    for (i = 0; i < PATIENT_NUMBER; i++)
    {

        printf("Patient %d left from home to hospital.\n", i);
        usleep(millisecToMicrosec(20));

        pthread_create(threads + i, NULL, startPatient, (void *)i);
    }

    for (i = 0; i < PATIENT_NUMBER; i++)
    {
        pthread_join(*(threads + i), NULL);
    }

    printf("The day has finished.\n");

    //Destroy semaphores
    sem_destroy(&GPsemaphore);
    sem_destroy(&RESTROOMsemaphore);
    sem_destroy(&REGISTERsemaphore);
    sem_destroy(&CAFEsemaphore);
    sem_destroy(&PHARMACYsemaphore);
    sem_destroy(&BLOOD_LABsemaphore);

    printf("Total money in Hospital Wallet at the end of the day %d.\n", HOSPITAL_WALLET);
    return 0;
}