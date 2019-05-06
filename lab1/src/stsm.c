#include <stdio.h>
#include <stdlib.h>
#include "tsm.h"

void dfs(
    int* matrix, //
    const int* num_cities, //
    int current_layer, //
    int previous_city, //
    int current_city, //
    int current_distance, // 
    int visited_cities, //
    int* min_distance, //
    int* current_route, //
    int* best_route //
) 
{
    current_route[current_layer] = current_city;
    current_distance += matrix[previous_city * *num_cities + current_city];
    if (current_distance >= *min_distance) {
        return;
    }
    if (current_layer == *num_cities - 1) {
        *min_distance = current_distance;
        for (int i = 0; i < *num_cities; i++) {
            best_route[i] = current_route[i];
        }
    } else {
        visited_cities |= (1 << current_city);
        for (int i = 1; i < *num_cities; i++) {
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
        printf("Usage: ./stsm <num_cities> <filename>\n");
        return ILLEGAL_ARGUMENT_NUMBER;
    }
    
    const int num_cities = atoi(argv[1]);
    const char* filename = argv[2];

    if (num_cities <= 0) {
        printf("Illegal argument: number of cities must be greater than zero.\n");
        return ILLEGAL_ARGUMENT;
    }

    int matrix[num_cities * num_cities];

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File not found: the file does not exist.\n");
        return FILE_NOT_FOUND;
    }
    for (int i = 0; i < num_cities; i++) {
        for (int j = 0; j < num_cities; j++) {
            int temp_distance;
            if (fscanf(fp, "%d", &temp_distance) == 1) {
                matrix[i * num_cities + j] = temp_distance;
            } else {
                printf("Illegal file format: the file does not correspond to the standard format.\n");
                return ILLEGAL_FILE_FORMAT;
            }
        }
    }                 
    fclose(fp);

    #ifdef DEBUG_MODE
    for (int i = 0; i < num_cities; i++) {
        for (int j = 0; j < num_cities; j++) {
            printf("%d\t", matrix[i * num_cities + j]);
        }
        printf("\n");
    }
    #endif

    int min_distance = INT32_MAX;
    int current_route[num_cities];
    int best_route[num_cities];
    current_route[0] = 0;
    for (int i = 1; i < num_cities; i++) {
        dfs(
            matrix,
            &num_cities,
            1,
            0,
            i,
            0,
            1,
            &min_distance,
            current_route,
            best_route
        );
    }

    printf("Best path: ");
    for (int i = 0; i < num_cities; i++) {
        printf("%d ", best_route[i]);
    }
    printf("\n");
    printf("Distance: %d\n", min_distance);

    return SUCCESS;
}
