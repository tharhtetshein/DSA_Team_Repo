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

static void printGameShort(const Game* g, const GameService& gs, bool isAdmin)
{
    if (g == nullptr)
    {
        return;
    }

    // Admin sees ID and copies; members do not.
    if (isAdmin)
    {
        std::cout << "ID: " << g->gameID << " | " << g->title
            << " | Players: " << g->minPlayers << "-" << g->maxPlayers
            << " | Year: " << g->yearPublished
            << " | Copies: " << g->copiesAvailable << "/" << g->copiesTotal
            << " | Avg Rating: " << gs.getAverageRating(g)
            << " | Reviews: " << g->reviewCount << "\n";
    }
    else
    {
        std::cout << g->title
            << " | Players: " << g->minPlayers << "-" << g->maxPlayers
            << " | Year: " << g->yearPublished
            << " | Avg Rating: " << gs.getAverageRating(g)
            << " | Reviews: " << g->reviewCount << "\n";
    }
}

static void printGameDetails(const Game* g, const GameService& gs, bool isAdmin)
{
    // Reuse printGameShort semantics for detailed display
    printGameShort(g, gs, isAdmin);
}

static int selectGameByName(GameService& gs, bool isAdmin = true)
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
        printGameShort(matches[i], gs, isAdmin);
    }

    // Keep existing selection-by-ID behavior.
    int id = readInt("Enter Game ID from the list: ");
    delete[] matches;
    return id;
}

static void showGameDetails(GameService& gs, bool isAdmin)
{
    // Ask for a name query and immediately display the first matched game's details.
    char query[101];
    readLine("Find by name: ", query, sizeof(query));
    if (query[0] == '\0')
    {
        return;
    }

    Game** matches = nullptr;
    int count = gs.findGamesByTitleContains(query, matches);
    if (count <= 0)
    {
        std::cout << "No matching games found.\n";
        return;
    }

    // Show the first match's details (per user's earlier request).
    if (count > 1)
    {
        std::cout << "Multiple matches found. Showing the best / first match:\n";
    }

    Game* g = matches[0];
    delete[] matches;

    std::cout << "Game Details\n";
    printGameDetails(g, gs, isAdmin);
}

static void showGameReviews(GameService& gs, MemberService& ms, bool isAdmin)
{
    // Simplified: ask for name, show first match, display reviews without asking for sort or ID.
    char query[101];
    readLine("Find by name: ", query, sizeof(query));
    if (query[0] == '\0')
    {
        return;
    }

    Game** matches = nullptr;
    int countMatches = gs.findGamesByTitleContains(query, matches);
    if (countMatches <= 0)
    {
        std::cout << "No matching games found.\n";
        return;
    }

    Game* selected = matches[0];
    if (countMatches > 1)
    {
        std::cout << "Multiple matches found. Showing the best / first match:\n";
    }

    std::cout << "Matches:\n";
    // show the first match (consistent with showGameDetails behavior)
    printGameShort(selected, gs, isAdmin);

    int gameId = selected->gameID;
    delete[] matches;

    // Always use no-sorting (natural order) and display reviews immediately
    ReviewSortOption sortOption = REVIEW_SORT_NONE;
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
    // Prompt for a name (per request) and select the first match automatically.
    char query[101];
    readLine("Name of game to review: ", query, sizeof(query));
    if (query[0] == '\0')
    {
        return;
    }

    Game** matches = nullptr;
    int count = gs.findGamesByTitleContains(query, matches);
    if (count <= 0)
    {
        std::cout << "No matching games found.\n";
        return;
    }

    std::cout << "Matches:\n";
    for (int i = 0; i < count; i++)
    {
        printGameShort(matches[i], gs, false);
    }

    // Do not ask for Game ID — use the first match's ID and continue to rating/review.
    Game* selected = matches[0];
    if (count > 1)
    {
        std::cout << "Multiple matches found. Using the first match: " << selected->title
            << " (ID " << selected->gameID << ")\n";
    }
    int gameId = selected->gameID;
    delete[] matches;

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

static void listGamesByPlayers(GameService& gs, bool isAdmin = true)
{
    int players = readInt("Enter number of players: ");
    int sortChoice = readInt("Display list by:\n(1) Year of publication ascending\n(2) Average rating\n");
    SortOption option = SORT_NONE;
    if (sortChoice == 1) option = SORT_YEAR_ASC;
    else if (sortChoice == 2) option = SORT_RATING_DESC;

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
        printGameShort(list[i], gs, isAdmin);
    }
    delete[] list;
}

static void listAllGames(GameService& gs, bool isAdmin = true)
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
        printGameShort(list[i], gs, isAdmin);
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
                    listAllGames(gs, true);
                }
                else if (a == 7)
                {
                    // admin view should show ID and copies
                    showGameDetails(gs, true);
                }
                else if (a == 8)
                {
                    listGamesByPlayers(gs, true);
                }
                else if (a == 9)
                {
                    showGameReviews(gs, ms, true);
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
                        gameId = selectGameByName(gs, false);
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
                    // member view should hide ID and copies
                    showGameDetails(gs, false);
                }
                else if (m == 7)
                {
                    listGamesByPlayers(gs, false);
                }
                else if (m == 8)
                {
                    showGameReviews(gs, ms, false);
                }
            }
        }
    }

    return 0;
}
