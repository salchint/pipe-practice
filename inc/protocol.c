/*
 *protocol.c
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include "../inc/errorReturn.h"
#include "../inc/protocol.h"

/*
 *Calculate the maximum byte count of a given path.
 *Pass the number of players in the game and the number of sites in the path.
*/
int calculate_path_length(int playersCount, int siteCount) {
    int maxDigitCountPlayers = 0;
    int maxDigitCountSites = 0;

    maxDigitCountPlayers = (int)(1 + log10((double)playersCount));
    maxDigitCountSites = (int)(1 + log10((double)siteCount));
    return maxDigitCountSites                       /*site count*/
            + 1                         /*separator semi-colon*/
            /*site type and capacity*/
            + siteCount * (2 + maxDigitCountPlayers)
            + 1                         /*line break*/
            + 1;                        /*terminating '\0'*/
}

/*
 *Initialize all the player structure's fields.
 */
void reset_player(Player* player) {
    memset(player, 0, sizeof(Player));
    player->money = 7;
}

/*
 *Initialize all the path structure's fields.
 */
void reset_path(Path* path) {
    path->buffer = NULL;
    path->sites = NULL;
    path->siteCount = 0u;
    path->bufferLength = 0u;
}

/*
 *Zero the fields of the deck instance.
 */
void reset_deck(Deck* deck) {
    memset(deck, 0, sizeof(Deck));
}

/*
 *Allocates memory for buffer and sites.
 */
int build_path(FILE* stream, int playersCount, Path* path) {
    int siteCount = 0;
    int readChars = 0;
    char separator = '\0';

    if ((path->bufferLength > 0) || (path->siteCount > 0)) {
        fprintf(stderr, "  !!! Path was not freed !!!\n");
    }
    reset_path(path);

    readChars = fscanf(stream, "%d%c", &siteCount, &separator);
    if (EOF == readChars || ';' != separator) {
        return E_INVALID_PATH;
    }

    path->bufferLength = calculate_path_length(playersCount, siteCount);
    path->buffer = (char*)malloc(path->bufferLength);
    path->sites = (Site*)malloc(siteCount * sizeof(Site));
    path->siteCount = siteCount;

    return E_OK;
}

/*
 *Deallocate the sites and the path buffer.
 */
void free_path(Path* path) {
    if (path) {
        if (path->siteCount) {
            if (path->sites) {
                free((void*)path->sites);
                path->sites = NULL;
                path->siteCount = 0u;
            }
        }
        if (path->bufferLength) {
            if (path->buffer) {
                free((void*)path->buffer);
                path->buffer = NULL;
                path->bufferLength = 0u;
            }
        }
    }
}

/*
 *Check if the current site is a barrier.
 */
int is_barrier(const char* site) {
    return ':' == site[0]
            && ':' == site[1]
            && '-' == site[2];
}

/*
 *Convert site names to enumeration types.
 */
enum SiteTypes convert_site_type(const char* siteName) {
    if (0 == strcmp("Mo", siteName)) {
        return MO;
    } else if (0 == strcmp("V1", siteName)) {
        return V1;
    } else if (0 == strcmp("V2", siteName)) {
        return V2;
    } else if (0 == strcmp("Do", siteName)) {
        return DO;
    } else if (0 == strcmp("Ri", siteName)) {
        return RI;
    } else if (0 == strcmp("::", siteName)) {
        return BARRIER;
    }
    return UNKNOWN_SITE_TYPE;
}

/*
 *Convert site type enumeration to names.
 */
const char* convert_site_name(enum SiteTypes type) {
    switch (type) {
        case MO:
            return "Mo";
        case V1:
            return "V1";
        case V2:
            return "V2";
        case DO:
            return "Do";
        case RI:
            return "Ri";
        case BARRIER:
            return "::";
        default:
            return "__";
    }
}

