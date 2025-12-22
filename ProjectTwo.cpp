//============================================================================
// Name        : ProjectTwo.cpp
// Author      : Travis LeBlanc
// Version     : 1.0
// Copyright   : Copyright - 2023 SNHU COCE
// Description : Module 7-1 Project Two
//============================================================================

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <limits>
#include <filesystem>

using namespace std;

// Define structure to hold course information
struct Course {
	
	string id;
	string name;
	vector<string> prereqs;

};

// Remove leading and trailing whitespace from a string
static string trim(const string& s) {

	size_t start = 0;
	size_t end = s.size();

	while (start < s.size() && (s[start] == ' ' || s[start] == '\t'
		|| s[start] == '\r' || s[start] == '\n')) {
		start++;
	}

	while (end > start && (s[end-1] == ' ' || s[end-1] == '\t'
		|| s[end-1] == '\r' || s[end-1] == '\n')) {
		end--;
	}

	return s.substr(start, end - start);

}

// Convert a string to uppercase, mainly for course IDs
static string upperStr(string s) {

	for (size_t i = 0; i < s.size(); i++) {
		s[i] = (char)toupper((unsigned char)s[i]);
	}

	return s;

}

// Split a line by comma
static vector<string> splitComma(const string& line) {

	vector<string> out;
	string curr;

	for (size_t i = 0; i < line.size(); i++) {
		char ch = line[i];
		if (ch == ',') {
			out.push_back(trim(curr));
			curr.clear();
		}
		else {
			curr.push_back(ch);
		}
	}

	out.push_back(trim(curr));
	return out;

}

// Custom Hash Table for Courses
class CourseHashTable {

	private:

		struct Node {
			Course course;
			string key;
			Node* next;

			Node() : next(nullptr) {}
			Node(const Course& c) : course(c), key(c.id), next(nullptr) {}
		};

        unsigned int tableSize;
		vector<Node*> table;

		// Simple polynomial rolling hash
		unsigned int hashKey(const string& key) const {
			unsigned int h = 0;
			for (char c : key) {
				h = (h * 31u) + (unsigned char)c;
			}
			return h % tableSize;
		}

	public:
		
        CourseHashTable(unsigned int size = 179)
            : tableSize(size == 0 ? 179 : size),
            table(tableSize, nullptr) {
        }

		~CourseHashTable() {
			clear();
		}

        void clear() {
            for (unsigned int i = 0; i < table.size(); i++) {
                Node* current = table[i];
                while (current != nullptr) {
                    Node* temp = current;
                    current = current->next;
                    delete temp;
                }
                table[i] = nullptr;
            }
        }

        void Insert(const Course& course) {
            unsigned int idx = hashKey(course.id);

            Node* head = table[idx];
            if (head == nullptr) {
                table[idx] = new Node(course);
                return;
            }

            // If duplicate key exists, replace course data
            Node* current = head;
            while (current != nullptr) {
                if (current->key == course.id) {
                    current->course = course;
                    return;
                }
                if (current->next == nullptr) break;
                current = current->next;
            }

            current->next = new Node(course);
        }

        Course* Search(const string& id) {
            unsigned int idx = hashKey(id);
            Node* current = table[idx];

            while (current != nullptr) {
                if (current->key == id) {
                    return &current->course;
                }
                current = current->next;
            }

            return nullptr;
        }

        bool Contains(const string& id) {
            return Search(id) != nullptr;
        }

        vector<Course> GetAllCourses() const {
            vector<Course> out;
            for (unsigned int i = 0; i < table.size(); i++) {
                Node* current = table[i];
                while (current != nullptr) {
                    out.push_back(current->course);
                    current = current->next;
                }
            }
            return out;
        }

        size_t Size() const {
            size_t count = 0;
            for (unsigned int i = 0; i < table.size(); i++) {
                Node* current = table[i];
                while (current != nullptr) {
                    count++;
                    current = current->next;
                }
            }
            return count;
        }

};

