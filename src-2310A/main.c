#include <stdlib.h>
#include <stdio.h>
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
 *This player's earnings.
 */
Player thisPlayer;

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
 *Determine the target of the next move according to the player's strategy.
 *We start at the given current position not taking the site's capacity into
 *account.
 *ignoreMo: As this function might be called repeatedly, rule #2 only applies in the
 *first iteration.
 */
unsigned int calculate_move_to(int playersCount, unsigned int ownPosition,
        int ignoreMo) {
    int doSiteAhead = -1;
    unsigned int v1SiteAhead = -1u;
    unsigned int v2SiteAhead = -1u;
    unsigned int barrierAhead = -1u;
    unsigned int siteToGo = -1u;

    /*Rule #1: Go to next Do if you have money*/
    if (0 < thisPlayer.money) {
        doSiteAhead = player_find_x_site_ahead(DO, ownPosition, &path);
        if (-1 != doSiteAhead) {
            siteToGo = doSiteAhead;
        }
    }

    /*Rule #2: Go to the next site if it is Mo.*/
    if (-1u == siteToGo && !ignoreMo) {
        if (MO == path.sites[ownPosition + 1].type) {
            siteToGo = ownPosition + 1;
            /*BTW, money balance is adapted upon receiving the dealer's
             * broadcast.*/
        }
    }

    /*Rule #3: Stop at the closest V1, V2 or barrier site.*/
    if (-1u == siteToGo) {
        v1SiteAhead =  (unsigned int)player_find_x_site_ahead(V1, ownPosition,
                &path);
        v2SiteAhead =  (unsigned int)player_find_x_site_ahead(V2, ownPosition,
                &path);
        barrierAhead = (unsigned int)player_find_x_site_ahead(BARRIER,
                ownPosition, &path);
        siteToGo = MIN(v1SiteAhead, v2SiteAhead);
        siteToGo = MIN(siteToGo, barrierAhead);
    }

    return siteToGo;
}

/*
 *Calculate the next move and send it.
 */
void make_move(int playersCount) {
    unsigned int barrierAhead = -1u;
    unsigned int siteToGo = -1u;
    int ownPosition = playerPositions[ownId];
    int moved = 0;
    int ignoreMo = 0;

    /*Rule #0: Don't move beyond the end of the path.*/
    if (path.siteCount <= ownPosition + 1) {
        return;
    }

    /*Rule #0.1: Always stop at a barrier*/
    barrierAhead = (unsigned int)player_find_x_site_ahead(BARRIER, ownPosition,
            &path);

    do {
        siteToGo = calculate_move_to(playersCount, ownPosition, ignoreMo);
        ignoreMo = 1;
        /*fprintf(stderr, "Positions: %2d %2d %2d \n",*/
                /*playerPositions[0],*/
                /*playerPositions[1],*/
                /*playerPositions[2]*/
               /*);*/
        if (-1u != siteToGo) {
            /*Make sure to not move beyond the end of the path*/
            moved = player_forward_to(stdout, siteToGo, barrierAhead,
                    playersCount, playerPositions, ownId, &path);
        }
        ownPosition = siteToGo;
    } while (!moved && (-1 != siteToGo));

    return;
}

/*
 *Upon receiving some message, execute it as long as it is valid.
 */
int process_command(const char* command, int playersCount) {
    if (0 == strncmp("EARLY", command, 5u)) {
        errorReturn(stderr, E_EARLY_GAME_OVER);
    } else if (0 == strncmp("DONE", command, 4u)) {
        return 0;
    } else if (0 == strncmp("YT", command, 2u)) {
        make_move(playersCount);
    } else if (0 == strncmp("HAP", command, 3u)) {
        player_process_move_broadcast(command, playerPositions, playerRankings,
                playersCount, ownId, &thisPlayer);
    }
    return 1;
}

/*
 *Game play loop.
 */
void run_game(int playersCount) {
    char command[100];
    int run = 1;

    getPath(playersCount);

    /*player_print_path(stderr, &path, playersCount, path.siteCount,*/
            /*playerPositions, playerRankings);*/

    while (run) {
        if (!fgets(command, sizeof(command), stdin)) {
            player_free_path(&path);
            errorReturn(stderr, E_COMMS_ERROR);
        }
        run = process_command(command, playersCount);
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
    dealer_reset_player(&thisPlayer);

    init_player_positions(playersCount);

    run_game(playersCount);

    free(playerPositions);
    free(playerRankings);

    return EXIT_SUCCESS;
}