/*
 *Validate and de-serialize the given path.
 *Unusual long function: Breaking the path deserialization routine into pieces
 *would only scatter the logic, but does not improve readability.
 */
int deserialize_path(FILE* stream, Path* path, int playersCount) {
    int success = E_OK;
    char* pos = NULL;
    size_t siteIdx = 0;
    int i = 0;
    int readChars = 0;
    char siteName[3];
    int siteCapacity = 0;

    siteName[2] = '\0';

    if (!fgets(path->buffer, path->bufferLength, stream)) {
        free_path(path);
        return E_INVALID_PATH;
    }

    /*First there needs to be a barrier */
    pos = path->buffer;
    if (!is_barrier(pos)) {
        free_path(path);
        return E_INVALID_PATH;
    }
    path->sites[siteIdx].capacity = playersCount;
    path->sites[siteIdx].type = BARRIER;
    pos += 3;
    siteIdx += 1;

    /*Deserialize all the sites*/
    for (i = 0; i < (int)path->siteCount && '\n' != *pos
            && '\0' != *pos; i++) {
        if (is_barrier(pos)) {
            path->sites[siteIdx].capacity = playersCount;
            path->sites[siteIdx].type = BARRIER;
            pos += 3;
            siteIdx += 1;
        } else {
            readChars = sscanf(pos, "%c%c%d;",
                    &siteName[0], &siteName[1], &siteCapacity);
            if (EOF == readChars || readChars < 3) {
                free_path(path);
                return E_INVALID_PATH;
            }
            path->sites[siteIdx].capacity = siteCapacity;
            path->sites[siteIdx].type = convert_site_type(siteName);
            pos += readChars;
            siteIdx += 1;
        }

        /*Verify the buffer boundary*/
        if (path->buffer + path->bufferLength < pos) {
            free_path(path);
            return E_INVALID_PATH;
        }
    }

    return success;
}

/*
 *Verify the path's integrity.
 */
int verify_path(Path* path) {
    if (BARRIER != path->sites[0].type) {
        return E_INVALID_PATH;
    }
    if (BARRIER != path->sites[path->siteCount - 1].type) {
        return E_INVALID_PATH;
    }

    return E_OK;
}

/*
 *Determine the rankings of players if they are on the same site.
 */
void calculate_initial_rankings(const int* positions, int* rankings,
        int playersCount) {
    int playerIdx = 0;
    int higherIdx = 0;

    memset(rankings, 0, playersCount * sizeof(int));

    for (playerIdx = playersCount - 1; 0 <= playerIdx; playerIdx--) {
        for (higherIdx = playerIdx + 1; higherIdx < playersCount;
                higherIdx++) {
            /*Increment this player's ranking if there is a "higher" player*/
            /*on the same position.*/
            if (positions[higherIdx] == positions[playerIdx]) {
                rankings[playerIdx]++;
            }
        }
    }
}

/*
 *Allocate the map as a contignuous chunk.
 */
int** alloc_map(int rows, int columns) {
    int bodySize = 0;
    int headerSize = 0;
    int** row = NULL;
    int* buf = NULL;
    int i = 0;

    headerSize = rows * sizeof(int*);
    bodySize = rows * columns * sizeof(int);

    row = (int**)malloc(headerSize + bodySize);
    memset(row, -1, headerSize + bodySize);

    buf = (int*)(row + rows);
    row[0] = buf;
    for(i = 1; i < rows; i++) {
        row[i] = row[i - 1] + columns;
    }

    return row;
}

/*
 *Player asks the dealer for the path information.
 *You need to pass a file stream parameter, which writes to the pipe which the
 *dealer is listening at.
 */
void player_request_path(FILE* stream) {
    fprintf(stream, "^");
    fflush(stream);
}

/*
 *Dealer asks the player for his next move.
 */
void dealer_request_next_move(FILE* stream) {
    fprintf(stream, "YT\n");
    fflush(stream);
}

/*
 *Send informations of a player's move to all participating players.
 */
