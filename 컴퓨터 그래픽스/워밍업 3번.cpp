#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define MAX_POINTS 10

// ANSI 이스케이프 코드 (색상 변경용)
#define COLOR_RED   "\x1b[31m"
#define COLOR_RESET "\x1b[0m"

// 3차원 점 구조체 정의
typedef struct {
    int x, y, z;
    int is_valid; // 데이터 저장 여부 (1: 존재, 0: 빈 칸)
} Point;

Point list[MAX_POINTS];
int toggle_f = 0; // f 명령어 토글 플래그

// 각 기능별 독립 토글 플래그 (1: 활성, 0: 비활성)
int toggle_case = 0;   // a: 대소문자 반전 토글
int toggle_color = 0;  // c: 대문자 강조 색상 토글

// 리스트 상태 출력 함수 (9번이 맨 위, 0번이 맨 아래)
void printList() {
    printf("-------------------------\n");
    if (toggle_f) {
        // f 명령어 활성화 시: 유효한 점들을 추출 후 원점 거리 기준 오름차순 정렬하여 0번부터 출력
        Point sorted[MAX_POINTS];
        int count = 0;
        for (int i = 0; i < MAX_POINTS; i++) {
            if (list[i].is_valid) {
                sorted[count++] = list[i];
            }
        }

        // 거리 기준 오름차순 버블 정렬
        for (int i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - 1 - i; j++) {
                double d1 = sqrt(sorted[j].x * sorted[j].x + sorted[j].y * sorted[j].y + sorted[j].z * sorted[j].z);
                double d2 = sqrt(sorted[j + 1].x * sorted[j + 1].x + sorted[j + 1].y * sorted[j + 1].y + sorted[j + 1].z * sorted[j + 1].z);
                if (d1 > d2) {
                    Point temp = sorted[j];
                    sorted[j] = sorted[j + 1];
                    sorted[j + 1] = temp;
                }
            }
        }

        for (int i = MAX_POINTS - 1; i >= 0; i--) {
            if (i < count) {
                double dist = sqrt(sorted[i].x * sorted[i].x + sorted[i].y * sorted[i].y + sorted[i].z * sorted[i].z);
                printf("%d | %d %d %d  (dist: %.2f)\n", i, sorted[i].x, sorted[i].y, sorted[i].z, dist);
            }
            else {
                printf("%d |\n", i);
            }
        }
    }
    else {
        // 일반 리스트 출력
        for (int i = MAX_POINTS - 1; i >= 0; i--) {
            if (list[i].is_valid) {
                printf("%d | %d %d %d\n", i, list[i].x, list[i].y, list[i].z);
            }
            else {
                printf("%d |\n", i);
            }
        }
    }
    printf("-------------------------\n");
}

// + xyz: 맨 위에 삽입 (0번부터 탐색하여 빈 첫 칸에 삽입, 9번까지 차있으면 빈 0번에 삽입)
void insertTop(int x, int y, int z) {
    int target_idx = -1;
    for (int i = 9; i >= 0; i--) {
        if (list[i].is_valid || i == 0) {
            if (i == 0) {
                if (!list[0].is_valid) {
                    target_idx = 0;
                    break;
                }
            }
            target_idx = i + 1;
            break;
        }
    }
    if (target_idx == -1) {
        printf("리스트가 가득 찼습니다.\n");
        return;
    }
    list[target_idx].x = x;
    list[target_idx].y = y;
    list[target_idx].z = z;
    list[target_idx].is_valid = 1;
}

// - : 맨 위의 요소 삭제
void deleteTop() {
    int target_idx = -1;
    for (int i = MAX_POINTS - 1; i >= 0; i--) {
        if (list[i].is_valid) {
            target_idx = i;
            break;
        }
    }
    if (target_idx == -1) {
        printf("삭제할 데이터가 없습니다.\n");
        return;
    }
    list[target_idx].is_valid = 0;
}

// e xyz: 맨 아래(0번)에 삽입, 기존 데이터들을 위로 1칸씩 민다
void insertBottom(int x, int y, int z) {
    if (list[MAX_POINTS - 1].is_valid) {
        printf("맨 위 칸이 차 있어 더 이상 밀어올릴 수 없습니다.\n");
        return;
    }

    // 데이터 위로 1칸씩 이동
    if(list[0].is_valid == 1) {
        for (int i = MAX_POINTS - 1; i > 0; i--) {
            list[i] = list[i - 1];
        }
    }

    list[0].x = x;
    list[0].y = y;
    list[0].z = z;
    list[0].is_valid = 1;

}

