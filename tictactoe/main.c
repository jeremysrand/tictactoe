/*
 * main.c
 * tictactoe
 *
 * Created by Jeremy Rand on 2019-08-09.
 * Copyright (c) 2019 Jeremy Rand. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <apple2enh.h>


typedef unsigned char tUint8;
typedef signed char tInt8;
typedef unsigned int tUint16;
typedef signed int tInt16;

typedef tInt8 tPos;
typedef tUint8 tBoolean;
typedef tInt8 tPlayer;
typedef tInt8 tScore;
typedef tUint16 tState;


#define BOARD_DIMENSIONS 3
#define BOARD_SQUARES (BOARD_DIMENSIONS * BOARD_DIMENSIONS)
#define VERBOSE 1

#define OTHER_PLAYER(PLAYER) ((PLAYER == 1) ? -1 : 1)

#define TRUE 1
#define FALSE 0

#define MAX_HASH 256
#define MAX_ROWS_IN_HASH 10

/* Maps a position to its X and Y coordinate */
tPos gXpos[BOARD_SQUARES];
tPos gYpos[BOARD_SQUARES];

/* Maps an X and Y coordinate to a position */
tPos gXYtoPos[BOARD_DIMENSIONS][BOARD_DIMENSIONS];

/* Value is 1 if a position is on a diagonal, 0 otherwise */
tBoolean gDiag1[BOARD_SQUARES];
tBoolean gDiag2[BOARD_SQUARES];


/* Powers of three for a position */
tUint16 gThrees[BOARD_SQUARES];


typedef enum {
    NULL_SYMMETRY,
    LEFT_RIGHT_SYMMETRY,
    TOP_BOTTOM_SYMMETRY,
    FORWARD_SLASH_SYMMETRY,
    BACKWARD_SLASH_SYMMETRY,
    TURN_90_SYMMETRY,
    TURN_180_SYMMETRY,
    TURN_270_SYMMETY,
    MAX_SYMMETRY
} tSymmetry;


tPos gSymmetryPos[MAX_SYMMETRY][BOARD_SQUARES];
tPos gRevSymmetryPos[MAX_SYMMETRY][BOARD_SQUARES];

typedef struct tBoardType {
    tPlayer squares[BOARD_SQUARES];
} tBoardType;


typedef struct tCacheEntry {
    tState boardState;
    tPos bestMove;
    tScore score;
} tCacheEntry;


tCacheEntry gCache[MAX_HASH][MAX_ROWS_IN_HASH];


void initGame(void)
{
    tPos pos;
    
    memset(gCache, 0, sizeof(gCache));
    
    for (pos = 0; pos < BOARD_SQUARES; pos++) {
        tPos x, y;
        
        x = (pos % BOARD_DIMENSIONS);
        gXpos[pos] = x;
        
        y = (pos / BOARD_DIMENSIONS);
        gYpos[pos] = y;
        
        gXYtoPos[x][y] = pos;
        
        if (x == y) {
            gDiag1[pos] = TRUE;
        } else {
            gDiag1[pos] = FALSE;
        }
        
        if ((x+y) == (BOARD_DIMENSIONS - 1)) {
            gDiag2[pos] = 1;
        } else {
            gDiag2[pos] = 0;
        }
        
        if (pos == 0) {
            gThrees[pos] = 1;
        } else {
            gThrees[pos] = 3 * gThrees[pos - 1];
        }
    }
    
    for (pos = 0; pos < BOARD_SQUARES; pos++) {
        tPos x = gXpos[pos];
        tPos y = gYpos[pos];
        tPos newPos;
        tSymmetry sym;
        
        for (sym = NULL_SYMMETRY; sym < MAX_SYMMETRY; sym++) {
            switch (sym) {
                case NULL_SYMMETRY:
                    newPos = pos;
                    break;
                    
                case LEFT_RIGHT_SYMMETRY:
                    newPos = gXYtoPos[BOARD_DIMENSIONS - 1 - x][y];
                    break;
                    
                case TOP_BOTTOM_SYMMETRY:
                    newPos = gXYtoPos[x][BOARD_DIMENSIONS - 1 - y];
                    break;
                    
                case FORWARD_SLASH_SYMMETRY:
                    newPos = gXYtoPos[BOARD_DIMENSIONS - 1 - y][BOARD_DIMENSIONS - 1 - x];
                    break;
                    
                case BACKWARD_SLASH_SYMMETRY:
                    newPos = gXYtoPos[y][x];
                    break;
                    
                case TURN_90_SYMMETRY:
                    newPos = gXYtoPos[y][BOARD_DIMENSIONS - 1 - x];
                    break;
                    
                case TURN_180_SYMMETRY:
                    newPos = gXYtoPos[BOARD_DIMENSIONS - 1 - x][BOARD_DIMENSIONS - 1 - y];
                    break;
                    
                case TURN_270_SYMMETY:
                    newPos = gXYtoPos[BOARD_DIMENSIONS - 1 - y][x];
                    break;
                    
                default:
                    break;
            }
            gSymmetryPos[sym][pos] = newPos;
            gRevSymmetryPos[sym][newPos] = pos;
        }
    }
}