void dealer_broadcast_player_move(FILE** streams, int playersCount,
        int id, int targetSite, int pointDiff, int moneyDiff, int newCard) {
    int i = 0;

    for (i = 0; i < playersCount; i++) {
        fprintf(streams[i], "HAP%d,%d,%d,%d,%d\n",
                id, targetSite, pointDiff, moneyDiff, newCard);
        fflush(streams[i]);
    }
}

/*
 *Send DONE to all participating players.
 */
void dealer_broadcast_end(FILE** streams, int playersCount) {
    int i = 0;

    for (i = 0; i < playersCount; i++) {
        fprintf(streams[i], "DONE\n");
        fflush(streams[i]);
    }
}

/*
 *Initialize all the path structure's fields.
 */
void player_reset_path(Path* path) {
    reset_path(path);
}

/*
 *Read the path from stream and verify it.
 *Example: 7;::-Mo1V11V22Mo1Mo1::-
 */
int player_read_path(FILE* stream, int playersCount, Path* path) {
    int success = E_OK;

    success = build_path(stream, playersCount, path);
    if (E_OK != success) {
        return success;
    }
    /*
     *printf("Path built\n");
     */

    success = deserialize_path(stream, path, playersCount);
    if (E_OK != success) {
        return success;
    }
    /*
     *printf("Path deserialized\n");
     */

    success = verify_path(path);
    if (E_OK != success) {
        return success;
    }
    /*
     *printf("Path verified\n");
     */

    return success;
}

/*
 *Free resources associated with the received path.
 */
void player_free_path(Path* path) {
    free_path(path);
}

/*
 *Print the path including all players' positions.
 */
void player_print_path(FILE* output, Path* path, int playersCount,
        int siteCount, const int* positions, int* rankings,
        int initialSorting) {
    int i, row, column = 0;
    int lineLength = 0;
    int** map = NULL;
    int playerNo = 0;
    int count = 0;

    if (initialSorting) {
        /*Get the initial rankings of all the players on their
         * positions/sites.*/
        calculate_initial_rankings(positions, rankings, playersCount);
    }

    /*Print the first line representing the path.*/
    for (i = 0; i < (int)path->siteCount; i++) {
        fprintf(output, "%s ", convert_site_name(path->sites[i].type));
        lineLength += 3;
    }
    fputs("\n", output);

    /*Generate a map representing all players' positions.*/
    map = alloc_map(playersCount, siteCount);
    for (i = 0; i < playersCount; i++) {
        map[rankings[i]][positions[i]] = i;
    }

    /*Print the players' positions line by line.*/
    for (row = 0; row < playersCount && count < playersCount; row++) {
        for (column = 0; column < siteCount; column++) {
            playerNo = map[row][column];
            if (-1 == playerNo) {
                fputs("   ", output);
            } else {
                fprintf(output, "%-3d", playerNo);
                count += 1;
            }
        }
        fputs("\n", output);
    }

    free(map);
}

/*
 *Find the next site matching the given site type.
 */
int player_find_x_site_ahead(enum SiteTypes type, int ownPosition,
        const Path* path) {
    size_t i = 0;
    for (i = ownPosition + 1; i < path->siteCount; i++) {
        if (type == path->sites[i].type) {
            return i;
        }
    }
    return -1;
}

/*
 *Calculate the number of players positioned on the given site.
 */
unsigned int player_get_site_usage(const int* positions, int playersCount,
        int siteIdx) {
    int i = 0;
    int usage = 0;

    for (i = 0; i < playersCount; i++) {
        if (siteIdx == positions[i]) {
            usage += 1;
        }
    }
    return usage;
}

/*
 *Let this player move forward to the site specified.
 *Returns 1 if successful, 0 if the site is full.
 */
