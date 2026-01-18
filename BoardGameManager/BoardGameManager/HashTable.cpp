#include "HashTable.h"

HashTable::HashTable()
{
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        table[i] = nullptr;
    }
}

int HashTable::hashFunction(int gameID)
{
    return gameID % HASH_TABLE_SIZE;
}

bool HashTable::insertGame(const Game& game)
{
    return false; // Phase 0 stub
}

Game* HashTable::findGame(int gameID)
{
    return nullptr; // Phase 0 stub
}

bool HashTable::removeGame(int gameID)
{
    return false; // Phase 0 stub
}

void HashTable::displayAllGames()
{
    // Phase 0 stub
}
