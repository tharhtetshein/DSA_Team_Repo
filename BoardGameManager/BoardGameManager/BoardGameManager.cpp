#include <iostream>
#include <cstring>
#include <direct.h>
#include <io.h>
#include "MemberService.h"
#include "TransactionService.h"
#include "AdminService.h"
#include "GameService.h"

static void clearLine()
{
    std::cin.clear();
    while (std::cin.get() != '\n')
    {
        if (std::cin.eof())
        {
            break;
        }
    }
}

static int readInt(const char* prompt)
{
    int value = 0;
    while (true)
    {
        std::cout << prompt;
        if (std::cin >> value)
        {
            clearLine();
            return value;
        }
        clearLine();
        std::cout << "Invalid input. Please enter a number.\n";
    }
}

static void readLine(const char* prompt, char* buffer, int size)
{
    if (buffer == nullptr || size <= 0)
    {
        return;
    }

    std::cout << prompt;
    std::cin.getline(buffer, size);
    if (std::cin.fail())
    {
        clearLine();
        buffer[0] = '\0';
    }
}

static bool fileExists(const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }
    return _access(path, 0) == 0;
}

static void joinPath(char* out, int size, const char* base, const char* file)
{
    if (out == nullptr || size <= 0 || base == nullptr || file == nullptr)
    {
        return;
    }

    strncpy_s(out, size, base, _TRUNCATE);
    out[size - 1] = '\0';
    int len = (int)strlen(out);
    if (len > 0 && out[len - 1] != '\\' && out[len - 1] != '/')
    {
        strcat_s(out, size, "\\");
    }
    strcat_s(out, size, file);
}

static bool tryLoadCsv(GameService& gs, const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }

    if (!fileExists(path))
    {
        return false;
    }

    if (gs.loadFromCsv(path))
    {
        std::cout << "Loaded games from: " << path << "\n";
        return true;
    }
    return false;
}

static bool tryLoadCsvFromParents(GameService& gs)
{
    char cwd[260];
    if (_getcwd(cwd, sizeof(cwd)) == nullptr)
    {
        return false;
    }

    char cur[260];
    strncpy_s(cur, sizeof(cur), cwd, _TRUNCATE);
    cur[sizeof(cur) - 1] = '\0';

    for (int depth = 0; depth < 8; depth++)
    {
        char candidate[260];
        joinPath(candidate, sizeof(candidate), cur, "games.csv");
        if (tryLoadCsv(gs, candidate))
        {
            return true;
        }

        char* lastSep = strrchr(cur, '\\');
        if (lastSep == nullptr)
        {
            break;
        }
        *lastSep = '\0';
        if (cur[0] == '\0')
        {
            break;
        }
    }

    std::cout << "CSV not found. Current working directory: " << cwd << "\n";
    return false;
}

static void printGameShort(const Game* g, const GameService& gs)
{
    if (g == nullptr)
    {
        return;
    }

    std::cout << "ID: " << g->gameID << " | " << g->title
        << " | Players: " << g->minPlayers << "-" << g->maxPlayers
        << " | Year: " << g->yearPublished
        << " | Copies: " << g->copiesAvailable << "/" << g->copiesTotal
        << " | Avg Rating: " << gs.getAverageRating(g)
        << " | Reviews: " << g->reviewCount << "\n";
}

static int selectGameByName(GameService& gs)
{
    char query[101];
    readLine("Enter part of the game name: ", query, sizeof(query));
    if (query[0] == '\0')
    {
        return -1;
    }

    Game** matches = nullptr;
    int count = gs.findGamesByTitleContains(query, matches);
    if (count <= 0)
    {
        std::cout << "No matching games found.\n";
        return -1;
    }

    std::cout << "Matches:\n";
    for (int i = 0; i < count; i++)
    {
        printGameShort(matches[i], gs);
    }

    int id = readInt("Enter Game ID from the list: ");
    delete[] matches;
    return id;
}

static void showGameDetails(GameService& gs)
{
    int choice = readInt("Find by (1) ID or (2) name: ");
    int gameId = -1;

    if (choice == 1)
    {
        gameId = readInt("Enter Game ID: ");
    }
    else if (choice == 2)
    {
        gameId = selectGameByName(gs);
    }

    if (gameId <= 0)
    {
        return;
    }

    Game* g = gs.findById(gameId);
    if (g == nullptr)
    {
        std::cout << "Game not found.\n";
        return;
    }

    std::cout << "Game Details\n";
    printGameShort(g, gs);
}

