#include <iostream>
#include <cstring>
#include <direct.h>
#include <io.h>
#include <fstream>
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <cstdio>
#include <ctime>

#include "MemberService.h"
#include "TransactionService.h"
#include "AdminService.h"
#include "GameService.h"

struct PlayRecord
{
    int gameId;
    char gameTitle[101];
    char participants[256];
    char winners[256];
    char playedAt[20];
    PlayRecord* next;

    PlayRecord()
        : gameId(0), next(nullptr)
    {
        gameTitle[0] = '\0';
        participants[0] = '\0';
        winners[0] = '\0';
        playedAt[0] = '\0';
    }
};

static PlayRecord* g_playHead = nullptr;
static PlayRecord* g_playTail = nullptr;

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

static bool hasBorrowedGame(const Member* member, int gameId)
{
    if (member == nullptr)
    {
        return false;
    }

    const BorrowedNode* cur = member->borrowedHead;
    while (cur != nullptr)
    {
        if (cur->gameId == gameId)
        {
            return true;
        }
        cur = cur->next;
    }
    return false;
}

static void addBorrowedGame(Member* member, int gameId)
{
    if (member == nullptr)
    {
        return;
    }

    BorrowedNode* node = new BorrowedNode();
    node->gameId = gameId;
    node->next = member->borrowedHead;
    member->borrowedHead = node;
}

static void sanitizeForSave(const char* input, char* output, int size)
{
    if (output == nullptr || size <= 0)
    {
        return;
    }

    if (input == nullptr)
    {
        output[0] = '\0';
        return;
    }

    int out = 0;
    for (int i = 0; input[i] != '\0' && out < size - 1; i++)
    {
        char c = input[i];
        if (c == '|')
        {
            c = '/';
        }
        else if (c == '\n' || c == '\r')
        {
            c = ' ';
        }
        output[out++] = c;
    }
    output[out] = '\0';
}

static bool tryParseInt(const char* s, int& out)
{
    if (s == nullptr || s[0] == '\0')
    {
        return false;
    }

    char* end = nullptr;
    long value = strtol(s, &end, 10);
    if (end == s || *end != '\0')
    {
        return false;
    }

    out = (int)value;
    return true;
}

