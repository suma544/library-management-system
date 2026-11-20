// Library Management System
// C++ console application that manages books, members and borrowing records
// using Object-Oriented Programming and file handling.
//
// Classes:
//   Book          - book details and copy availability
//   Member        - member details and borrow limit
//   IssueRecord   - one borrowing record (issue date / return date)
//   Library       - manages all data, menu operations and file storage
//
// Data files: books.txt, members.txt, issues.txt (persist between runs)

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cctype>

using namespace std;

const string BOOKS_FILE = "books.txt";
const string MEMBERS_FILE = "members.txt";
const string ISSUES_FILE = "issues.txt";
const int MAX_BOOKS_PER_MEMBER = 3;

string today() {
    time_t t = time(nullptr);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
    return buf;
}

string lower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
    return s;
}

vector<string> split(const string &line) {
    vector<string> parts;
    stringstream ss(line);
    string p;
    while (getline(ss, p, '|')) parts.push_back(p);
    return parts;
}

// ---------- Book ----------
class Book {
    int id;
    string title, author;
    int totalCopies, availableCopies;

public:
    Book(int id, string t, string a, int total, int avail)
        : id(id), title(std::move(t)), author(std::move(a)), totalCopies(total), availableCopies(avail) {}

    int getId() const { return id; }
    const string &getTitle() const { return title; }
    const string &getAuthor() const { return author; }
    int getAvailable() const { return availableCopies; }
    int getTotal() const { return totalCopies; }

    bool issueCopy() { if (availableCopies == 0) return false; --availableCopies; return true; }
    void returnCopy() { if (availableCopies < totalCopies) ++availableCopies; }

    string serialize() const {
        return to_string(id) + "|" + title + "|" + author + "|" + to_string(totalCopies) + "|" +
               to_string(availableCopies);
    }

    static void header() {
        cout << left << setw(6) << "ID" << setw(32) << "Title" << setw(22) << "Author"
             << "Available\n" << string(72, '-') << '\n';
    }
    void print() const {
        cout << left << setw(6) << id << setw(32) << title << setw(22) << author
             << availableCopies << " / " << totalCopies << '\n';
    }
};

// ---------- Member ----------
class Member {
    int id;
    string name;
    int borrowedCount;

public:
    Member(int id, string n, int borrowed = 0) : id(id), name(std::move(n)), borrowedCount(borrowed) {}

    int getId() const { return id; }
    const string &getName() const { return name; }
    int getBorrowed() const { return borrowedCount; }
    bool canBorrow() const { return borrowedCount < MAX_BOOKS_PER_MEMBER; }
    void borrow() { ++borrowedCount; }
    void giveBack() { if (borrowedCount > 0) --borrowedCount; }

    string serialize() const { return to_string(id) + "|" + name + "|" + to_string(borrowedCount); }
};

// ---------- Borrowing record ----------
struct IssueRecord {
    int bookId;
    int memberId;
    string issueDate;
    string returnDate;   // "-" while the book is still issued

    bool active() const { return returnDate == "-"; }
    string serialize() const {
        return to_string(bookId) + "|" + to_string(memberId) + "|" + issueDate + "|" + returnDate;
    }
};

// ---------- Library ----------
class Library {
    vector<Book> books;
    vector<Member> members;
    vector<IssueRecord> records;
    int nextBookId = 101, nextMemberId = 201;

    Book *findBook(int id) {
        for (auto &b : books) if (b.getId() == id) return &b;
        return nullptr;
    }
    Member *findMember(int id) {
        for (auto &m : members) if (m.getId() == id) return &m;
        return nullptr;
    }

    void load() {
        ifstream bf(BOOKS_FILE), mf(MEMBERS_FILE), rf(ISSUES_FILE);
        string line;
        while (getline(bf, line)) {
            auto p = split(line);
            if (p.size() != 5) continue;
            try {
                books.emplace_back(stoi(p[0]), p[1], p[2], stoi(p[3]), stoi(p[4]));
                nextBookId = max(nextBookId, stoi(p[0]) + 1);
            } catch (...) {}
        }
        while (getline(mf, line)) {
            auto p = split(line);
            if (p.size() != 3) continue;
            try {
                members.emplace_back(stoi(p[0]), p[1], stoi(p[2]));
                nextMemberId = max(nextMemberId, stoi(p[0]) + 1);
            } catch (...) {}
        }
        while (getline(rf, line)) {
            auto p = split(line);
            if (p.size() != 4) continue;
            try { records.push_back({stoi(p[0]), stoi(p[1]), p[2], p[3]}); } catch (...) {}
        }
    }

    void save() const {
        ofstream bf(BOOKS_FILE, ios::trunc), mf(MEMBERS_FILE, ios::trunc), rf(ISSUES_FILE, ios::trunc);
        for (const auto &b : books) bf << b.serialize() << '\n';
        for (const auto &m : members) mf << m.serialize() << '\n';
        for (const auto &r : records) rf << r.serialize() << '\n';
    }

public:
    Library() { load(); }

    // ----- input helpers -----
    static int readInt(const string &prompt) {
        int v;
        while (true) {
            cout << prompt;
            if (cin >> v) { cin.ignore(numeric_limits<streamsize>::max(), '\n'); return v; }
            if (cin.eof()) exit(0);
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "  Invalid number, try again.\n";
        }
    }
    static string readLine(const string &prompt) {
        string s;
        while (true) {
            cout << prompt;
            if (!getline(cin, s)) exit(0);
            s.erase(remove(s.begin(), s.end(), '|'), s.end());
            if (!s.empty()) return s;
            cout << "  Cannot be empty.\n";
        }
    }

