/*
* Creston Getz
* 7/19/26
*
* This program will allow the user to load a CSV file holding course data into a Hash Map.
* The program is optomized for fast lookups at the cost of memeory overhead and sorting.
* The user can print a list of the courses and search for a specific course using a course number.
*/


#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <sstream>
#include <algorithm>
#include <limits>
#include <cctype>
#include <utility>
#include <functional>
using namespace std;


//Course class
class Course {
    public:
        std::string courseNumber;
        std::string title;
        std::vector<string> prerequisites;

        Course() {} //constructor

        //override < operator
        bool operator<(const Course& other) const {
            return this->courseNumber < other.courseNumber;
        }

};


// This class creates a hash table without using the C++ map library.
// The courseNumber is used in the hash function.
// To store values we will use a hash function to determine what index each course will be stored at.
class HashTable {
    private:
        size_t tableSize = 0;
        static constexpr size_t maxChainLength = 5; // complie time constant. https://omegaup.com/docs/cpp/en/cpp/language/constexpr.html

        // A HashTable in C++ uses a vector as the table
        // To handle collisions, this is a vector of doubly linked lists
        // key is courseNumber. Value is a course object
        // std::vector is the vector for the hash table
        // std::list is a doubly linked list
        // std::pair is a class template that allows two values in a single object. In this case, a key (courseNumber) and a value (course object).
        //      https://en.cppreference.com/cpp/utility/pair
        std::vector<std::list<std::pair<std::string, Course>>> hashTable;


        // Hash function used to find table index
        // courseNumber (string) is the key
        size_t hashFunction(const std::string& key) const{
            std::hash<std::string> hasher;
            size_t hash_value = hasher(key);

            return hash_value % tableSize;
        }


        // Method to rebuild the hash table.
        // Inspired by https://www.geeksforgeeks.org/dsa/load-factor-and-rehashing/ and examples from Claude
        void rehash() {
            // Build new hash table double the size
            size_t newTableSize = tableSize * 2;
            std::vector<std::list<std::pair<std::string, Course>>> newHashTable(newTableSize);

            // Put data into new hash table. Insert is not used to prevent an infinite loop.
            // % newTableSize is used over hashFunction to use the new size for the hash
            for (const auto& bucket : hashTable) {
                for (const auto& pair : bucket) {
                    size_t index = std::hash<std::string>{}(pair.first) % newTableSize;
                    newHashTable[index].push_back(pair);
                }
            }
            // Swap in the new table
            tableSize = newTableSize;
            hashTable = std::move(newHashTable);
        }


    public:
        // Constructor
        HashTable(int size) {

            if (size <= 0){
                cerr << "Table Size must be positive. Defaulting to 10." << endl;
                size = 10;
            }
            tableSize = size;
    
            // Creates a vector of size tableSize
            hashTable.resize(tableSize);
        }


        // Get method for hash table. Returns a bool if key was found or not.
        // Result is the course object that can be accessed via pass by reference
        bool get(const std::string& key, Course& result) const {
            size_t index = hashFunction(key); // get index in vector

            // Look in the linked list for key. hashTable[index] is a linked list of pairs
            // O(1) if it's the first value in the linked list
            // Otherwise O(n) where n is the size of the linked list
            for (const auto& pair : hashTable[index]) {
                if (pair.first == key) {
                    result = pair.second;
                    return true; // Key was found
                }
            }
            return false; // the key was not found
        }


        // Insert method adds or updates a key value pair to the hash table.
        // This method does handle collisions using the doubly linked list.
        void insert(const std::string& key, const Course& course) {
            size_t index = hashFunction(key); // get index in vector

            // Go to the index the key returns, then search through the linked list to add or update a course
            // If the key already exists it will be updated. If not it will be appended to the closest index in the linked list
            for (auto& pair : hashTable[index]) {
                if (pair.first == key) {
                    pair.second = course;
                    return;
                }
            }
            // If not, it will be appended to the end of the linked list using the built-in emplace_back
            // emplace_back will automatically update the linked list's tail pointer
            hashTable[index].emplace_back(key, course);
            if (hashTable[index].size() > maxChainLength) {
                rehash();
            }
        }

        
        // This method loops every bucket in the hash table and adds it to a vector for printing.
        std::vector<Course> getAllCourses() const {
            std::vector<Course> courses;

            // Loops over table size and adds pair.second or course object to vector
            for (size_t i = 0; i < tableSize; ++i) {
                for (const auto& pair : hashTable[i]) { 
                    courses.push_back(pair.second); 
                }
            }
            return courses;
        }


