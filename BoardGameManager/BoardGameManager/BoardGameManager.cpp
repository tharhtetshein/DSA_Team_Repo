#include <iostream>
#include "MemberService.h"
#include "TransactionService.h"
#include "HashTable.h"
#include "AdminService.h"

int main() {
    MemberService ms;
    TransactionService ts(&ms);

    std::cout << "Phase 0 Member B test\n";

    std::cout << "Add member: " << ms.addMember(1, "Ray") << "\n";
    std::cout << "Borrow game: " << ts.borrowGame(1, 101) << "\n";
    std::cout << "Borrow same game again: " << ts.borrowGame(1, 101) << "\n";
    std::cout << "Return game: " << ts.returnGame(1, 101) << "\n";

    ts.adminBorrowReturnSummary();
    ts.memberBorrowReturnSummary(1);

    return 0;
}
