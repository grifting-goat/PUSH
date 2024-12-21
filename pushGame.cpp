//i got the idea for a an advent of code thing, now i have a game where you must squish boxes into the walls and try to maximize your score
//WASD to move, esc to escape

#pragma comment(lib, "User32.lib")

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <Windows.h>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <limits>

using namespace std;

//has function of a pair(used for unordered set)
template <typename T1, typename T2>
struct pair_hash {
    size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

//build the map from a file
bool mapBuilder(string fileName, vector<vector<char>> &mat);

//check if a move is valid
bool isValid(int i, int j, pair<int,int> command, vector<vector<char>> &map, int &recs);

//move a box up or down
void moveBox(int i, int j, bool up,vector<vector<char>> &map);

//run a command
void runCom(pair <int, int> com, vector<vector<char>> &mat);

//handle inputs
void handleInput(char input, pair<int,int> &commands);

//turn the map into a cool screen
void map2screen(wchar_t *frame, vector<vector<char>> mat, HANDLE hCmd);

//check to see if a suficient stack has been made
void checkStacks(pair<int,int> com, vector<vector<char>> &mat);

//initial info // oooh no dont use global variables
int pR = 0;
int pC = 0;
int score = 0;
int maxScore = 0;
bool shutDown = 0;

auto tp0 = chrono::system_clock::now();
auto tp1 = chrono::system_clock::now();
auto tp2 = chrono::system_clock::now();


int main() {

    string fileName = "";
    string folderPath = "./";
    vector<vector<char>> mat; //store the map
    vector<string> files;

    //starting sequence
    cout << "Welcome to PUSH!\n\n";
    Sleep(1500);

    //meta game loop
    do {
        do {
            cout << "Press enter to use the standard map.\nIf you would like to use a custom map, type the index of the name below:\n";
            int index = 0;
            try { //printing out the .txt files in the folder
                for (const auto& entry : filesystem::directory_iterator(folderPath)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                        files.push_back(entry.path().filename().string());
                        cout << "File: " << index++ << "    " << entry.path().filename() << "\n";
                    }
                }
            }
            catch(const filesystem::filesystem_error& e) {
                cerr << "Error: " << e.what() << "\n";
            }

            while(!getline(cin, fileName)) {continue;}
            try { //make sure they typed an integer or nothing
                if (fileName == "") {fileName = "map.txt";}
                else if (stoi(fileName) < files.size() && stoi(fileName) >= 0) {fileName = files[stoi(fileName)];}
                else {fileName = "bad";}
            }
            catch (const std::invalid_argument& e) {continue;}

        }
        while (!mapBuilder(folderPath + fileName, mat));
        

        // Create Frame // Get Windows dimensions  // goofy windows stuff
        HANDLE hCmd = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
        SetConsoleActiveScreenBuffer(hCmd);
        DWORD dwBytesWritten = 0;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hCmd, &csbi);

        int terminalWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int terminalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        wchar_t *frame = new wchar_t[terminalWidth * terminalHeight];

        //inital frame
        map2screen(frame, mat, hCmd);
        frame[terminalWidth * terminalHeight - 1] = '\0'; //Ensure null-terminated bc windows
        WriteConsoleOutputCharacterW(hCmd, frame, terminalWidth * terminalHeight, {0, 0}, &dwBytesWritten);

        //track command
        pair<int, int> command;

        //handle timing 
        tp0 = chrono::system_clock::now();
        tp1 = chrono::system_clock::now();
        tp2 = chrono::system_clock::now();

