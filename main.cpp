//Created by Jakub Lewandowski

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <stack>
#include <algorithm>

using namespace std;

const int MAX_BOARD_SIZE = 10;
const char EMPTY_SPACE = '.';
const char SHOT_PATH = '*';
const char DIRECT_HIT = 'X';
const char REFLECTION_HIT = 'Z';
const char ATOM = 'H';

//Front-End

//Czyszczenie ekranu
void clearScreen() {
#if defined(_WIN32)
    system("cls");
#else
    system("clear");
#endif
}

void displayMenu() {
    clearScreen();
    cout << ".---. .-.               .-.     .---.\n";
    cout << ": .; :: :               : :.-.  : .; :\n";
    cout << ":   .': :   .--.   .--. : `'.'  :   .' .--. .-.,-.\n";
    cout << ": .; :: :_ ' .; ; '  ..': . `.  : .; :' .; :`.  .'\n";
    cout << ":___.'`.__;`.__,_;`.__.':_;:_;  :___.'`.__.':_,._;\n";
    cout << endl;
    cout << "Jakub Lewandowski 193142 - Na podstawie gry Erica Solomona\n\n";
    cout << "1. Start gry\n";
    cout << "2. Wyjscie z gry\n";
    cout << "3. Pomoc\n";
    cout << "Wybierz opcje: ";
}

void displayHelp() {
    clearScreen();
    cout << "Pomoc - Zasady Gry Blackbox:\n";
    cout << "Znajdz wszystkie atomy, aby wygrac!\n";
    cout << "Znaki na krawedzi ekranu to podpowiedz czy laser jest strzalem prosto w atom czy strzalem zakrzywionym.\n";
    cout << "X - strzal prosto w atom, Z - strzal ktory po przekatnej ma atom.\n";
    cout << "Spis komend:\n";
    cout << "w - gora\n";
    cout << "a - lewo\n";
    cout << "s - dol\n";
    cout << "d - prawo\n";
    cout << "p - strzelanie\n";
    cout << "u - cofnij\n";
    cout << "r - powtorz (cofniety ruch)\n";
    cout << "q - wyjscie do menu\n";
    cout << "k - zakonczenie gry\n";
    cout << "h - 'pomoc', na 2 sekundy wyswietla pozostale ukryte atomy\n";
    // Tutaj reszta komend
    cout << "\nWcisnij dowolny klawisz, aby wrocic do menu...";
    cin.ignore(); cin.get();
}

//Back-End

struct GameState {
    char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    char edgeHits[4];
    int cursorX, cursorY;
};

stack<GameState> undoStack;
stack<GameState> redoStack;

void saveState(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], char edgeHits[4], int cursorX, int cursorY) {
    GameState currentState;
    copy(&board[0][0], &board[0][0] + MAX_BOARD_SIZE*MAX_BOARD_SIZE, &currentState.board[0][0]);
    copy(&edgeHits[0], &edgeHits[4], &currentState.edgeHits[0]);
    currentState.cursorX = cursorX;
    currentState.cursorY = cursorY;
    undoStack.push(currentState);
}

void undo(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], char edgeHits[4], int &cursorX, int &cursorY) {
    if (!undoStack.empty()) {
        GameState previousState = undoStack.top();
        undoStack.pop();

        redoStack.push(previousState);

        copy(&previousState.board[0][0], &previousState.board[0][0] + MAX_BOARD_SIZE*MAX_BOARD_SIZE, &board[0][0]);
        copy(&previousState.edgeHits[0], &previousState.edgeHits[4], &edgeHits[0]);

        cursorX = previousState.cursorX;
        cursorY = previousState.cursorY;
    }
}

void redo(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], char edgeHits[4], int &cursorX, int &cursorY) {
    if (!redoStack.empty()) {
        GameState nextState = redoStack.top();
        redoStack.pop();

        undoStack.push(nextState);

        copy(&nextState.board[0][0], &nextState.board[0][0] + MAX_BOARD_SIZE*MAX_BOARD_SIZE, &board[0][0]);
        copy(&nextState.edgeHits[0], &nextState.edgeHits[4], &edgeHits[0]);

        cursorX = nextState.cursorX;
        cursorY = nextState.cursorY;
    }
}

bool isOnEdge(int x, int y, int boardSize) {
    return x == 0 || x == boardSize - 1 || y == 0 || y == boardSize - 1;
}

