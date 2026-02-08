#include "MemberService.h"
#include <cstring>

MemberService::MemberService() {}

Status MemberService::addMember(int memberId, const char* name) {
    // Ray Feature: Add a new member with duplicate-ID validation.
    if (memberId <= 0 || name == nullptr || name[0] == '\0') {
        return INVALID_INPUT;
    }

    if (members.findMember(memberId) != nullptr) {
        return ALREADY_EXISTS;
    }

    Member m;
    m.memberId = memberId;
    strncpy_s(m.name, sizeof(m.name), name, _TRUNCATE);
    m.name[sizeof(m.name) - 1] = '\0';
    m.borrowedHead = nullptr;
    m.totalPlays = 0;
    m.totalWins = 0;

    bool ok = members.addMember(m);
    return ok ? OK : ALREADY_EXISTS;
}

Member* MemberService::getMemberById(int memberId) {
    return members.findMember(memberId);
}

MemberList& MemberService::getMemberList() {
    return members;
}
