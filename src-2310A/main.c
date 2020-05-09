#include <stdlib.h>
#include <stdio.h>
/*#include <unistd.h>*/
#include <string.h>
#include <ctype.h>
#include "../inc/errorReturn.h"
#include "../inc/protocol.h"

/*
 *The path retrieved from the dealer;
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
 *Initialize the global field representing all players' positions.
 */
void init_player_positions(int playersCount) {
    playerPositions = malloc(playersCount * sizeof(int));
    playerRankings = malloc(playersCount * sizeof(int));
    memset(playerPositions, 0, playersCount * sizeof(int));
    memset(playerRankings, 0, playersCount * sizeof(int));
}

/*
 *Request the path information from the dealer.
 */
void getPath(int playersCount) {
    player_request_path(stdout);
    player_read_path(stdin, playersCount, &path);
}

void process_command(const char* command) {
    if (0 == strncmp("YT", command, 2u)) {
        printf("It's my turn\n");
    }
}

/*
 *Game play loop.
 */
void run_game(int playersCount) {
    char command[100];

    getPath(playersCount);

    player_print_path(stderr, &path, playersCount, path.siteCount,
            playerPositions, playerRankings);

    for (;;) {
        if (!fgets(command, sizeof(command), stdin)) {
            player_free_path(&path);
            errorReturn(stderr, E_COMMS_ERROR);
        }
        process_command(command);
    }
}

int main(int argc, char* argv[]) {
    int playersCount = 0;
    int playerID = 0;
    int i = 0;

    /*Check for valid number of parameters*/
    if (3 != argc) {
        errorReturn(stderr, E_INVALID_ARGS_COUNT);
    }

    /*Check for valid number of players*/
    for (i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            errorReturn(stderr, E_INVALID_PLAYER_COUNT);
        }
    }
    playersCount = atoi(argv[1]);
    if (1 > playersCount) {
        errorReturn(stderr, E_INVALID_PLAYER_COUNT);
    }

    /*Check for valid player ID*/
    for (i = 0; i < strlen(argv[2]); i++) {
        if (!isdigit(argv[2][i])) {
            errorReturn(stderr, E_INVALID_PLAYER_ID);
        }
    }
    playerID = atoi(argv[2]);
    if (playersCount <= playerID) {
        errorReturn(stderr, E_INVALID_PLAYER_ID);
    }

    init_player_positions(playersCount);

    run_game(playersCount);

    free(playerPositions);
    free(playerRankings);

    return EXIT_SUCCESS;
}