// Load the course information from a CSV file into the custom hash table
static bool loadFile(const string& filename, CourseHashTable& table, string& err) {
    table.clear();
    err.clear();

    ifstream fin(filename);
    if (!fin) {
        err = "Unable to open file: " + filename;
        return false;
    }

    vector<Course> temp;
    string line;
    int lineNum = 0;

    while (getline(fin, line)) {
        lineNum++;
        line = trim(line);

        if (line.size() == 0) {
            continue;
        }

        vector<string> parts = splitComma(line);
        if (parts.size() < 2) {
            err = "File format error on line " + to_string(lineNum) + ".";
            return false;
        }

        string cid = upperStr(trim(parts[0]));
        string title = trim(parts[1]);

        if (cid.empty() || title.empty()) {
            err = "File format error on line " + to_string(lineNum) + ".";
            return false;
        }

        Course c;
        c.id = cid;
        c.name = title;

        for (size_t i = 2; i < parts.size(); i++) {
            string pre = upperStr(trim(parts[i]));
            if (!pre.empty()) {
                c.prereqs.push_back(pre);
            }
        }

        temp.push_back(c);
        table.Insert(c);
    }

    // Validate that prerequisites exist
    for (size_t i = 0; i < temp.size(); i++) {
        for (size_t j = 0; j < temp[i].prereqs.size(); j++) {
            const string& pre = temp[i].prereqs[j];
            if (!table.Contains(pre)) {
                err = "File format error: prerequisite " + pre + " not found (referenced by " + temp[i].id + ").";
                return false;
            }
        }
    }

    return true;
}

// Print all courses in alphanumeric order
static void printSortedList(CourseHashTable& table) {
    vector<Course> list = table.GetAllCourses();

    sort(list.begin(), list.end(), [](const Course& a, const Course& b) {
        return a.id < b.id;
        });

    cout << "Here is a sample schedule:" << endl;
    for (size_t i = 0; i < list.size(); i++) {
        cout << list[i].id << ", " << list[i].name << endl;
    }
}

// Print information of a selected course
static void printOneCourse(CourseHashTable& table, string inputId) {
    inputId = upperStr(trim(inputId));

    Course* c = table.Search(inputId);
    if (c == nullptr) {
        cout << "Course not found." << endl;
        return;
    }

    cout << c->id << ", " << c->name << endl;

    if (c->prereqs.size() == 0) {
        cout << "Prerequisites: None" << endl;
        return;
    }

    cout << "Prerequisites: ";
    for (size_t i = 0; i < c->prereqs.size(); i++) {
        cout << c->prereqs[i];
        if (i + 1 < c->prereqs.size()) cout << ", ";
    }
    cout << endl;
}

// Show menu
static void showMenu() {
    cout << "1. Load Data Structure." << endl;
    cout << "2. Print Course List." << endl;
    cout << "3. Print Course." << endl;
    cout << "9. Exit" << endl;
}

// Main function
int main() {
    CourseHashTable courses;
    bool finished = false;
    bool hasData = false;

    cout << "Welcome to the course planner." << endl << endl;

    while (!finished) {
        int choice;
        string filename;
        string err;

        showMenu();

        cout << endl << "What would you like to do? ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Please enter a number from the menu." << endl;
            continue;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        // Switch cases related to menu selections
        switch (choice) {
        case 1:
            // Loading the file, otherwise displaying error
            cout << "Enter file name: ";
            getline(cin, filename);
            filename = trim(filename);

            if (loadFile(filename, courses, err)) {
                hasData = true;
                cout << "Courses loaded." << endl;
            }
            else {
                hasData = false;
                cout << err << endl;
            }

            cout << endl;
            break;

        case 2:
            // Printing the course list
            if (!hasData) {
                cout << "Please load data first." << endl;
            }
            else {
                printSortedList(courses);
            }

            cout << endl;
            break;

        case 3:
            // Print course information after choosing course
            if (!hasData) {
                cout << "Please load data first." << endl;
            }
            else {
                cout << "What course do you want to know about? ";
                string cid;
                getline(cin, cid);
                printOneCourse(courses, cid);
            }

            cout << endl;
            break;

        case 9:
            // Saying goodbye and exiting program
            cout << "Thank you for using the course planner!" << endl << endl;
            finished = true;
            break;

        default:
            //Providing default case for selection not in menu
            cout << choice << " is not a valid option." << endl << endl;
            break;
        }
    }

    return 0;
}