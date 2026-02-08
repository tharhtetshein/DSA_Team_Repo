#include "AdminService.h"

AdminService::AdminService(GameService* service)
{
    gameService = service;
}

bool AdminService::addGame(const char* title, int minPlayers, int maxPlayers, int year, int copies)
{
    // Ray Feature: Admin entry point for adding games.
    if (gameService == nullptr)
    {
        return false;
    }

    return gameService->addNewGame(title, minPlayers, maxPlayers, year, copies);
}

bool AdminService::overwriteGame(int gameID, const char* title, int minPlayers, int maxPlayers, int year, int copies)
{
    // Ray Feature: Admin entry point for overwriting existing game details.
    if (gameService == nullptr)
    {
        return false;
    }

    return gameService->overwriteGame(gameID, title, minPlayers, maxPlayers, year, copies);
}

bool AdminService::removeGame(int gameID)
{
    // Ray Feature: Admin entry point for removing games.
    if (gameService == nullptr)
    {
        return false;
    }

    return gameService->removeGame(gameID);
}