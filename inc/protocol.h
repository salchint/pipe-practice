/*
 *protocol.h
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "errorReturn.h"

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
 *Calculate the maximum byte count of a given path.
 *Pass the number of players in the game and the number of sites in the path.
*/
int calculate_path_length(int playersCount, int siteCount) {
    int maxDigitCountPlayers = 0;
    int maxDigitCountSites = 0;

    maxDigitCountPlayers = (int)(1 + log10((double)playersCount));
    maxDigitCountSites = (int)(1 + log10((double)siteCount));
    return maxDigitCountSites                       /*site count*/
        + 1                                         /*separator semi-colon*/
        + siteCount * (2 + maxDigitCountPlayers)    /*site type and capacity*/
        + 1                                         /*line break*/
        + 1;                                        /*terminating '\0'*/
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
 *Allocates memory for buffer and sites.
 */
int build_path(FILE* stream, int playersCount, Path* path) {
    int siteCount = 0;
    int readChars = 0;
    char separator = '\0';

    if ( (path->bufferLength > 0) || (path->siteCount > 0) ) {
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
    for (i = 0; i < path->siteCount && '\n' != *pos && '\0' != *pos; i++) {
        if (is_barrier(pos)) {
            path->sites[siteIdx].capacity = playersCount;
            path->sites[siteIdx].type = BARRIER;
            pos += 3;
            siteIdx += 1;
            /*
             *printf("    site name=:: cap=-\n");
             */
        } else {
            readChars = sscanf(pos, "%c%c%d;",
                    &siteName[0], &siteName[1], &siteCapacity);
            /*
             *printf("    site name=%s cap=%d; readCh=%d\n", siteName, siteCapacity, readChars);
             */
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
void calculate_rankings(const int* positions, int* rankings,
        int playersCount) {
    int playerIdx = 0;
    int higherIdx = 0;

    memset(rankings, 0, playersCount * sizeof(int));

    for (playerIdx = playersCount - 1; 0 <= playerIdx; playerIdx--) {
        for (higherIdx = playerIdx + 1; higherIdx < playersCount; higherIdx++) {
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

    buf  = (int*)(row + rows);
    row[0] = buf;
    for(i = 1; i < rows; i++) {
        row[i] = row[i-1] + columns;
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

    success = deserialize_path(stream, path, playersCount);
    if (E_OK != success) {
        return success;
    }

    success = verify_path(path);
    if (E_OK != success) {
        return success;
    }

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
        int siteCount, const int* positions, int* rankings) {
    int i, row, column = 0;
    int lineLength = 0;
    int** map = NULL;
    int playerNo = 0;
    int count = 0;

    /*Get the rankings of all the players on their positions/sites.*/
    calculate_rankings(positions, rankings, playersCount);

    /*Print the first line representing the path.*/
    for (i = 0; i < path->siteCount; i++) {
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
 *Determine the rankings of players if they are on the same site.
 */
void player_calculate_rankings(const int* positions, int* rankings,
        int playersCount) {
    calculate_rankings(positions, rankings, playersCount);
}

#endif