void shootRay(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int boardSize, int &x, int &y, int &dx, int &dy, char edgeHits[4], int edgeIndex) {
    bool hitAtom = false;
    bool reflection = false;
    int startX = x, startY = y;

    while (x >= 0 && x < boardSize && y >= 0 && y < boardSize) {
        if (board[x][y] == ATOM || board[x][y] == 'A') {  // Zatrzymaj laser na ukrytym lub odkrytym atomie
            hitAtom = true;
            break;
        }

        if ((dx != 0 && ((y > 0 && (board[x][y - 1] == ATOM || board[x][y - 1] == 'A')) ||
                         (y < boardSize - 1 && (board[x][y + 1] == ATOM || board[x][y + 1] == 'A')))) ||
            (dy != 0 && ((x > 0 && (board[x - 1][y] == ATOM || board[x - 1][y] == 'A')) ||
                         (x < boardSize - 1 && (board[x + 1][y] == ATOM || board[x + 1][y] == 'A'))))) {
            reflection = true;
            break;
        }

        board[x][y] = SHOT_PATH;
        x += dx;
        y += dy;
    }

    if (hitAtom) {
        edgeHits[edgeIndex] = DIRECT_HIT;
    } else if (reflection) {
        edgeHits[edgeIndex] = REFLECTION_HIT;
    }

    x = startX;
    y = startY;
}

void determineShotDirection(int startX, int startY, int boardSize, int &dx, int &dy, int &edgeIndex) {
    dx = dy = 0;
    edgeIndex = -1;

    if (startX == 0) {
        dx = 1;
        edgeIndex = 3;
    } else if (startX == boardSize - 1) {
        dx = -1;
        edgeIndex = 1;
    } else if (startY == 0) {
        dy = 1;
        edgeIndex = 0;
    } else if (startY == boardSize - 1) {
        dy = -1;
        edgeIndex = 2;
    }
}

void initializeBoard(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int boardSize) {
    for (int i = 0; i < boardSize; i++) {
        for (int j = 0; j < boardSize; j++) {
            board[i][j] = EMPTY_SPACE;
        }
    }
}

void placeAtoms(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int boardSize, int numOfAtoms) {
    srand(time(NULL));
    for (int i = 0; i < numOfAtoms; i++) {
        int x, y;
        bool isNearAnotherAtom;
        do {
            x = rand() % (boardSize - 2) + 1;
            y = rand() % (boardSize - 2) + 1;
            isNearAnotherAtom = false;

            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < boardSize && ny >= 0 && ny < boardSize && board[nx][ny] == ATOM) {
                        isNearAnotherAtom = true;
                        break;
                    }
                }
                if (isNearAnotherAtom) {
                    break;
                }
            }
        } while (board[x][y] == ATOM || isNearAnotherAtom);

        board[x][y] = ATOM;
    }
}

void displayBoard(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int boardSize, const char edgeHits[4], int cursorX, int cursorY, bool revealAtoms = false) {
    cout << "+";
    for (int j = 0; j < boardSize; j++) {
        cout << (j == 0 && edgeHits[0] != EMPTY_SPACE ? edgeHits[0] : '-');
        cout << "-";
    }
    cout << "+\n";

    for (int i = 0; i < boardSize; i++) {
        cout << (i == 0 && edgeHits[3] != EMPTY_SPACE ? edgeHits[3] : '|');
        for (int j = 0; j < boardSize; j++) {
            if (revealAtoms && board[i][j] == ATOM) { // Wyświetl atomy jeśli revealAtoms jest true
                cout << ATOM << " ";
            } else if (i == cursorX && j == cursorY) {
                cout << 'O' << " "; // Wyświetl kursor
            } else if (board[i][j] == ATOM) { // Ukryj atomy
                cout << EMPTY_SPACE << " ";
            } else {
                cout << board[i][j] << " "; // Wyświetl pozostałe elementy
            }
        }
        cout << (i == boardSize - 1 && edgeHits[1] != EMPTY_SPACE ? edgeHits[1] : '|');
        cout << "\n";
    }

    cout << "+";
    for (int j = 0; j < boardSize; j++) {
        cout << (j == boardSize - 1 && edgeHits[2] != EMPTY_SPACE ? edgeHits[2] : '-');
        cout << "-";
    }
    cout << "+\n";
}

int countRemainingAtoms(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int boardSize) {
    int remaining = 0;
    for (int i = 1; i < boardSize - 1; i++) {
        for (int j = 1; j < boardSize - 1; j++) {
            if (board[i][j] == ATOM) {  // Zliczamy tylko ukryte atomy
                remaining++;
            }
        }
    }
    return remaining;
}

