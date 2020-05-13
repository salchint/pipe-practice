/*
 *errorReturn.h
 */

#ifndef __ERRORO_RETURN_H__
#define __ERRORO_RETURN_H__

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
const char* playerErrorTexts[] = {
    "",
    "Usage: player pcount ID",
    "Invalid player count",
    "Invalid ID",
    "Invalid path",
    "Early game over",
    "Communications error"
};

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
const char* dealerErrorTexts[] = {
    "",
    "Usage: 2310dealer deck path p1 {p2}",
    "Error reading deck",
    "Error reading path",
    "Error starting process",
    "Communications error"
};

/*
 *Print an error message to stderr and exit the program.
 */
void error_return(FILE* destination, enum PlayerErrorCodes code) {
    fprintf(destination, "%s\n", playerErrorTexts[code]);
    exit(code);
}

/*
 *Print an error message to stderr and exit the program.
 */
void error_return_dealer(FILE* destination, enum DealerErrorCodes code,
    int dealerContext) {
    fprintf(destination, "%s\n", dealerErrorTexts[code]);
    if (dealerContext) {
        exit(code);
    }
    _exit(code);
}

#endif