    // ----- operations -----
    void addBook() {
        cout << "\n--- Add Book ---\n";
        string title = readLine("Title: ");
        string author = readLine("Author: ");
        int copies;
        while ((copies = readInt("Number of copies: ")) <= 0) cout << "  Must be at least 1.\n";
        books.emplace_back(nextBookId, title, author, copies, copies);
        save();
        cout << "Book added with ID " << nextBookId++ << ".\n";
    }

    void addMember() {
        cout << "\n--- Add Member ---\n";
        string name = readLine("Member name: ");
        members.emplace_back(nextMemberId, name);
        save();
        cout << "Member registered with ID " << nextMemberId++ << ".\n";
    }

    void displayBooks() const {
        cout << "\n--- All Books (" << books.size() << ") ---\n";
        if (books.empty()) { cout << "No books in library.\n"; return; }
        Book::header();
        for (const auto &b : books) b.print();
    }

    void displayMembers() const {
        cout << "\n--- All Members (" << members.size() << ") ---\n";
        if (members.empty()) { cout << "No members registered.\n"; return; }
        cout << left << setw(6) << "ID" << setw(22) << "Name" << "Books Borrowed\n" << string(42, '-') << '\n';
        for (const auto &m : members)
            cout << left << setw(6) << m.getId() << setw(22) << m.getName()
                 << m.getBorrowed() << " / " << MAX_BOOKS_PER_MEMBER << '\n';
    }

    void searchBooks() const {
        cout << "\n--- Search Book ---\n";
        int by;
        do { by = readInt("Search by (1 = Title, 2 = Author): "); } while (by != 1 && by != 2);
        string key = lower(readLine("Enter keyword: "));
        vector<const Book *> found;
        for (const auto &b : books) {
            const string &field = (by == 1) ? b.getTitle() : b.getAuthor();
            if (lower(field).find(key) != string::npos) found.push_back(&b);
        }
        cout << found.size() << " book(s) found matching \"" << key << "\" in "
             << (by == 1 ? "title" : "author") << ":\n";
        if (found.empty()) return;
        Book::header();
        for (const auto *b : found) b->print();
    }

    void issueBook() {
        cout << "\n--- Issue Book ---\n";
        Book *b = findBook(readInt("Book ID: "));
        Member *m = findMember(readInt("Member ID: "));
        if (!b) { cout << "Book not found.\n"; return; }
        if (!m) { cout << "Member not found.\n"; return; }
        for (const auto &r : records)
            if (r.active() && r.bookId == b->getId() && r.memberId == m->getId()) {
                cout << m->getName() << " already has \"" << b->getTitle() << "\".\n"; return;
            }
        if (!m->canBorrow()) {
            cout << "Issue failed: " << m->getName() << " already has " << MAX_BOOKS_PER_MEMBER << " books.\n"; return;
        }
        if (!b->issueCopy()) { cout << "Issue failed: no copies of \"" << b->getTitle() << "\" available.\n"; return; }
        m->borrow();
        records.push_back({b->getId(), m->getId(), today(), "-"});
        save();
        cout << "\"" << b->getTitle() << "\" issued to " << m->getName() << " on " << today() << ".\n";
    }

    void returnBook() {
        cout << "\n--- Return Book ---\n";
        int bookId = readInt("Book ID: ");
        int memberId = readInt("Member ID: ");
        for (auto &r : records) {
            if (r.active() && r.bookId == bookId && r.memberId == memberId) {
                r.returnDate = today();
                findBook(bookId)->returnCopy();
                findMember(memberId)->giveBack();
                save();
                cout << "\"" << findBook(bookId)->getTitle() << "\" returned by "
                     << findMember(memberId)->getName() << " on " << r.returnDate << ".\n";
                return;
            }
        }
        cout << "Return failed: book " << bookId << " is not issued to member " << memberId << ".\n";
    }

    void borrowingRecords() {
        cout << "\n--- Borrowing Records (" << records.size() << ") ---\n";
        if (records.empty()) { cout << "No records yet.\n"; return; }
        cout << left << setw(30) << "Book" << setw(16) << "Member" << setw(13) << "Issued"
             << setw(13) << "Returned" << "Status\n" << string(80, '-') << '\n';
        for (const auto &r : records) {
            Book *b = findBook(r.bookId);
            Member *m = findMember(r.memberId);
            cout << left << setw(30) << (b ? b->getTitle() : "?") << setw(16) << (m ? m->getName() : "?")
                 << setw(13) << r.issueDate << setw(13) << r.returnDate
                 << (r.active() ? "Issued" : "Returned") << '\n';
        }
    }
};

int main() {
    Library lib;
    while (true) {
        cout << "\n======= LIBRARY MANAGEMENT SYSTEM =======\n"
             << "1. Add Book\n"
             << "2. Add Member\n"
             << "3. Display All Books\n"
             << "4. Display All Members\n"
             << "5. Search Book (Title / Author)\n"
             << "6. Issue Book\n"
             << "7. Return Book\n"
             << "8. View Borrowing Records\n"
             << "9. Exit\n";
        switch (Library::readInt("Enter your choice: ")) {
            case 1: lib.addBook(); break;
            case 2: lib.addMember(); break;
            case 3: lib.displayBooks(); break;
            case 4: lib.displayMembers(); break;
            case 5: lib.searchBooks(); break;
            case 6: lib.issueBook(); break;
            case 7: lib.returnBook(); break;
            case 8: lib.borrowingRecords(); break;
            case 9: cout << "All records saved. Goodbye!\n"; return 0;
            default: cout << "Invalid choice. Please select 1-9.\n";
        }
    }
}
