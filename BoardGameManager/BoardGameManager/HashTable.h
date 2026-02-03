#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "Game.h"

const int HASH_TABLE_SIZE = 101;

struct HashNode
{
    Game data;
    HashNode* next;
};

class HashTable
{
private:
    HashNode* table[HASH_TABLE_SIZE];

    int hashFunction(int gameID);
    int getCountInBucket(int index) const;

public:
    HashTable();
    ~HashTable();

    bool insertGame(const Game& game);
    Game* findGame(int gameID);
    bool removeGame(int gameID);
    Game* findGameByTitle(const char* title);

    int getCount() const;
    int toArray(Game**& outArray);

    void displayAllGames();
};

#endif
#pragma once
