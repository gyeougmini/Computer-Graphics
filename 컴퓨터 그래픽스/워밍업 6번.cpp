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

// 3D ���� ����ü
typedef struct {
    float x, y, z;
} Vertex;

// 2D �ؽ�ó ��ǥ ����ü
typedef struct {
    float u, v;
} TexCoord;

// �ﰢ�� �� ���� ����ü
typedef struct {
    int v[3];   // 1-based vertex indices
    int vt[3];  // 1-based texture indices (�ؽ�ó ������ �� 0)
    int has_texture;
} Face;

Vertex g_vertices[MAX_VERTICES + 1]; // 1-based index ���
TexCoord g_textures[MAX_TEXTURES + 1];
Face g_faces[MAX_FACES];

int g_vertex_count = 0;
int g_texture_count = 0;
int g_face_count = 0;

// ����/���� ���� ����
char* trim_whitespace(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// �ε��Ҽ��� ��
int are_vertices_equal(Vertex a, Vertex b) {
    return (fabsf(a.x - b.x) < EPSILON &&
        fabsf(a.y - b.y) < EPSILON &&
        fabsf(a.z - b.z) < EPSILON);
}

// ��ü ���� �ߺ� üũ
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

// face ���� �Ľ� �� ��ȿ�� �˻�
int parse_face_line(char* args, int line_num) {
    // ������ �������� ��ū �и�
    char* tokens[10];
    int token_count = 0;

    char* token = strtok(args, " \t\r\n");
    while (token != NULL) {
        // �ζ��� �ּ�(#)�� ������ �� ���Ĵ� ����
        if (token[0] == '#') break;
        if (token_count < 10) {
            tokens[token_count++] = token;
        }
        token = strtok(NULL, " \t\r\n");
    }

    // ���� ó�� 1: �ﰢ�� �������� 3���� �ƴ� ���
    if (token_count != 3) {
        printf("[Error Line %d] Face does not form a triangle! (Found %d vertices, expected 3)\n", line_num, token_count);
        return 0;
    }

    Face f;
    memset(&f, 0, sizeof(Face));

    for (int i = 0; i < 3; i++) {
        int v_idx = 0;
        int vt_idx = 0;

        // "v/vt" �Ǵ� "v" ���� �Ľ�
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

        // ���� ó�� 4: 0 ���� �Ǵ� ���ڰ� �ƴ� �߸��� �ε���
        if (v_idx <= 0) {
            printf("[Error Line %d] Invalid vertex index '%s' in face.\n", line_num, tokens[i]);
            return 0;
        }

        // ���� ó�� 3: �ε��� ���� �ʰ�
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

    // ���� ó�� 2: �� ������ ������ ���� �ε����� �ߺ� ���� ��� (�ﰢ�� �Ұ�)
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

// ��� ���
void print_results() {
    printf("\n==================== [ Parsing Results ] ====================\n");
    for (int i = 0; i < g_face_count; i++) {
        Face f = g_faces[i];
        printf("Face %d (%d, %d, %d): ", i + 1, f.v[0], f.v[1], f.v[2]);

        // Vertex ��ǥ ���
        printf("vertex (%.1f, %.1f, %.1f) (%.1f, %.1f, %.1f) (%.1f, %.1f, %.1f)",
            g_vertices[f.v[0]].x, g_vertices[f.v[0]].y, g_vertices[f.v[0]].z,
            g_vertices[f.v[1]].x, g_vertices[f.v[1]].y, g_vertices[f.v[1]].z,
            g_vertices[f.v[2]].x, g_vertices[f.v[2]].y, g_vertices[f.v[2]].z);

        // Texture ��ǥ ��� (�����ϴ� ���)
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
    printf("�ؽ�Ʈ ������ ���� ��θ� �Է��ϼ��� (��: test.txt): ");
    if (scanf("%255s", filename) != 1) {
        return 1;
    }

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("������ �� �� �����ϴ�: %s\n", filename);
        return 1;
    }

    char line[MAX_LINE_LEN];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        char* p = trim_whitespace(line);

        // �� ���̰ų� �ּ��� ��� �ǳʶ�
        if (*p == '\0' || *p == '#') continue;

        // �ζ��� �ּ� ���� (��: v 0.0 0.8 0.0 #5: �����...)
        char* comment_pos = strchr(p, '#');
        if (comment_pos) {
            *comment_pos = '\0';
            p = trim_whitespace(p);
        }

        // ��ɾ� �ĺ�
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

    // �Ľ� �Ϸ� �� ��� �� ����ó�� �˻� ��� ���
    print_results();

    return 0;
}