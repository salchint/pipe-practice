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
 *This player's ID.
 */
int ownId;

/*
 *This player's money balance.
 */
int money;

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
    int success = E_OK;

    player_request_path(stdout);
    success = player_read_path(stdin, playersCount, &path);
    if(E_OK != success) {
        errorReturn(stderr, success);
    }
}

/*
 *Calculate the next move and send it.
 */
void make_move(int playersCount) {
    int doSiteAhead = -1;
    unsigned int v1SiteAhead = -1u;
    unsigned int v2SiteAhead = -1u;
    unsigned int barrierAhead = -1u;
    unsigned int siteToGo = -1u;
    int ownPosition = playerPositions[ownId];

    /*Rule #0: Don't move bejond the end of the path.*/
    if (path.siteCount <= ownPosition) {
        return;
    }

    /*Rule #0.1: Always stop at a barrier*/
    barrierAhead = (unsigned int)player_find_x_site_ahead(BARRIER, ownPosition,
            &path);

    /*Rule #1: Go to next Do if you have money*/
    if (0 < money) {
        doSiteAhead = player_find_x_site_ahead(DO, ownPosition, &path);
        if (-1 != doSiteAhead) {
            player_forward_to(stdout, doSiteAhead, barrierAhead,
                    playerPositions, ownId);
            return;
        }
    }

    /*Rule #2: Go to the next site if it is Mo.*/
    if (MO == path.sites[ownPosition + 1].type) {
        money += 3;
        siteToGo = ownPosition + 1;
        player_forward_to(stdout, siteToGo, barrierAhead, playerPositions,
                ownId);
        return;
    }

    /*Rule #3: Stop at the closest V1, V2 or barrier site.*/
    v1SiteAhead =  (unsigned int)player_find_x_site_ahead(V1, ownPosition,
            &path);
    v2SiteAhead =  (unsigned int)player_find_x_site_ahead(V2, ownPosition,
            &path);
    siteToGo = MIN(v1SiteAhead, v2SiteAhead);
    siteToGo = MIN(siteToGo, barrierAhead);
    if (-1 != siteToGo) {
        player_forward_to(stdout, siteToGo, barrierAhead, playerPositions,
                ownId);
    }


     /*
      *player_print_path(stderr, &path, playersCount, path.siteCount,
      *        playerPositions, playerRankings);
      */
    return;
}

/*
 *Upon receiving some message, execute it as long as it is valid.
 */
void process_command(const char* command, int playersCount) {
    if (0 == strncmp("YT", command, 2u)) {
        make_move(playersCount);
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
        process_command(command, playersCount);
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
    ownId = playerID;
    money = 7;

    init_player_positions(playersCount);

    run_game(playersCount);

    free(playerPositions);
    free(playerRankings);

    return EXIT_SUCCESS;
}

