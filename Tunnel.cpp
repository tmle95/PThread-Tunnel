
//COSC 3360 Assignment 3

#include <iostream>
#include <pthread.h>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

//create struct table to store car threads information
struct Thread{
    string Destination;
    string arrivalTime;
    string travelTime;
    int threadID;
};

//global and condition variables
static int wbCars = 0;
static int bbCars = 0;
static pthread_mutex_t traffic_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wb_can = PTHREAD_COND_INITIALIZER;
static pthread_cond_t bb_can = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cleared = PTHREAD_COND_INITIALIZER;
static int curCars = 0;
static int maxCars;
static int delCars;
static string traffic;

//function to call for when creating a tunnel thread
//function starts the traffic for Whittier bound cars and then sleep for 5s before changing to the next traffic direction or if closed
void* Tunnel(void *arg){
    while(true){
        pthread_mutex_lock(&traffic_lock);
        traffic = "WB";
        pthread_cond_broadcast(&wb_can);
        cout << "The tunnel is now open for Whittier-bound traffic." << endl;
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);

        pthread_mutex_lock(&traffic_lock);
        traffic = "N";
        cout << "The tunnel is now closed to ALL traffic." << endl;
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);

        pthread_mutex_lock(&traffic_lock);
        traffic = "BB";
        pthread_cond_broadcast(&bb_can);
        cout << "The tunnel is now open for Bear Valley-bound traffic." << endl;
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);

        pthread_mutex_lock(&traffic_lock);
        traffic = "N";
        cout << "The tunnel is now closed to ALL traffic." << endl;
        pthread_mutex_unlock(&traffic_lock);
        sleep(5);
    }
    //exits this thread
    pthread_exit(NULL);
}
//checks the status of the car tunnel
static int clear(string location){
    if(curCars == 0)
        return true;
    else if(curCars < maxCars)
        return true;
    else
        return false;
}
//function that the car thread will call to enter the tunnel, checks for conditions to meet before accessing the tunnel
void enterTunnel(string car, string location){
    string templocation;
    if(location == "WB"){
        templocation = "Whittier";
    }
    else if(location == "BB"){
        templocation = "Bear Valley";
    }
   if(location != traffic){
        if(location == "WB"){
            pthread_mutex_lock(&traffic_lock);
            pthread_cond_wait(&wb_can, &traffic_lock); //wait for wb_can broadcast if car is going to Whittier
            pthread_mutex_unlock(&traffic_lock);
        }
        else if(location == "BB"){
            pthread_mutex_lock(&traffic_lock);
            pthread_cond_wait(&bb_can, &traffic_lock); //wait for bb_can broadcast if car is going to Bear Valley
            pthread_mutex_unlock(&traffic_lock);
        }
   }
   pthread_mutex_lock(&traffic_lock);
   if(!clear(location)){
        delCars++; //keep count of delayed cars
        pthread_cond_wait(&cleared, &traffic_lock); //wait for cleared condition to signal
   }
   curCars++; //keep count of cars inside tunnel
   cout << "Car # " << car << " going to " << templocation << " enters the tunnel." << endl;
   pthread_mutex_unlock(&traffic_lock);

   if(location == "WB") //count # of cars going to Whittier
       wbCars++;
   if(location == "BB") //count # of cars going to Bear Valley
       bbCars++;
}
//function to call for car to leave the tunnel
void leaveTunnel(string car, string location){
    string templocation;
    if(location == "WB"){
        templocation = "Whittier";
    }
    else if(location == "BB"){
        templocation = "Bear Valley";
    }
    pthread_mutex_lock(&traffic_lock);
    curCars--; //keeps count of cars in tunnel
    cout << "Car # " << car << " going to " << templocation << " exits the tunnel." << endl;
    pthread_cond_signal(&cleared); //signals that the tunnel is clear for another car to enter
    pthread_mutex_unlock(&traffic_lock);
}
//function that will car threads will call for, takes all information from struct,
void *car(void *arg){
    int temp, cTime, tempCarID;
    string location, templocation, travTime, carID;
    struct Thread *my_data;
    my_data = (struct Thread *) arg;
    tempCarID = my_data->threadID;
    carID = to_string(tempCarID);
    location = my_data->Destination;
    travTime = my_data->travelTime;
    cTime = stoi(travTime); //travel time converted to an integer
    if(location == "WB"){
        templocation = "Whittier";
    }
    else if(location == "BB"){
        templocation = "Bear Valley";
    }
    cout << "Car # " << carID << " going to " << templocation << " arrives at the tunnel." << endl;
    enterTunnel(carID, location); //enter tunnel
    for(int i = 0; i < cTime; i++){
        sleep(1); //sleeps while the car is crossing the tunnel
    }
    leaveTunnel(carID, location); //leave tunnel
    pthread_exit(NULL);
}
int main(){
    Thread thread[128];
    int count = 0;
    int i, rc, numThreads, tempTime;
    string data, a ,b ,c, maxNCars, fileName;
    cout << "Enter name of file: ";
    cin >> fileName;
    ifstream ifs;
    stringstream ss;
    ifs.open(fileName.c_str());
    while(getline(ifs, data)){
       ss << data;
        ss >> a >> b >> c;
       ss.clear();
       thread[count].arrivalTime = a;
       thread[count].Destination = b;
       thread[count].travelTime = c;
       count++;
    }
    ifs.close();
    maxNCars = thread[0].arrivalTime;
    maxCars = stoi(maxNCars); //number of cars that the tunnel will limit for
    numThreads = count - 1; //amount of car threads to make
    pthread_mutex_init(&traffic_lock, NULL); //initiate lock
    pthread_t tunnelThread; //declare tunnel thread
    pthread_create(&tunnelThread,NULL, Tunnel, NULL); //creates tunnel thread
    pthread_t cars[numThreads]; //declare car threads
    for(int i = 0; i < numThreads; i++){
        thread[i+1].threadID = i+1;
        tempTime = stoi(thread[i+1].arrivalTime); //arrival time
        for(int i = 0; i < tempTime; i++){
            sleep(1); //sleeps for amount of time it takes for car to arrive
        }
        rc = pthread_create(&cars[i], NULL, car, (void *) &thread[i+1]); //creates amount of car threads accordingly from the input file
        if(rc){
            cout << "Unable to create thread" << endl;
            exit(1);
        }
    }
    for(int i = 0;i < numThreads; i++){
        pthread_join(cars[i], NULL); //join car threads until all car threads finishes
    }

    cout <<  endl << wbCars << " Car(s) going to Whittier arrived at the tunnel." << endl;
    cout << bbCars << " Car(s) going to Bear Valley arrived at the tunnel." << endl;
    cout << delCars << " Car(s) were delayed." << endl;


    pthread_mutex_destroy(&traffic_lock);

    return 0;
}