        //internal game loop
        while(true) { 
            //check inputs
            if      (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {shutDown = 1; break;}
            if      (GetAsyncKeyState(VK_BACK) & 0x8000) {break;}
            else if (GetAsyncKeyState('W') & 0x8000) {handleInput('W', command);}
            else if (GetAsyncKeyState('A') & 0x8000) {handleInput('A', command);}
            else if (GetAsyncKeyState('S') & 0x8000) {handleInput('S', command);}
            else if (GetAsyncKeyState('D') & 0x8000) {handleInput('D', command);}
            else { 
                continue;
            }

            //handle more timing
            tp2 = chrono::system_clock::now();
            chrono::duration<float> elapsedTime = tp2 - tp1;
            float fTick = elapsedTime.count();

            //cool trick
            if (fTick >= 0.11f) { // movement cooldown
                runCom(command, mat); //run the command
                checkStacks(command, mat); //check if any boxes have been squished
                tp1 = tp2; //reset the timer
            }

            //windows buffer stuff
            map2screen(frame, mat, hCmd);
            frame[terminalWidth * terminalHeight - 1] = '\0';
            WriteConsoleOutputCharacterW(hCmd, frame, terminalWidth * terminalHeight, {0, 0}, &dwBytesWritten);
        }
        //post running cleanup
        CloseHandle(hCmd);
        delete[] frame;
        mat.clear();
        cout << endl << fileName.substr(0, fileName.size()-4) << " has been stopped with a score of " << score << "\n\n";
        score = 0;
    }
    while(!shutDown);

    return 0;
}

//check if a push is valid
bool isValid(int i, int j, pair<int,int> command, vector<vector<char>> &map, int &recs) {
    if (i < 0 || i >= map.size() || j < 0 || j >= map[0].size()) {
        return 0;
    }
    if(map[i][j] == '#') {
        return 0;
    }
    if(map[i][j] == '.'){
        return 1;
    }

    if (command.first == 0){
        if(map[i][j] == '[' || map[i][j] == ']') {
            recs+=1;
            return isValid(i + command.first,j + command.second, command,map, recs);
        }
    }
    else {
        if(map[i][j] == '[') {
            recs+=1;
            return isValid(i + command.first,j + command.second, command,map, recs) && isValid(i + command.first, j + command.second + 1, command,map, recs);
        }

        if(map[i][j] == ']') {
            recs+=1;
            return isValid(i + command.first,j + command.second, command,map, recs) && isValid(i + command.first, j + command.second - 1, command,map, recs);
        }
    }
    return 0;
}

//run a given command
void runCom(pair <int, int> com, vector<vector<char>> &mat) {
        int boxCount = 0;
        if (isValid(pR + com.first, pC + com.second, com, mat, boxCount)) {
            pR += com.first;
            pC += com.second;
            if (com.first == 0) {
                for (int n = 1; n <= boxCount; n++) { // for left and right
                    if (com.second == 1)
                        mat[pR + com.first*n][pC + com.second*n] = ']' -(2*(n%2));
                    else {
                        mat[pR + com.first*n][pC + com.second*n] = '[' + (2*(n%2));
                    }
                }
                if (mat[pR][pC] == '[' || mat[pR][pC] == ']') {
                    mat[pR][pC] = '.';
                }
            }
            else { // for up and down
                if (mat[pR][pC] == '[' || mat[pR][pC] == ']') {
                    moveBox(pR, pC, (com.first == -1), mat);
                }
                
            }
        }
    }

//move a box up or down
void moveBox(int i, int j, bool up,vector<vector<char>> &map) {
    queue<pair<int,int>> q;
    short dir = up ? -1 : 1;
    q.push({i,j}); //
    vector<pair<int, int>> boxIndex;
    unordered_set<pair<int, int>, pair_hash<int, int>> visited;
    int n = 0;

    while(!q.empty()) {
        pair <int, int> test = q.front(); q.pop();
        

        if (map[test.first][test.second] == '[' || map[test.first][test.second] == ']') {
            if(visited.find(test) == visited.end()) { //if not visited
                int trash = 0;
                if (map[test.first][test.second] == '[' 
                && map[test.first+dir][test.second] != '#' && map[test.first+dir][test.second+1] != '#') {
                    boxIndex.push_back(test);
                    q.push({test.first, test.second + 1});
                }
                
                if (map[test.first][test.second] == ']')
                    q.push({test.first, test.second - 1});
                
                q.push({test.first+dir, test.second});
            }
        }

        visited.insert(test);
    }

    //update the map
    for (auto coor : boxIndex) {
        if  (map[coor.first][coor.second] == '[' || map[coor.first][coor.second] == ']') {
            map[coor.first][coor.second] = '.';
        }
        if  (map[coor.first][coor.second+1] == '[' || map[coor.first][coor.second+1] == ']') {
            map[coor.first][coor.second+1] = '.';
        }
        
    }
    for (auto coor : boxIndex) {
        map[coor.first + dir][coor.second] = '[';
        map[coor.first + dir][coor.second+1] = ']';
    }


}