void emptyBoard(tBoardType *board)
{
    tPos pos;
    
    for (pos = 0; pos < BOARD_SQUARES; pos++) {
        board->squares[pos] = 0;
    }
}


tUint8 getEmptyPositions(tBoardType *board, tPos *positions)
{
    tPos pos;
    tUint8 numPositions = 0;
    tPlayer *squares = board->squares;
    
    for (pos = 0; pos < BOARD_SQUARES; pos++) {
        if (squares[pos] == 0) {
            if (positions != NULL) {
                positions[numPositions] = pos;
            }
            numPositions++;
        }
    }
    return numPositions;
}


tState getBoardState(tBoardType *board, tSymmetry *bestSym)
{
    tState bestState = -1;
    tPlayer *squares = board->squares;
    tSymmetry sym;
    
    for (sym = NULL_SYMMETRY; sym < MAX_SYMMETRY; sym++) {
        tState state = 0;
        tPos pos;
        for (pos = 0; pos < BOARD_SQUARES; pos++) {
            tPos symPos = gSymmetryPos[sym][pos];
            switch (squares[symPos]) {
                case 1:
                    state += gThrees[symPos];
                    break;
                case -1:
                    state += (gThrees[symPos] << 1);
                    break;
                default:
                    break;
            }
        }
        
        if ((bestState == -1) ||
            (state < bestState)) {
            *bestSym = sym;
            bestState = state;
        }
        
    }
    return bestState;
}


void setCachedBestMove(tState boardState, tSymmetry sym, tPos move, tScore score)
{
    tUint8 hash;
    tCacheEntry *hashEntry;
    tCacheEntry *hashEnd;
    
    if (boardState == 0)
        return;
    
    hash = (boardState & 0xff);
    hashEntry = &(gCache[hash][0]);
    hashEnd = &(hashEntry[MAX_ROWS_IN_HASH]);
    
    move = gSymmetryPos[sym][move];
    
    for (; hashEntry < hashEnd; hashEntry++) {
        if (hashEntry->boardState == boardState) {
            return;
        }
        if (hashEntry->boardState == 0) {
            hashEntry->boardState = boardState;
            hashEntry->bestMove = move;
            hashEntry->score = score;
            return;
        }
    }
}


tPos getCachedBestMove(tState boardState, tSymmetry sym, tScore *score)
{
    tUint8 hash;
    tCacheEntry *hashEntry;
    tCacheEntry *hashEnd;
    
    if (boardState == 0)
        return -1;
    
    hash = (boardState & 0xff);
    hashEntry = &(gCache[hash][0]);
    hashEnd = &(hashEntry[MAX_ROWS_IN_HASH]);
    
    for (; hashEntry < hashEnd; hashEntry++) {
        if (hashEntry->boardState == 0)
            return -1;
        if (hashEntry->boardState == boardState) {
            *score = hashEntry->score;
            return(gRevSymmetryPos[sym][hashEntry->bestMove]);
        }
    }
    return -1;
}


void setPosition(tPlayer player, tPos pos, tBoardType *board)
{
    board->squares[pos] = player;
}


