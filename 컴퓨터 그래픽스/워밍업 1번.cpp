#include <stdio.h>
#include <random>
#include <Windows.h>
#include <time.h>

#define rowcol 4

int matrix1[rowcol][rowcol];
int matrix2[rowcol][rowcol];
int matrix3[rowcol][rowcol];
int matrix4[rowcol][rowcol];
int minimum[rowcol];
int maximum[rowcol];

bool min = false;
bool max = false;

char c;
int sum;

int main() {

	srand((unsigned int)time(NULL));

	for (int i = 0; i < rowcol; ++i) {
		for (int j = 0; j < rowcol; ++j) {
			matrix1[i][j] = rand() % 10;
			matrix2[i][j] = rand() % 10;
		}
	}

	for (int i = 0; i < rowcol; ++i) {
		for (int j = 0; j < rowcol; ++j) {
			printf("%d ", matrix1[i][j]);
		}
		printf("\n");
	}

	printf("\n");

	for (int i = 0; i < rowcol; ++i) {
		for (int j = 0; j < rowcol; ++j) {
			printf("%d ", matrix2[i][j]);
		}
		printf("\n");
	}

	while (1) {

		printf("\n명령어 입력\n[m - 곱셈 / a - 덧셈 / d - 뺄셈 / r - 행렬식의값 / t - 전치행렬 / e - 최솟값 빼기 / f - 최대값 더하기 /\n +, - / s - 재설정 / q - 종료]: ");

		scanf("%s", &c);

		switch (c) {
		case 'a':

			if (min == true || max == true) {
				printf("e 또는 f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						printf("%d ", matrix1[i][j] + matrix2[i][j]);
					}
					printf("\n");
				}
				printf("\n");
			}

			break;

		case 'd':
			if (min == true || max == true) {
				printf("e 또는 f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						printf("%d ", matrix1[i][j] - matrix2[i][j]);
					}
					printf("\n");
				}
				printf("\n");
			}
			break;

		case 'm':
			if (min == true || max == true) {
				printf("e 또는 f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						for (int k = 0; k < rowcol; ++k) {
							sum += matrix1[i][k] * matrix2[k][j];
						}
						printf("%d ", sum);
						sum = 0;
					}
					printf("\n");
				}
				printf("\n");
			}
			break;

		case 'r':
			for (int i = 0; i < rowcol; ++i) {
				for (int j = 0; j < rowcol; ++j) {
					if (i == 0) continue;
				}
			}

			break;

		case 't':
			if (min == true || max == true) {
				printf("e 또는 f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else{
				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						printf("%d ", matrix1[j][i]);
						matrix3[i][j] = matrix1[j][i];
					}
					printf("\n");
				}

				printf("\n");

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						printf("%d ", matrix2[j][i]);
						matrix4[i][j] = matrix2[j][i];
					}
					printf("\n");
				}
				printf("\n");

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						matrix1[i][j] = matrix3[i][j];
						matrix2[i][j] = matrix4[i][j];
					}
				}
			}

			break;

		case 'e':
			if (max == true) {
				printf("f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				if (min == false) {
					for (int i = 0; i < rowcol; ++i) {
						minimum[i] = 10;
						for (int j = 0; j < rowcol; ++j) {
							if (minimum[i] > matrix1[i][j]) {
								minimum[i] = matrix1[i][j];
							}
						}
					}
					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							matrix3[i][j] = matrix1[i][j] - minimum[i];
							printf("%d ", matrix3[i][j]);
						}
						printf("\n");
					}

					printf("\n");

					for (int i = 0; i < rowcol; ++i) {
						minimum[i] = 10;
						for (int j = 0; j < rowcol; ++j) {
							if (minimum[i] > matrix2[i][j]) {
								minimum[i] = matrix2[i][j];
							}
						}
					}
					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							matrix4[i][j] = matrix2[i][j] - minimum[i];
							printf("%d ", matrix4[i][j]);
						}
						printf("\n");
					}
					printf("\n");
					min = true;
				}
				else {
					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							printf("%d ", matrix1[i][j]);
						}
						printf("\n");
					}

					printf("\n");

					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							printf("%d ", matrix2[i][j]);
						}
						printf("\n");
					}
					min = false;
				}
			}
			break;

		case 'f':
			if (min == true) {
			printf("e가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				if (max == false) {
					for (int j = 0; j < rowcol; ++j) {
						maximum[j] = -1;
						for (int i = 0; i < rowcol; ++i) {
							if (maximum[j] < matrix1[i][j]) {
								maximum[j] = matrix1[i][j];
							}
						}
					}
					for (int j = 0; j < rowcol; ++j) {
						for (int i = 0; i < rowcol; ++i) {
							matrix3[i][j] = matrix1[i][j] + maximum[j];
						}
					}

					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							printf("%d ", matrix3[i][j]);
						}
						printf("\n");
					}

					printf("\n");

					for (int j = 0; j < rowcol; ++j) {
						maximum[j] = -1;
						for (int i = 0; i < rowcol; ++i) {
							if (maximum[j] < matrix2[i][j]) {
								maximum[j] = matrix2[i][j];
							}
						}
					}
					for (int j = 0; j < rowcol; ++j) {
						for (int i = 0; i < rowcol; ++i) {
							matrix4[i][j] = matrix2[i][j] + maximum[j];
						}
					}

					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							printf("%d ", matrix4[i][j]);
						}
						printf("\n");
					}

					max = true;
				}
				else {
					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							printf("%d ", matrix1[i][j]);
						}
						printf("\n");
					}

					printf("\n");

					for (int i = 0; i < rowcol; ++i) {
						for (int j = 0; j < rowcol; ++j) {
							printf("%d ", matrix2[i][j]);
						}
						printf("\n");
					}
					max = false;
				}
			}
			break;

		case '+':
			if (min == true || max == true) {
				printf("e 또는 f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {

						matrix3[i][j] = matrix1[i][j] + 1;

						if (matrix3[i][j] >= 10) {
							matrix3[i][j] = 0;
						}

						printf("%d ", matrix3[i][j]);
					}
					printf("\n");
				}

				printf("\n");

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {

						matrix4[i][j] = matrix2[i][j] + 1;

						if (matrix4[i][j] >= 10) {
							matrix4[i][j] = 0;
						}

						printf("%d ", matrix4[i][j]);
					}
					printf("\n");
				}
				printf("\n");

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						matrix1[i][j] = matrix3[i][j];
						matrix2[i][j] = matrix4[i][j];
					}
				}
			}
			break;

		case '-':
			if (min == true || max == true) {
				printf("e 또는 f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {

						matrix3[i][j] = matrix1[i][j] - 1;

						if (matrix3[i][j] < 0) {
							matrix3[i][j] = 9;
						}

						printf("%d ", matrix3[i][j]);
					}
					printf("\n");
				}

				printf("\n");

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {

						matrix4[i][j] = matrix2[i][j] - 1;

						if (matrix4[i][j] < 0) {
							matrix4[i][j] = 9;
						}

						printf("%d ", matrix4[i][j]);
					}
					printf("\n");
				}
				printf("\n");

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						matrix1[i][j] = matrix3[i][j];
						matrix2[i][j] = matrix4[i][j];
					}
				}
			}
			break;

		case 's':
			if (min == true || max == true) {
				printf("e 또는 f가 켜져있습니다. 해제한 후 실행해주십시오.\n");
			}
			else {
				for (int i = 0; i <= rowcol; ++i) {
					for (int j = 0; j <= rowcol; ++j) {
						matrix1[i][j] = rand() % 10;
						matrix2[i][j] = rand() % 10;
					}
				}

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						printf("%d ", matrix1[i][j]);
					}
					printf("\n");
				}

				printf("\n");

				for (int i = 0; i < rowcol; ++i) {
					for (int j = 0; j < rowcol; ++j) {
						printf("%d ", matrix2[i][j]);
					}
					printf("\n");
				}
			}
			break;

		case 'q':
			return 0;
		}
	}
}