static void showGameReviews(GameService& gs, MemberService& ms)
{
    int choice = readInt("Find by (1) ID or (2) name: ");
    int gameId = -1;

    if (choice == 1)
    {
        gameId = readInt("Enter Game ID: ");
    }
    else if (choice == 2)
    {
        gameId = selectGameByName(gs);
    }

    if (gameId <= 0)
    {
        return;
    }

    ReviewSortOption sortOption = REVIEW_SORT_NONE;
    int sortChoice = readInt("Sort reviews by (1) Rating desc, (2) Member ID asc, (0) None: ");
    if (sortChoice == 1)
    {
        sortOption = REVIEW_SORT_RATING_DESC;
    }
    else if (sortChoice == 2)
    {
        sortOption = REVIEW_SORT_MEMBER_ASC;
    }

    ReviewNode** list = nullptr;
    int count = gs.getReviewsForGame(gameId, list, sortOption);
    if (count <= 0)
    {
        std::cout << "No reviews for this game.\n";
        return;
    }

    std::cout << "Reviews:\n";
    for (int i = 0; i < count; i++)
    {
        ReviewNode* r = list[i];
        Member* m = ms.getMemberById(r->memberId);
        if (m != nullptr)
        {
            std::cout << "Member: " << m->name << " (ID " << r->memberId << ")";
        }
        else
        {
            std::cout << "Member ID " << r->memberId;
        }
        std::cout << " | Rating: " << r->rating << "\n";
        if (r->text[0] != '\0')
        {
            std::cout << "Review: " << r->text << "\n";
        }
    }

    delete[] list;
}

static void writeGameReview(GameService& gs, int memberId)
{
    int gameId = readInt("Game ID to review: ");
    int rating = readInt("Rating (1-10): ");
    char text[256];
    readLine("Review text (optional): ", text, sizeof(text));

    if (gs.addReview(memberId, gameId, rating, text))
    {
        std::cout << "Review recorded.\n";
    }
    else
    {
        std::cout << "Failed to add review. You may have already reviewed this game.\n";
    }
}

static void listGamesByPlayers(GameService& gs)
{
    int players = readInt("Enter number of players: ");
    int sortChoice = readInt("Sort by (1) Year asc, (2) Avg rating desc, (0) None: ");
    SortOption option = SORT_NONE;
    if (sortChoice == 1) option = SORT_YEAR_ASC;
    if (sortChoice == 2) option = SORT_RATING_DESC;

    Game** list = nullptr;
    int count = gs.listGamesByPlayers(players, option, list);
    if (count <= 0)
    {
        std::cout << "No games support " << players << " players.\n";
        return;
    }

    std::cout << "Games for " << players << " players:\n";
    for (int i = 0; i < count; i++)
    {
        printGameShort(list[i], gs);
    }
    delete[] list;
}

static void listAllGames(GameService& gs)
{
    Game** list = nullptr;
    int count = gs.getAllGames(list);
    if (count <= 0)
    {
        std::cout << "No games loaded.\n";
        return;
    }

    for (int i = 0; i < count; i++)
    {
        printGameShort(list[i], gs);
    }
    delete[] list;
}