// d: 맨 아래(0번) 데이터 삭제
void deleteBottom() {
    for (int i = 0; i < 10; ++i) {
        if (list[i].is_valid) {
            list[i].is_valid = 0;
            break;
        }
    }
}

// a: 저장된 점의 개수 카운트
void countPoints() {
    int count = 0;
    for (int i = 0; i < MAX_POINTS; i++) {
        if (list[i].is_valid) count++;
    }
    printf("저장된 점의 개수: %d개\n", count);
}

// b: 점들을 한 칸씩 내림 (0->9, 1->0, 2->1, ..., 9->8)
void shiftDown() {
    Point temp = list[0];
    for (int i = 0; i < MAX_POINTS - 1; i++) {
        list[i] = list[i + 1];
    }
    list[MAX_POINTS - 1] = temp;
}

// c: 리스트 초기화
void clearList() {
    for (int i = 0; i < MAX_POINTS; i++) {
        list[i].is_valid = 0;
    }
    printf("리스트를 초기화했습니다.\n");
}

// g: 점들 간 최장 거리 및 최단 거리 계산
void calculateDistances() {
    Point valid_pts[MAX_POINTS];
    int count = 0;
    for (int i = 0; i < MAX_POINTS; i++) {
        if (list[i].is_valid) {
            valid_pts[count++] = list[i];
        }
    }

    if (count < 2) {
        printf("거리 계산을 위해서는 최소 2개 이상의 점이 필요합니다.\n");
        return;
    }

    double min_d = 1e9, max_d = -1.0;
    Point min_p1, min_p2, max_p1, max_p2;

    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            double dx = valid_pts[i].x - valid_pts[j].x;
            double dy = valid_pts[i].y - valid_pts[j].y;
            double dz = valid_pts[i].z - valid_pts[j].z;
            double dist = sqrt(dx * dx + dy * dy + dz * dz);

            if (dist < min_d) {
                min_d = dist;
                min_p1 = valid_pts[i];
                min_p2 = valid_pts[j];
            }
            if (dist > max_d) {
                max_d = dist;
                max_p1 = valid_pts[i];
                max_p2 = valid_pts[j];
            }
        }
    }

    printf("[가장 가까운 두 점]\n");
    printf("점1: (%d, %d, %d), 점2: (%d, %d, %d) | 거리: %.4f\n",
        min_p1.x, min_p1.y, min_p1.z, min_p2.x, min_p2.y, min_p2.z, min_d);
    printf("[가장 먼 두 점]\n");
    printf("점1: (%d, %d, %d), 점2: (%d, %d, %d) | 거리: %.4f\n",
        max_p1.x, max_p1.y, max_p1.z, max_p2.x, max_p2.y, max_p2.z, max_d);
}

int main() {
    clearList();
    printList();

    char input[64];
    while (1) {
        printf("\n명령어 입력: ");
        if (scanf("%s", input) != 1) break;

        if (input[0] == 'q') {
            break;
        }
        else if (input[0] == '+') {
            int x, y, z;
            if (sscanf(input + 1, "%1d%1d%1d", &x, &y, &z) == 3 || sscanf(input + 1, "%d %d %d", &x, &y, &z) == 3) {
                insertTop(x, y, z);
            }
            else {
                scanf("%d %d %d", &x, &y, &z);
                insertTop(x, y, z);
            }
            printList();
        }
        else if (input[0] == '-') {
            deleteTop();
            printList();
        }
        else if (input[0] == 'e') {
            int x, y, z;
            if (sscanf(input + 1, "%1d%1d%1d", &x, &y, &z) == 3 || sscanf(input + 1, "%d %d %d", &x, &y, &z) == 3) {
                insertBottom(x, y, z);
            }
            else {
                scanf("%d %d %d", &x, &y, &z);
                insertBottom(x, y, z);
            }
            printList();
        }
        else if (input[0] == 'd') {
            deleteBottom();
            printList();
        }
        else if (input[0] == 'a') {
            countPoints();
        }
        else if (input[0] == 'b') {
            shiftDown();
            printList();
        }
        else if (input[0] == 'c') {
            clearList();
            printList();
        }
        else if (input[0] == 'f') {
            toggle_f = !toggle_f;
            printList();
        }
        else if (input[0] == 'g') {
            calculateDistances();
        }
        else {
            printf("알 수 없는 명령어입니다.\n");
        }
    }

    return 0;
}