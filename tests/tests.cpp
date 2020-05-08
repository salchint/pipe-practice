#include "../inc/protocol.h"
#include "../inc/errorReturn.h"
#include <vector>
#include <array>
#include <string>
#include <iostream>

#include <unistd.h>

using std::string;

#include "gtest/gtest.h"
//#include "gmock/gmock.h"

using std::array;
using std::vector;

// Forward declarations
//using ::testing::Lt;

// Prototyping

// Fixture
class PlayerASuite : public ::testing::Test {
public:
    int fileDesc[2];
    FILE* fileStream[2];
    Path path;

public:
    void SetUp() override {
        ASSERT_NE(-1, pipe(fileDesc));
        fileStream[0] = fdopen(fileDesc[0], "r");
        fileStream[1] = fdopen(fileDesc[1], "w");
        ASSERT_NE(nullptr, fileStream[0]);
        ASSERT_NE(nullptr, fileStream[1]);

        player_reset_path(&path);
    }

    void TearDown() override {
        if (fileStream[1]) {
            fclose(fileStream[1]);
            fileStream[1] = nullptr;
        }
        if (fileStream[0]) {
            fclose(fileStream[0]);
            fileStream[0] = nullptr;
        }

        player_free_path(&path);
    }
};

// Tests
TEST_F(PlayerASuite, test00) {
    EXPECT_EQ(1, 1);
    //EXPECT_THAT(1, Lt(2));
}

TEST_F(PlayerASuite, test_request_path) {
    player_request_path(fileStream[1]);
    fclose(fileStream[1]);
    EXPECT_EQ('^', fgetc(fileStream[0]));
}

TEST_F(PlayerASuite, test_read_path_success) {
    const char buffer[] = "7;::-Mo1V11V22Mo1Mo1::-\n";
    fputs(buffer, fileStream[1]);
    fclose(fileStream[1]);
    EXPECT_EQ(E_OK, player_read_path(fileStream[0], 2, &path));
}

TEST_F(PlayerASuite, test_read_path) {
    const char buffer[] = "7;::-Mo1V11V22Mo1Mo1::-\n";
    fputs(buffer, fileStream[1]);
    fclose(fileStream[1]);
    EXPECT_EQ(E_OK, player_read_path(fileStream[0], 2, &path));
    EXPECT_EQ(7, path.siteCount);
    EXPECT_EQ(25, path.bufferLength);
    EXPECT_EQ(BARRIER, path.sites[0].type);
    EXPECT_EQ(MO, path.sites[1].type);
    EXPECT_EQ(V1, path.sites[2].type);
    EXPECT_EQ(V2, path.sites[3].type);
    EXPECT_EQ(MO, path.sites[4].type);
    EXPECT_EQ(MO, path.sites[5].type);
    EXPECT_EQ(BARRIER, path.sites[6].type);
    EXPECT_EQ(1, path.sites[1].capacity);
    EXPECT_EQ(1, path.sites[2].capacity);
    EXPECT_EQ(2, path.sites[3].capacity);
    EXPECT_EQ(1, path.sites[4].capacity);
    EXPECT_EQ(1, path.sites[5].capacity);
}

TEST_F(PlayerASuite, test_read_path_wrong_beginning) {
    const char buffer[] = "X7;::-Mo1V11V22Mo1Mo1::-\n";
    fputs(buffer, fileStream[1]);
    fclose(fileStream[1]);
    EXPECT_EQ(E_INVALID_PATH, player_read_path(fileStream[0], 2, &path));
}

TEST_F(PlayerASuite, test_read_path_no_beginning_barrier) {
    const char buffer[] = "7;:-Mo1V11V22Mo1Mo1::-\n";
    fputs(buffer, fileStream[1]);
    fclose(fileStream[1]);
    EXPECT_EQ(E_INVALID_PATH, player_read_path(fileStream[0], 2, &path));
}

TEST_F(PlayerASuite, test_read_path_no_barrier) {
    const char buffer[] = "7;::-Mo1V11V22Mo1Mo1:-\n";
    fputs(buffer, fileStream[1]);
    fclose(fileStream[1]);
    EXPECT_EQ(E_INVALID_PATH, player_read_path(fileStream[0], 2, &path));
}

//TEST_F(PlayerASuite, test_read_path_missing_separator) {
    //const char buffer[] = "7::-Mo1V11V22Mo1Mo1::-\n";
    //fputs(buffer, fileStream[1]);
    //fclose(fileStream[1]);
    //EXPECT_EQ(E_INVALID_PATH, player_read_path(fileStream[0], 2, &path));
//}

//TEST_F(PlayerASuite, test_read_path_wrong_site) {
    //const char buffer[] = "7::-Mo1V11V22MoMo1::-\n";
    //fputs(buffer, fileStream[1]);
    //fclose(fileStream[1]);
    //EXPECT_EQ(E_INVALID_PATH, player_read_path(fileStream[0], 2, &path));
//}

TEST_F(PlayerASuite, test_read_path_wrong_buffer_length) {
    const char buffer[] = "77::-Mo1V11V22Mo1Mo1::-\n";
    fputs(buffer, fileStream[1]);
    fclose(fileStream[1]);
    EXPECT_EQ(E_INVALID_PATH, player_read_path(fileStream[0], 2, &path));
}

TEST_F(PlayerASuite, test_calc_path_length) {
    /* "7;::-Mo1V11V22Mo1Mo1::-\n"*/
    EXPECT_EQ(25, calculate_path_length(2, 7));
}

TEST_F(PlayerASuite, test_calc_path_length_many_players) {
    /* "3;::-Mo999::-\n"*/
    EXPECT_EQ(19, calculate_path_length(999, 3));
}

TEST_F(PlayerASuite, test_calc_path_length_many_sites) {
    /* "10000;::-Mo9..V19::-\n"*/
    EXPECT_EQ(30008, calculate_path_length(9, 10000));
}

TEST_F(PlayerASuite, test_convert_site_type) {
    EXPECT_EQ(MO, convert_site_type("Mo"));
    EXPECT_EQ(V1, convert_site_type("V1"));
    EXPECT_EQ(V2, convert_site_type("V2"));
    EXPECT_EQ(DO, convert_site_type("Do"));
    EXPECT_EQ(RI, convert_site_type("Ri"));
    EXPECT_EQ(BARRIER, convert_site_type("::"));
}

