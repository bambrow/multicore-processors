#include <iostream>
#include <omp.h>
#include <string>
#include <fstream>
#include <sstream>

#include "tsm.h"

/* This is the parallel solution of traveling salesman problem. */

using namespace std;

/* using recursive DFS to find the best solution. */
void dfs(
    int** matrix, // the distance matrix; shared and only read operations are used
    const int& num_cities, // number of cities; shared and only read operations are used
    int current_layer, // how many cities, including the current one, have been visited
    int previous_city, // the id of previous city visited
    int current_city, // the id of current city visited
    int current_distance, // the current total distance so far
    int visited_cities, // indicator of visited cities, using bit manipulation
    int& min_distance, // minimum distance so far; shared and put inside critical section
    int* current_route, // array of current path
    int* best_route // array of the best path found so far; shared and put inside critical section
) 
{
    // update the route and distance so far
    current_route[current_layer] = current_city;
    current_distance += matrix[previous_city][current_city];
    // abandon the current search if the distance is already greater than the minimum
    // use atomic read here
    int min_distance_holder = INT32_MAX;
    #pragma omp atomic read 
    min_distance_holder = min_distance;
    if (current_distance >= min_distance_holder) {
        return;
    }
    // if we have visited all the cities, and the distance so far is smaller
    if (current_layer == num_cities - 1) {
        // update the minimum distance inside critical section
        // also update the best route inside critical section
        #pragma omp critical 
        {
            if (current_distance < min_distance) {
                min_distance = current_distance;
                for (int i = 0; i < num_cities; i++) {
                    best_route[i] = current_route[i];
                }
            }
        }
    } else {
        // if we have not visited all the cities, should continue to visit the next one
        // update the visited cities in the indicator
        visited_cities |= (1 << current_city);
        // traverse the following cities
        for (int i = 1; i < num_cities; i++) {
            // if the city chosen has not been visited, recursively run DFS
            if (!(visited_cities & (1 << i))) {
                dfs(
                    matrix, // same distance matrix
                    num_cities, // same number of cities
                    current_layer + 1, // we have visited one more city
                    current_city, // the current city should become the previous one for next round
                    i, // the next city
                    current_distance, // current distance has been updated
                    visited_cities, // visited cities have been updated
                    min_distance, // same minimum distance
                    current_route, // current route has been updated
                    best_route // same best route
                );
            }
        }
    }
}

/* main function. */
int main(int argc, char const *argv[])
{
    // check number of command line arguments
    if (argc != 4) {
        cout << "Usage: ./ptsm <num_cities> <num_threads> <filename>" << endl;
        return ILLEGAL_ARGUMENT_NUMBER;
    }
    
    // read information from command line arguments
    const int num_cities = atoi(argv[1]);
    const int num_threads = atoi(argv[2]);
    const string filename = argv[3];

    // check the validity of number of cities and number of threads
    // this is a very simple check, which only forces the arguments to be positive integer
    if (num_cities <= 0 || num_threads <= 0) {
        cout << "Illegal argument: number of cities and number of threads must be greater than zero." << endl;
        return ILLEGAL_ARGUMENT;
    }

    // handle special case: there is only one city
    if (num_cities == 1) {
        cout << "Best path: 0" << endl;
        cout << "Distance: 0" << endl;
        return SUCCESS;
    }

    // construct the distance matrix 
    int** matrix = new int*[num_cities];
    for (int i = 0; i < num_cities; i++) {
        matrix[i] = new int[num_cities];
    }

    // start to read the file using the given filename
    ifstream in(filename);
    // if the file cannot be opened, report the error
    if (!in.is_open()) {
        cout << "File not found: the file does not exist." << endl;
        return FILE_NOT_FOUND;
    }
    // if the file can be opened, read it line by line
    string line;
    try {
        // only need to read the lines corresponding to the number of cities
        for (int i = 0; i < num_cities; i++) {
            // read the line and report the error, if any
            if (!getline(in, line)) {
                throw ILLEGAL_FILE_FORMAT;
            }
            // read the distances from the current line
            istringstream iss(line);
            // only need to read the numbers corresponding to the number of cities
            for (int j = 0; j < num_cities; j++) {
                int temp_distance;
                // read the number and report the error, if any
                if (iss >> temp_distance) {
                    matrix[i][j] = temp_distance;
                } else {
                    throw ILLEGAL_FILE_FORMAT;
                }
            }
        } 
    }
    catch (...) {
        cout << "Illegal file format: the file does not correspond to the standard format." << endl;
        return ILLEGAL_FILE_FORMAT;
    }
    in.close();

    #ifdef DEBUG_MODE
    for (int i = 0; i < num_cities; i++) {
        for (int j = 0; j < num_cities; j++) {
            cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }
    #endif

    // handle special case: there are only two cities
    if (num_cities == 2) {
        cout << "Best path: 0 1" << endl;
        cout << "Distance: " << matrix[0][1] << endl;
        return SUCCESS;
    }

    // initialize the shared variables
    // the minimum distance is set to the maximum of 32 bit integer
    int min_distance = INT32_MAX;
    // the best route is set to an array with elements corresponding to the number of cities
    int* best_route = new int[num_cities];

    // prepare for the parallel block
    // use default(none) to explicitly specify the scope of all variables
    // use firstprivate for matrix and num_cities because they are read only
    // use shared for min_distance and best_route
    #pragma omp parallel num_threads(num_threads) \
        default(none) \
        firstprivate(matrix, num_cities) \
        shared(min_distance, best_route)
    for (int i = 1; i < num_cities; i++) {
        // use static scheduling
        // manually extend the first loop, compared to the serial solution
        // this is because the tasks between threads might not be even with the original version
        // here, after extending the first loop, the DFS recursion starts with the third city
        // and therefore the work between threads can be distributed more evenly
        #pragma omp for schedule(static, 1)
        for (int j = 1; j < num_cities; j++) {
            // skip when the third city is the same as the second
            if (j == i) {
                continue;
            }            
            #ifdef DEBUG_MODE
            int thread_id = omp_get_thread_num();
            printf("Current thread number: %d for i = %d and j = %d\n", thread_id, i, j);
            #endif
            // construct the array for current route and fill the first two indices
            int* current_route = new int[num_cities];
            current_route[0] = 0;
            current_route[1] = i;
            // run DFS
            dfs(
                matrix, // the distance matrix
                num_cities, // the number of cities
                2, // start with layer 2 (the 3rd city)
                i, // previous city should be i
                j, // the current city should be j
                matrix[0][i], // current distance so far; should be the distance between 0 and i
                (1 | (1 << i)), // indicator of visited cities; should place the 0th and ith bit with 1
                min_distance, // shared minimum distance
                current_route, // current route constructed inside the loop
                best_route // shared best route
            );
        }
    }

    // print the best path
    cout << "Best path: ";
    for (int i = 0; i < num_cities; i++) {
        cout << best_route[i] << " ";
    }
    cout << endl;
    // print the minimum distance
    cout << "Distance: " << min_distance << endl;

    #ifdef DEBUG_MODE
    int distance_check = 0;
    for (int i = 0; i < num_cities - 1; i++) {
        distance_check += matrix[best_route[i]][best_route[i+1]];
    }
    cout << "Distance double check: " << distance_check;
    if (distance_check == min_distance) {
        cout << " == minimum distance. Test passed." << endl;
    } else {
        cout << " != minimum distance. Test failed." << endl;
    }
    #endif

    return SUCCESS;
}
