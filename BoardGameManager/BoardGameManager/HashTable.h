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

public:
    HashTable();

    bool insertGame(const Game& game);
    Game* findGame(int gameID);
    bool removeGame(int gameID);

    void displayAllGames();
};

#endif
#pragma once
