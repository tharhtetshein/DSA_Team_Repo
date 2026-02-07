#include "HashTable.h"
#include <cstring>
#include <cctype>

HashTable::HashTable()
{
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        table[i] = nullptr;
    }
}

HashTable::~HashTable()
{
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        HashNode* cur = table[i];
        while (cur != nullptr)
        {
            ReviewNode* r = cur->data.reviewsHead;
            while (r != nullptr)
            {
                ReviewNode* nextR = r->next;
                delete r;
                r = nextR;
            }
            cur->data.reviewsHead = nullptr;
            cur->data.reviewCount = 0;
            HashNode* next = cur->next;
            delete cur;
            cur = next;
        }
        table[i] = nullptr;
    }
}

int HashTable::hashFunction(int gameID)
{
    return gameID % HASH_TABLE_SIZE;
}

bool HashTable::insertGame(const Game& game)
{
    // Ray Feature DS: Insert game into hash bucket chain by gameID.
    if (game.gameID <= 0)
    {
        return false;
    }

    if (findGame(game.gameID) != nullptr)
    {
        return false;
    }

    int idx = hashFunction(game.gameID);
    HashNode* node = new HashNode();
    node->data = game;
    node->next = table[idx];
    table[idx] = node;
    return true;
}

Game* HashTable::findGame(int gameID)
{
    // Ray Feature DS: Average O(1) lookup by hashed gameID.
    if (gameID <= 0)
    {
        return nullptr;
    }

    int idx = hashFunction(gameID);
    HashNode* cur = table[idx];
    while (cur != nullptr)
    {
        if (cur->data.gameID == gameID)
        {
            return &(cur->data);
        }
        cur = cur->next;
    }
    return nullptr;
}

bool HashTable::removeGame(int gameID)
{
    // Ray Feature DS: Remove game node from hash bucket chain.
    if (gameID <= 0)
    {
        return false;
    }

    int idx = hashFunction(gameID);
    HashNode* cur = table[idx];
    HashNode* prev = nullptr;

    while (cur != nullptr)
    {
        if (cur->data.gameID == gameID)
        {
            if (prev == nullptr)
            {
                table[idx] = cur->next;
            }
            else
            {
                prev->next = cur->next;
            }
            ReviewNode* r = cur->data.reviewsHead;
            while (r != nullptr)
            {
                ReviewNode* nextR = r->next;
                delete r;
                r = nextR;
            }
            cur->data.reviewsHead = nullptr;
            cur->data.reviewCount = 0;
            delete cur;
            return true;
        }
        prev = cur;
        cur = cur->next;
    }
    return false;
}

static int toLowerAscii(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c - 'A' + 'a';
    }
    return c;
}

static int compareTitleInsensitive(const char* a, const char* b)
{
    if (a == nullptr || b == nullptr)
    {
        return (a == b) ? 0 : (a == nullptr ? -1 : 1);
    }

    while (*a != '\0' && *b != '\0')
    {
        int ca = toLowerAscii(*a);
        int cb = toLowerAscii(*b);
        if (ca != cb)
        {
            return ca - cb;
        }
        a++;
        b++;
    }
    return toLowerAscii(*a) - toLowerAscii(*b);
}

Game* HashTable::findGameByTitle(const char* title)
{
    if (title == nullptr || title[0] == '\0')
    {
        return nullptr;
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        HashNode* cur = table[i];
        while (cur != nullptr)
        {
            if (compareTitleInsensitive(cur->data.title, title) == 0)
            {
                return &(cur->data);
            }
            cur = cur->next;
        }
    }
    return nullptr;
}

int HashTable::getCountInBucket(int index) const
{
    int count = 0;
    HashNode* cur = table[index];
    while (cur != nullptr)
    {
        count++;
        cur = cur->next;
    }
    return count;
}

int HashTable::getCount() const
{
    int total = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        total += getCountInBucket(i);
    }
    return total;
}

int HashTable::toArray(Game**& outArray)
{
    int total = getCount();
    if (total <= 0)
    {
        outArray = nullptr;
        return 0;
    }

    outArray = new Game * [total];
    int index = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        HashNode* cur = table[i];
        while (cur != nullptr)
        {
            outArray[index++] = &(cur->data);
            cur = cur->next;
        }
    }
    return total;
}

void HashTable::displayAllGames()
{
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        HashNode* cur = table[i];
        while (cur != nullptr)
        {
            // Placeholder: actual display handled by GameService/UI
            cur = cur->next;
        }
    }
}
