/*
 *protocol.h
 */

#pragma once

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#include "../inc/errorReturn.h"

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/*
 *Maximum number of players.
 */
#define MAX_PLAYERS 1024u

/*
 *The number of card types (A..E).
 */
#define CARD_TYPES_COUNT 5u

/*
 *Site types.
 *MO .. Gain 3 Money.
 *V1 .. Gain 1 Point for each visited V1 at the end of the game.
 *V2 .. Gain 1 Point for each visited V2 at the end of the game.
 *DO .. Convert Money to points. (1 point for 2 money, rounded down).
 *RI .. Draw card.
 *BARRIER .. Player stops move here.
 */
enum SiteTypes {
    MO, V1, V2, DO, RI, BARRIER, UNKNOWN_SITE_TYPE
};

/*
 *Enumerate all card types.
 */
enum CardTypes {
    CARD_A = 1,
    CARD_B,
    CARD_C,
    CARD_D,
    CARD_E,
};

/*
 *Descriptor of a site including its type and then number player it can
 *habitate.
 */
typedef struct {
    enum SiteTypes type;
    int capacity;
} Site;

/*
 *Descriptor of the path including a list of all sites in order.
 */
typedef struct {
    size_t siteCount;
    Site* sites;
    char* buffer;
    size_t bufferLength;
} Path;

/*
 *Descriptor of the player and its earnings.
 */
typedef struct {
    int money;
    int v1;
    int v2;
    int points;
    int cards[CARD_TYPES_COUNT + 1];
    int overallCards;
} Player;

/*
 *Descriptor of the card deck.
 */
typedef struct {
    size_t size;
    char* buffer;
    char* nextCard;
} Deck;


/*
 *Player asks the dealer for the path information.
 *You need to pass a file stream parameter, which writes to the pipe which the
 *dealer is listening at.
 */
void player_request_path(FILE* stream);

/*
 *Dealer asks the player for his next move.
 */
void dealer_request_next_move(FILE* stream);

/*
 *Send informations of a player's move to all participating players.
 */
void dealer_broadcast_player_move(FILE** streams, int playersCount,
        int id, int targetSite, int pointDiff, int moneyDiff, int newCard);

/*
 *Send DONE to all participating players.
 */
void dealer_broadcast_end(FILE** streams, int playersCount);

/*
 *Initialize all the path structure's fields.
 */
void player_reset_path(Path* path);

/*
 *Read the path from stream and verify it.
 *Example: 7;::-Mo1V11V22Mo1Mo1::-
 */
int player_read_path(FILE* stream, int playersCount, Path* path);

/*
 *Free resources associated with the received path.
 */
void player_free_path(Path* path);

/*
 *Print the path including all players' positions.
 */
void player_print_path(FILE* output, Path* path, int playersCount,
        int siteCount, const int* positions, int* rankings,
        int initialSorting);

/*
 *Find the next site matching the given site type.
 */
int player_find_x_site_ahead(enum SiteTypes type, int ownPosition,
        const Path* path);

/*
 *Calculate the number of players positioned on the given site.
 */
unsigned int player_get_site_usage(const int* positions, int playersCount,
        int siteIdx);

/*
 *Let this player move forward to the site specified.
 *Returns 1 if successful, 0 if the site is full.
 */
int player_forward_to(FILE* output, int siteIdx, int barrierIdx,
        int playersCount, int* positions, int* rankings, int ownId,
        Path* path);

/*
 *Update the player positions map for the given move.
 */
void player_update_position(int id, int playersCount, int* positions,
        int* rankings, int siteIdx);

/*
 *Print the given player's statistics.
 */
void player_print_earnings(FILE* output, int id, const Player* player);

/*
 *Draw the next card from the deck.
 *Wrap around if we ran out of cards.
 */
int dealer_draw_card_from_deck(Deck* deck);

/*
 *Update the given player's earnings.
 */
void dealer_calculate_player_earnings(int id, int targetSite, int* pointDiff,
        int* moneyDiff, int* newCard, Path* path, Player* player, Deck* deck);

/*
 *Update the given player's reported earnings.
 */
void player_calculate_player_earnings(int id, int targetSite, Path* path,
        Player* player);

/*
 *Deserialize the move operation of the given player for own book-keeping.
 */
void player_process_move_broadcast(const char* command, int* positions,
        int* rankings, int playersCount, int ownId, Player* thisPlayer,
        Player** otherPlayers, Path* path);

/*
 *Initialize all the player structure's fields.
 */
void dealer_reset_player(Player* player);

/*
 *Check if the game has ended, i.e. all players are at the final site.
 *Returns non-zero if it is.
 */
int dealer_is_finished(int playersCount, int siteCount,
        const int* positions, const int* rankings);

/*
 *Calculate the number of additional points from collected cards.
 */
int dealer_calculate_card_points(Player* player);

/*
 *Print the final scores of all players.
 */
void player_print_scores(FILE* output, int playersCount, Player* players);

/*
 *Build a new instance of a deck and initialize it with data from stream.
 */
void dealer_init_deck(FILE* stream, Deck* deck);

#endif

