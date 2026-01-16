#ifndef SERVICES_MEMBERSERVICE_H
#define SERVICES_MEMBERSERVICE_H

#include "MemberList.h"

// Member B - Phase 0

enum Status {
    OK,
    NOT_FOUND,
    ALREADY_EXISTS,
    NOT_AVAILABLE,
    NOT_BORROWED,
    INVALID_INPUT,
    FULL
};

class MemberService {
private:
    MemberList members;

public:
    MemberService();

    Status addMember(int memberId, const char* name);
    Member* getMemberById(int memberId);

    // For summaries/iteration later
    MemberList& getMemberList();
};

#endif
