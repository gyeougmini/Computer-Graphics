#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <ctype.h>

#define MAX_SIZE 6
#define MAX_ATTEMPTS 20

// 콘솔 텍스트 색상 변경 함수
void setColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

int rows, cols;
char board[MAX_SIZE][MAX_SIZE];       // 카드 알파벳 저장
int revealed[MAX_SIZE][MAX_SIZE];    // 1: 맞춘 카드(대문자), 0: 뒷면(*)
int score = 0;
int remaining_attempts = MAX_ATTEMPTS;

// 보드 출력 함수
void printBoard(int showAll, int tempR1, int tempC1, int tempR2, int tempC2) {
    printf("\n    ");
    for (int c = 0; c < cols; c++) {
        printf("%c ", 'a' + c);
    }
    printf("\n");

    for (int r = 0; r < rows; r++) {
        printf(" %d  ", r + 1);
        for (int c = 0; c < cols; c++) {
            if (showAll) {
                // 힌트(h) 명령일 때 전체 보드 공개
                setColor(14); // 노란색
                printf("%c ", board[r][c]);
                setColor(7);
            }
            else if (revealed[r][c]) {
                // 이미 맞춘 카드 (대문자 출력 및 초록색 강조)
                setColor(10); // 밝은 초록색
                printf("%c ", (board[r][c] == '@') ? '@' : toupper(board[r][c]));
                setColor(7);
            }
            else if ((r == tempR1 && c == tempC1) || (r == tempR2 && c == tempC2)) {
                // 일시적으로 뒤집어 확인하는 카드 (하늘색 강조)
                setColor(11); // 밝은 하늘색
                printf("%c ", board[r][c]);
                setColor(7);
            }
            else {
                // 가려진 카드
                printf("* ");
            }
        }
        printf("\n");
    }
    printf("\n");
}

// 게임 보드 초기화 및 카드 셔플 배치
void initGame() {
    int totalCells = rows * cols;
    int pairs = totalCells / 2;
    int hasJoker = (totalCells % 2 != 0);

    // 1차원 임시 배열에 문자 채우기
    char temp[MAX_SIZE * MAX_SIZE];
    int idx = 0;

    for (int i = 0; i < pairs; i++) {
        temp[idx++] = 'a' + i;
        temp[idx++] = 'a' + i;
    }

    if (hasJoker) {
        temp[idx++] = '@'; // 조커 문자
    }

    // 카드 무작위 셔플 (Fisher-Yates shuffle)
    for (int i = totalCells - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        char t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
    }

    // 2차원 보드로 복사
    idx = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            board[r][c] = temp[idx++];
            revealed[r][c] = 0;
        }
    }

    score = 0;
    remaining_attempts = MAX_ATTEMPTS;
}

// 게임 완료 여부 확인
int isGameFinished() {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (!revealed[r][c]) return 0;
        }
    }
    return 1;
}

