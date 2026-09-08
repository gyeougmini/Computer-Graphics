#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

#define BASE_BOARD_SIZE 30
#define MIN_BOARD_SIZE 10
#define MAX_BOARD_SIZE 40

typedef struct {
    int x; // 좌측 상단 x
    int y; // 좌측 상단 y
    int w; // 너비 (폭)
    int h; // 높이
} Rect;

int board_size = BASE_BOARD_SIZE;
Rect r1, r2;

void setColor(unsigned short color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// 모듈러 연산 래퍼 (음수 인덱스 대응)
int wrap(int val, int max_val) {
    int res = val % max_val;
    if (res < 0) res += max_val;
    return res;
}

// 점 (x, y)가 도형 내부에 포함되는지 체크 (보드 경계 순환 고려)
bool is_inside(Rect r, int x, int y, int b_size) {
    int rel_x = wrap(x - r.x, b_size);
    int rel_y = wrap(y - r.y, b_size);
    return (rel_x >= 0 && rel_x < r.w && rel_y >= 0 && rel_y < r.h);
}

// 보드 화면 출력
void draw_board() {
    for (int y = 0; y < board_size; y++) {
        for (int x = 0; x < board_size; x++) {
            bool in1 = is_inside(r1, x, y, board_size);
            bool in2 = is_inside(r2, x, y, board_size);

            if (in1 && in2) {
                setColor(11);
                printf("# ");      // 두 도형이 겹치는 영역
            }
            else if (in1) {
                setColor(7);
                printf("O ");      // 1번 도형
            }
            else if (in2) {
                setColor(7);
                printf("X ");      // 2번 도형
            }
            else {
                setColor(7);
                printf(". ");      // 빈 칸
            }
        }
        printf("\n");
    }
}

// 두 좌표 (x1, y1), (x2, y2)를 받아 도형 구조체 초기화
Rect create_rect(int x1, int y1, int x2, int y2) {
    Rect r;
    int min_x = (x1 < x2) ? x1 : x2;
    int max_x = (x1 > x2) ? x1 : x2;
    int min_y = (y1 < y2) ? y1 : y2;
    int max_y = (y1 > y2) ? y1 : y2;

    r.x = wrap(min_x, board_size);
    r.y = wrap(min_y, board_size);
    r.w = max_x - min_x + 1;
    r.h = max_y - min_y + 1;

    // 보드 크기보다 큰 경우 보정
    if (r.w > board_size) r.w = board_size;
    if (r.h > board_size) r.h = board_size;

    return r;
}

// 초기 좌표 입력 함수
void init_input() {
    int x1, y1, x2, y2;
    printf("input coord value1: ");
    scanf("%d %d %d %d", &x1, &y1, &x2, &y2);
    r1 = create_rect(x1, y1, x2, y2);

    printf("input coord value2: ");
    scanf("%d %d %d %d", &x1, &y1, &x2, &y2);
    r2 = create_rect(x1, y1, x2, y2);
}

// 개별 도형 변형 처리 함수
void transform_rect(Rect* r, char cmd,
    char right, char left, char down, char up,
    char shrink_all, char expand_all,
    char expand_x, char shrink_x,
    char expand_y, char shrink_y,
    char ex_x_sh_y, char sh_x_ex_y,
    char area_cmd, int rect_num) {

    // 이동 (가장자리 도달 시 반대편으로)
    if (cmd == right) {
        r->x = wrap(r->x + 1, board_size);
    }
    else if (cmd == left) {
        r->x = wrap(r->x - 1, board_size);
    }
    else if (cmd == down) {
        r->y = wrap(r->y + 1, board_size);
    }
    else if (cmd == up) {
        r->y = wrap(r->y - 1, board_size);
    }
    // 동시 축소/확대
    else if (cmd == shrink_all) {
        if (r->w > 1 && r->h > 1) {
            r->w--;
            r->h--;
        }
    }
    else if (cmd == expand_all) {
        if (r->w < board_size && r->h < board_size) {
            r->w++;
            r->h++;
        }
    }
    // X축 확대/축소
    else if (cmd == expand_x) {
        if (r->w < board_size) r->w++;
    }
    else if (cmd == shrink_x) {
        if (r->w > 1) r->w--;
    }
    // Y축 확대/축소
    else if (cmd == expand_y) {
        if (r->h < board_size) r->h++;
    }
    else if (cmd == shrink_y) {
        if (r->h > 1) r->h--;
    }
    // 복합 비율 변경
    else if (cmd == ex_x_sh_y) {
        if (r->w < board_size && r->h > 1) {
            r->w++;
            r->h--;
        }
    }
    else if (cmd == sh_x_ex_y) {
        if (r->w > 1 && r->h < board_size) {
            r->w--;
            r->h++;
        }
    }
    // 면적 출력
    else if (cmd == area_cmd) {
        printf("도형 %d 면적: %d x %d = %d\n", rect_num, r->w, r->h, r->w * r->h);
    }
}

int main() {
    init_input();
    draw_board();

    char order;
    while (1) {
        printf("input order: ");
        if (scanf(" %c", &order) != 1) break;

        // 프로그램 종료
        if (order == 'q') {
            break;
        }

        // 전체 리셋
        if (order == 'r') {
            board_size = BASE_BOARD_SIZE;
            init_input();
            draw_board();
            continue;
        }

        // 보드칸 확대 (최대 +10 = 40)
        if (order == 'c') {
            if (board_size < MAX_BOARD_SIZE) {
                board_size++;
                printf("보드 크기가 %d x %d 로 늘어났습니다.\n", board_size, board_size);
            }
            else {
                printf("더 이상 늘릴 수 없습니다 (최대 40x40).\n");
            }
            draw_board();
            continue;
        }

        // 보드칸 축소 (최대 -20 = 10, 단 도형 크기 유지 조건)
        if (order == 'd') {
            if (board_size > MIN_BOARD_SIZE) {
                if (r1.w <= board_size - 1 && r1.h <= board_size - 1 &&
                    r2.w <= board_size - 1 && r2.h <= board_size - 1) {
                    board_size--;
                    r1.x = wrap(r1.x, board_size);
                    r1.y = wrap(r1.y, board_size);
                    r2.x = wrap(r2.x, board_size);
                    r2.y = wrap(r2.y, board_size);
                    printf("보드 크기가 %d x %d 로 줄어들었습니다.\n", board_size, board_size);
                }
                else {
                    printf("도형 크기가 축소될 보드보다 커서 줄일 수 없습니다.\n");
                }
            }
            else {
                printf("더 이상 줄일 수 없습니다 (최소 10x10).\n");
            }
            draw_board();
            continue;
        }

        // 1번 도형 명령어 처리
        transform_rect(&r1, order,
            'x', 'X', 'y', 'Y',
            's', 'S',
            'i', 'I',
            'j', 'J',
            'a', 'A',
            'b', 1);
        
        // 2번 도형 명령어 처리 (독립된 명령어)
        transform_rect(&r2, order,
            '1', '2', '3', '4',
            '5', '6',
            '7', '8',
            '9', '0',
            'k', 'K',
            'v', 2);

        draw_board();
    }

    return 0;
}