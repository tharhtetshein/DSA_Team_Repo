#ifndef MEMBER_SERVICE_H
#define MEMBER_SERVICE_H

#include "../ds/MemberList.h"

class MemberService {
private:
    MemberList* memberList;

public:
    explicit MemberService(MemberList* list);

    Status addMember(int memberId, const char* name);
    Status getMemberById(int memberId, Member*& outMember);
};

#endif // MEMBER_SERVICE_H
