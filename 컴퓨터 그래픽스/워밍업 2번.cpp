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
        original_lines[line_count][strcspn(original_lines[line_count], "\r\n")] = '\0';
        line_count++;
    }
    fclose(fp);
    return 1;
}

// 단어 개수 계산
int countWords(const char* str) {
    int count = 0;
    int in_word = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isspace((unsigned char)str[i]) && str[i] != '*') {
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

// 단일 줄에 대해 텍스트 조작 토글(a, g, d, f, e)을 순서대로 적용
void transformLine(const char* src, char* dst) {
    strcpy(dst, src);
    int len = (int)strlen(dst);

    // a: 대소문자 반전
    if (toggle_a) {
        for (int j = 0; j < len; j++) {
            if (islower((unsigned char)dst[j])) dst[j] = (char)toupper((unsigned char)dst[j]);
            else if (isupper((unsigned char)dst[j])) dst[j] = (char)tolower((unsigned char)dst[j]);
        }
    }

    // g: 특정 문자 치환
    if (toggle_g) {
        for (int j = 0; j < len; j++) {
            if (dst[j] == g_old_char) dst[j] = g_new_char;
        }
    }

    // d: 문장 전체 거꾸로
    if (toggle_d) {
        for (int j = 0; j < len / 2; j++) {
            char t = dst[j];
            dst[j] = dst[len - 1 - j];
            dst[len - 1 - j] = t;
        }
    }

    // f: 각 단어 거꾸로
    if (toggle_f) {
        int start = 0;
        while (start < len) {
            while (start < len && (dst[start] == ' ' || dst[start] == '*')) start++;
            int end = start;
            while (end < len && dst[end] != ' ' && dst[end] != '*') end++;
            for (int l = start, r = end - 1; l < r; l++, r--) {
                char t = dst[l];
                dst[l] = dst[r];
                dst[r] = t;
            }
            start = end;
        }
    }

    // e: 공백에 '*' 삽입
    if (toggle_e) {
        for (int j = 0; j < len; j++) {
            if (dst[j] == ' ') dst[j] = '*';
        }
    }
}

// 기본 및 누적 토글 기반 출력 함수
void printLines() {
    int cap_word_count = 0;

    for (int i = 0; i < line_count; i++) {
        char transformed[MAX_LEN];
        // 1. 텍스트 자체를 변경하는 토글들 먼저 누적 적용
        transformLine(original_lines[i], transformed);
        int len = (int)strlen(transformed);

        // 2. toggle_c(대문자 시작 단어 강조) 및 toggle_h(숫자 뒤 줄바꿈)를 반영하여 출력
        int idx = 0;
        while (idx < len) {
            // 공백 또는 '*' 구분자 처리
            if (isspace((unsigned char)transformed[idx]) || transformed[idx] == '*') {
                char ch = transformed[idx++];
                putchar(ch);
                if (toggle_h && isdigit((unsigned char)ch)) putchar('\n');
                continue;
            }

            // 단어 추출
            int start = idx;
            while (idx < len && !isspace((unsigned char)transformed[idx]) && transformed[idx] != '*') {
                idx++;
            }

            // toggle_c 활성화 상태이고 단어 첫 글자가 대문자인 경우 강조
            int is_capital = toggle_c && isupper((unsigned char)transformed[start]);
            if (is_capital) {
                setColor(11); // 강조 색상
                cap_word_count++;
            }

            for (int k = start; k < idx; k++) {
                putchar(transformed[k]);
                if (toggle_h && isdigit((unsigned char)transformed[k])) {
                    putchar('\n');
                }
            }

            if (is_capital) {
                setColor(7); // 색상 원복
            }
        }
        putchar('\n');
    }

    if (toggle_c) {
        printf("[Count of words starting with uppercase: %d]\n", cap_word_count);
    }
}

// b: 현재 누적 변환된 문장 기준으로 단어 개수 출력
void executeB() {
    for (int i = 0; i < line_count; i++) {
        char transformed[MAX_LEN];
        transformLine(original_lines[i], transformed);
        printf("%s (Word count: %d)\n", transformed, countWords(transformed));
    }
}

// i: 단어 검색 (누적 변환된 문자열 기준 검색)
void executeI(const char* target) {
    int target_len = (int)strlen(target);
    if (target_len == 0) return;

    int total_match = 0;
    for (int i = 0; i < line_count; i++) {
        char transformed[MAX_LEN];
        transformLine(original_lines[i], transformed);

        int len = (int)strlen(transformed);
        int j = 0;

        while (j < len) {
            if (_strnicmp(&transformed[j], target, target_len) == 0) {
                setColor(11);
                for (int k = 0; k < target_len; k++) putchar(transformed[j + k]);
                setColor(7);
                total_match++;
                j += target_len;
            }
            else {
                putchar(transformed[j]);
                j++;
            }
        }
        putchar('\n');
    }
    printf("[Matched count: %d]\n", total_match);
}

// j: 문장 순서 회전
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