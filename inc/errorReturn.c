/*
 *errorReturn.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "errorReturn.h"

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