int player_forward_to(FILE* output, int siteIdx, int barrierIdx,
        int playersCount, int* positions, int* rankings, int ownId,
        Path* path) {
    int siteUsage = 0;

    siteIdx = MIN(siteIdx, barrierIdx);

    /*Check if the targeted site still has capacity*/
    siteUsage = player_get_site_usage(positions, playersCount,
            siteIdx);
    /*
     *fprintf(stderr, "Make move to %d cap:%d use:%d\n", siteIdx,
     *        path->sites[siteIdx].capacity, siteUsage);
     */

    if (path->sites[siteIdx].capacity <= siteUsage) {
        /*This site is full*/
        return 0;
    }

    positions[ownId] = siteIdx;
    rankings[ownId] = siteUsage;
    fprintf(output, "DO%d\n", siteIdx);
    fflush(output);
    return 1;
}

/*
 *Update the player positions map for the given move.
 */
void player_update_position(int id, int playersCount, int* positions,
        int* rankings, int siteIdx) {
    int ranking = 0;

    ranking = player_get_site_usage(positions, playersCount, siteIdx);

    positions[id] = siteIdx;
    rankings[id] = ranking;
}

/*
 *Print the given player's statistics.
 */
void player_print_earnings(FILE* output, int id, const Player* player) {
    fprintf(output,
            "Player %d "
            "Money=%d V1=%d V2=%d Points=%d A=%d B=%d C=%d D=%d E=%d\n",
            id, player->money, player->v1, player->v2, player->points,
            player->cards[CARD_A],
            player->cards[CARD_B],
            player->cards[CARD_C],
            player->cards[CARD_D],
            player->cards[CARD_E]
            );
}

/*
 *Draw the next card from the deck.
 *Wrap around if we ran out of cards.
 */
int dealer_draw_card_from_deck(Deck* deck) {
    char newCard = *deck->nextCard;

    /*Convert to integer */
    newCard -= 'A' - 1;

    deck->nextCard += 1;
    if ((deck->buffer + deck->size) <= deck->nextCard) {
        deck->nextCard = deck->buffer;
    }

    return (int)newCard;
}

/*
 *Update the given player's earnings.
 */
void dealer_calculate_player_earnings(int id, int targetSite, int* pointDiff,
        int* moneyDiff, int* newCard, Path* path, Player* player, Deck* deck) {
    *pointDiff = 0;
    *moneyDiff = 0;
    *newCard = 0;

    switch (path->sites[targetSite].type) {
        case MO:
            player->money += 3;
            *moneyDiff = 3;
            break;
        case DO:
            *pointDiff = (int)(player->money / 2);
            player->points += *pointDiff;
            *moneyDiff = -(player->money);
            player->money = 0;
            break;
        case V1:
            player->v1 += 1;
            break;
        case V2:
            player->v2 += 1;
            break;
        case RI:
            *newCard = dealer_draw_card_from_deck(deck);
            player->overallCards += 1;
            player->cards[*newCard] += 1;
            break;
        default:
            break;
    }
}

/*
 *Update the given player's reported earnings.
 */
void player_calculate_player_earnings(int id, int targetSite, Path* path,
        Player* player) {

    switch (path->sites[targetSite].type) {
        case V1:
            player->v1 += 1;
            break;
        case V2:
            player->v2 += 1;
            break;
        default:
            break;
    }
}

/*
 *Deserialize the move operation of the given player for own book-keeping.
 */
