#ifndef SERVICES_GAMESERVICE_H
#define SERVICES_GAMESERVICE_H

#include "HashTable.h"

enum SortOption
{
    SORT_NONE = 0,
    SORT_YEAR_ASC = 1,
    SORT_RATING_DESC = 2
};

enum ReviewSortOption
{
    REVIEW_SORT_NONE = 0,
    REVIEW_SORT_RATING_DESC = 1,
    REVIEW_SORT_MEMBER_ASC = 2
};

class GameService
{
private:
    HashTable games;
    int nextGameId;

    void updateStatus(Game* game);
    int readCsvLine(char* line, int lineSize, int& outMinPlayers, int& outMaxPlayers, int& outYear);
    bool titleContains(const char* haystack, const char* needle) const;
    void sortGames(Game** list, int count, SortOption option);
    void sortReviews(ReviewNode** list, int count, ReviewSortOption option) const;

public:
    GameService();

    bool loadFromCsv(const char* path);
    int getGameCount();

    Game* findById(int gameId);
    Game* findByTitleExact(const char* title);

    bool addNewGame(const char* title, int minPlayers, int maxPlayers, int year, int copies);
    bool removeGame(int gameId);

    bool addCopies(int gameId, int copies);

    bool rateGame(int memberId, int gameId, int rating);
    bool addReview(int memberId, int gameId, int rating, const char* text);
    double getAverageRating(const Game* game) const;

    int listGamesByPlayers(int players, SortOption option, Game**& outList);
    int findGamesByTitleContains(const char* titlePart, Game**& outList);
    int getAllGames(Game**& outList);
    int getReviewsForGame(int gameId, ReviewNode**& outList, ReviewSortOption option);
};

#endif
