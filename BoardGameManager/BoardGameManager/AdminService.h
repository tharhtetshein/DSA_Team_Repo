#ifndef ADMIN_SERVICE_H
#define ADMIN_SERVICE_H

#include "HashTable.h"

class AdminService
{
private:
    HashTable* gameTable;

public:
    AdminService(HashTable* table);

    bool addGame(const Game& game);
    bool removeGame(int gameID);

    void showBorrowSummary();
};

#endif
