#ifndef ADMIN_SERVICE_H
#define ADMIN_SERVICE_H

#include "GameService.h"

class AdminService
{
private:
    GameService* gameService;

public:
    AdminService(GameService* service);

    bool addGame(const char* title, int minPlayers, int maxPlayers, int year, int copies);
    bool removeGame(int gameID);
};

#endif
