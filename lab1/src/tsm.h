#pragma once

#ifndef tsm_h
#define tsm_h

/* uncomment the following line to turn on debug mode */
// #define DEBUG_MODE

/* command line argument indices */
enum argv_index {
    PROGRAM_NAME,
    NUM_CITIES,
    NUM_THREADS,
    FILENAME
};

/* return codes */
enum return_code {
     SUCCESS,
     ILLEGAL_ARGUMENT_NUMBER,
     ILLEGAL_ARGUMENT,
     FILE_NOT_FOUND,
     ILLEGAL_FILE_FORMAT,
     UNKNOWN_ERROR
};

#endif