void guessAtom(char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE], int boardSize, int cursorX, int cursorY, int &remainingAtoms) {
    if (board[cursorX][cursorY] == ATOM) {
        cout << "Trafiony atom! ";
        board[cursorX][cursorY] = 'A'; // Odkrywamy atom
        remainingAtoms--;
    } else {
        cout << "Brak atomu w tym miejscu. ";
    }
    cout << "Pozostalo atomow: " << remainingAtoms << endl;
    this_thread::sleep_for(chrono::seconds(2));  // Dajemy graczowi czas na przeczytanie komunikatu
}

int main() {
    char board[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
    char edgeHits[4] = {EMPTY_SPACE, EMPTY_SPACE, EMPTY_SPACE, EMPTY_SPACE};
    int boardSize, numOfAtoms;
    int cursorX, cursorY;
    int remainingAtoms;

    while (true) {
        displayMenu();
        int menuChoice;
        cin >> menuChoice;

        if (menuChoice == 1) {
            cout << "Wybierz opcje planszy: \n";
            cout << "1 - Plansza 5x5 z 3 atomami\n";
            cout << "2 - Plansza 8x8 z 5 atomami\n";
            cout << "3 - Plansza 10x10 z 8 atomami\n";
            cout << "Twoj wybor: ";
            cin >> menuChoice;

            switch (menuChoice) {
                case 1:
                    boardSize = 5;
                    numOfAtoms = 3;
                    cursorX = 2;
                    cursorY = 2;
                    break;
                case 2:
                    boardSize = 8;
                    numOfAtoms = 5;
                    cursorX = 4;
                    cursorY = 4;
                    break;
                case 3:
                    boardSize = 10;
                    numOfAtoms = 8;
                    cursorX = 5;
                    cursorY = 5;
                    break;
                default:
                    cout << "Nieprawidlowy wybor. Sprobuj ponownie.\n";
                    continue;
            }

            initializeBoard(board, boardSize);
            placeAtoms(board, boardSize, numOfAtoms);
            remainingAtoms = numOfAtoms;

            while (true) {
                clearScreen();
                displayBoard(board, boardSize, edgeHits, cursorX, cursorY);

                cout << "Wpisz komende: ";
                char command;
                cin >> command;

                bool isActionCommand = (command != 'u' && command != 'r');

                if (command == 'q') {
                    break;
                } else if (command == 'w' && cursorX > 0) {
                    saveState(board, edgeHits, cursorX, cursorY);
                    cursorX--;
                } else if (command == 's' && cursorX < boardSize - 1) {
                    saveState(board, edgeHits, cursorX, cursorY);
                    cursorX++;
                } else if (command == 'a' && cursorY > 0) {
                    saveState(board, edgeHits, cursorX, cursorY);
                    cursorY--;
                } else if (command == 'd' && cursorY < boardSize - 1) {
                    saveState(board, edgeHits, cursorX, cursorY);
                    cursorY++;
                } else if (command == 'p' && isOnEdge(cursorX, cursorY, boardSize)) {
                    saveState(board, edgeHits, cursorX, cursorY);
                    int dx, dy, edgeIndex;
                    determineShotDirection(cursorX, cursorY, boardSize, dx, dy, edgeIndex);
                    shootRay(board, boardSize, cursorX, cursorY, dx, dy, edgeHits, edgeIndex);
                } else if (command == 'o') {
                    saveState(board, edgeHits, cursorX, cursorY);
                    guessAtom(board, boardSize, cursorX, cursorY, remainingAtoms);
                    if (remainingAtoms == 0) {
                        clearScreen();
                        cout << "Gratulacje! Wygrales gre, odgadujac wszystkie atomy!" << endl;
                        this_thread::sleep_for(chrono::seconds(2));
                        break;
                    }
                } else if (command == 'h') {
                    clearScreen();
                    displayBoard(board, boardSize, edgeHits, cursorX, cursorY, true); // Wywołanie funkcji z revealAtoms = true
                    this_thread::sleep_for(chrono::seconds(2));
                    continue;
                } else if (command == 'k') {
                    clearScreen();
                    cout << "Gra zostala zakonczona. Dziekujemy za gre!" << endl;
                    this_thread::sleep_for(chrono::seconds(2));
                    exit(0); // Wyjście z pętli gry
                } else if (command == 'u') {
                    undo(board, edgeHits, cursorX, cursorY);
                } else if (command == 'r') {
                    redo(board, edgeHits, cursorX, cursorY);
                }

                // Czyszczenie redoStack po każdym ruchu, który nie jest ani undo ani redo
                if (isActionCommand) {
                    while (!redoStack.empty()) redoStack.pop();
                }
            }
        } else if (menuChoice == 2) {
            break;
        } else if (menuChoice == 3) {
            displayHelp();
        } else {
            cout << "Nieprawidlowy wybor. Sprobuj ponownie.\n";
        }
    }
    return 0;
}
