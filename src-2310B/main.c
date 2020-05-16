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
Player* thisPlayer;

/*
 *Book-keeping representation of participating players.
 */
Player* players;


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
void get_path(int playersCount) {
    int success = E_OK;

    player_request_path(stdout);
    success = player_read_path(stdin, playersCount, &path);
    if(E_OK != success) {
        error_return(stderr, success);
    }
}

/*
 *Determine the next site according to this rule and return it.
 *Rule: If the next site is not full and all other players are on later sites
 *than us, move forward one site.
 */
unsigned int rule_we_are_last(int playersCount, int ownPosition) {
    int i = 0;
    int siteUsage = 0;

    siteUsage = player_get_site_usage(playerPositions, playersCount,
            ownPosition + 1);

    if (siteUsage < path.sites[ownPosition + 1].capacity) {
        if (0 == playerRankings[ownId]) {
            for (i = 0; i < playersCount; i++) {
                if (ownPosition >= playerPositions[i] && ownId != i) {
                    return -1u;
                }
            }
            return ownPosition + 1;
        }
    }
    return -1u;
}

/*
 *Determine the next site according to this rule and return it.
 *Rule: If we have an odd amount of money, and there is a Mo between us and the
 *next barrier, then go there.
 */
unsigned int rule_odd_money(int playersCount, unsigned int barrierAhead,
        int ownPosition) {
    unsigned int moSiteAhead = -1u;

    if (1 == (thisPlayer->money % 2)) {
        moSiteAhead = (unsigned int)player_find_x_site_ahead(MO, ownPosition,
                &path);
        if (moSiteAhead < barrierAhead) {
            return moSiteAhead;
        }
    }
    return -1u;
}

/*
 *Count the cards of all the other players.
 */
int get_max_collected_cards(int playersCount) {
    int i = 0;
    int maxCards = 0;

    for (i = 0; i < playersCount; i++) {
        if (ownId != i) {
            maxCards = MAX(maxCards, players[i].overallCards);
        }
    }
    return maxCards;
}

/*
 *Determine the next site according to this rule and return it.
 *Rule: If we have the most cards or if everyone has zero cards and there is a
 *Ri between us and the next barrier, then go there.
 */
unsigned int rule_draw_card(int playersCount, unsigned int barrierAhead,
        int ownPosition) {
    unsigned int riSiteAhead = -1u;
    int maxCards = 0;

    riSiteAhead = (unsigned int)player_find_x_site_ahead(RI, ownPosition,
            &path);
    if (riSiteAhead < barrierAhead) {
        maxCards = get_max_collected_cards(playersCount);
        if (thisPlayer->overallCards > maxCards
                || MAX(thisPlayer->overallCards, maxCards) == 0) {
            return riSiteAhead;
        }
    }
    return -1u;
}

/*
 *Determine the next site according to this rule and return it.
 *Rule: If there is a V2 between us and the next barrier, then go there.
 */
unsigned int rule_goto_v2(int playersCount, unsigned int barrierAhead,
        int ownPosition) {
    unsigned int v2SiteAhead = -1u;

    v2SiteAhead = (unsigned int)player_find_x_site_ahead(V2, ownPosition,
            &path);
    if (v2SiteAhead < barrierAhead) {
        return v2SiteAhead;
    }
    return -1u;
}


/*
 *Determine the next site according to this rule and return it.
 *Rule: Move forward to the earliest site which has room.
 */
unsigned int rule_next_free(int playersCount, int ownPosition) {
    unsigned int i = 0;
    int siteUsage = 0;

    for (i = ownPosition + 1; i < path.siteCount; i++) {
        siteUsage = player_get_site_usage(playerPositions, playersCount, i);
        if (siteUsage < path.sites[i].capacity) {
            return i;
        }
    }
    return -1u;
}

/*
 *Determine the target of the next move according to the player's strategy.
 *We start at the given current position not taking the site's capacity into
 *account.
 */