//set the commands based on input
void handleInput(char input, pair<int,int> &commands) {
    switch (input) {
        case 'W': commands = {-1,0}; break;
        case 'A': commands = {0,-1}; break;
        case 'S': commands = {1,0}; break;
        case 'D': commands = {0,1}; break;
        default: break;
    }
}

//use windows.h to print the map to screen
void map2screen(wchar_t *frame, vector<vector<char>> mat, HANDLE hCmd) {
    //get the size of the tterminal so can center
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hCmd, &csbi);
    int terminalWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int terminalHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    //get the map rows and cols
    int mapHeight = mat.size();
    int mapWidth = mat[0].size();

    // find where to start so as to center it
    int startRow = max(0, (terminalHeight - mapHeight) / 2);
    int startCol = max(0, (terminalWidth - mapWidth) / 2);

    //make spaces
    int frameSize = terminalWidth * terminalHeight;
    fill(frame, frame + frameSize, L' ');

    //put the map in the center
    for (int i = 0; i < min(mapHeight, terminalHeight); i++) {
        for (int j = 0; j < min(mapWidth, terminalWidth); j++) {
            frame[(startRow + i) * terminalWidth + (startCol + j)] = mat[i][j];
        }
    }
    string scr = "score: " + to_string(score) + "/" + to_string(maxScore);
    for (int i = 0; i < scr.length(); i++) {
        frame[i] = scr[i];
    }
    
    /* future timing block
    chrono::duration<float> elapsedTime = tp2 - tp0;
    float time = elapsedTime.count();
    string timestr = "time moving: " + to_string(time);
    for (int i = 0; i < scr.length(); i++) {
        frame[i+terminalWidth] = timestr[i];
    }
    */

    frame[(startRow + pR) * terminalWidth + (startCol + pC)] = '@';
    
}


//build the map from a input file
bool mapBuilder(string fileName, vector<vector<char>> &mat) {
    ifstream file(fileName);
    if (!file.is_open()) {
        cerr << "Error: Could not open file.\n";
        return 0;
    }
    string line;
    //get map
    while (getline(file, line)) {
        vector<char> row;  
        for (auto c : line) {
            
            if (c == '@') {
                pC = row.size();
                pR = mat.size();
                row.push_back('.');
                row.push_back('.');
            }
            else if (c == '.') {
                row.push_back(c);
                row.push_back(c);
            }
            else if (c == 'O') {
                row.push_back('[');
                row.push_back(']');
                maxScore++;
            }
            else if (c == '#') {
                row.push_back(c);
                row.push_back(c);
            }
            else {
                row.push_back(' ');
                row.push_back(' ');
            }

        }
        // check if its empty
        if (row.size() == 0) {break;}

        //add row to vector
        mat.push_back(row);
    }
    
    return 1;
}

//check if there is a valid stack of boxes squished
void checkStacks(pair<int,int> com, vector<vector<char>> &mat) {
    queue<pair<int,int>> q;
    q.push({pR+com.first,pC+com.second}); //
    unordered_set<pair<int, int>, pair_hash<int, int>> visited;
    unordered_set<pair<int, int>, pair_hash<int, int>> pushedBoxes;
    int n = 0; // in line pushed boxes
    bool squished = 0;
 
    while(!q.empty()) {
        pair <int, int> test = q.front(); q.pop();

        if (mat[test.first][test.second] == '[' || mat[test.first][test.second] == ']') {
            if(visited.find(test) == visited.end()) {
                n++;
                pushedBoxes.insert(test);
                q.push({test.first+com.first,test.second+com.second});
            }
        }
        else if (mat[test.first][test.second] == '#') {
            squished = 1;
        }
        else {
            return;
        }
        visited.insert(test);
    }
    n = com.first == 0 ? n/2 : n;
    if (n > 4 && squished) {
        for (auto &p : pushedBoxes) {
            if (mat[p.first][p.second] == '[') {
                mat[p.first][p.second] = '.';
                mat[p.first][p.second+1] = '.';
            }
            if (mat[p.first][p.second] == ']') {
                mat[p.first][p.second] = '.';
                mat[p.first][p.second-1] = '.';
            }
                
        }
        
        score += n;
    }
    
}