int main()
{
    GameService gs;
    MemberService ms;
    TransactionService ts(&ms, &gs);
    AdminService admin(&gs);

    char csvPath[260];
    strncpy_s(csvPath, sizeof(csvPath), "games.csv", _TRUNCATE);

    if (!tryLoadCsv(gs, csvPath))
    {
        bool loaded = tryLoadCsvFromParents(gs);
        if (!loaded)
        {
            readLine("Enter CSV path (or leave blank to skip): ", csvPath, sizeof(csvPath));
            if (csvPath[0] != '\0')
            {
                if (!tryLoadCsv(gs, csvPath))
                {
                    std::cout << "Failed to load CSV.\n";
                }
            }
        }
    }

    bool running = true;
    while (running)
    {
        std::cout << "\n=== NPTTGC Board Game Manager ===\n";
        std::cout << "1. Admin\n";
        std::cout << "2. Member\n";
        std::cout << "0. Exit\n";

        int choice = readInt("Select: ");
        if (choice == 0)
        {
            running = false;
        }
        else if (choice == 1)
        {
            bool adminMenu = true;
            while (adminMenu)
            {
                std::cout << "\n--- Admin Menu ---\n";
                std::cout << "1. Add new game\n";
                std::cout << "2. Remove game\n";
                std::cout << "3. Add member\n";
                std::cout << "4. Borrow/Return summary (total)\n";
                std::cout << "5. Borrow/Return summary by game\n";
                std::cout << "6. List all games\n";
                std::cout << "7. View game details\n";
                std::cout << "8. List games by players\n";
                std::cout << "9. View game reviews\n";
                std::cout << "0. Back\n";

                int a = readInt("Select: ");
                if (a == 0)
                {
                    adminMenu = false;
                }
                else if (a == 1)
                {
                    char title[101];
                    readLine("Title: ", title, sizeof(title));
                    int minP = readInt("Min players: ");
                    int maxP = readInt("Max players: ");
                    int year = readInt("Year published: ");
                    int copies = readInt("Copies: ");

                    if (admin.addGame(title, minP, maxP, year, copies))
                    {
                        std::cout << "Game added.\n";
                    }
                    else
                    {
                        std::cout << "Failed to add game.\n";
                    }
                }
                else if (a == 2)
                {
                    int id = readInt("Game ID to remove: ");
                    if (admin.removeGame(id))
                    {
                        std::cout << "Game removed.\n";
                    }
                    else
                    {
                        std::cout << "Failed to remove. Check if copies are borrowed.\n";
                    }
                }
                else if (a == 3)
                {
                    int memberId = readInt("Member ID: ");
                    char name[60];
                    readLine("Member name: ", name, sizeof(name));
                    Status s = ms.addMember(memberId, name);
                    if (s == OK)
                    {
                        std::cout << "Member added.\n";
                    }
                    else
                    {
                        std::cout << "Failed to add member.\n";
                    }
                }
                else if (a == 4)
                {
                    ts.adminBorrowReturnSummary();
                }
                else if (a == 5)
                {
                    ts.adminBorrowReturnSummaryByGame();
                }
                else if (a == 6)
                {
                    listAllGames(gs);
                }
                else if (a == 7)
                {
                    showGameDetails(gs);
                }
                else if (a == 8)
                {
                    listGamesByPlayers(gs);
                }
                else if (a == 9)
                {
                    showGameReviews(gs, ms);
                }
            }
        }
        else if (choice == 2)
        {
            int memberId = readInt("Enter your member ID: ");
            Member* member = ms.getMemberById(memberId);
            if (member == nullptr)
            {
                std::cout << "Member not found. Ask admin to register you.\n";
                continue;
            }

            bool memberMenu = true;
            while (memberMenu)
            {
                std::cout << "\n--- Member Menu ---\n";
                std::cout << "1. Borrow game\n";
                std::cout << "2. Return game\n";
                std::cout << "3. My borrow/return summary\n";
                std::cout << "4. Rate game\n";
                std::cout << "5. Write review\n";
                std::cout << "6. View game details\n";
                std::cout << "7. List games by players\n";
                std::cout << "8. View game reviews\n";
                std::cout << "0. Back\n";

                int m = readInt("Select: ");
                if (m == 0)
                {
                    memberMenu = false;
                }
                else if (m == 1 || m == 2)
                {
                    int mode = readInt("Find by (1) ID or (2) name: ");
                    int gameId = -1;
                    if (mode == 1)
                    {
                        gameId = readInt("Enter Game ID: ");
                    }
                    else if (mode == 2)
                    {
                        gameId = selectGameByName(gs);
                    }

                    if (gameId <= 0)
                    {
                        continue;
                    }

                    Status result = (m == 1) ? ts.borrowGame(memberId, gameId) : ts.returnGame(memberId, gameId);
                    if (result == OK)
                    {
                        std::cout << (m == 1 ? "Borrowed.\n" : "Returned.\n");
                    }
                    else if (result == NOT_FOUND)
                    {
                        std::cout << "Game not found.\n";
                    }
                    else if (result == NOT_AVAILABLE)
                    {
                        std::cout << "No available copies.\n";
                    }
                    else if (result == NOT_BORROWED)
                    {
                        std::cout << "You have not borrowed this game.\n";
                    }
                    else if (result == ALREADY_EXISTS)
                    {
                        std::cout << "You already borrowed this game.\n";
                    }
                    else
                    {
                        std::cout << "Action failed.\n";
                    }
                }
                else if (m == 3)
                {
                    ts.memberBorrowReturnSummary(memberId);
                }
                else if (m == 4)
                {
                    int gameId = readInt("Game ID to rate: ");
                    int rating = readInt("Rating (1-10): ");
                    if (gs.rateGame(memberId, gameId, rating))
                    {
                        std::cout << "Rating recorded.\n";
                    }
                    else
                    {
                        std::cout << "Failed to rate game.\n";
                    }
                }
                else if (m == 5)
                {
                    writeGameReview(gs, memberId);
                }
                else if (m == 6)
                {
                    showGameDetails(gs);
                }
                else if (m == 7)
                {
                    listGamesByPlayers(gs);
                }
                else if (m == 8)
                {
                    showGameReviews(gs, ms);
                }
            }
        }
    }

    return 0;
}