void player_process_move_broadcast(const char* command, int* positions,
        int* rankings, int playersCount, int ownId, Player* thisPlayer,
        Player** otherPlayers, Path* path) {
    int id = 0;
    int siteIdx = 0;
    int pointDiff = 0;
    int moneyDiff = 0;
    int newCard = 0;
    int readChars = 0;
    Player* printPlayer = NULL;

    readChars = sscanf(command, "HAP%d,%d,%d,%d,%d",
            &id, &siteIdx, &pointDiff, &moneyDiff, &newCard);
    if (5 > readChars || EOF == readChars) {
        error_return(stderr, E_COMMS_ERROR);
    }
    if (!(0 <= id && id < playersCount)) {
        error_return(stderr, E_COMMS_ERROR);
    }
    if (!(0 <= siteIdx && siteIdx < (int)path->siteCount)) {
        error_return(stderr, E_COMMS_ERROR);
    }

    if (ownId == id) {
        printPlayer = thisPlayer;
    } else {
        player_update_position(id, playersCount, positions, rankings, siteIdx);
        printPlayer = (*otherPlayers) + id;

        /*
         *fprintf(stderr, "Player stats: pos=%d money=%d cards=%d points=%d\n",
         *    siteIdx,
         *    (*otherPlayers)[id].money,
         *    (*otherPlayers)[id].overallCards,
         *    (*otherPlayers)[id].points
         *    );
         */
    }
    printPlayer->money += moneyDiff;
    printPlayer->points += pointDiff;
    if (newCard) {
        printPlayer->overallCards += 1;
        printPlayer->cards[newCard] += 1;
    }

    player_calculate_player_earnings(id, siteIdx, path, printPlayer);
    player_print_earnings(stderr, id, printPlayer);
    player_print_path(stderr, path, playersCount, path->siteCount,
            positions, rankings, 0);
}


/*
 *Initialize all the player structure's fields.
 */
void dealer_reset_player(Player* player) {
    reset_player(player);
}

/*
 *Check if the game has ended, i.e. all players are at the final site.
 *Returns non-zero if it is.
 */
int dealer_is_finished(int playersCount, int siteCount,
        const int* positions, const int* rankings) {
    int i = 0;

    /*The game is over if we found a player at the very end of the path*/
    /*with the maximum possible ranking;*/
    for (i = 0; i < playersCount; i++) {
        if ((playersCount - 1) == rankings[i]) {
            if ((siteCount - 1) == positions[i]) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 *Calculate the number of additional points from collected cards.
 */
int dealer_calculate_card_points(Player* player) {
    int cardCount = 0;
    int points = 0;
    int sum = 0;
    int i = 0;

    do {
        cardCount = 0;
        points = 0;

        for (i = 1; i < (int)CARD_TYPES_COUNT + 1; i++) {
            if (player->cards[i]) {
                cardCount += 1;
                player->cards[i] -= 1;
            }
        }

        switch (cardCount) {
            case 5:
                points = 10;
                break;
            case 4:
                points = 7;
                break;
            case 3:
                points = 5;
                break;
            case 2:
                points = 3;
                break;
            case 1:
                points = 1;
                break;
            default:
                points = 0;
        }

        sum += points;
    } while (cardCount);

    return sum;
}

/*
 *Print the final scores of all players.
 */
void player_print_scores(FILE* output, int playersCount, Player* players) {
    int i = 0;

    fprintf(output, "Scores: ");
    for (i = 0; i < playersCount; i++) {
        players[i].points += players[i].v1;
        players[i].points += players[i].v2;
        players[i].points += dealer_calculate_card_points(players + i);
        fprintf(output, "%d", players[i].points);
        if (i < playersCount - 1) {
            fputc(',', output);
        }
    }
    fputc('\n', output);
}

/*
 *Build a new instance of a deck and initialize it with data from stream.
 */
void dealer_init_deck(FILE* stream, Deck* deck) {
    int readChars = 0;

    readChars = fscanf(stream, "%zu",
            &(deck->size));
    if (EOF == readChars || readChars < 1) {
        error_return_dealer(stderr, E_DEALER_INVALID_DECK, 1);
    }
    deck->buffer = (char*)malloc((deck->size + 2) * sizeof(int));
    deck->nextCard = deck->buffer;

    if (!fgets(deck->buffer, deck->size + 1, stream)) {
        free(deck->buffer);
        error_return_dealer(stderr, E_DEALER_INVALID_DECK, 1);
    }
}

