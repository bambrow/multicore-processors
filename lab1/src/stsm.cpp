#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

#include "tsm.h"

/* This is the serial solution of traveling salesman problem. */

using namespace std;

/* using recursive DFS to find the best solution. */
void dfs(
    int** matrix, // the distance matrix
    const int& num_cities, // number of cities
    int current_layer, // how many cities, including the current one, have been visited
    int previous_city, // the id of previous city visited
    int current_city, // the id of current city visited
    int current_distance, // the current total distance so far
    int visited_cities, // indicator of visited cities, using bit manipulation
    int& min_distance, // minimum distance so far
    int* current_route, // array of current path
    int* best_route // array of the best path found so far
) 
{
    current_route[current_layer] = current_city;
    current_distance += matrix[previous_city][current_city];
    if (current_distance >= min_distance) {
        return;
    }
    if (current_layer == num_cities - 1) {
        min_distance = current_distance;
        for (int i = 0; i < num_cities; i++) {
            best_route[i] = current_route[i];
        }
    } else {
        visited_cities |= (1 << current_city);
        for (int i = 1; i < num_cities; i++) {
            if (!(visited_cities & (1 << i))) {
                dfs(
                    matrix,
                    num_cities,
                    current_layer + 1,
                    current_city,
                    i,
                    current_distance,
                    visited_cities,
                    min_distance,
                    current_route,
                    best_route
                );
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        cout << "Usage: ./stsm <num_cities> <filename>" << endl;
        return ILLEGAL_ARGUMENT_NUMBER;
    }
    
    const int num_cities = atoi(argv[1]);
    const string filename = argv[2];

    if (num_cities <= 0) {
        cout << "Illegal argument: number of cities must be greater than zero." << endl;
        return ILLEGAL_ARGUMENT;
    }

    int** matrix = new int*[num_cities];
    for (int i = 0; i < num_cities; i++) {
        matrix[i] = new int[num_cities];
    }

    ifstream in(filename);
    if (!in.is_open()) {
        cout << "File not found: the file does not exist." << endl;
        return FILE_NOT_FOUND;
    }
    string line;
    try {
        for (int i = 0; i < num_cities; i++) {
            if (!getline(in, line)) {
                throw ILLEGAL_FILE_FORMAT;
            }
            istringstream iss(line);
            for (int j = 0; j < num_cities; j++) {
                int temp_distance;
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

    int min_distance = INT32_MAX;
    int* current_route = new int[num_cities];
    int* best_route = new int[num_cities];
    current_route[0] = 0;
    for (int i = 1; i < num_cities; i++) {
        dfs(
            matrix,
            num_cities,
            1,
            0,
            i,
            0,
            1,
            min_distance,
            current_route,
            best_route
        );
    }

    cout << "Best path: ";
    for (int i = 0; i < num_cities; i++) {
        cout << best_route[i] << " ";
    }
    cout << endl;
    cout << "Distance: " << min_distance << endl;

    return SUCCESS;
}
