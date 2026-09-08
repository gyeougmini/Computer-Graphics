#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_LINE_LEN 512
#define MAX_VERTICES 1000
#define MAX_TEXTURES 1000
#define MAX_FACES    1000
#define EPSILON      1e-6f

// 3D 정점 구조체
typedef struct {
    float x, y, z;
} Vertex;

// 2D 텍스처 좌표 구조체
typedef struct {
    float u, v;
} TexCoord;

// 삼각형 면 정보 구조체
typedef struct {
    int v[3];   // 1-based vertex indices
    int vt[3];  // 1-based texture indices (텍스처 미지정 시 0)
    int has_texture;
} Face;

Vertex g_vertices[MAX_VERTICES + 1]; // 1-based index 사용
TexCoord g_textures[MAX_TEXTURES + 1];
Face g_faces[MAX_FACES];

int g_vertex_count = 0;
int g_texture_count = 0;
int g_face_count = 0;

// 선행/후행 공백 제거
char* trim_whitespace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// 부동소수점 비교
int are_vertices_equal(Vertex a, Vertex b) {
    return (fabsf(a.x - b.x) < EPSILON &&
        fabsf(a.y - b.y) < EPSILON &&
        fabsf(a.z - b.z) < EPSILON);
}

// 전체 정점 중복 체크
void check_duplicate_vertices() {
    int duplicate_found = 0;
    for (int i = 1; i <= g_vertex_count; i++) {
        for (int j = i + 1; j <= g_vertex_count; j++) {
            if (are_vertices_equal(g_vertices[i], g_vertices[j])) {
                printf("[Warning] Duplicate vertex found: Index %d and %d have identical coordinates (%.3f, %.3f, %.3f)\n",
                    i, j, g_vertices[i].x, g_vertices[i].y, g_vertices[i].z);
                duplicate_found = 1;
            }
        }
    }
    if (!duplicate_found) {
        printf("No duplicate vertex value\n");
    }
}

// face 라인 파싱 및 유효성 검사
int parse_face_line(char* args, int line_num) {
    // 공백을 기준으로 토큰 분리
    char* tokens[10];
    int token_count = 0;

    char* token = strtok(args, " \t\r\n");
    while (token != NULL) {
        // 인라인 주석(#)이 나오면 그 이후는 무시
        if (token[0] == '#') break;
        if (token_count < 10) {
            tokens[token_count++] = token;
        }
        token = strtok(NULL, " \t\r\n");
    }

    // 예외 처리 1: 삼각형 꼭짓점이 3개가 아닌 경우
    if (token_count != 3) {
        printf("[Error Line %d] Face does not form a triangle! (Found %d vertices, expected 3)\n", line_num, token_count);
        return 0;
    }

    Face f;
    memset(&f, 0, sizeof(Face));

    for (int i = 0; i < 3; i++) {
        int v_idx = 0;
        int vt_idx = 0;

        // "v/vt" 또는 "v" 형식 파싱
        char* slash = strchr(tokens[i], '/');
        if (slash != NULL) {
            *slash = '\0';
            v_idx = atoi(tokens[i]);
            vt_idx = atoi(slash + 1);
            f.has_texture = 1;
        }
        else {
            v_idx = atoi(tokens[i]);
            vt_idx = 0;
        }

        // 예외 처리 4: 0 이하 또는 숫자가 아닌 잘못된 인덱스
        if (v_idx <= 0) {
            printf("[Error Line %d] Invalid vertex index '%s' in face.\n", line_num, tokens[i]);
            return 0;
        }

        // 예외 처리 3: 인덱스 범위 초과
        if (v_idx > g_vertex_count) {
            printf("[Error Line %d] Vertex index %d out of range (Total vertices: %d)!\n", line_num, v_idx, g_vertex_count);
            return 0;
        }

        if (f.has_texture) {
            if (vt_idx <= 0 || vt_idx > g_texture_count) {
                printf("[Error Line %d] Texture index %d out of range (Total textures: %d)!\n", line_num, vt_idx, g_texture_count);
                return 0;
            }
        }

        f.v[i] = v_idx;
        f.vt[i] = vt_idx;
    }

    // 예외 처리 2: 면 내에서 동일한 정점 인덱스가 중복 사용된 경우 (삼각형 불가)
    if (f.v[0] == f.v[1] || f.v[1] == f.v[2] || f.v[0] == f.v[2]) {
        printf("[Error Line %d] Degenerate triangle! Duplicate vertex indices in face (%d, %d, %d)\n",
            line_num, f.v[0], f.v[1], f.v[2]);
        return 0;
    }

    if (g_face_count < MAX_FACES) {
        g_faces[g_face_count++] = f;
    }
    return 1;
}

