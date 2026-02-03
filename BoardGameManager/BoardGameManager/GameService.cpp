#include "GameService.h"
#include <fstream>
#include <cstring>
#include <cctype>

GameService::GameService()
    : nextGameId(1)
{
}

void GameService::updateStatus(Game* game)
{
    if (game == nullptr)
    {
        return;
    }

    if (game->copiesAvailable <= 0)
    {
        game->status = GameStatus::BORROWED;
    }
    else
    {
        game->status = GameStatus::AVAILABLE;
    }
}

int GameService::readCsvLine(char* line, int lineSize, int& outMinPlayers, int& outMaxPlayers, int& outYear)
{
    if (line == nullptr || lineSize <= 0)
    {
        return 0;
    }

    line[lineSize - 1] = '\0';
    char* token = nullptr;
    char* context = nullptr;

    token = strtok_s(line, ",", &context);
    if (token == nullptr)
    {
        return 0;
    }

    int nameLen = (int)strlen(token);
    if (nameLen >= 100)
    {
        nameLen = 100;
    }

    char titleBuffer[101];
    strncpy_s(titleBuffer, sizeof(titleBuffer), token, nameLen);
    titleBuffer[nameLen] = '\0';

    token = strtok_s(nullptr, ",", &context);
    if (token == nullptr)
    {
        return 0;
    }
    outMinPlayers = atoi(token);

    token = strtok_s(nullptr, ",", &context);
    if (token == nullptr)
    {
        return 0;
    }
    outMaxPlayers = atoi(token);

    token = strtok_s(nullptr, ",", &context); // maxplaytime
    if (token == nullptr)
    {
        return 0;
    }

    token = strtok_s(nullptr, ",", &context); // minplaytime
    if (token == nullptr)
    {
        return 0;
    }

    token = strtok_s(nullptr, ",", &context); // yearpublished
    if (token == nullptr)
    {
        return 0;
    }
    outYear = atoi(token);

    Game* existing = games.findGameByTitle(titleBuffer);
    if (existing != nullptr)
    {
        existing->copiesTotal += 1;
        existing->copiesAvailable += 1;
        updateStatus(existing);
        return 1;
    }

    Game g;
    g.gameID = nextGameId++;
    strncpy_s(g.title, sizeof(g.title), titleBuffer, _TRUNCATE);
    g.title[sizeof(g.title) - 1] = '\0';
    g.minPlayers = outMinPlayers;
    g.maxPlayers = outMaxPlayers;
    g.yearPublished = outYear;
    g.copiesTotal = 1;
    g.copiesAvailable = 1;
    g.ratingSum = 0;
    g.ratingCount = 0;
    g.reviewsHead = nullptr;
    g.reviewCount = 0;
    g.status = GameStatus::AVAILABLE;

    games.insertGame(g);
    return 1;
}

bool GameService::loadFromCsv(const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }

    std::ifstream in(path);
    if (!in.is_open())
    {
        return false;
    }

    char line[512];
    if (!in.getline(line, sizeof(line)))
    {
        return false;
    }

    while (in.getline(line, sizeof(line)))
    {
        int minP = 0;
        int maxP = 0;
        int year = 0;
        readCsvLine(line, sizeof(line), minP, maxP, year);
    }

    in.close();
    return true;
}

int GameService::getGameCount()
{
    return games.getCount();
}

Game* GameService::findById(int gameId)
{
    return games.findGame(gameId);
}

Game* GameService::findByTitleExact(const char* title)
{
    return games.findGameByTitle(title);
}

bool GameService::addNewGame(const char* title, int minPlayers, int maxPlayers, int year, int copies)
{
    if (title == nullptr || title[0] == '\0' || minPlayers <= 0 || maxPlayers < minPlayers || year <= 0 || copies <= 0)
    {
        return false;
    }

    Game* existing = games.findGameByTitle(title);
    if (existing != nullptr)
    {
        existing->copiesTotal += copies;
        existing->copiesAvailable += copies;
        updateStatus(existing);
        return true;
    }

    Game g;
    g.gameID = nextGameId++;
    strncpy_s(g.title, sizeof(g.title), title, _TRUNCATE);
    g.title[sizeof(g.title) - 1] = '\0';
    g.minPlayers = minPlayers;
    g.maxPlayers = maxPlayers;
    g.yearPublished = year;
    g.copiesTotal = copies;
    g.copiesAvailable = copies;
    g.ratingSum = 0;
    g.ratingCount = 0;
    g.reviewsHead = nullptr;
    g.reviewCount = 0;
    g.status = GameStatus::AVAILABLE;

    return games.insertGame(g);
}

