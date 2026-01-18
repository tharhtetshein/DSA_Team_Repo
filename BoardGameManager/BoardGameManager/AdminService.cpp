#include "AdminService.h"

AdminService::AdminService(HashTable* table)
{
    gameTable = table;
}

bool AdminService::addGame(const Game& game)
{
    return false; // Phase 0 stub
}

bool AdminService::removeGame(int gameID)
{
    return false; // Phase 0 stub
}

void AdminService::showBorrowSummary()
{
    // Phase 0 stub
}
