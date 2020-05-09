/*
 *errorReturn.h
 */

#ifndef __ERRORO_RETURN_H__
#define __ERRORO_RETURN_H__

#include <stdio.h>
#include <stdlib.h>

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
const char* PlayerErrorTexts[] = {
    "",
    "Usage: player pcount ID",
    "Invalid player count",
    "Invalid ID",
    "Invalid path",
    "Early game over",
    "Communications error"
};

/*
 *Print an error message to stderr and exit the program.
 */
void errorReturn(FILE* destination, enum PlayerErrorCodes code) {
    fprintf(destination, "%s\n", PlayerErrorTexts[code]);
    exit(code);
}

#endif

