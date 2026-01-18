#ifndef GAME_H
#define GAME_H

enum class GameStatus
{
    AVAILABLE,
    BORROWED,
    REMOVED
};

struct Game
{
    int gameID;
    char title[101];

    int minPlayers;
    int maxPlayers;
    int yearPublished;

    int copiesTotal;
    int copiesAvailable;

    int ratingSum;
    int ratingCount;

    GameStatus status;
};

#endif
#pragma once
