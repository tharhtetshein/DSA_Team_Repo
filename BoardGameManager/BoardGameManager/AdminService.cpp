#include "AdminService.h"

AdminService::AdminService(GameService* service)
{
    gameService = service;
}

bool AdminService::addGame(const char* title, int minPlayers, int maxPlayers, int year, int copies)
{
    if (gameService == nullptr)
    {
        return false;
    }

    return gameService->addNewGame(title, minPlayers, maxPlayers, year, copies);
}

bool AdminService::removeGame(int gameID)
{
    if (gameService == nullptr)
    {
        return false;
    }

    return gameService->removeGame(gameID);
}