tPlayer checkForWinner(tPlayer player, tPos pos, tBoardType *board)
{
    tPos xPos = gXpos[pos];
    tPos yPos = gYpos[pos];
    tPlayer *squares = board->squares;
    tPos x, y;
    tPlayer result;
    
    /* Check row */
    result = player;
    for (x = 0; x < BOARD_DIMENSIONS; x++) {
        tPos newPos = gXYtoPos[x][yPos];
        if (squares[newPos] != player) {
            result = 0;
            break;
        }
    }
    if (result != 0) {
        return result;
    }
    
    /* Check column */
    result = player;
    for (y = 0; y < BOARD_DIMENSIONS; y++) {
        tPos newPos = gXYtoPos[xPos][y];
        if (squares[newPos] != player) {
            result = 0;
            break;
        }
    }
    if (result != 0) {
        return result;
    }
    
    /* Check first diagonal */
    if (gDiag1[pos]) {
        result = player;
        for (x = 0; x < BOARD_DIMENSIONS; x++) {
            tPos newPos = gXYtoPos[x][x];
            if (squares[newPos] != player) {
                result = 0;
                break;
            }
        }
        if (result != 0) {
            return result;
        }
    }
    
    /* Check second diagonal */
    if (gDiag2[pos]) {
        result = player;
        for (x = 0; x < BOARD_DIMENSIONS; x++) {
            tPos newPos;
            
            y = BOARD_DIMENSIONS - 1 - x;
            newPos = gXYtoPos[x][y];
            if (squares[newPos] != player) {
                result = 0;
                break;
            }
        }
        if (result != 0) {
            return result;
        }
    }
    return 0;
}


tBoolean checkForDraw(tBoardType *board)
{
    return (getEmptyPositions(board, NULL) == 0);
}


tUint8 filterSymmetry(tPos *moves, tUint8 numMoves, tBoardType *board)
{
    tBoolean result[BOARD_SQUARES];
    tPos move;
    tBoolean isSymmetric;
    tPos midX, midY, x, y;
    tPlayer *squares = board->squares;
    
    memset(result, 0, sizeof(result));
    for (move = 0; move < numMoves; move++) {
        result[moves[move]] = TRUE;
    }
    
    // Check for left/right symmetry
    isSymmetric = TRUE;
    midX = (BOARD_DIMENSIONS + 1) / 2;
    for (x = 0; x < midX; x++) {
        for (y = 0; y < BOARD_DIMENSIONS; y++) {
            tPos origPos = gXYtoPos[x][y];
            tPos otherPos = gSymmetryPos[LEFT_RIGHT_SYMMETRY][origPos];
            
            if (origPos == otherPos) {
                continue;
            }
            
            if (squares[origPos] != squares[otherPos]) {
                isSymmetric = FALSE;
                x = midX;
                break;
            }
        }
    }
    if (isSymmetric) {
        for (move = 0; move < BOARD_SQUARES; move++) {
            x = gXpos[move];
            if (x >= midX) {
                result[move] = FALSE;
            }
        }
    }
    
    // Check for top/bottom symmetry
    isSymmetric = TRUE;
    midY = (BOARD_DIMENSIONS + 1) / 2;
    for (y = 0; y < midY; y++) {
        for (x = 0; x < BOARD_DIMENSIONS; x++) {
            tPos origPos = gXYtoPos[x][y];
            tPos otherPos = gSymmetryPos[TOP_BOTTOM_SYMMETRY][origPos];
            
            if (origPos == otherPos) {
                continue;
            }
            
            if (squares[origPos] != squares[otherPos]) {
                isSymmetric = FALSE;
                y = midY;
                break;
            }
        }
    }
    if (isSymmetric) {
        for (move = 0; move < BOARD_SQUARES; move++) {
            y = gYpos[move];
            if (y >= midY) {
                result[move] = FALSE;
            }
        }
    }
    
    // Check for first diagonal symmetry
    isSymmetric = TRUE;
    for (x = 0; x < BOARD_DIMENSIONS; x++) {
        for (y = x; y < BOARD_DIMENSIONS; y++) {
            tPos origPos = gXYtoPos[x][y];
            tPos otherPos = gSymmetryPos[BACKWARD_SLASH_SYMMETRY][origPos];
            
            if (origPos == otherPos) {
                continue;
            }
            
            if (squares[origPos] != squares[otherPos]) {
                isSymmetric = FALSE;
                x = BOARD_DIMENSIONS;
                break;
            }
        }
    }
    if (isSymmetric) {
        for (move = 0; move < BOARD_SQUARES; move++) {
            x = gXpos[move];
            y = gYpos[move];
            if (x > y) {
                result[move] = FALSE;
            }
        }
    }
    
    // Check for second diagonal symmetry
    isSymmetric = TRUE;
    for (x = 0; x < BOARD_DIMENSIONS; x++) {
        for (y = 0; y < (BOARD_DIMENSIONS - x); y++) {
            tPos origPos = gXYtoPos[x][y];
            tPos otherPos = gSymmetryPos[FORWARD_SLASH_SYMMETRY][origPos];
            
            if (origPos == otherPos) {
                continue;
            }
            
            if (squares[origPos] != squares[otherPos]) {
                isSymmetric = FALSE;
                x = BOARD_DIMENSIONS;
                break;
            }
        }
    }
    if (isSymmetric) {
        for (move = 0; move < BOARD_SQUARES; move++) {
            tPos otherX;
            
            x = gXpos[move];
            y = gYpos[move];
            otherX = BOARD_DIMENSIONS - 1 - y;
            if (x > otherX) {
                result[move] = FALSE;
            }
        }
    }
    
    numMoves = 0;
    for (move = 0; move < BOARD_SQUARES; move++) {
        if (result[move]) {
            moves[numMoves] = move;
            numMoves++;
        }
    }
    
    return numMoves;
}