int main() {
    srand((unsigned int)time(NULL));

    // 보드 크기 입력 (범위: 3~6)
    while (1) {
        printf("가로와 세로 크기를 입력하세요 (3~6 사이, 예: 3 4): ");
        if (scanf("%d %d", &cols, &rows) == 2) {
            if (cols >= 3 && cols <= 6 && rows >= 3 && rows <= 6) {
                break;
            }
        }
        while (getchar() != '\n'); // 입력 버퍼 비우기
        printf("올바른 범위(3~6)의 정수를 입력해주세요.\n");
    }

    initGame();

    char input[64];
    while (1) {
        system("cls");
        printf("=== 카드 짝 맞추기 게임 ===\n");
        printf("현재 점수: %d점 | 남은 기회: %d회\n", score, remaining_attempts);
        printf("명령어: r(리셋), h(힌트), q(종료) | 좌표 입력 예시: a1 c3 또는 a1c3\n");

        printBoard(0, -1, -1, -1, -1);

        // 게임 승리 체크
        if (isGameFinished()) {
            setColor(10);
            printf("축하합니다! 모든 카드를 맞추셨습니다! (최종 점수: %d점)\n", score);
            setColor(7);
            break;
        }

        // 시도 횟수 초과 체크
        if (remaining_attempts <= 0) {
            setColor(12); // 빨간색
            printf("기회를 모두 소진하였습니다. 게임 오버! (최종 점수: %d점)\n", score);
            setColor(7);
            break;
        }

        printf("입력 >> ");
        if (scanf("%s", input) != 1) break;

        // 명령어 처리
        if (input[0] == 'q' || input[0] == 'Q') {
            printf("게임을 종료합니다.\n");
            break;
        }
        if (input[0] == 'r' || input[0] == 'R') {
            initGame();
            continue;
        }
        if (input[0] == 'h' || input[0] == 'H') {
            system("cls");
            printf("=== [힌트] 전체 카드 잠시 보기 ===\n");
            printBoard(1, -1, -1, -1, -1);
            printf("2초 후 다시 원래대로 가려집니다...\n");
            Sleep(2000); // 2초 대기
            continue;
        }

        // 좌표 파싱
        char c1_char, c2_char;
        int r1, r2;
        int parsed = 0;

        // "a1c3" 형태의 단일 토큰으로 들어온 경우
        if (sscanf(input, " %c%d%c%d", &c1_char, &r1, &c2_char, &r2) == 4) {
            parsed = 1;
        }
        else {
            // "a1 c3" 형태로 공백을 두고 들어온 경우
            char input2[32];
            if (scanf("%s", input2) == 1) {
                if (sscanf(input, " %c%d", &c1_char, &r1) == 2 &&
                    sscanf(input2, " %c%d", &c2_char, &r2) == 2) {
                    parsed = 1;
                }
            }
        }

        if (!parsed) {
            printf("입력 형식이 올바르지 않습니다. 다시 입력해주세요.\n");
            Sleep(1200);
            continue;
        }

        int col1 = tolower(c1_char) - 'a';
        int row1 = r1 - 1;
        int col2 = tolower(c2_char) - 'a';
        int row2 = r2 - 1;

        // 유효 범위 및 중복 선택 검사
        if (row1 < 0 || row1 >= rows || col1 < 0 || col1 >= cols ||
            row2 < 0 || row2 >= rows || col2 < 0 || col2 >= cols) {
            printf("범위를 벗어난 좌표입니다.\n");
            Sleep(1200);
            continue;
        }

        if (row1 == row2 && col1 == col2) {
            printf("서로 다른 두 칸을 선택해야 합니다.\n");
            Sleep(1200);
            continue;
        }

        if (revealed[row1][col1] || revealed[row2][col2]) {
            printf("이미 열려있는 카드가 포함되어 있습니다.\n");
            Sleep(1200);
            continue;
        }

        // 선택한 두 칸을 화면에 보여줌
        system("cls");
        printf("선택한 카드를 확인합니다...\n");
        printBoard(0, row1, col1, row2, col2);

        remaining_attempts--;

        // 매칭 판정 로직
        int matched = 0;
        if (board[row1][col1] == '@' || board[row2][col2] == '@') {
            // 1. 선택한 두 카드(조커 + 선택한 카드)를 먼저 열어둠
            revealed[row1][col1] = 1;
            revealed[row2][col2] = 1;

            // 2. 조커가 아닌 다른 카드의 알파벳 확인
            char target = (board[row1][col1] == '@') ? board[row2][col2] : board[row1][col1];

            // 3. 보드 전체에서 해당 알파벳의 나머지 짝 카드를 찾아 함께 열어줌
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    if (board[r][c] == target) {
                        revealed[r][c] = 1;
                    }
                }
            }

            score += 15;
            matched = 1;
            setColor(10);
            printf("조커 카드 발동! 선택한 카드와 그 짝 카드가 함께 열립니다. (+15점)\n");
            setColor(7);
        }
        else if (board[row1][col1] == board[row2][col2]) {
            // 두 카드가 일치할 경우
            revealed[row1][col1] = 1;
            revealed[row2][col2] = 1;
            score += 10;
            matched = 1;
            setColor(10);
            printf("일치합니다! 문자가 고정됩니다. (+10점)\n");
            setColor(7);
        }
        else {
            // 불일치할 경우
            score = (score >= 2) ? score - 2 : 0;
            setColor(12);
            printf("불일치! 카드가 다시 뒤집힙니다. (-2점)\n");
            setColor(7);
        }

        Sleep(1500); // 플레이어가 결과를 확인할 수 있도록 1.5초 대기
    }

    return 0;
}