        // Prints all courses in the hash table in no order
        // pair.second is the course object inside the hash table
        void printAll() const {
            // loop tableSize times to iterate over the entire hash table (the vector is the underlying data structure)
            for (size_t i = 0; i < tableSize; ++i) {
                // Loops through pairs in linked list. Outputs only course number and title
                for (const auto& pair : hashTable[i]) {
                    cout << pair.second.courseNumber << ", " << pair.second.title;

                    // If the course has prerequisites print them
                    if (!pair.second.prerequisites.empty()) {
                        cout << ", ";
                        for (size_t j = 0; j < pair.second.prerequisites.size(); ++j) {
                            cout << pair.second.prerequisites[j];
                            if (j < pair.second.prerequisites.size() - 1) { // avoids trailing comma
                                cout << ", ";
                            }
                        }
                    }
                    cout << endl;
                }
            }
            cout << endl;
        }
};


// trim method to remove whitespace so keys will match
// Inspired by https://cplusplus.com/forum/beginner/863/
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n"); // first non-whitespace

    // If the string is only whitespace return an empty string
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n"); // last non-whitespace
    return str.substr(start, end - start + 1);
}


// Used to verify prerequisites. All prerequisites should map to an existing courseNumber.
void validatePrerequisites(const HashTable& courses, const std::vector<Course>& allCourses) {
    Course tempCourse;
    for (const auto& course : allCourses) {
        for (const auto& prerequisite : course.prerequisites) {
            if (!courses.get(prerequisite, tempCourse)) {
                cerr << "Error: " << course.courseNumber << " has a prerequisite " << prerequisite
                     << " that is not found in the course list" << endl;
            }
        }
    }
}


//takes a string and splits it (by comma) into tokens
vector<string> getTokens(string &line) {
        std::vector<string> tokens;
        std::string token;
        std::istringstream ss(line); //treat the line string as an input stream called ss

        //splits the line by commas
        while (getline(ss, token, ',')) { 
            tokens.push_back(trim(token));
        }   
        return tokens; // return a vector of string tokens 
}

// Takes a CSV file path and stores the data in the hash table data structure. 
// Uses pass by reference to edit hash table, does not return any values.
void readFileAndStore(const string &fileName, HashTable& courses){

    std::string line; 

    // Constants to limit length of input. Application can handle 10k courses max. 
    // No course line should be longer than 500 characters.
    // No token should be longer than 100 characters.
    const size_t MAX_LINE_LENGTH = 500;
    const size_t MAX_TOKEN_LENGTH = 100;
    const size_t MAX_COURSES = 10000;
    size_t courseCount = 0; // keeps track of number of courses

    std::ifstream file(fileName); //open file

    // check if the file is open; if not, print an error and return
    if(!file.is_open()) {
        cerr << "Error: Cannot open file " << fileName << endl;
        return;
    }

    //If the file is open, read it line by line
    while (getline(file, line)) {

        // error handling for large file inputs
        if (line.size() > MAX_LINE_LENGTH) {
            cerr << "Error: Line too long, moving to next line." << endl;
            continue;
        }

        // If the application reaches the max number of courses, break the loop. See the const above.
        if (courseCount >= MAX_COURSES) {
            cerr << "Error: Too many courses." << endl;
            break;
        }

        // helps with formatting output
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }   
        
        std::vector<string> tokens = getTokens(line); //vector holding tokens for current line
        
        // If a token is larger than the max token length, break the loop. See the const above
        // This method will still load the line before rejecting it.
        bool tokenTooLong = false;
        for (const auto& token: tokens) {
            if (token.size() > MAX_TOKEN_LENGTH) {
                tokenTooLong = true;
                break;
            }
        }

        // if the line has fewer than two tokens or the token is too long, output an error.
        // Each line should have a title and a course number, and each token should be less than MAX_TOKEN_LENGTH characters.
        if (tokens.size() < 2 || tokenTooLong || tokens[0].empty() || tokens[1].empty()) {
            cerr << "Error: Line incorrectly formatted." << endl;
            continue;
        }

        //create a new course object
        Course course;

        //Using the tokens, add data to the course object
        course.courseNumber = tokens[0];
        course.title = tokens[1];

        //If there are more than 2 tokens, there are prerequisites
        if (tokens.size() > 2) {
            //After the course number and title, the rest of the line is prerequisites
            //Start at index 2 and loop until the end of the line, adding each token to the course.prerequisites vector
            for (size_t i = 2; i < tokens.size(); ++i) {
                if (!tokens[i].empty()) {
                    course.prerequisites.push_back(tokens[i]);
                }
            }
        }

        // Insert the course into the hash table using course number as the key
        courses.insert(course.courseNumber, course);
        ++courseCount;
    }

    //close the file
    file.close();

    return;
}


