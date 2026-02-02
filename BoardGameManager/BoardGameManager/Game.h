#ifndef GAME_H
#define GAME_H

enum class GameStatus
{
    AVAILABLE,
    BORROWED,
    REMOVED
};

struct ReviewNode
{
    int memberId;
    int rating;
    char text[256];
    ReviewNode* next;

    ReviewNode()
        : memberId(0), rating(0), next(nullptr)
    {
        text[0] = '\0';
    }
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

    ReviewNode* reviewsHead;
    int reviewCount;

    GameStatus status;
};

#endif
#pragma once