unsigned int calculate_move_to(int playersCount, unsigned int ownPosition,
        unsigned int barrierAhead) {
    unsigned int siteToGo = -1u;

    siteToGo = rule_we_are_last(playersCount, ownPosition);
    /*fprintf(stderr, "Chose %d", siteToGo);*/

    if (-1u == siteToGo) {
        siteToGo = rule_odd_money(playersCount, barrierAhead, ownPosition);
        /*fprintf(stderr, " 2->%d", siteToGo);*/
    }

    if (-1u == siteToGo) {
        siteToGo = rule_draw_card(playersCount, barrierAhead, ownPosition);
        /*fprintf(stderr, " 3->%d", siteToGo);*/
    }

    if (-1u == siteToGo) {
        siteToGo = rule_goto_v2(playersCount, barrierAhead, ownPosition);
        /*fprintf(stderr, " 4->%d", siteToGo);*/
    }

    if (-1u == siteToGo) {
        siteToGo = rule_next_free(playersCount, ownPosition);
        /*fprintf(stderr, " 5->%d", siteToGo);*/
    }

    /*fprintf(stderr, " finally %d\n", siteToGo);*/
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

    /*Rule #0: Don't move beyond the end of the path.*/
    if (path.siteCount <= ownPosition + 1) {
        return;
    }

    /*Rule #0.1: Always stop at a barrier*/
    barrierAhead = (unsigned int)player_find_x_site_ahead(BARRIER, ownPosition,
            &path);

    do {
        siteToGo = calculate_move_to(playersCount, ownPosition, barrierAhead);
        /*fprintf(stderr, "Positions: %2d %2d %2d \n",*/
                /*playerPositions[0],*/
                /*playerPositions[1],*/
                /*playerPositions[2]*/
               /*);*/
        if (-1u != siteToGo) {
            /*Make sure to not move beyond the end of the path*/
            moved = player_forward_to(stdout, siteToGo, barrierAhead,
                    playersCount, playerPositions, playerRankings, ownId,
                    &path);
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
        error_return(stderr, E_EARLY_GAME_OVER);
    } else if (0 == strncmp("DONE", command, 4u)) {
        return 0;
    } else if (0 == strncmp("YT", command, 2u)) {
        if (!('\0' == command[2]
                || '\n' == command[2]
                || EOF == command[2])) {
            error_return(stderr, E_COMMS_ERROR);
        }
        make_move(playersCount);
    } else if (0 == strncmp("HAP", command, 3u)) {
        player_process_move_broadcast(command, playerPositions, playerRankings,
                playersCount, ownId, thisPlayer, &players, &path);
    } else {
        error_return(stderr, E_COMMS_ERROR);
    }
    return 1;
}

/*
 *Game play loop.
 */
void run_game(int playersCount) {
    char command[100];
    int run = 1;

    get_path(playersCount);

    player_print_path(stderr, &path, playersCount, path.siteCount,
            playerPositions, playerRankings, 1);

    while (run) {
        if (!fgets(command, sizeof(command), stdin)) {
            player_free_path(&path);
            error_return(stderr, E_COMMS_ERROR);
        }
        run = process_command(command, playersCount);
    }

    player_print_scores(stderr, playersCount, players);
}

int main(int argc, char* argv[]) {
    int playersCount = 0;
    int playerID = 0;
    int i = 0;

    /*Check for valid number of parameters*/
    if (3 != argc) {
        error_return(stderr, E_INVALID_ARGS_COUNT);
    }

    /*Check for valid number of players*/
    for (i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            error_return(stderr, E_INVALID_PLAYER_COUNT);
        }
    }
    playersCount = atoi(argv[1]);
    if (1 > playersCount) {
        error_return(stderr, E_INVALID_PLAYER_COUNT);
    }

    /*Check for valid player ID*/
    for (i = 0; i < strlen(argv[2]); i++) {
        if (!isdigit(argv[2][i])) {
            error_return(stderr, E_INVALID_PLAYER_ID);
        }
    }
    playerID = atoi(argv[2]);
    if (playersCount <= playerID) {
        error_return(stderr, E_INVALID_PLAYER_ID);
    }
    ownId = playerID;
    players = malloc(MAX_PLAYERS * sizeof(Player));
    thisPlayer = &(players[ownId]);
    for (i = 0; i < MAX_PLAYERS; i++) {
        dealer_reset_player(&(players[i]));
    }

    init_player_positions(playersCount);

    run_game(playersCount);

    free(players);
    free(playerPositions);
    free(playerRankings);

    return EXIT_SUCCESS;
}

