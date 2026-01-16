#include "MemberService.h"
#include <cstring>

MemberService::MemberService(MemberList* list) : memberList(list) {}

Status MemberService::addMember(int memberId, const char* name) {
    if (!memberList || !name) {
        return INVALID_INPUT;
    }

    // TODO: Phase 1 - check for duplicates using findById
    Member newMember;
    newMember.id = memberId;
    std::strncpy(newMember.name, name, sizeof(newMember.name) - 1);
    newMember.name[sizeof(newMember.name) - 1] = '\0';

    return memberList->add(newMember);
}

Status MemberService::getMemberById(int memberId, Member*& outMember) {
    if (!memberList) {
        return INVALID_INPUT;
    }

    Member* found = memberList->findById(memberId);
    if (found) {
        outMember = found;
        return OK;
    }

    outMember = 0;
    return NOT_FOUND;
}