tPos getBestMove(tPlayer player, tScore *score, tBoardType *board, tUint8 depth)
{
    tPlayer otherPlayer = OTHER_PLAYER(player);
    tPos bestMove = -1;
    tUint8 i;
    tScore winningMoves = 0;
    tPos moveValues[BOARD_SQUARES];
    tPos allMoves[BOARD_SQUARES];
    tUint8 numMoves = getEmptyPositions(board, allMoves);
    tBoolean verbose = (depth == 0);
    tState boardState;
    tSymmetry bestSym;
    
    depth++;
    
    numMoves = filterSymmetry(allMoves, numMoves, board);
    
    if (numMoves == 0) {
        return -1;
    }
    
    if (verbose) {
        printf("Checking ");
        
        for (i = 0; i < numMoves; i++) {
            if (i == 0) {
                printf("%d", (allMoves[i]+1));
            } else {
                printf(",%d", (allMoves[i]+1));
            }
        }
        printf(" for a winning move...\n");
    }
    
    for (i = 0; i < numMoves; i++) {
        tPos move = allMoves[i];
        setPosition(player, move, board);
        if (checkForWinner(player, move, board)) {
            tScore currScore = BOARD_SQUARES + 1 - depth;
            winningMoves += currScore;
            moveValues[move] = currScore;
            if (bestMove == -1) {
                bestMove = move;
            }
        }
        /* Undo move */
        setPosition(0, move, board);
    }
    
    if (verbose) {
        printf("Checking for the best move...\n");
    }
    
    boardState = getBoardState(board, &bestSym);
    if (bestMove == -1) {
        bestMove = getCachedBestMove(boardState, bestSym, &winningMoves);
    }
    
    if (bestMove == -1) {
        for (i = 0; i < numMoves; i++) {
            tPos move = allMoves[i];
            tScore opponentScore;
            
            if (verbose) {
                printf("Checking move number %d...\n", (i+1));
            }
            
            setPosition(player, move, board);
            moveValues[move] = 0;
            if (getBestMove(otherPlayer, &opponentScore, board, depth) != -1) {
                moveValues[move] = -opponentScore;
            }
            /* Undo move */
            setPosition(0, move, board);
        }
        
        for (i = 0; i < numMoves; i++) {
            tPos move = allMoves[i];
            if ((bestMove == -1) ||
                (winningMoves < moveValues[move])) {
                bestMove = move;
                winningMoves = moveValues[move];
            }
        }
        
        setCachedBestMove(boardState, bestSym, bestMove, winningMoves);
    }
    
    if (score != NULL) {
        *score = winningMoves;
    }
    
    return bestMove;
}