// ✅ kept from raybranch because main.cpp uses it to load from games.txt
bool GameService::addNewGameWithId(int gameId, const char* title, int minPlayers, int maxPlayers, int year, int copies)
{
    if (gameId <= 0 || title == nullptr || title[0] == '\0' || minPlayers <= 0 || maxPlayers < minPlayers || year <= 0 || copies <= 0)
    {
        return false;
    }

    Game* byId = games.findGame(gameId);
    if (byId != nullptr)
    {
        Game* byTitle = games.findGameByTitle(title);
        if (byTitle != nullptr && byTitle != byId)
        {
            return false;
        }
        byId->copiesTotal += copies;
        byId->copiesAvailable += copies;
        updateStatus(byId);
        return true;
    }

    Game* existing = games.findGameByTitle(title);
    if (existing != nullptr)
    {
        existing->copiesTotal += copies;
        existing->copiesAvailable += copies;
        updateStatus(existing);
        return true;
    }

    Game g;
    g.gameID = gameId;
    strncpy_s(g.title, sizeof(g.title), title, _TRUNCATE);
    g.title[sizeof(g.title) - 1] = '\0';
    g.minPlayers = minPlayers;
    g.maxPlayers = maxPlayers;
    g.yearPublished = year;
    g.copiesTotal = copies;
    g.copiesAvailable = copies;
    g.ratingSum = 0;
    g.ratingCount = 0;
    g.reviewsHead = nullptr;
    g.reviewCount = 0;
    g.status = GameStatus::AVAILABLE;

    if (!games.insertGame(g))
    {
        return false;
    }

    if (gameId >= nextGameId)
    {
        nextGameId = gameId + 1;
    }

    return true;
}

bool GameService::removeGame(int gameId)
{
    Game* game = games.findGame(gameId);
    if (game == nullptr)
    {
        return false;
    }

    if (game->copiesAvailable < game->copiesTotal)
    {
        return false; // borrowed copies exist
    }

    return games.removeGame(gameId);
}

bool GameService::addCopies(int gameId, int copies)
{
    if (copies <= 0)
    {
        return false;
    }

    Game* game = games.findGame(gameId);
    if (game == nullptr)
    {
        return false;
    }

    game->copiesTotal += copies;
    game->copiesAvailable += copies;
    updateStatus(game);
    return true;
}

bool GameService::rateGame(int memberId, int gameId, int rating)
{
    if (memberId <= 0 || rating < 1 || rating > 10)
    {
        return false;
    }

    Game* game = games.findGame(gameId);
    if (game == nullptr)
    {
        return false;
    }

    ReviewNode* cur = game->reviewsHead;
    while (cur != nullptr)
    {
        if (cur->memberId == memberId)
        {
            if (cur->rating != rating)
            {
                game->ratingSum -= cur->rating;
                cur->rating = rating;
                game->ratingSum += rating;
            }
            return true;
        }
        cur = cur->next;
    }

    ReviewNode* node = new ReviewNode();
    node->memberId = memberId;
    node->rating = rating;
    node->next = game->reviewsHead;
    game->reviewsHead = node;
    game->reviewCount += 1;
    game->ratingSum += rating;
    game->ratingCount += 1;
    return true;
}

bool GameService::addReview(int memberId, int gameId, int rating, const char* text)
{
    if (memberId <= 0 || rating < 1 || rating > 10)
    {
        return false;
    }

    Game* game = games.findGame(gameId);
    if (game == nullptr)
    {
        return false;
    }

    ReviewNode* cur = game->reviewsHead;
    while (cur != nullptr)
    {
        if (cur->memberId == memberId)
        {
            return false;
        }
        cur = cur->next;
    }

    ReviewNode* node = new ReviewNode();
    node->memberId = memberId;
    node->rating = rating;
    if (text != nullptr)
    {
        strncpy_s(node->text, sizeof(node->text), text, _TRUNCATE);
        node->text[sizeof(node->text) - 1] = '\0';
    }
    node->next = game->reviewsHead;
    game->reviewsHead = node;
    game->reviewCount += 1;

    game->ratingSum += rating;
    game->ratingCount += 1;
    return true;
}

double GameService::getAverageRating(const Game* game) const
{
    if (game == nullptr || game->ratingCount == 0)
    {
        return 0.0;
    }
    return (double)game->ratingSum / (double)game->ratingCount;
}

