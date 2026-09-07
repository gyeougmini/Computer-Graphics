#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>

#define MAX_LINES 10
#define MAX_LEN 512

char original_lines[MAX_LINES][MAX_LEN];
int line_count = 0;

// 토글 상태 변수들
int toggle_a = 0;
int toggle_c = 0;
int toggle_d = 0;
int toggle_e = 0;
int toggle_f = 0;
int toggle_g = 0;
int toggle_h = 0;

char g_old_char = '\0';
char g_new_char = '\0';

// 콘솔 색상 지정 함수
void setColor(unsigned short color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// 파일 읽기
int loadFile(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return 0;

    line_count = 0;
    while (line_count < MAX_LINES && fgets(original_lines[line_count], MAX_LEN, fp)) {
        // 줄바꿈 문자 제거
        original_lines[line_count][strcspn(original_lines[line_count], "\r\n")] = '\0';
        line_count++;
    }
    fclose(fp);
    return 1;
}

// 각 줄의 단어 개수 계산
int countWords(const char* str) {
    int count = 0;
    int in_word = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isspace((unsigned char)str[i])) {
            if (!in_word) {
                count++;
                in_word = 1;
            }
        }
        else {
            in_word = 0;
        }
    }
    return count;
}

// 기본 및 토글 기반 문장 출력 함수
void printLines() {
    // b, c, i 기능은 전용 함수에서 출력 처리
    if (toggle_c) {
        int cap_word_count = 0;
        for (int i = 0; i < line_count; i++) {
            char* line = original_lines[i];
            int len = (int)strlen(line);
            int idx = 0;

            while (idx < len) {
                if (isspace((unsigned char)line[idx])) {
                    putchar(line[idx++]);
                    continue;
                }
                int start = idx;
                while (idx < len && !isspace((unsigned char)line[idx])) idx++;

                if (isupper((unsigned char)line[start])) {
                    setColor(11); // 강조 색상
                    for (int k = start; k < idx; k++) putchar(line[k]);
                    setColor(7);  // 원복
                    cap_word_count++;
                }
                else {
                    for (int k = start; k < idx; k++) putchar(line[k]);
                }
            }
            putchar('\n');
        }
        printf("[Count of words starting with uppercase: %d]\n", cap_word_count);
        return;
    }

    if (toggle_h) {
        for (int i = 0; i < line_count; i++) {
            for (int j = 0; original_lines[i][j] != '\0'; j++) {
                putchar(original_lines[i][j]);
                if (isdigit((unsigned char)original_lines[i][j])) {
                    putchar('\n');
                }
            }
            putchar('\n');
        }
        return;
    }

    for (int i = 0; i < line_count; i++) {
        char temp[MAX_LEN];
        strcpy(temp, original_lines[i]);
        int len = (int)strlen(temp);

        // a: 대소문자 반전
        if (toggle_a) {
            for (int j = 0; j < len; j++) {
                if (islower((unsigned char)temp[j])) temp[j] = (char)toupper((unsigned char)temp[j]);
                else if (isupper((unsigned char)temp[j])) temp[j] = (char)tolower((unsigned char)temp[j]);
            }
        }

        // g: 특정 문자 치환
        if (toggle_g) {
            for (int j = 0; j < len; j++) {
                if (temp[j] == g_old_char) temp[j] = g_new_char;
            }
        }

        // d: 문장 전체 거꾸로 출력
        if (toggle_d) {
            for (int j = 0; j < len / 2; j++) {
                char t = temp[j];
                temp[j] = temp[len - 1 - j];
                temp[len - 1 - j] = t;
            }
        }

        // f: 각 단어 거꾸로 출력
        if (toggle_f) {
            int start = 0;
            while (start < len) {
                while (start < len && (temp[start] == ' ' || temp[start] == '*')) start++;
                int end = start;
                while (end < len && temp[end] != ' ' && temp[end] != '*') end++;
                for (int l = start, r = end - 1; l < r; l++, r--) {
                    char t = temp[l];
                    temp[l] = temp[r];
                    temp[r] = t;
                }
                start = end;
            }
        }

        // e: 공백에 '*' 삽입
        if (toggle_e) {
            for (int j = 0; j < len; j++) {
                if (temp[j] == ' ') temp[j] = '*';
            }
        }

        printf("%s\n", temp);
    }
}

// b: 각 줄의 문장 뒤에 단어 개수 출력
void executeB() {
    for (int i = 0; i < line_count; i++) {
        printf("%s (Word count: %d)\n", original_lines[i], countWords(original_lines[i]));
    }
}

// i: 단어 검색 및 대소문자 무시 색상 강조 + 개수 카운트
void executeI(const char* target) {
    int target_len = (int)strlen(target);
    if (target_len == 0) return;

    int total_match = 0;
    for (int i = 0; i < line_count; i++) {
        char* line = original_lines[i];
        int len = (int)strlen(line);
        int j = 0;

        while (j < len) {
            if (_strnicmp(&line[j], target, target_len) == 0) {
                setColor(11);
                for (int k = 0; k < target_len; k++) putchar(line[j + k]);
                setColor(7);
                total_match++;
                j += target_len;
            }
            else {
                putchar(line[j]);
                j++;
            }
        }
        putchar('\n');
    }
    printf("[Matched count: %d]\n", total_match);
}

// j: 문장 순서 회전 (1번 -> 2번, ..., 9번 -> 1번)
void executeJ() {
    if (line_count <= 1) return;
    char last[MAX_LEN];
    strcpy(last, original_lines[line_count - 1]);
    for (int i = line_count - 1; i > 0; i--) {
        strcpy(original_lines[i], original_lines[i - 1]);
    }
    strcpy(original_lines[0], last);
    printLines();
}

int main() {
    char filename[128];
    printf("input data file name: ");
    if (scanf("%127s", filename) != 1) return 0;

    if (!loadFile(filename)) {
        printf("Error: Failed to open file '%s'\n", filename);
        return 1;
    }

    // 파일 로드 후 원본 출력
    printf("\n--- File Content ---\n");
    printLines();
    printf("--------------------\n");

    char cmd;
    while (1) {
        printf("\ninput the command: ");
        if (scanf(" %c", &cmd) != 1) break;

        if (cmd == 'q') {
            break;
        }
        else if (cmd == 'a') {
            toggle_a = !toggle_a;
            printLines();
        }
        else if (cmd == 'b') {
            executeB();
        }
        else if (cmd == 'c') {
            toggle_c = !toggle_c;
            printLines();
        }
        else if (cmd == 'd') {
            toggle_d = !toggle_d;
            printLines();
        }
        else if (cmd == 'e') {
            toggle_e = !toggle_e;
            printLines();
        }
        else if (cmd == 'f') {
            toggle_f = !toggle_f;
            printLines();
        }
        else if (cmd == 'g') {
            toggle_g = !toggle_g;
            if (toggle_g) {
                printf("Enter target char and new char (e.g. a X): ");
                scanf(" %c %c", &g_old_char, &g_new_char);
            }
            printLines();
        }
        else if (cmd == 'h') {
            toggle_h = !toggle_h;
            printLines();
        }
        else if (cmd == 'i') {
            char target[64];
            printf("Enter search word: ");
            scanf("%63s", target);
            executeI(target);
        }
        else if (cmd == 'j') {
            executeJ();
        }
        else {
            printf("Unknown command.\n");
        }
    }

    return 0;
}