tPlayer mapPlayerLetterToNum(char player)
{
    switch (player) {
        case 'X':
            return 1;
            break;
        case 'Y':
            return -1;
            break;
    }
    return 0;
}


char mapPlayerNumToLetter(tPlayer player)
{
    switch (player) {
        case 1:
            return 'X';
            break;
        case -1:
            return 'Y';
            break;
    }
    return ' ';
}


void printBoard(tBoardType *board)
{
    tPos pos;
    
    for (pos = 0; pos < BOARD_SQUARES; pos++) {
        tPos x = gXpos[pos];
        tPos y = gYpos[pos];
        tPlayer player = board->squares[pos];
        
        if (x == 0) {
            printf("\n");
            if (y != 0) {
                tPos i;
                for (i = 0; i < BOARD_DIMENSIONS; i++) {
                    printf("----");
                }
                printf("\n");
            }
        }
        
        if (player == 0) {
            printf("%2d ", (pos + 1));
        } else {
            printf(" %c ", mapPlayerNumToLetter(player));
        }
        if (x != BOARD_DIMENSIONS - 1) {
            printf("|");
        }
    }
    printf("\n\n");
}


char getCharCommand(void)
{
    char buffer[16];
    tUint8 len;
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        printf("\nGoodbye\n");
        cgetc();
        exit(0);
    }
    
    len = strlen(buffer);
    if ((len != 2) ||
        (buffer[1] != '\n')) {
        return -1;
    }
    
    return toupper(buffer[0]);
}


tPos getPlayersTurn(tBoardType *board)
{
    tPos pos;
    while (1) {
        char move;
        
        printf("\nEnter your move as a number from 1 to %d: ", BOARD_SQUARES);
        move = getCharCommand();
        if (!isdigit(move)) {
            printf("Bad move, try again\n");
            continue;
        }
        
        pos = (move - '0');
        if ((pos < 1) ||
            (pos > BOARD_SQUARES)) {
            printf("Bad move %d, try again\n", pos);
            continue;
        }
        
        pos--;
        if (board->squares[pos]) {
            printf("Position is already used, try again\n");
            continue;
        }
        
        break;
    }
    return pos;
}


void playGame(void)
{
    tBoardType board;
    char whoseTurn;
    tPlayer player;
    
    emptyBoard(&board);
    printf("\n\nI am X, you are Y.  Who goes first? (X/Y) ");
    while (1) {
        char input = getCharCommand();
        
        if ((input != 'X') &&
            (input != 'Y')) {
            printf("Please enter X or Y: ");
        } else {
            whoseTurn = input;
            break;
        }
    }
    
    printBoard(&board);
    
    player = mapPlayerLetterToNum(whoseTurn);
    while (1) {
        tPos move;
        whoseTurn = mapPlayerNumToLetter(player);
        
        if (whoseTurn == 'Y') {
            move = getPlayersTurn(&board);
        } else {
            printf("Thinking...\n");
            move = getBestMove(player, NULL, &board, 0);
        }
        setPosition(player, move, &board);
        
        printBoard(&board);
        
        if (checkForWinner(player, move, &board)) {
            printf("%c wins!\n", whoseTurn);
            return;
        }
        
        if (checkForDraw(&board)) {
            printf("Draw!!\n");
            return;
        }
        
        player = OTHER_PLAYER(player);
    }
}


int main(void)
{
    char playAgain;
    
    videomode(VIDEOMODE_80COL);
    initGame();
    
    do {
        playGame();
        
        printf("\nPlay again (Y/N)? ");
        while (1) {
            playAgain = getCharCommand();
            
            if ((playAgain == 'Y') ||
                (playAgain == 'N')) {
                break;
            }
        }
    } while (playAgain == 'Y');
    
    cgetc();
    return 0;
}