static bool equalsIgnoreCase(const char* a, const char* b)
{
    if (a == nullptr || b == nullptr)
    {
        return false;
    }

    int i = 0;
    while (a[i] != '\0' && b[i] != '\0')
    {
        char ca = (char)std::tolower((unsigned char)a[i]);
        char cb = (char)std::tolower((unsigned char)b[i]);
        if (ca != cb)
        {
            return false;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static Game* findGameByTitleInsensitive(GameService& gs, const char* title)
{
    if (title == nullptr || title[0] == '\0')
    {
        return nullptr;
    }

    Game* exact = gs.findByTitleExact(title);
    if (exact != nullptr)
    {
        return exact;
    }

    Game** all = nullptr;
    int total = gs.getAllGames(all);
    if (total <= 0 || all == nullptr)
    {
        return nullptr;
    }

    Game* found = nullptr;
    for (int i = 0; i < total; i++)
    {
        if (equalsIgnoreCase(all[i]->title, title))
        {
            found = all[i];
            break;
        }
    }

    delete[] all;
    return found;
}

static bool containsId(const int* ids, int count, int value)
{
    if (ids == nullptr || count <= 0)
    {
        return false;
    }
    for (int i = 0; i < count; i++)
    {
        if (ids[i] == value)
        {
            return true;
        }
    }
    return false;
}

static void printIdList(const int* ids, int count)
{
    if (ids == nullptr || count <= 0)
    {
        std::cout << "None";
        return;
    }

    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            std::cout << ", ";
        }
        std::cout << ids[i];
    }
}

static void printMemberListWithNames(const int* ids, int count, MemberService& ms)
{
    if (ids == nullptr || count <= 0)
    {
        std::cout << "None";
        return;
    }

    for (int i = 0; i < count; i++)
    {
        if (i > 0)
        {
            std::cout << ", ";
        }
        Member* m = ms.getMemberById(ids[i]);
        if (m != nullptr && m->name[0] != '\0')
        {
            std::cout << ids[i] << " - " << m->name;
        }
        else
        {
            std::cout << ids[i];
        }
    }
}

static void printMemberDirectory(MemberService& ms)
{
    MemberList& list = ms.getMemberList();
    MemberNode* cur = list.getHead();
    if (cur == nullptr)
    {
        std::cout << "No members registered.\n";
        return;
    }

    std::cout << "Members:\n";
    while (cur != nullptr)
    {
        std::cout << "ID " << cur->data.memberId << " - " << cur->data.name << "\n";
        cur = cur->next;
    }
}

static int parseIdList(const char* list, int* outIds, int maxIds)
{
    if (list == nullptr || outIds == nullptr || maxIds <= 0)
    {
        return 0;
    }

    char buf[256];
    strncpy_s(buf, sizeof(buf), list, _TRUNCATE);
    buf[sizeof(buf) - 1] = '\0';

    int count = 0;
    char* context = nullptr;
    char* token = strtok_s(buf, ",", &context);
    while (token != nullptr && count < maxIds)
    {
        while (*token == ' ' || *token == '\t')
        {
            token++;
        }
        char* end = token + strlen(token);
        while (end > token && (end[-1] == ' ' || end[-1] == '\t'))
        {
            end--;
        }
        *end = '\0';

        int id = 0;
        if (tryParseInt(token, id) && id > 0)
        {
            if (!containsId(outIds, count, id))
            {
                outIds[count++] = id;
            }
        }
        token = strtok_s(nullptr, ",", &context);
    }
    return count;
}

static void buildIdListString(const int* ids, int count, char* out, int size)
{
    if (out == nullptr || size <= 0)
    {
        return;
    }
    out[0] = '\0';

    for (int i = 0; i < count; i++)
    {
        char buf[16];
        sprintf_s(buf, sizeof(buf), "%d", ids[i]);
        if (i > 0)
        {
            strcat_s(out, size, ",");
        }
        strcat_s(out, size, buf);
    }
}

static void resetMemberTotals(MemberService& ms)
{
    MemberList& list = ms.getMemberList();
    MemberNode* cur = list.getHead();
    while (cur != nullptr)
    {
        cur->data.totalPlays = 0;
        cur->data.totalWins = 0;
        cur = cur->next;
    }
}

static void getCurrentTimestamp(char* out, int size)
{
    if (out == nullptr || size <= 0)
    {
        return;
    }

    std::time_t now = std::time(nullptr);
    std::tm localTime;
    if (localtime_s(&localTime, &now) != 0)
    {
        out[0] = '\0';
        return;
    }

    if (std::strftime(out, size, "%Y-%m-%d %H:%M", &localTime) == 0)
    {
        out[0] = '\0';
    }
}

static void addPlayRecord(int gameId, const char* title, const char* participants, const char* winners, const char* playedAt)
{
    // Ray Feature: Keep play history in a linked list (append at tail).
    PlayRecord* node = new PlayRecord();
    node->gameId = gameId;
    sanitizeForSave(title, node->gameTitle, sizeof(node->gameTitle));
    sanitizeForSave(participants, node->participants, sizeof(node->participants));
    sanitizeForSave(winners, node->winners, sizeof(node->winners));
    sanitizeForSave(playedAt, node->playedAt, sizeof(node->playedAt));

    if (g_playTail == nullptr)
    {
        g_playHead = g_playTail = node;
    }
    else
    {
        g_playTail->next = node;
        g_playTail = node;
    }
}

static void applyPlayStats(MemberService& ms, const char* participants, const char* winners)
{
    // Ray Feature: Update member play/win counters from a recorded match.
    int ids[64];
    int count = parseIdList(participants, ids, 64);
    for (int i = 0; i < count; i++)
    {
        Member* m = ms.getMemberById(ids[i]);
        if (m != nullptr)
        {
            m->totalPlays += 1;
        }
    }

    int winIds[64];
    int winCount = parseIdList(winners, winIds, 64);
    for (int i = 0; i < winCount; i++)
    {
        Member* m = ms.getMemberById(winIds[i]);
        if (m != nullptr)
        {
            m->totalWins += 1;
        }
    }
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

// Tries to find games.txt by walking up. Always sets some output path.
static bool resolveGamesPath(char* outPath, int size)
{
    if (outPath == nullptr || size <= 0)
    {
        return false;
    }

    char cwd[260];
    if (_getcwd(cwd, sizeof(cwd)) == nullptr)
    {
        outPath[0] = '\0';
        return false;
    }

    char cur[260];
    strncpy_s(cur, sizeof(cur), cwd, _TRUNCATE);
    cur[sizeof(cur) - 1] = '\0';

    for (int depth = 0; depth < 8; depth++)
    {
        char candidate[260];
        joinPath(candidate, sizeof(candidate), cur, "games.txt");
        if (fileExists(candidate))
        {
            strncpy_s(outPath, size, candidate, _TRUNCATE);
            outPath[size - 1] = '\0';
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

    // default path
    joinPath(outPath, size, cwd, "games.txt");
    return false;
}

static bool resolveMembersPath(char* outPath, int size)
{
    if (outPath == nullptr || size <= 0)
    {
        return false;
    }

    char cwd[260];
    if (_getcwd(cwd, sizeof(cwd)) == nullptr)
    {
        outPath[0] = '\0';
        return false;
    }

    char cur[260];
    strncpy_s(cur, sizeof(cur), cwd, _TRUNCATE);
    cur[sizeof(cur) - 1] = '\0';

    for (int depth = 0; depth < 8; depth++)
    {
        char candidate[260];
        joinPath(candidate, sizeof(candidate), cur, "members.txt");
        if (fileExists(candidate))
        {
            strncpy_s(outPath, size, candidate, _TRUNCATE);
            outPath[size - 1] = '\0';
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

    joinPath(outPath, size, cwd, "members.txt");
    return false;
}

static bool loadGamesFromText(GameService& gs, const char* path)
{
    if (path == nullptr || path[0] == '\0' || !fileExists(path))
    {
        return false;
    }

    std::ifstream in(path);
    if (!in.is_open())
    {
        return false;
    }

    int loadedCount = 0;
    char line[512];
    while (in.getline(line, sizeof(line)))
    {
        if (line[0] == '\0' || line[0] == '#')
        {
            continue;
        }
        if (strncmp(line, "GAME|", 5) != 0)
        {
            continue;
        }

        char* context = nullptr;
        (void)strtok_s(line, "|", &context); // GAME
        char* idStr = strtok_s(nullptr, "|", &context);
        char* title = strtok_s(nullptr, "|", &context);
        char* minStr = strtok_s(nullptr, "|", &context);
        char* maxStr = strtok_s(nullptr, "|", &context);
        char* yearStr = strtok_s(nullptr, "|", &context);
        char* copiesStr = strtok_s(nullptr, "|", &context);

        if (idStr == nullptr || title == nullptr || minStr == nullptr || maxStr == nullptr || yearStr == nullptr || copiesStr == nullptr)
        {
            continue;
        }

        int gameId = 0;
        int minP = 0;
        int maxP = 0;
        int year = 0;
        int copies = 0;
        if (!tryParseInt(idStr, gameId) || !tryParseInt(minStr, minP) || !tryParseInt(maxStr, maxP) ||
            !tryParseInt(yearStr, year) || !tryParseInt(copiesStr, copies))
        {
            continue;
        }

        if (gs.addNewGameWithId(gameId, title, minP, maxP, year, copies))
        {
            loadedCount++;
        }
    }

    in.close();
    return loadedCount > 0;
}

static void sortGamesById(Game** list, int count)
{
    if (list == nullptr || count <= 1)
    {
        return;
    }

    for (int i = 0; i < count - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < count; j++)
        {
            if (list[j]->gameID < list[best]->gameID)
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

static int compareTitlesIgnoreCase(const char* a, const char* b)
{
    if (a == nullptr && b == nullptr)
    {
        return 0;
    }
    if (a == nullptr)
    {
        return -1;
    }
    if (b == nullptr)
    {
        return 1;
    }

    int i = 0;
    while (a[i] != '\0' && b[i] != '\0')
    {
        char ca = (char)std::tolower((unsigned char)a[i]);
        char cb = (char)std::tolower((unsigned char)b[i]);
        if (ca != cb)
        {
            return (ca < cb) ? -1 : 1;
        }
        i++;
    }

    if (a[i] == '\0' && b[i] == '\0')
    {
        return 0;
    }
    return (a[i] == '\0') ? -1 : 1;
}

static void sortGamesByTitle(Game** list, int count)
{
    if (list == nullptr || count <= 1)
    {
        return;
    }

    for (int i = 0; i < count - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < count; j++)
        {
            int cmp = compareTitlesIgnoreCase(list[j]->title, list[best]->title);
            bool swapNeeded = false;
            if (cmp < 0)
            {
                swapNeeded = true;
            }
            else if (cmp == 0)
            {
                int caseCmp = strcmp(list[j]->title, list[best]->title);
                if (caseCmp < 0)
                {
                    swapNeeded = true;
                }
                else if (caseCmp == 0 && list[j]->gameID < list[best]->gameID)
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

static bool saveGames(GameService& gs, const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    out << "# BoardGameManager games v1\n";

    Game** all = nullptr;
    int total = gs.getAllGames(all);
    if (total > 0 && all != nullptr)
    {
        sortGamesById(all, total);
        for (int i = 0; i < total; i++)
        {
            Game* g = all[i];
            char titleBuf[101];
            sanitizeForSave(g->title, titleBuf, sizeof(titleBuf));
            out << "GAME|" << g->gameID << "|" << titleBuf << "|"
                << g->minPlayers << "|" << g->maxPlayers << "|"
                << g->yearPublished << "|" << g->copiesTotal << "\n";
        }
        delete[] all;
    }

    return true;
}

static void autoSaveGames(GameService& gs, const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return;
    }

    if (!saveGames(gs, path))
    {
        std::cout << "Warning: failed to save games to " << path << "\n";
    }
}

static bool loadMembers(MemberService& ms, GameService& gs, const char* path)
{
    if (path == nullptr || path[0] == '\0' || !fileExists(path))
    {
        return false;
    }

    std::ifstream in(path);
    if (!in.is_open())
    {
        return false;
    }

    char line[600];
    while (in.getline(line, sizeof(line)))
    {
        if (line[0] == '\0' || line[0] == '#')
        {
            continue;
        }
        if (strncmp(line, "MEMBER|", 7) != 0)
        {
            continue;
        }

        char* context = nullptr;
        (void)strtok_s(line, "|", &context); // MEMBER
        char* idStr = strtok_s(nullptr, "|", &context);
        char* name = strtok_s(nullptr, "|", &context);
        char* playsStr = strtok_s(nullptr, "|", &context);
        char* winsStr = strtok_s(nullptr, "|", &context);

        if (idStr == nullptr || name == nullptr)
        {
            continue;
        }

        int memberId = atoi(idStr);
        if (memberId <= 0)
        {
            continue;
        }

        (void)ms.addMember(memberId, name);
        Member* m = ms.getMemberById(memberId);
        if (m != nullptr)
        {
            if (playsStr != nullptr)
            {
                m->totalPlays = atoi(playsStr);
            }
            if (winsStr != nullptr)
            {
                m->totalWins = atoi(winsStr);
            }
        }
    }
    in.close();

    std::ifstream in2(path);
    if (!in2.is_open())
    {
        return true;
    }

    bool resetTotalsOnPlay = false;
    while (in2.getline(line, sizeof(line)))
    {
        if (line[0] == '\0' || line[0] == '#')
        {
            continue;
        }

        if (strncmp(line, "BORROWED|", 9) == 0)
        {
            char* context = nullptr;
            (void)strtok_s(line, "|", &context); // BORROWED
            char* memberStr = strtok_s(nullptr, "|", &context);
            char* gameStr = strtok_s(nullptr, "|", &context);
            char* titleStr = strtok_s(nullptr, "|", &context);

            if (memberStr == nullptr || gameStr == nullptr)
            {
                continue;
            }

            int memberId = 0;
            int gameId = 0;
            if (!tryParseInt(memberStr, memberId) || memberId <= 0)
            {
                continue;
            }
            (void)tryParseInt(gameStr, gameId);

            Member* m = ms.getMemberById(memberId);
            Game* g = nullptr;
            if (gameId > 0)
            {
                g = gs.findById(gameId);
            }
            if (g == nullptr && titleStr != nullptr && titleStr[0] != '\0')
            {
                g = findGameByTitleInsensitive(gs, titleStr);
            }
            if (g != nullptr)
            {
                gameId = g->gameID;
            }
            if (gameId <= 0 || g == nullptr)
            {
                continue;
            }

            if (m != nullptr && !hasBorrowedGame(m, gameId))
            {
                addBorrowedGame(m, gameId);
            }

            if (g != nullptr)
            {
                if (g->copiesAvailable > 0)
                {
                    g->copiesAvailable -= 1;
                }
                if (g->copiesAvailable <= 0)
                {
                    g->status = GameStatus::BORROWED;
                }
            }
        }
        else if (strncmp(line, "REVIEW|", 7) == 0)
        {
            char* context = nullptr;
            (void)strtok_s(line, "|", &context); // REVIEW
            char* memberStr = strtok_s(nullptr, "|", &context);
            char* gameStr = strtok_s(nullptr, "|", &context);
            char* token3 = strtok_s(nullptr, "|", &context);
            char* token4 = strtok_s(nullptr, "|", &context);
            char* token5 = strtok_s(nullptr, "|", &context);

            if (memberStr == nullptr || gameStr == nullptr || token3 == nullptr)
            {
                continue;
            }

            int memberId = 0;
            int gameId = 0;
            if (!tryParseInt(memberStr, memberId) || memberId <= 0)
            {
                continue;
            }

            (void)tryParseInt(gameStr, gameId);

            const char* titleStr = nullptr;
            const char* ratingStr = nullptr;
            const char* textStr = nullptr;

            int rating = 0;
            if (tryParseInt(token3, rating) && rating >= 1 && rating <= 10)
            {
                ratingStr = token3;
                textStr = token4;
            }
            else
            {
                titleStr = token3;
                ratingStr = token4;
                textStr = token5;
                if (ratingStr == nullptr || !tryParseInt(ratingStr, rating) || rating < 1 || rating > 10)
                {
                    continue;
                }
            }

            Game* g = nullptr;
            if (gameId > 0)
            {
                g = gs.findById(gameId);
            }
            if (g == nullptr && titleStr != nullptr && titleStr[0] != '\0')
            {
                g = findGameByTitleInsensitive(gs, titleStr);
            }
            if (g != nullptr)
            {
                gameId = g->gameID;
            }
            if (gameId <= 0 || g == nullptr)
            {
                continue;
            }

            const char* text = (textStr != nullptr) ? textStr : "";
            (void)gs.addReview(memberId, gameId, rating, text);
        }
        else if (strncmp(line, "PLAY|", 5) == 0)
        {
            char* context = nullptr;
            (void)strtok_s(line, "|", &context); // PLAY
            char* gameStr = strtok_s(nullptr, "|", &context);
            char* titleStr = strtok_s(nullptr, "|", &context);
            char* participantsStr = strtok_s(nullptr, "|", &context);
            char* winnersStr = strtok_s(nullptr, "|", &context);
            char* playedAtStr = strtok_s(nullptr, "|", &context);

            if (gameStr == nullptr || participantsStr == nullptr)
            {
                continue;
            }

            int gameId = 0;
            (void)tryParseInt(gameStr, gameId);
            Game* g = nullptr;
            if (gameId > 0)
            {
                g = gs.findById(gameId);
            }
            if (g == nullptr && titleStr != nullptr && titleStr[0] != '\0')
            {
                g = findGameByTitleInsensitive(gs, titleStr);
            }
            if (g != nullptr)
            {
                gameId = g->gameID;
            }
            if (gameId <= 0 || g == nullptr)
            {
                continue;
            }

            if (!resetTotalsOnPlay)
            {
                resetMemberTotals(ms);
                resetTotalsOnPlay = true;
            }

            const char* winners = (winnersStr != nullptr) ? winnersStr : "";
            const char* playedAt = (playedAtStr != nullptr) ? playedAtStr : "";
            addPlayRecord(gameId, g->title, participantsStr, winners, playedAt);
            applyPlayStats(ms, participantsStr, winners);
        }
    }

    in2.close();
    return true;
}

static bool saveMembers(MemberService& ms, GameService& gs, const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return false;
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    out << "# BoardGameManager members v3\n";

    MemberList& members = ms.getMemberList();
    MemberNode* cur = members.getHead();
    while (cur != nullptr)
    {
        Member& m = cur->data;
        char nameBuf[60];
        sanitizeForSave(m.name, nameBuf, sizeof(nameBuf));

        out << "MEMBER|" << m.memberId << "|" << nameBuf << "|"
            << m.totalPlays << "|" << m.totalWins << "\n";

        BorrowedNode* b = m.borrowedHead;
        while (b != nullptr)
        {
            Game* g = gs.findById(b->gameId);
            char titleBuf[101];
            titleBuf[0] = '\0';
            if (g != nullptr)
            {
                sanitizeForSave(g->title, titleBuf, sizeof(titleBuf));
            }
            out << "BORROWED|" << m.memberId << "|" << b->gameId << "|" << titleBuf << "\n";
            b = b->next;
        }

        cur = cur->next;
    }

    Game** all = nullptr;
    int total = gs.getAllGames(all);
    if (total > 0 && all != nullptr)
    {
        for (int i = 0; i < total; i++)
        {
            Game* g = all[i];
            ReviewNode* r = g->reviewsHead;
            while (r != nullptr)
            {
                char textBuf[256];
                sanitizeForSave(r->text, textBuf, sizeof(textBuf));
                char titleBuf[101];
                titleBuf[0] = '\0';
                sanitizeForSave(g->title, titleBuf, sizeof(titleBuf));
                out << "REVIEW|" << r->memberId << "|" << g->gameID << "|"
                    << titleBuf << "|" << r->rating << "|" << textBuf << "\n";
                r = r->next;
            }
        }
        delete[] all;
    }

    PlayRecord* pr = g_playHead;
    while (pr != nullptr)
    {
        out << "PLAY|" << pr->gameId << "|" << pr->gameTitle << "|"
            << pr->participants << "|" << pr->winners << "|"
            << pr->playedAt << "\n";
        pr = pr->next;
    }

    return true;
}

static void autoSaveMembers(MemberService& ms, GameService& gs, const char* path)
{
    if (path == nullptr || path[0] == '\0')
    {
        return;
    }

    if (!saveMembers(ms, gs, path))
    {
        std::cout << "Warning: failed to save members to " << path << "\n";
    }
}

static void printGameShort(const Game* g, const GameService& gs, bool isAdmin)
{
    if (g == nullptr)
    {
        return;
    }

    if (isAdmin)
    {
        std::cout << "ID: " << std::setw(3) << g->gameID << " | Title: " << g->title << "\n"
            << "        | Players: " << g->minPlayers << "-" << g->maxPlayers << "\n"
            << "        | Year: " << g->yearPublished << "\n"
            << "        | Copies: " << g->copiesAvailable << "/" << g->copiesTotal << "\n";
        if (g->reviewCount > 0)
        {
            std::cout << "        | Avg Rating: " << gs.getAverageRating(g) << "\n"
                << "        | Reviews: " << g->reviewCount << "\n";
        }
        std::cout << "\n";
    }
    else
    {
        std::cout << "| Title: " << g->title << "\n"
            << "        | Players: " << g->minPlayers << "-" << g->maxPlayers << "\n"
            << "        | Year: " << g->yearPublished << "\n";
        if (g->reviewCount > 0)
        {
            std::cout << "        | Avg Rating: " << gs.getAverageRating(g) << "\n"
                << "        | Reviews: " << g->reviewCount << "\n";
        }
        std::cout << "\n";
    }
}

// Admin: select by ID (since ID is shown). Member: select by number (since ID is hidden).
static int selectGameByName(GameService& gs, bool isAdmin)
{
    char query[101];
    readLine("Enter the game name: ", query, sizeof(query));
    if (query[0] == '\0')
    {
        return -1;
    }

    Game** matches = nullptr;
    int count = gs.findGamesByTitleContains(query, matches);
    if (count <= 0 || matches == nullptr)
    {
        std::cout << "No matching games found.\n";
        return -1;
    }

    std::cout << "Matches:\n";
    for (int i = 0; i < count; i++)
    {
        if (isAdmin)
        {
            printGameShort(matches[i], gs, true);
        }
        else
        {
            int idx = i + 1;
            char prefix[16];
            snprintf(prefix, sizeof(prefix), "%d)", idx);
            std::cout << prefix;
            const int targetWidth = 8;
            int pad = targetWidth - (int)strlen(prefix);
            for (int p = 0; p < pad; p++)
            {
                std::cout << " ";
            }
            printGameShort(matches[i], gs, false);
        }
    }

    int chosenId = -1;
    if (isAdmin)
    {
        chosenId = readInt("Enter Game ID from the list (0 to cancel): ");
        if (chosenId == 0)
        {
            chosenId = -1;
        }
    }
    else
    {
        int idx = readInt("Select a number from the list (0 to cancel): ");
        if (idx >= 1 && idx <= count)
        {
            chosenId = matches[idx - 1]->gameID;
        }
        else
        {
            chosenId = -1;
        }
    }

    delete[] matches;
    return chosenId;
}

static void showGameDetails(GameService& gs, bool isAdmin)
{
    int gameId = -1;

    if (isAdmin)
    {
        int choice = readInt("Find by (1) ID or (2) name: ");
        if (choice == 1)
        {
            gameId = readInt("Enter Game ID: ");
        }
        else if (choice == 2)
        {
            gameId = selectGameByName(gs, isAdmin);
        }
    }
    else
    {
        gameId = selectGameByName(gs, false);
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
    printGameShort(g, gs, isAdmin);
}

static void showGameReviews(GameService& gs, MemberService& ms, bool isAdmin)
{
    int gameId = -1;

    if (isAdmin)
    {
        int choice = readInt("Find by (1) ID or (2) name: ");
        if (choice == 1)
        {
            gameId = readInt("Enter Game ID: ");
        }
        else if (choice == 2)
        {
            gameId = selectGameByName(gs, isAdmin);
        }
    }
    else
    {
        gameId = selectGameByName(gs, false);
    }

    if (gameId <= 0)
    {
        return;
    }

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

static bool writeGameReview(GameService& gs, bool isAdmin, int memberId)
{
    int gameId = -1;

    if (isAdmin)
    {
        int mode = readInt("Review by (1) Game ID or (2) name: ");
        if (mode == 1)
        {
            gameId = readInt("Game ID to review: ");
        }
        else if (mode == 2)
        {
            gameId = selectGameByName(gs, isAdmin);
        }
    }
    else
    {
        gameId = selectGameByName(gs, false);
    }

    if (gameId <= 0)
    {
        return false;
    }

    int rating = readInt("Rating (1-10): ");
    char text[256];
    readLine("Review text (optional): ", text, sizeof(text));

    if (gs.addReview(memberId, gameId, rating, text))
    {
        std::cout << "Review recorded.\n";
        return true;
    }
    else
    {
        std::cout << "Failed to add review. You may have already reviewed this game.\n";
        return false;
    }
}

static void listGamesByPlayers(GameService& gs, bool isAdmin)
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
        printGameShort(list[i], gs, isAdmin);
    }
    delete[] list;
}

static bool recordGamePlay(GameService& gs, MemberService& ms, bool isAdmin)
{
    // Ray Feature: Collect participants/winners and save a game-play record.
    std::cout << "\n--- Record Game Play ---\n";
    std::cout << "Tip: type 'list' to view members, 'cancel' to abort.\n";

    int gameId = -1;
    if (isAdmin)
    {
        int choice = 0;
        while (choice != 1 && choice != 2)
        {
            choice = readInt("Find game by (1) ID or (2) name: ");
            if (choice != 1 && choice != 2)
            {
                std::cout << "Please choose 1 or 2.\n";
            }
        }

        if (choice == 1)
        {
            gameId = readInt("Enter Game ID (0 to cancel): ");
            if (gameId == 0)
            {
                std::cout << "Cancelled.\n";
                return false;
            }
        }
        else
        {
            gameId = selectGameByName(gs, isAdmin);
        }
    }
    else
    {
        gameId = selectGameByName(gs, false);
    }

    if (gameId <= 0)
    {
        return false;
    }

    Game* g = gs.findById(gameId);
    if (g == nullptr)
    {
        std::cout << "Game not found.\n";
        return false;
    }

    std::cout << "Selected game: " << g->title << " (ID " << g->gameID << ")\n";
    std::cout << "Players supported: " << g->minPlayers << "-" << g->maxPlayers << "\n";
    int confirmGame = readInt("Record play for this game? (1) Yes (0) No: ");
    if (confirmGame != 1)
    {
        std::cout << "Cancelled.\n";
        return false;
    }

    int tempIds[64];
    int participants[64];
    int participantCount = 0;
    while (true)
    {
        char participantsInput[256];
        readLine("Enter participant IDs (comma-separated): ", participantsInput, sizeof(participantsInput));
        if (equalsIgnoreCase(participantsInput, "list"))
        {
            printMemberDirectory(ms);
            continue;
        }
        if (equalsIgnoreCase(participantsInput, "cancel"))
        {
            std::cout << "Cancelled.\n";
            return false;
        }

        int tempCount = parseIdList(participantsInput, tempIds, 64);

        participantCount = 0;
        int invalidIds[64];
        int invalidCount = 0;
        for (int i = 0; i < tempCount; i++)
        {
            if (ms.getMemberById(tempIds[i]) != nullptr)
            {
                if (!containsId(participants, participantCount, tempIds[i]))
                {
                    participants[participantCount++] = tempIds[i];
                }
            }
            else
            {
                invalidIds[invalidCount++] = tempIds[i];
            }
        }

        if (participantCount <= 0)
        {
            std::cout << "No valid participants.\n";
            int action = readInt("Re-enter participants? (1) Yes (0) Cancel: ");
            if (action == 0)
            {
                std::cout << "Cancelled.\n";
                return false;
            }
            continue;
        }

        std::cout << "Participants: ";
        printMemberListWithNames(participants, participantCount, ms);
        std::cout << "\n";

        if (invalidCount > 0)
        {
            std::cout << "Invalid participant IDs: ";
            printIdList(invalidIds, invalidCount);
            std::cout << "\n";
        }

        bool outOfRange = (participantCount < g->minPlayers || participantCount > g->maxPlayers);
        if (outOfRange)
        {
            std::cout << "Warning: " << participantCount << " participants, but game supports "
                << g->minPlayers << "-" << g->maxPlayers << ".\n";
        }

        const char* prompt = outOfRange
            ? "Participants outside range. (1) Continue (0) Re-enter (2) Cancel: "
            : "Confirm participants? (1) Yes (0) Re-enter (2) Cancel: ";
        int confirm = readInt(prompt);
        if (confirm == 1)
        {
            break;
        }
        if (confirm == 2)
        {
            std::cout << "Cancelled.\n";
            return false;
        }
    }

    int winners[64];
    int winnerCount = 0;
    while (true)
    {
        char winnersInput[256];
        readLine("Enter winner IDs (comma-separated, blank = none, 'all' = everyone): ", winnersInput, sizeof(winnersInput));
        if (equalsIgnoreCase(winnersInput, "list"))
        {
            printMemberDirectory(ms);
            continue;
        }
        if (equalsIgnoreCase(winnersInput, "cancel"))
        {
            std::cout << "Cancelled.\n";
            return false;
        }
        if (equalsIgnoreCase(winnersInput, "all"))
        {
            winnerCount = participantCount;
            for (int i = 0; i < participantCount; i++)
            {
                winners[i] = participants[i];
            }
            break;
        }
        if (winnersInput[0] == '\0' || equalsIgnoreCase(winnersInput, "none"))
        {
            winnerCount = 0;
            break;
        }

        int tempWinCount = parseIdList(winnersInput, tempIds, 64);

        winnerCount = 0;
        int invalidWinnerIds[64];
        int invalidWinnerCount = 0;
        int notParticipantIds[64];
        int notParticipantCount = 0;
        for (int i = 0; i < tempWinCount; i++)
        {
            if (ms.getMemberById(tempIds[i]) == nullptr)
            {
                invalidWinnerIds[invalidWinnerCount++] = tempIds[i];
            }
            else if (!containsId(participants, participantCount, tempIds[i]))
            {
                notParticipantIds[notParticipantCount++] = tempIds[i];
            }
            else
            {
                if (!containsId(winners, winnerCount, tempIds[i]))
                {
                    winners[winnerCount++] = tempIds[i];
                }
            }
        }

        if (winnerCount <= 0)
        {
            std::cout << "No valid winners entered.\n";
            if (invalidWinnerCount > 0)
            {
                std::cout << "Invalid winner IDs: ";
                printIdList(invalidWinnerIds, invalidWinnerCount);
                std::cout << "\n";
            }
            if (notParticipantCount > 0)
            {
                std::cout << "Winner IDs not in participant list: ";
                printIdList(notParticipantIds, notParticipantCount);
                std::cout << "\n";
            }
            int action = readInt("No valid winners. (1) Re-enter (2) Skip winners (0) Cancel: ");
            if (action == 2)
            {
                winnerCount = 0;
                break;
            }
            if (action == 0)
            {
                std::cout << "Cancelled.\n";
                return false;
            }
            continue;
        }

        std::cout << "Winners: ";
        printMemberListWithNames(winners, winnerCount, ms);
        std::cout << "\n";

        if (invalidWinnerCount > 0)
        {
            std::cout << "Invalid winner IDs: ";
            printIdList(invalidWinnerIds, invalidWinnerCount);
            std::cout << "\n";
        }
        if (notParticipantCount > 0)
        {
            std::cout << "Winner IDs not in participant list: ";
            printIdList(notParticipantIds, notParticipantCount);
            std::cout << "\n";
        }

        int confirm = readInt("Confirm winners? (1) Yes (0) Re-enter (2) Skip winners (3) Cancel: ");
        if (confirm == 1)
        {
            break;
        }
        if (confirm == 2)
        {
            winnerCount = 0;
            break;
        }
        if (confirm == 3)
        {
            std::cout << "Cancelled.\n";
            return false;
        }
    }

    std::cout << "\nPlay Summary\n";
    std::cout << "Game: " << g->title << " (ID " << g->gameID << ")\n";
    std::cout << "Participants (" << participantCount << "): ";
    printMemberListWithNames(participants, participantCount, ms);
    std::cout << "\n";
    std::cout << "Winners (" << winnerCount << "): ";
    if (winnerCount > 0)
    {
        printMemberListWithNames(winners, winnerCount, ms);
    }
    else
    {
        std::cout << "None recorded";
    }
    std::cout << "\n";

    int finalConfirm = readInt("Save this play record? (1) Yes (0) No: ");
    if (finalConfirm != 1)
    {
        std::cout << "Cancelled.\n";
        return false;
    }

    for (int i = 0; i < participantCount; i++)
    {
        Member* m = ms.getMemberById(participants[i]);
        if (m != nullptr)
        {
            m->totalPlays += 1;
        }
    }
    for (int i = 0; i < winnerCount; i++)
    {
        Member* m = ms.getMemberById(winners[i]);
        if (m != nullptr)
        {
            m->totalWins += 1;
        }
    }

    char participantsStr[256];
    char winnersStr[256];
    buildIdListString(participants, participantCount, participantsStr, sizeof(participantsStr));
    buildIdListString(winners, winnerCount, winnersStr, sizeof(winnersStr));
    char playedAt[20];
    getCurrentTimestamp(playedAt, sizeof(playedAt));
    addPlayRecord(gameId, g->title, participantsStr, winnersStr, playedAt);

    std::cout << "Play recorded.\n";

    return true;
}

static void showPlayRecords(MemberService& ms)
{
    // Ray Feature: Display saved play history to users.
    if (g_playHead == nullptr)
    {
        std::cout << "No play records.\n";
        return;
    }

    int index = 1;
    PlayRecord* pr = g_playHead;
    while (pr != nullptr)
    {
        std::cout << "\n" << index << ") ";
        if (pr->playedAt[0] != '\0')
        {
            std::cout << pr->playedAt;
        }
        else
        {
            std::cout << "Unknown date";
        }
        std::cout << " | Game: " << pr->gameTitle << " (ID " << pr->gameId << ")\n";

        int participants[64];
        int participantCount = parseIdList(pr->participants, participants, 64);
        int winners[64];
        int winnerCount = parseIdList(pr->winners, winners, 64);

        std::cout << "Participants: ";
        printMemberListWithNames(participants, participantCount, ms);
        std::cout << "\n";

        std::cout << "Winners: ";
        if (winnerCount > 0)
        {
            printMemberListWithNames(winners, winnerCount, ms);
        }
        else
        {
            std::cout << "None recorded";
        }
        std::cout << "\n";

        std::cout << "Losers: ";
        if (winnerCount > 0)
        {
            int losers[64];
            int loserCount = 0;
            for (int i = 0; i < participantCount; i++)
            {
                if (!containsId(winners, winnerCount, participants[i]))
                {
                    losers[loserCount++] = participants[i];
                }
            }
            if (loserCount > 0)
            {
                printMemberListWithNames(losers, loserCount, ms);
            }
            else
            {
                std::cout << "None";
            }
        }
        else
        {
            std::cout << "None recorded";
        }
        std::cout << "\n";

        pr = pr->next;
        index++;
    }
}

static void showRecommendations(GameService& gs, int memberId)
{
    const int likeThreshold = 7;

    std::cout << "\n========================================\n";
    std::cout << "GAME RECOMMENDATIONS\n";
    std::cout << "========================================\n";

    Game** all = nullptr;
    int total = gs.getAllGames(all);
    if (total <= 0 || all == nullptr)
    {
        std::cout << "No games available in the system.\n";
        return;
    }

    int* ratedGameIds = new int[total];
    int* likedGameIds = new int[total];
    int ratedCount = 0;
    int likedCount = 0;
    int totalReviews = 0;

    for (int i = 0; i < total; i++)
    {
        totalReviews += all[i]->reviewCount;
        ReviewNode* r = all[i]->reviewsHead;
        while (r != nullptr)
        {
            if (r->memberId == memberId)
            {
                ratedGameIds[ratedCount++] = all[i]->gameID;
                if (r->rating >= likeThreshold)
                {
                    likedGameIds[likedCount++] = all[i]->gameID;
                }
                break;
            }
            r = r->next;
        }
    }

    if (likedCount == 0)
    {
        std::cout << "You have not rated any games " << likeThreshold << " or higher yet.\n";
        std::cout << "Tip: Rate games you enjoy to unlock personalised recommendations.\n";
        delete[] ratedGameIds;
        delete[] likedGameIds;
        delete[] all;
        return;
    }

    if (totalReviews <= 0)
    {
        std::cout << "There are no reviews in the system yet.\n";
        std::cout << "Recommendations will improve as more members leave reviews.\n";
        delete[] ratedGameIds;
        delete[] likedGameIds;
        delete[] all;
        return;
    }

    int* similarMembers = new int[totalReviews];
    int simCount = 0;

    for (int i = 0; i < likedCount; i++)
    {
        Game* g = gs.findById(likedGameIds[i]);
        if (g == nullptr)
        {
            continue;
        }

        ReviewNode* r = g->reviewsHead;
        while (r != nullptr)
        {
            if (r->memberId != memberId && r->rating >= likeThreshold)
            {
                if (!containsId(similarMembers, simCount, r->memberId))
                {
                    similarMembers[simCount++] = r->memberId;
                }
            }
            r = r->next;
        }
    }

    if (simCount == 0)
    {
        std::cout << "No similar reviewers found yet.\n";
        std::cout << "Try again after more members have reviewed games.\n";
        delete[] ratedGameIds;
        delete[] likedGameIds;
        delete[] similarMembers;
        delete[] all;
        return;
    }

    double* scoreSum = new double[total];
    int* scoreCount = new int[total];
    for (int i = 0; i < total; i++)
    {
        scoreSum[i] = 0.0;
        scoreCount[i] = 0;
    }

    for (int i = 0; i < total; i++)
    {
        if (containsId(ratedGameIds, ratedCount, all[i]->gameID))
        {
            continue;
        }

        ReviewNode* r = all[i]->reviewsHead;
        while (r != nullptr)
        {
            if (r->rating >= likeThreshold &&
                containsId(similarMembers, simCount, r->memberId))
            {
                scoreSum[i] += r->rating;
                scoreCount[i] += 1;
            }
            r = r->next;
        }
    }

    int* candidates = new int[total];
    int candCount = 0;
    for (int i = 0; i < total; i++)
    {
        if (scoreCount[i] > 0)
        {
            candidates[candCount++] = i;
        }
    }

    if (candCount == 0)
    {
        std::cout << "No suitable recommendations available at the moment.\n";
        std::cout << "Rate more games to improve recommendation accuracy.\n";
        delete[] ratedGameIds;
        delete[] likedGameIds;
        delete[] similarMembers;
        delete[] scoreSum;
        delete[] scoreCount;
        delete[] candidates;
        delete[] all;
        return;
    }

    for (int i = 0; i < candCount - 1; i++)
    {
        int best = i;
        for (int j = i + 1; j < candCount; j++)
        {
            int a = candidates[best];
            int b = candidates[j];

            double avgA = scoreSum[a] / scoreCount[a];
            double avgB = scoreSum[b] / scoreCount[b];

            bool swapNeeded = false;
            if (avgB > avgA)
            {
                swapNeeded = true;
            }
            else if (avgB == avgA)
            {
                if (scoreCount[b] > scoreCount[a])
                {
                    swapNeeded = true;
                }
                else if (scoreCount[b] == scoreCount[a] &&
                    strcmp(all[b]->title, all[a]->title) < 0)
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
            int tmp = candidates[i];
            candidates[i] = candidates[best];
            candidates[best] = tmp;
        }
    }

    int limit = (candCount < 10) ? candCount : 10;
    std::cout << "Based on your ratings (>= " << likeThreshold << "), here are your top recommendations:\n\n";
    std::cout << std::fixed << std::setprecision(2);

    for (int i = 0; i < limit; i++)
    {
        int idx = candidates[i];
        double avg = scoreSum[idx] / scoreCount[idx];

        std::cout << (i + 1) << ". " << all[idx]->title << "\n";
        std::cout << "   Similar users’ average rating: " << avg << "\n";
        std::cout << "   Based on " << scoreCount[idx] << " similar rating(s)\n\n";
    }

    std::cout.unsetf(std::ios::floatfield);
    std::cout << "Tip: Recommendations improve as more reviews are added.\n";

    delete[] ratedGameIds;
    delete[] likedGameIds;
    delete[] similarMembers;
    delete[] scoreSum;
    delete[] scoreCount;
    delete[] candidates;
    delete[] all;
}

static void listAllGames(GameService& gs, bool isAdmin)
{
    Game** list = nullptr;
    int count = gs.getAllGames(list);
    if (count <= 0)
    {
        std::cout << "No games loaded.\n";
        return;
    }

    sortGamesByTitle(list, count);
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

    char gamesPath[260];
    (void)resolveGamesPath(gamesPath, sizeof(gamesPath));

    bool gamesLoaded = false;
    if (fileExists(gamesPath))
    {
        if (loadGamesFromText(gs, gamesPath))
        {
            std::cout << "Loaded games from: " << gamesPath << "\n";
            gamesLoaded = true;
        }
        else
        {
            std::cout << "Failed to load games from: " << gamesPath << "\n";
        }
    }

    char csvPath[260];
    strncpy_s(csvPath, sizeof(csvPath), "games.csv", _TRUNCATE);

    if (!gamesLoaded)
    {
        bool csvLoaded = false;
        if (tryLoadCsv(gs, csvPath))
        {
            csvLoaded = true;
        }
        else if (tryLoadCsvFromParents(gs))
        {
            csvLoaded = true;
        }
        else
        {
            readLine("Enter CSV path (or leave blank to skip): ", csvPath, sizeof(csvPath));
            if (csvPath[0] != '\0')
            {
                if (tryLoadCsv(gs, csvPath))
                {
                    csvLoaded = true;
                }
                else
                {
                    std::cout << "Failed to load CSV.\n";
                }
            }
        }

        if (csvLoaded)
        {
            autoSaveGames(gs, gamesPath);
        }
    }

    char membersPath[260];
    (void)resolveMembersPath(membersPath, sizeof(membersPath));
    if (fileExists(membersPath))
    {
        if (loadMembers(ms, gs, membersPath))
        {
            std::cout << "Loaded members from: " << membersPath << "\n";
        }
        else
        {
            std::cout << "Failed to load members from: " << membersPath << "\n";
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
                    // Ray Feature: Add new board game (with duplicate-title handling).
                    std::cout << "\n--- Add New Game ---\n";
                    std::cout << "Leave title blank to cancel.\n";
                    char title[101];
                    readLine("Title: ", title, sizeof(title));
                    if (title[0] == '\0')
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }

                    int minP = 0;
                    while (true)
                    {
                        minP = readInt("Min players (>=1, 0 to cancel): ");
                        if (minP == 0)
                        {
                            std::cout << "Cancelled.\n";
                            break;
                        }
                        if (minP > 0)
                        {
                            break;
                        }
                        std::cout << "Min players must be at least 1.\n";
                    }
                    if (minP == 0)
                    {
                        continue;
                    }

                    int maxP = 0;
                    while (true)
                    {
                        maxP = readInt("Max players (>= min, 0 to cancel): ");
                        if (maxP == 0)
                        {
                            std::cout << "Cancelled.\n";
                            break;
                        }
                        if (maxP >= minP)
                        {
                            break;
                        }
                        std::cout << "Max players must be >= min players.\n";
                    }
                    if (maxP == 0)
                    {
                        continue;
                    }

                    int year = 0;
                    while (true)
                    {
                        year = readInt("Year published (e.g., 1995, 0 to cancel): ");
                        if (year == 0)
                        {
                            std::cout << "Cancelled.\n";
                            break;
                        }
                        if (year > 0)
                        {
                            break;
                        }
                        std::cout << "Year must be a positive number.\n";
                    }
                    if (year == 0)
                    {
                        continue;
                    }

                    int copies = 0;
                    while (true)
                    {
                        copies = readInt("Copies (>=1, 0 to cancel): ");
                        if (copies == 0)
                        {
                            std::cout << "Cancelled.\n";
                            break;
                        }
                        if (copies > 0)
                        {
                            break;
                        }
                        std::cout << "Copies must be at least 1.\n";
                    }
                    if (copies == 0)
                    {
                        continue;
                    }

                    Game* existing = findGameByTitleInsensitive(gs, title);
                    bool overwriteExisting = false;
                    if (existing != nullptr)
                    {
                        int borrowedCopies = existing->copiesTotal - existing->copiesAvailable;
                        if (borrowedCopies < 0)
                        {
                            borrowedCopies = 0;
                        }

                        std::cout << "Note: This title already exists (ID " << existing->gameID << ").\n";
                        std::cout << "1. Add copies to existing game\n";
                        std::cout << "2. Overwrite existing game details\n";
                        std::cout << "0. Cancel\n";

                        int action = -1;
                        while (true)
                        {
                            action = readInt("Select action: ");
                            if (action == 0 || action == 1 || action == 2)
                            {
                                break;
                            }
                            std::cout << "Invalid choice.\n";
                        }

                        if (action == 0)
                        {
                            std::cout << "Cancelled.\n";
                            continue;
                        }

                        overwriteExisting = (action == 2);
                        if (overwriteExisting)
                        {
                            if (copies < borrowedCopies)
                            {
                                std::cout << "Cannot overwrite: " << borrowedCopies
                                    << " copy(ies) currently borrowed, so copies must be >= " << borrowedCopies << ".\n";
                                continue;
                            }

                            std::cout << "Current copies: " << existing->copiesAvailable << "/" << existing->copiesTotal
                                << " | After overwrite: " << (copies - borrowedCopies) << "/" << copies << "\n";
                        }
                        else
                        {
                            std::cout << "Current copies: " << existing->copiesAvailable << "/" << existing->copiesTotal
                                << " | After add: " << (existing->copiesAvailable + copies) << "/" << (existing->copiesTotal + copies) << "\n";
                        }
                    }

                    std::cout << (overwriteExisting ? "Overwrite game: " : "Add game: ") << title
                        << " | Players: " << minP << "-" << maxP
                        << " | Year: " << year
                        << " | Copies: " << copies << "\n";
                    int confirm = readInt(overwriteExisting ? "Confirm overwrite? (1) Yes (0) No: " : "Confirm add? (1) Yes (0) No: ");
                    if (confirm != 1)
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }

                    bool ok = false;
                    if (overwriteExisting && existing != nullptr)
                    {
                        ok = admin.overwriteGame(existing->gameID, title, minP, maxP, year, copies);
                    }
                    else
                    {
                        ok = admin.addGame(title, minP, maxP, year, copies);
                    }

                    if (ok)
                    {
                        Game* added = findGameByTitleInsensitive(gs, title);
                        if (added != nullptr)
                        {
                            std::cout << (overwriteExisting ? "Game overwritten. ID " : "Game added. ID ") << added->gameID
                                << " | Copies: " << added->copiesAvailable << "/" << added->copiesTotal << "\n";
                        }
                        else
                        {
                            std::cout << (overwriteExisting ? "Game overwritten.\n" : "Game added.\n");
                        }
                        autoSaveGames(gs, gamesPath);
                    }
                    else
                    {
                        std::cout << (overwriteExisting
                            ? "Failed to overwrite game. Check inputs and borrowed copies.\n"
                            : "Failed to add game. Check inputs.\n");
                    }
                }
                else if (a == 2)
                {
                    // Ray Feature: Remove board game by ID or by name search.
                    std::cout << "\n--- Remove Game ---\n";
                    int mode = readInt("Remove by (1) ID or (2) name (0 to cancel): ");
                    int id = -1;
                    if (mode == 0)
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }
                    else if (mode == 1)
                    {
                        id = readInt("Enter Game ID (0 to cancel): ");
                        if (id == 0)
                        {
                            std::cout << "Cancelled.\n";
                            continue;
                        }
                    }
                    else if (mode == 2)
                    {
                        id = selectGameByName(gs, true);
                    }
                    else
                    {
                        std::cout << "Invalid choice.\n";
                        continue;
                    }

                    if (id <= 0)
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }

                    Game* g = gs.findById(id);
                    if (g == nullptr)
                    {
                        std::cout << "Game not found.\n";
                        continue;
                    }

                    std::cout << "Selected game:\n";
                    printGameShort(g, gs, true);

                    int borrowedCopies = g->copiesTotal - g->copiesAvailable;
                    if (borrowedCopies > 0)
                    {
                        std::cout << "Cannot remove: " << borrowedCopies << " copy(ies) currently borrowed.\n";
                        continue;
                    }

                    int confirm = readInt("Remove this game? (1) Yes (0) No: ");
                    if (confirm != 1)
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }

                    if (admin.removeGame(id))
                    {
                        std::cout << "Game removed.\n";
                        autoSaveGames(gs, gamesPath);
                    }
                    else
                    {
                        std::cout << "Failed to remove game.\n";
                    }
                }
                else if (a == 3)
                {
                    // Ray Feature: Register a new member.
                    std::cout << "\n--- Add Member ---\n";
                    int memberId = readInt("Member ID (0 to cancel): ");
                    if (memberId == 0)
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }

                    Member* existingMember = ms.getMemberById(memberId);
                    if (existingMember != nullptr)
                    {
                        std::cout << "Member ID already exists: " << existingMember->name << "\n";
                        continue;
                    }

                    char name[60];
                    while (true)
                    {
                        readLine("Member name (blank to cancel): ", name, sizeof(name));
                        if (name[0] == '\0')
                        {
                            std::cout << "Cancelled.\n";
                            break;
                        }
                        break;
                    }
                    if (name[0] == '\0')
                    {
                        continue;
                    }

                    std::cout << "Add member: " << memberId << " - " << name << "\n";
                    int confirm = readInt("Confirm add? (1) Yes (0) No: ");
                    if (confirm != 1)
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }

                    Status s = ms.addMember(memberId, name);
                    if (s == OK)
                    {
                        std::cout << "Member added.\n";
                        autoSaveMembers(ms, gs, membersPath);
                    }
                    else if (s == ALREADY_EXISTS)
                    {
                        std::cout << "Member ID already exists.\n";
                    }
                    else if (s == INVALID_INPUT)
                    {
                        std::cout << "Invalid input. Check the ID and name.\n";
                    }
                    else
                    {
                        std::cout << "Failed to add member.\n";
                    }
                }
                else if (a == 4)
                {
                    // Ray Feature: Show overall borrow/return summary.
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

            std::cout << "Member: " << member->name << "\n";

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
                std::cout << "9. Record game play\n";
                std::cout << "10. Recommendations\n";
                std::cout << "11. View play records\n";
                std::cout << "0. Back\n";

                int m = readInt("Select: ");
                if (m == 0)
                {
                    memberMenu = false;
                }
                else if (m == 1 || m == 2)
                {
                    std::cout << "\n--- " << (m == 1 ? "Borrow Game" : "Return Game") << " ---\n";

                    int gameId = selectGameByName(gs, false);
                    if (gameId <= 0)
                    {
                        std::cout << "No game selected.\n";
                        continue;
                    }

                    Game* g = gs.findById(gameId);
                    if (g == nullptr)
                    {
                        std::cout << "Game not found.\n";
                        continue;
                    }

                    std::cout << "\nGame Details\n";
                    std::cout << "---------------------------------\n";
                    std::cout << "Title   : " << g->title << "\n";
                    std::cout << "Game ID : " << g->gameID << "\n";
                    std::cout << "Players : " << g->minPlayers << "-" << g->maxPlayers << "\n";
                    std::cout << "Copies  : " << g->copiesAvailable << "/" << g->copiesTotal << "\n";
                    std::cout << "---------------------------------\n";

                    int confirm = readInt(m == 1
                        ? "Confirm borrow? (1 = Yes, 0 = Cancel): "
                        : "Confirm return? (1 = Yes, 0 = Cancel): ");

                    if (confirm != 1)
                    {
                        std::cout << "Action cancelled.\n";
                        continue;
                    }

                    Status result = (m == 1)
                        ? ts.borrowGame(memberId, gameId)
                        : ts.returnGame(memberId, gameId);

                    std::cout << "\nResult\n";
                    std::cout << "---------------------------------\n";

                    if (result == OK)
                    {
                        std::cout << (m == 1 ? "Game successfully borrowed.\n"
                            : "Game successfully returned.\n");
                        autoSaveMembers(ms, gs, membersPath);
                    }
                    else if (result == NOT_FOUND)
                    {
                        std::cout << "Game not found.\n";
                    }
                    else if (result == NOT_AVAILABLE)
                    {
                        std::cout << "No available copies to borrow.\n";
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

                    std::cout << "---------------------------------\n";
                }
                else if (m == 3)
                {
                    // Ray Feature: Show member-specific borrow/return summary.
                    ts.memberBorrowReturnSummary(memberId);
                }
                else if (m == 4)
                {
                    int gameId = selectGameByName(gs, false);

                    if (gameId <= 0)
                    {
                        continue;
                    }

                    Game* g = gs.findById(gameId);
                    if (g == nullptr)
                    {
                        std::cout << "Game not found.\n";
                        continue;
                    }
                    std::cout << "Selected game: " << g->title << " (ID " << g->gameID << ")\n";
                    int confirm = readInt("Rate this game? (1) Yes (0) No: ");
                    if (confirm != 1)
                    {
                        std::cout << "Cancelled.\n";
                        continue;
                    }

                    int rating = readInt("Rating (1-10): ");
                    if (gs.rateGame(memberId, gameId, rating))
                    {
                        std::cout << "Rating recorded.\n";
                        autoSaveMembers(ms, gs, membersPath);
                    }
                    else
                    {
                        std::cout << "Failed to rate game.\n";
                    }
                }
                else if (m == 5)
                {
                    if (writeGameReview(gs, false, memberId))
                    {
                        autoSaveMembers(ms, gs, membersPath);
                    }
                }
                else if (m == 6)
                {
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
                else if (m == 9)
                {
                    // Ray Feature: Record a game play with participants and winners.
                    if (recordGamePlay(gs, ms, false))
                    {
                        autoSaveMembers(ms, gs, membersPath);
                    }
                }
                else if (m == 10)
                {
                    showRecommendations(gs, memberId);
                }
                else if (m == 11)
                {
                    showPlayRecords(ms);
                }
            }
        }
    }

    return 0;
}
