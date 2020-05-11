#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "../inc/errorReturn.h"
#include "../inc/protocol.h"

/*
 *Maximum number of players.
 */
#define MAX_PLAYERS 1024ul
/*
 *The write end of a pipe.
 */
#define WRITE_END 1
/*
 *The read end of a pipe.
 */
#define READ_END 0

/*
 *File stream to read deck file.
 */
FILE* deckStream = NULL;
/*
 *File stream to read path file.
 */
FILE* pathStream = NULL;

/*
 *PIDs of all player processes.
 */
pid_t pids[MAX_PLAYERS];
/*
 *Pipes directing to all players.
 */
int pipeToPlayerNo[MAX_PLAYERS][2];
/*
 *Pipes sourcing from all players.
 */
int pipeToDealerNo[MAX_PLAYERS][2];
/*
 *Streams directing to all players.
 */
FILE* streamToPlayer[MAX_PLAYERS][2];
/*
 *Streams sourcing from all players.
 */
FILE* streamToDealer[MAX_PLAYERS][2];

/*
 *Open a set of streams representing bidirectional communication to a player.
 */
int open_stream(int playerId) {
    streamToPlayer[playerId][READ_END]
        = fdopen(pipeToPlayerNo[playerId][READ_END], "r");
    streamToPlayer[playerId][WRITE_END]
        = fdopen(pipeToPlayerNo[playerId][WRITE_END], "w");
    streamToDealer[playerId][READ_END]
        = fdopen(pipeToDealerNo[playerId][READ_END], "r");
    streamToDealer[playerId][WRITE_END]
        = fdopen(pipeToDealerNo[playerId][WRITE_END], "w");

    if (! (streamToPlayer[playerId][READ_END]
        || streamToPlayer[playerId][WRITE_END]
        || streamToDealer[playerId][READ_END]
        || streamToDealer[playerId][WRITE_END])) {
        errorReturnDealer(stderr, E_DEALER_INVALID_START_PLAYER, 0);
    }
    return E_DEALER_OK;
}

/*
 *Launch the player processes.
 */
void run_player(int id, const char** playerNames) {
    char buffer[100];

    *buffer = '\0';

    open_stream(id);

    fclose(streamToPlayer[id][WRITE_END]);
    fclose(streamToDealer[id][READ_END]);

    printf("Player%d> Waiting....\n", id);
    fgets(buffer, sizeof(buffer), streamToPlayer[id][READ_END]);
    printf("Player%d received %s\n", id, buffer);

    fprintf(streamToDealer[id][WRITE_END], "DO%d\n", id);
    fflush(streamToDealer[id][WRITE_END]);



    fclose(streamToPlayer[id][READ_END]);
    fclose(streamToDealer[id][WRITE_END]);

}

/*
 *Execute the dealer's business logic.
 */
void run_dealer(int playersCount) {
    int i = 0;
    int run = 3;
    char buffer[100];

    for (i = 0; i < playersCount; i++) {
        open_stream(i);

        fclose(streamToPlayer[i][READ_END]);
        fclose(streamToDealer[i][WRITE_END]);
    }

    while (run) {
        run -= 1;
        printf("Dealer: sending request\n");

        /*Request*/
        fprintf(streamToPlayer[run][WRITE_END], "YT\n");
        fflush(streamToPlayer[run][WRITE_END]);

        /*Response*/
        fgets(buffer, sizeof(buffer), streamToDealer[run][READ_END]);

        printf("Dealer> Received %s\n", buffer);
    }
}

/*
 *Create child processes for the given players.
 */
void start_players(int playersCount, const char** playerNames) {
    int i = 0;
    pid_t pid = 0;

    for (i = 0; i < playersCount; i++) {
        pipe(pipeToPlayerNo[i]);
        pipe(pipeToDealerNo[i]);
    }

    /*Create all the players*/
    for (i = 0; i < playersCount; i++) {
        pid = fork();

        if (0 > pid) {
            errorReturnDealer(stderr, E_DEALER_INVALID_START_PLAYER, 1);
        }

        if (0 == pid) {
        /*Player context*/
            /*dup2(pipeDescs[i + WRITE_END], STDOUT_FILENO);*/

            run_player(i, playerNames);
            break;
        } else {
        /*Dealer context*/
            pids[i] = pid;
        }
    }

    if (0 < pid) {
        run_dealer(playersCount);
    }
}

int main(int argc, char* argv[]) {
    char* deckName = NULL;
    char* pathName = NULL;
    char** playerNames = NULL;
    int playersCount = 0;
    int i = 0;


    /*Check for valid number of parameters*/
    if (4 > argc) {
        errorReturnDealer(stderr, E_DEALER_INVALID_ARGS_COUNT, 1);
    }

    /*Check opening the deck file*/
    deckName = argv[1];
    deckStream = fopen(deckName, "r");
    if (NULL == deckStream) {
        errorReturnDealer(stderr, E_DEALER_INVALID_DECK, 1);
    }

    /*Check opening the path file*/
    pathName = argv[2];
    pathStream = fopen(pathName, "r");
    if (NULL == pathStream) {
        errorReturnDealer(stderr, E_DEALER_INVALID_PATH, 1);
    }

    /*Remember the player program names*/
    playerNames = malloc((argc - 3) * sizeof(char*));
    for (i = 3; i < argc; i++, playersCount++) {
        playerNames[i - 3] = argv[i];
    }

    start_players(playersCount, (const char**)playerNames);



    free(playerNames);
    return EXIT_SUCCESS;
}