// 결과 출력
void print_results() {
    printf("\n==================== [ Parsing Results ] ====================\n");
    for (int i = 0; i < g_face_count; i++) {
        Face f = g_faces[i];
        printf("Face %d (%d, %d, %d): ", i + 1, f.v[0], f.v[1], f.v[2]);

        // Vertex 좌표 출력
        printf("vertex (%.1f, %.1f, %.1f) (%.1f, %.1f, %.1f) (%.1f, %.1f, %.1f)",
            g_vertices[f.v[0]].x, g_vertices[f.v[0]].y, g_vertices[f.v[0]].z,
            g_vertices[f.v[1]].x, g_vertices[f.v[1]].y, g_vertices[f.v[1]].z,
            g_vertices[f.v[2]].x, g_vertices[f.v[2]].y, g_vertices[f.v[2]].z);

        // Texture 좌표 출력 (존재하는 경우)
        if (f.has_texture) {
            printf("\n          texture (%.1f, %.1f) (%.1f, %.1f) (%.1f, %.1f)",
                g_textures[f.vt[0]].u, g_textures[f.vt[0]].v,
                g_textures[f.vt[1]].u, g_textures[f.vt[1]].v,
                g_textures[f.vt[2]].u, g_textures[f.vt[2]].v);
        }
        printf("\n");
    }
    printf("\n");
    check_duplicate_vertices();
    printf("=============================================================\n\n");
}

int main(void) {
    char filename[256];
    printf("텍스트 데이터 파일 경로를 입력하세요 (예: test.txt): ");
    if (scanf("%255s", filename) != 1) {
        return 1;
    }

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        return 1;
    }

    char line[MAX_LINE_LEN];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        char* p = trim_whitespace(line);

        // 빈 줄이거나 주석인 경우 건너뜀
        if (*p == '\0' || *p == '#') continue;

        // 인라인 주석 제거 (예: v 0.0 0.8 0.0 #5: 꼭대기...)
        char* comment_pos = strchr(p, '#');
        if (comment_pos) {
            *comment_pos = '\0';
            p = trim_whitespace(p);
        }

        // 명령어 식별
        if (strncmp(p, "vt", 2) == 0 && isspace((unsigned char)p[2])) {
            float u, v;
            if (sscanf(p + 2, "%f %f", &u, &v) == 2) {
                if (g_texture_count < MAX_TEXTURES) {
                    g_texture_count++;
                    g_textures[g_texture_count].u = u;
                    g_textures[g_texture_count].v = v;
                }
            }
            else {
                printf("[Error Line %d] Invalid vt format: %s\n", line_num, p);
            }
        }
        else if (p[0] == 'v' && isspace((unsigned char)p[1])) {
            float x, y, z;
            if (sscanf(p + 1, "%f %f %f", &x, &y, &z) == 3) {
                if (g_vertex_count < MAX_VERTICES) {
                    g_vertex_count++;
                    g_vertices[g_vertex_count].x = x;
                    g_vertices[g_vertex_count].y = y;
                    g_vertices[g_vertex_count].z = z;
                }
            }
            else {
                printf("[Error Line %d] Invalid v format: %s\n", line_num, p);
            }
        }
        else if (p[0] == 'f' && isspace((unsigned char)p[1])) {
            parse_face_line(p + 1, line_num);
        }
        else {
            printf("[Warning Line %d] Unknown command or format: %s\n", line_num, p);
        }
    }

    fclose(fp);

    // 파싱 완료 후 결과 및 예외처리 검사 결과 출력
    print_results();

    return 0;
}