bool GameService::titleContains(const char* haystack, const char* needle) const
{
    if (haystack == nullptr || needle == nullptr)
    {
        return false;
    }

    int hayLen = (int)strlen(haystack);
    int needleLen = (int)strlen(needle);
    if (needleLen == 0 || hayLen < needleLen)
    {
        return false;
    }

    for (int i = 0; i <= hayLen - needleLen; i++)
    {
        bool match = true;
        for (int j = 0; j < needleLen; j++)
        {
            char hc = haystack[i + j];
            char nc = needle[j];

            if (hc >= 'A' && hc <= 'Z') hc = (char)(hc - 'A' + 'a');
            if (nc >= 'A' && nc <= 'Z') nc = (char)(nc - 'A' + 'a');

            if (hc != nc)
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            return true;
        }
    }
    return false;
}

int GameService::findGamesByTitleContains(const char* titlePart, Game**& outList)
{
    outList = nullptr;
    if (titlePart == nullptr || titlePart[0] == '\0')
    {
        return 0;
    }

    Game** all = nullptr;
    int total = games.toArray(all);
    if (total <= 0 || all == nullptr)
    {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < total; i++)
    {
        if (titleContains(all[i]->title, titlePart))
        {
            count++;
        }
    }

    if (count == 0)
    {
        delete[] all;
        return 0;
    }

    outList = new Game * [count];
    int idx = 0;
    for (int i = 0; i < total; i++)
    {
        if (titleContains(all[i]->title, titlePart))
        {
            outList[idx++] = all[i];
        }
    }

    delete[] all;
    return count;
}

void GameService::sortGames(Game** list, int count, SortOption option)
{
    if (list == nullptr || count <= 1 || option == SORT_NONE)
    {
        return;
    }

    for (int i = 0; i < count - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < count; j++)
        {
            bool swapNeeded = false;
            if (option == SORT_YEAR_ASC)
            {
                if (list[j]->yearPublished < list[best]->yearPublished)
                {
                    swapNeeded = true;
                }
            }
            else if (option == SORT_RATING_DESC)
            {
                double rj = getAverageRating(list[j]);
                double rb = getAverageRating(list[best]);
                if (rj > rb)
                {
                    swapNeeded = true;
                }
            }

            if (swapNeeded)
            {
                best = j;
            }
        }

        if (best != i)
        {
            Game* tmp = list[i];
            list[i] = list[best];
            list[best] = tmp;
        }
    }
}

void GameService::sortReviews(ReviewNode** list, int count, ReviewSortOption option) const
{
    if (list == nullptr || count <= 1 || option == REVIEW_SORT_NONE)
    {
        return;
    }

    for (int i = 0; i < count - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < count; j++)
        {
            bool swapNeeded = false;
            if (option == REVIEW_SORT_RATING_DESC)
            {
                if (list[j]->rating > list[best]->rating)
                {
                    swapNeeded = true;
                }
            }
            else if (option == REVIEW_SORT_MEMBER_ASC)
            {
                if (list[j]->memberId < list[best]->memberId)
                {
                    swapNeeded = true;
                }
            }

            if (swapNeeded)
            {
                best = j;
            }
        }

        if (best != i)
        {
            ReviewNode* tmp = list[i];
            list[i] = list[best];
            list[best] = tmp;
        }
    }
}

int GameService::listGamesByPlayers(int players, SortOption option, Game**& outList)
{
    outList = nullptr;
    if (players <= 0)
    {
        return 0;
    }

    Game** all = nullptr;
    int total = games.toArray(all);
    if (total <= 0 || all == nullptr)
    {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < total; i++)
    {
        Game* g = all[i];
        if (players >= g->minPlayers && players <= g->maxPlayers)
        {
            count++;
        }
    }

    if (count == 0)
    {
        delete[] all;
        return 0;
    }

    outList = new Game * [count];
    int idx = 0;
    for (int i = 0; i < total; i++)
    {
        Game* g = all[i];
        if (players >= g->minPlayers && players <= g->maxPlayers)
        {
            outList[idx++] = g;
        }
    }

    delete[] all;
    sortGames(outList, count, option);
    return count;
}

int GameService::getAllGames(Game**& outList)
{
    return games.toArray(outList);
}

int GameService::getReviewsForGame(int gameId, ReviewNode**& outList, ReviewSortOption option)
{
    outList = nullptr;
    if (gameId <= 0)
    {
        return 0;
    }

    Game* game = games.findGame(gameId);
    if (game == nullptr || game->reviewCount <= 0)
    {
        return 0;
    }

    outList = new ReviewNode * [game->reviewCount];
    int idx = 0;
    ReviewNode* cur = game->reviewsHead;
    while (cur != nullptr && idx < game->reviewCount)
    {
        outList[idx++] = cur;
        cur = cur->next;
    }

    sortReviews(outList, idx, option);
    return idx;
}
