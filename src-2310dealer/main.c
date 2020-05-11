#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
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
 *Memory holding the card deck.
 */
char* deckBuffer = NULL;
/*
 *Number of cards in the deck.
 */
int deckSize = 0;
/*
 *Path object deserialized from the path file.
 */
Path path;
/*
 *All players' positions.
 */
int* playerPositions;
/*
 *The ranking is relevant if there are multiple players on the same site.
 */
int* playerRankings;
/*
 *The actual number of players in the game.
 */
int playersCount = 0;

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
 *Initialize the global field representing all players' positions.
 */
void init_player_positions() {
    playerPositions = malloc(playersCount * sizeof(int));
    playerRankings = malloc(playersCount * sizeof(int));
    memset(playerPositions, 0, playersCount * sizeof(int));
    memset(playerRankings, 0, playersCount * sizeof(int));
}

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
 *Determine the player, who is next.
 */
int calculate_next_player(const int* positions, const int* rankings) {
    int i = 0;
    int minSite = INT_MAX ;
    int maxRank = 0;

    /*Find the earliest used site*/
    for (i = 0; i < playersCount; i++) {
       minSite = MIN(minSite, positions[i]);
    }

    /*Find the highest ranking on that site*/
    for (i = 0; i < playersCount; i++) {
       if (minSite == positions[i]) {
          maxRank = MAX(maxRank, rankings[i]);
       }
    }

    /*Find the player on the evaluated site and ranking*/
    for (i = 0; i < playersCount; i++) {
       if (minSite == positions[i]
           && maxRank == rankings[i]) {
           return i;
       }
    }

    return 0;
}

/*
 *Adjust the gobal positions and ranking board.
 */
void move_player(int id, int targetSite, int* positions, int* rankings) {
    int i = 0;
    int ranking = 0;

    for (i = 0; i < playersCount; i++) {
        if (targetSite == positions[i]) {
            ranking += 1;
        }
    }

    positions[id] = targetSite;

    rankings[id] = ranking;
}

/*
 *Listen for the next move from the given player.
 */
void receive_next_move(FILE* readStream, int id, int* positions, int* rankings) {
    char buffer[100];
    int targetSite = 0;
    int readChars = 0;

    if (!fgets(buffer, sizeof(buffer), readStream)) {
        errorReturnDealer(stdout, E_DEALER_COMMS_ERROR, 1);
    }

    readChars = sscanf(buffer, "DO%d", &targetSite);
    if (1 > readChars || EOF == readChars) {
        errorReturnDealer(stdout, E_DEALER_COMMS_ERROR, 1);
    }

    move_player(id, targetSite, positions, rankings);
    player_print_path(stdout, &path, playersCount, path.siteCount,
            positions, rankings, 0);
}

/*
 *Execute the dealer's business logic.
 */
void run_dealer() {
    int i = 0;
    int run = 1;
    int nextPlayer = 0;

    for (i = 0; i < playersCount; i++) {
        open_stream(i);

        fclose(streamToPlayer[i][READ_END]);
        fclose(streamToDealer[i][WRITE_END]);
    }

    /*First, print the path*/
    player_print_path(stdout, &path, playersCount, path.siteCount,
            playerPositions, playerRankings, 1);
    fflush(stdout);

    /*Next, all players need to ask for the path*/
    for (i = 0; i < playersCount; i++) {
        if ('^' == fgetc(streamToDealer[i][READ_END])) {
            /*fprintf(stdout, "%zu;%s", path.siteCount, path.buffer);*/
            fprintf(streamToPlayer[i][WRITE_END], "%zu;%s", path.siteCount,
                    path.buffer);
            fflush(streamToPlayer[i][WRITE_END]);
        }
    }

    while (run) {
        /*Next, let the player make his move, which is furtherst back*/
        nextPlayer = calculate_next_player(playerPositions, playerRankings);
        fprintf(streamToPlayer[nextPlayer][WRITE_END], "YT\n");
        fflush(streamToPlayer[i][WRITE_END]);
        receive_next_move(streamToDealer[nextPlayer][READ_END], nextPlayer,
                playerPositions, playerRankings);
    }
}

/*
 *Launch the player processes.
 */
void run_player(int id, const char** playerNames) {
    char buffer[100];
    char bufferCount[10];
    char bufferId[10];
    /*int devNull = 0;*/

    *buffer = '\0';
    *bufferCount = '\0';
    *bufferId = '\0';

    open_stream(id);

    /*Redirect stdin, stdout of the players*/
    dup2(pipeToPlayerNo[id][READ_END], READ_END);
    dup2(pipeToDealerNo[id][WRITE_END], WRITE_END);

    /*[>Redirect stderr to /dev/null<]*/
    /*devNull = open("/dev/null", O_WRONLY);*/
    /*dup2(STDERR_FILENO, devNull);*/

    fclose(streamToPlayer[id][READ_END]);
    fclose(streamToPlayer[id][WRITE_END]);
    fclose(streamToDealer[id][READ_END]);
    fclose(streamToDealer[id][WRITE_END]);

    sprintf(bufferCount, "%d", playersCount);
    sprintf(bufferId, "%d", id);
    execlp(playerNames[id], playerNames[id], bufferCount, bufferId, NULL);

    /*Should not happen!*/
    errorReturnDealer(stderr, E_DEALER_INVALID_START_PLAYER, 0);
}

/*
 *Create child processes for the given players.
 */
void start_players(const char** playerNames) {
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
            run_player(i, playerNames);
            break;
        } else {
        /*Dealer context*/
            pids[i] = pid;
        }
    }

    if (0 < pid) {
        /*Dealer context*/
        run_dealer();
    }
}

/*
 *Request the path information from the dealer.
 */
void getPath(FILE* stream) {
    int success = E_OK;

    success = player_read_path(stream, playersCount, &path);
    if(E_OK != success) {
        errorReturnDealer(stderr, E_DEALER_INVALID_PATH, 1);
    }
}

/*
 *Handle the SIGHUP signal by interrupting the players.
 */
void sigHandler(int signal) {
    int i = 0;

    switch (signal) {
        case SIGHUP:
            for (i = 0; i < playersCount; i++) {
                fprintf(streamToPlayer[i][WRITE_END], "EARLY\n");
                fflush(streamToPlayer[i][WRITE_END]);
            }
            for (i = 0; i < playersCount; i++) {
                waitpid(pids[i], NULL, 0);
            }
    }

}

int main(int argc, char* argv[]) {
    char* deckName = NULL;
    char* pathName = NULL;
    char** playerNames = NULL;
    int i = 0;
    FILE* deckStream = NULL;
    FILE* pathStream = NULL;

    playersCount = 0;
    playerPositions = NULL;
    playerRankings = NULL;

    signal(SIGHUP, sigHandler);
    signal(SIGINT, sigHandler);

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

    init_player_positions();
    getPath(pathStream);
    fclose(pathStream);

    start_players((const char**)playerNames);

    free(playerPositions);
    free(playerRankings);
    free(playerNames);

    return EXIT_SUCCESS;
}