// Finds a course in the hash table using a course number and hash function (get method)
void searchCourse(const HashTable& courses, const std::string& courseNumber) { 
    Course course;
    bool found = courses.get(courseNumber, course); // uses hash function. get will return a course via reference

    // If the course was found using the hash table's get method
    if (found) {
        cout << course.courseNumber << ", " << course.title;

        // If the course has prerequisites
        if (!course.prerequisites.empty()) {
             cout << ", Prerequisites: ";
            for (size_t j = 0; j < course.prerequisites.size(); ++j) {
                cout << course.prerequisites[j];
                if (j < course.prerequisites.size() - 1) { // avoids trailing comma
                    cout << ", ";
                }
            }
        }
        cout << endl << endl;
    }
    // else if the course is not found
    else {
        cout << "Course not found. Try another course number." << endl;
        cout << "Enter the course in caps." << endl << endl;
    }
}


// insertion sort algorithm to sort the courses
// This algorithm was inspired by https://www.youtube.com/watch?v=JU767SDMDvA
// O(n^2) on first load and O(n) after
void insertionSort(std::vector<Course>& courses) {
    // loop courses.size() times
    for (size_t i = 1; i < courses.size(); ++i) {
        Course course = courses[i];
        size_t j = i;

        // shift to the right using overridden < operator
        while (j > 0 && course < courses[j-1]) {
            courses[j] = courses[j-1];
            --j;
        }
        courses[j] = course;
    }
}


// Print the hash table in alphanumeric order. Does not print prerequisites
void printCoursesAlphanumeric(vector<Course> &courses) {
    // sort the courses
    insertionSort(courses); // O(n^2) or O(n)
    // or
    // std::sort(courses.begin(), courses.end()); // O(n log n)

     cout << "Here is a sample schedule:" << endl << endl;
    //loop through the vector and print the sorted data
    for(const auto &course : courses) {
        cout << course.courseNumber << ", " << course.title << endl;
    }
    cout << endl;
} 


//Main function holding the program loop and menu
int main() {
    const std::string FILE_NAME = "CS 300 ABCU_Advising_Program_Input.csv";
    vector<Course> coursesVector;
    int userChoice = 0;
    std::string courseNumber; // string variable for the user to search for a course

    // Course hash table
    HashTable courses(10);

    //create the menu using a switch statement
    while (userChoice != 9) {
        cout << "Menu:" << endl;
        cout << "  1. Load Data" << endl;
        cout << "  2. Display Sorted Course List" << endl;
        cout << "  3. Print Course" << endl;
        cout << "  4. Display All" << endl;
        cout << "  9. Exit" << endl << endl;
        cout << "What would you like to do? ";
        cin >> userChoice;

        // Validate input
        if (cin.fail()) {
            cin.clear(); // Clear error flag
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard input
            cout << "Please enter a number from the menu." << endl;
            continue; // Restart loop
        }

        switch(userChoice) { 

            // 1. Load the file data into the data structure
            // Prepare vector for sorted printing and verify prerequisites  
            case 1:
                std::cout <<  "Loading File.... " << endl;
                readFileAndStore(FILE_NAME, courses);
                coursesVector = courses.getAllCourses();
                 validatePrerequisites(courses, coursesVector);
                std::cout <<  "File has been loaded." << endl;
                break;

            //2. Print an alphanumeric list of all the courses
            case 2:
                printCoursesAlphanumeric(coursesVector);
                break;

            //3. Print the course title and the prerequisites for any individual course
            case 3:
                //ask the user for a course number
                cout << "What course do you want to know about? ";
                cin >> courseNumber;
                cout << endl;
                searchCourse(courses, courseNumber);
                break;
            
            //4. Print all courses not sorted
            case 4:
                courses.printAll();
                break;

            //9. Exit the program
            case 9:
            cout << "Thank you for using the course planner!" << endl << endl;
            break;

            // If the user enters an invalid menu choice
            default:
                cout << userChoice << " is not a valid option." << endl;
        }
    }
    return 0;
}