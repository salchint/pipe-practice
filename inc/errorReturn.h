/*
 *errorReturn.h
 */

#pragma once

#ifndef __ERROR_RETURN_H__
#define __ERROR_RETURN_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 *Error codes used upon exiting the program.
 */
enum PlayerErrorCodes {
    E_OK = 0,
    E_INVALID_ARGS_COUNT = 1,
    E_INVALID_PLAYER_COUNT = 2,
    E_INVALID_PLAYER_ID = 3,
    E_INVALID_PATH = 4,
    E_EARLY_GAME_OVER = 5,
    E_COMMS_ERROR = 6
};

/*
 *Error messages sent to stderr.
 */
extern const char* playerErrorTexts[];

/*
 *Error codes used upon exiting the dealer program.
 */
enum DealerErrorCodes {
    E_DEALER_OK = 0,
    E_DEALER_INVALID_ARGS_COUNT = 1,
    E_DEALER_INVALID_DECK = 2,
    E_DEALER_INVALID_PATH = 3,
    E_DEALER_INVALID_START_PLAYER = 4,
    E_DEALER_COMMS_ERROR = 5
};

/*
 *Error messages sent to stderr.
 */
extern const char* dealerErrorTexts[];

/*
 *Print an error message to stderr and exit the program.
 */
void error_return(FILE* destination, enum PlayerErrorCodes code);

/*
 *Print an error message to stderr and exit the program.
 */
void error_return_dealer(FILE* destination, enum DealerErrorCodes code,
    int dealerContext);

#endif

