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
 *The write end of a pipe.
 */
#define WRITE_END 1
/*
 *The read end of a pipe.
 */
#define READ_END 0

/*
 *Deck object holding a sequence of cards to draw.
 */
Deck deck;
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
 *Array of players used for book-keeping.
 */
Player players[MAX_PLAYERS];

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
 *Streams that are to be used for broadcasts.
 */
FILE* broadcastStreams[MAX_PLAYERS];

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

    if (!(streamToPlayer[playerId][READ_END]
            || streamToPlayer[playerId][WRITE_END]
            || streamToDealer[playerId][READ_END]
            || streamToDealer[playerId][WRITE_END])) {
        error_return_dealer(stderr, E_DEALER_INVALID_START_PLAYER, 0);
    }

    broadcastStreams[playerId] = streamToPlayer[playerId][WRITE_END];
    return E_DEALER_OK;
}

/*
 *Determine the player, who is next.
 */
int calculate_next_player(const int* positions, const int* rankings) {
    int i = 0;
    int minSite = INT_MAX;
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
 *Returns zero in case the game has ended, non-zero else.
 */
int receive_next_move(FILE* readStream, int id, int* positions,
        int* rankings) {
    char buffer[100];
    int targetSite = 0;
    int readChars = 0;
    int pointDiff = 0;
    int moneyDiff = 0;
    int newCard = 0;

    if (!fgets(buffer, sizeof(buffer), readStream)) {
        error_return_dealer(stdout, E_DEALER_COMMS_ERROR, 1);
    }

    readChars = sscanf(buffer, "DO%d", &targetSite);
    if (1 > readChars || EOF == readChars) {
        error_return_dealer(stdout, E_DEALER_COMMS_ERROR, 1);
    }
    if (path.siteCount < targetSite + 1) {
        error_return_dealer(stdout, E_DEALER_COMMS_ERROR, 1);
    }

    move_player(id, targetSite, positions, rankings);
    dealer_calculate_player_earnings(id, targetSite, &pointDiff, &moneyDiff,
            &newCard, &path, players + id, &deck);
    player_print_earnings(stdout, id, players + id);
    player_print_path(stdout, &path, playersCount, path.siteCount,
            positions, rankings, 0);
    dealer_broadcast_player_move(broadcastStreams, playersCount, id,
            targetSite, pointDiff, moneyDiff, newCard);

    return dealer_is_finished(playersCount, path.siteCount, positions,
            rankings);
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
        dealer_request_next_move(streamToPlayer[nextPlayer][WRITE_END]);
        run = receive_next_move(streamToDealer[nextPlayer][READ_END],
                nextPlayer, playerPositions, playerRankings) ? 0 : 1;
    }

    /*Finally, quit all the players and print the scores*/
    dealer_broadcast_end(broadcastStreams, playersCount);
    player_print_scores(stdout, playersCount, players);
}

/*
 *Launch the player processes.
 */
void run_player(int id, const char** playerNames) {
    char buffer[100];
    char bufferCount[10];
    char bufferId[10];
    int devNull = 0;

    *buffer = '\0';
    *bufferCount = '\0';
    *bufferId = '\0';

    open_stream(id);

    /*Redirect stdin, stdout of the players*/
    dup2(pipeToPlayerNo[id][READ_END], READ_END);
    dup2(pipeToDealerNo[id][WRITE_END], WRITE_END);

    /*Redirect stderr to /dev/null*/
    devNull = open("/dev/null", O_WRONLY);
    dup2(devNull, STDERR_FILENO);

    fclose(streamToPlayer[id][READ_END]);
    fclose(streamToPlayer[id][WRITE_END]);
    fclose(streamToDealer[id][READ_END]);
    fclose(streamToDealer[id][WRITE_END]);

    sprintf(bufferCount, "%d", playersCount);
    sprintf(bufferId, "%d", id);
    execlp(playerNames[id], playerNames[id], bufferCount, bufferId, NULL);

    /*Should not happen!*/
    error_return_dealer(stderr, E_DEALER_INVALID_START_PLAYER, 0);
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
            error_return_dealer(stderr, E_DEALER_INVALID_START_PLAYER, 1);
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
void get_path(FILE* stream) {
    int success = E_OK;

    success = player_read_path(stream, playersCount, &path);
    if(E_OK != success) {
        error_return_dealer(stderr, E_DEALER_INVALID_PATH, 1);
    }
}

/*
 *Handle the SIGHUP signal by interrupting the players.
 */
void signal_handler(int signal) {
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

/*
 *Check the number of args and if the files are valid.
 */
void verify_args(int argc, char* argv[], FILE** pathStream,
        FILE** deckStream) {
    char* deckName = NULL;
    char* pathName = NULL;

    /*Check for valid number of parameters*/
    if (4 > argc) {
        error_return_dealer(stderr, E_DEALER_INVALID_ARGS_COUNT, 1);
    }

    /*Check opening the deck file*/
    deckName = argv[1];
    *deckStream = fopen(deckName, "r");
    if (NULL == *deckStream) {
        error_return_dealer(stderr, E_DEALER_INVALID_DECK, 1);
    }

    /*Check opening the path file*/
    pathName = argv[2];
    *pathStream = fopen(pathName, "r");
    if (NULL == *pathStream) {
        error_return_dealer(stderr, E_DEALER_INVALID_PATH, 1);
    }

}

int main(int argc, char* argv[]) {
    char** playerNames = NULL;
    int i = 0;
    FILE* pathStream = NULL;
    FILE* deckStream = NULL;
    FILE* file = NULL;

    playersCount = 0;
    playerPositions = NULL;
    playerRankings = NULL;

    signal(SIGHUP, signal_handler);

    verify_args(argc, argv, &pathStream, &deckStream);

    /*Remember the player program names*/
    playerNames = malloc((argc - 3) * sizeof(char*));
    for (i = 3; i < argc; i++, playersCount++) {
        playerNames[i - 3] = argv[i];

        file = fopen(playerNames[i - 3], "rb");
        if (file) {
            fclose(file);
        } else {
            error_return_dealer(stderr, E_DEALER_INVALID_START_PLAYER, 0);
        }

    }

    for (i = 0; i < playersCount; i++) {
        dealer_reset_player(players + i);
    }
    init_player_positions();
    get_path(pathStream);
    fclose(pathStream);
    dealer_init_deck(deckStream, &deck);
    fclose(deckStream);

    start_players((const char**)playerNames);

    for (i = 0; i < playersCount; i++) {
        waitpid(pids[i], NULL, 0);
    }

    free(playerPositions);
    free(playerRankings);
    free(playerNames);
    free(deck.buffer);

    return EXIT_SUCCESS;
}

