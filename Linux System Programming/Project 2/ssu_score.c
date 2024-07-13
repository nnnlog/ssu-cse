#include "ssu_score.h"
#include "blank.h"
#include "linked_list.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

extern struct ssu_scoreTable score_table[QNUM];
extern char id_table[SNUM][10];

struct ssu_scoreTable score_table[QNUM];
char id_table[SNUM][10];

char stuDir[BUFLEN]; // student dir
char ansDir[BUFLEN]; // answer dir
char scoreDir[BUFLEN]; // score.csv
char errorDir[BUFLEN]; // error directory
char threadFiles[ARGNUM][FILELEN]; // variables: -lthread
char printScoreStudent[ARGNUM][BUFLEN]; // variables: -c
char printWrongStudent[ARGNUM][BUFLEN]; // variables: -p
int sortByStudentId = true; // otherwise, sort by score
int sortAscending = true; // otherwise, descending

int eOption = false;
int cOption = false;
int cOptionForAll = false;
int pOption = false;
int pOptionForAll = false;
int tOption = false;
int tOptionForAll = false;
int sOption = false;
int mOption = false;

struct Node_sort *root_sort;

int cmp_sort_node(const void *x, const void *y) {
    const struct Node_sort *a = *(const struct Node_sort**)x, *b = *(const struct Node_sort**)y; // casting

    if (sortByStudentId) {
        return ((strcmp(a->name, b->name) < 0) ^ sortAscending); // ascending일 때 strcmp > 0이면, x와 y 위치 변경함, vice versa.
    } else {
        return ((a->score < b->score) ^ sortAscending); // ascending일 때 A_score > B_score이면, x와 y 위치 변경함, vice versa.
    }
}

void ssu_score(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0); // buffer 없이 바로 출력 (디버깅용)

	char saved_path[BUFLEN];
	int i;

	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-h")){ // 인자 중 하나라도 -h가 있으면 바로 도움말 출력
			print_usage();
            exit(0);
		}
	}

	memset(saved_path, 0, BUFLEN);
	if(argc >= 3){
		strcpy(stuDir, argv[1]); // STU_DIR
		strcpy(ansDir, argv[2]); // ANS_DIR
	}

	if(!check_option(argc, argv)) // 주어진 인자 확인
		exit(1);

    if (!*scoreDir) sprintf(scoreDir, "%s/%s", ansDir, "score.csv"); // -n 옵션에 의해 score.csv의 경로가 정해지지 않았다면 자동으로 ANS_DIR/score.csv 지정

	getcwd(saved_path, BUFLEN); // 현재 작업 디렉토리 가져옴

	if(chdir(stuDir) < 0){ // cd
		fprintf(stderr, "%s doesn't exist\n", stuDir);
		return;
	}
	getcwd(stuDir, BUFLEN); // 절대 경로로 설정

	chdir(saved_path); // 현재 작업 디렉토리 가져옴
	if(chdir(ansDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", ansDir);
		return;
	}
	getcwd(ansDir, BUFLEN); // 절대 경로로 설정

	chdir(saved_path); // 원래 작업 디렉토리로 돌아옴

	set_scoreTable(ansDir); // 문제별 배점(score_table.csv) 설정
	set_idTable(stuDir); // 학생 목록 생성

	if(mOption)
		do_mOption(); // -m 있으면 배점 수정 작업 진행

	printf("grading student's test papers..\n");
	score_students(); // 채점 진행
}

// 학번이 주어지면 STU_DIR 아래에 있는지 판별하는 함수
void check_student_exist(char *id) {
    char tmp[PATH_MAX];
    sprintf(tmp, "%s/%s", stuDir, id); // 경로 만들고
    if (access(tmp, F_OK)) { // 존재하지 않으면 에러 처리
        printf("%s doesn't exist in %s.\n", id, stuDir);
        exit(1);
    }
}

// 프로그램 파일 이름이 주어지면 ANS_DIR 아래에 있는지 판별하는 함수
void check_cfile_exist(char *probid) {
    char tmp[PATH_MAX];
    sprintf(tmp, "%s/%s.c", ansDir, probid); // 경로 만들고
    if (access(tmp, F_OK)) { // 존재하지 않으면 에러 처리
        printf("%s doesn't exist in %s.\n", probid, ansDir);
        exit(1);
    }
}

int check_option(int argc, char *argv[])
{
	int i, j, k;
	int c;

	while((c = getopt(argc, argv, "n:cpe:ts:m")) != -1) // -n <>, -c, -p, -e <>, -t, -s <>, -m 인자만 허용 (c, p는 가변 인자)
	{
		switch(c){
			case 'n':
				strcpy(scoreDir, optarg); // scoreDir에 복사

                int len = strlen(scoreDir);
                if (len < 5 || strcmp(scoreDir + strlen(scoreDir) - 4, ".csv")) { // |a.csv| = 5, 뒤에 4개만 떼서 봤을 때 .csv 인지 아닌지 확인
                    printf("given score path(%s) is not 'csv' extension\n", scoreDir);
                    return false;
                }
                break;
			case 'e':
				eOption = true;
				strcpy(errorDir, optarg); // errorDir에 복사

				if(access(errorDir, F_OK) < 0) // 존재하지 않으면 mkdir
					mkdir(errorDir, 0755);
				else{
					rmdirs(errorDir); // 존재하면 errorDir의 내용 전체 삭제
					mkdir(errorDir, 0755); // 후 mkdir
				}
				break;
            case 'c':
                cOption = true;
                i = optind;
                j = 0;

                while(i < argc && argv[i][0] != '-'){ // 현재 인자 인덱스부터 하나씩 증가하며 확인
                    if(j == ARGNUM)
                        printf("Maximum Number of Argument Exceeded. :: "); // 인자 개수 초과 시 경고 출력

                    if(j < ARGNUM) {
                        strcpy(printScoreStudent[j], argv[i]); // 인자 개수 미만이면 정상적으로 복사
                        check_student_exist(printScoreStudent[j]); // 실제 존재 여부 확인
                    } else {
                        printf("%s ", argv[i]); // 인자 개수 초과하면 출력만 하고 아무 것도 안 함
                    }
                    i++;
                    j++;
                }

                if (j >= ARGNUM) printf("\n"); // 인자 개수 초과에 의한 경고가 출력됐으면 개행 처리

                if (!j) {
                    cOptionForAll = true;
                }

                optind = i; // 다음 인자 인덱스 변경
                break;
            case 'p':
                pOption = true;
                i = optind;
                j = 0;

                while(i < argc && argv[i][0] != '-'){ // 현재 인자 인덱스부터 하나씩 증가하며 확인
                    if(j == ARGNUM)
                        printf("Maximum Number of Argument Exceeded. :: "); // 인자 개수 초과 시 경고 출력

                    if(j < ARGNUM) {
                        strcpy(printWrongStudent[j], argv[i]); // 인자 개수 미만이면 정상적으로 복사
                        check_student_exist(printWrongStudent[j]); // 실제 존재 여부 확인
                    } else {
                        printf("%s ", argv[i]);
                    }
                    i++;
                    j++;
                }

                if (j >= ARGNUM) printf("\n"); // 인자 개수 초과에 의한 경고가 출력됐으면 개행 처리

                if (!j) {
                    pOptionForAll = true;
                }

                optind = i; // 다음 인자 인덱스 변경
                break;
			case 't':
                tOption = true;
				i = optind;
				j = 0;

				while(i < argc && argv[i][0] != '-'){ // 현재 인자 인덱스부터 하나씩 증가하며 확인
                    if(j == ARGNUM)
                        printf("Maximum Number of Argument Exceeded. :: "); // 인자 개수 초과 시 경고 출력

                    if(j < ARGNUM) {
						strcpy(threadFiles[j], argv[i]); // 인자 개수 미만이면 정상적으로 복사
                        check_cfile_exist(threadFiles[j]); // 실제 존재 여부 확인
					} else {
                        printf("%s ", argv[i]);
                    }

					i++;
					j++;
				}

                if (j >= ARGNUM) printf("\n");

                if (!j) {
                    tOptionForAll = true;
                }

                optind = i; // 다음 인자 인덱스 변경
				break;
            case 's':
                sOption = true;

                // 현재 인자가 stdid인지 score인지 확인
                if (!strcmp(optarg, "stdid")) {
                    sortByStudentId = true;
                } else if (!strcmp(optarg, "score")) {
                    sortByStudentId = false;
                } else {
                    printf("Unknown option %s\n", optarg);
                    return false;
                }

                // overflow 확인
                if (argc <= optind) {
                    printf("Need <1|-1>\n");
                    return false;
                }

                // 1(asc)인지 -1(desc)인지 확인
                char *sortOp = argv[optind++];
                if (!strcmp(sortOp, "1")) {
                    sortAscending = true;
                } else if (!strcmp(sortOp, "-1")) {
                    sortAscending = false;
                } else {
                    printf("Unknown option %s\n", sortOp);
                    return false;
                }

                break;
			case 'm':
				mOption = true;
				break;
			case '?':
				printf("Unknown option %c\n", optopt);
				return false;
		}
	}

    if (cOption && pOption) { // -c와 -p는 인자 공유함
        if (!cOptionForAll && !pOptionForAll) { // 두 인자 모두 각각에 대한 가변인자가 주어지면 에러 처리
            printf("[STUDENTIDS...] variables of -p and -c was given\n");
            exit(1);
        } else if (cOptionForAll && pOptionForAll) { // 둘다 가변인자 입력하지 않으면 아무 것도 안 함

        } else if (cOptionForAll) { // copy: printWrong -> printScoreStudent
            cOptionForAll = false;
            for (int a = 0; a < ARGNUM; a++) strcpy(printScoreStudent[a], printWrongStudent[a]);
        } else { // copy: printScoreStudent -> printWrong
            pOptionForAll = false;
            for (int a = 0; a < ARGNUM; a++) strcpy(printWrongStudent[a], printScoreStudent[a]);
        }
    }

	return true;
}

void do_mOption(void)
{
	double newScore;
	char modiName[FILELEN];
	char filename[FILELEN];
	char *ptr;
	int i;

	ptr = malloc(sizeof(char) * FILELEN);

	while(1){

		printf("Input question's number to modify >> ");

		if(scanf("%s", modiName) == EOF || strcmp(modiName, "no") == 0) // EOF 또는 no 들어오면 입력 종료
			break;

		for(i=0; i < sizeof(score_table) / sizeof(score_table[0]); i++){
            if (!strcmp(score_table[i].qname, "")) continue; // null 문자면 아무 작업 안 함
			strcpy(ptr, score_table[i].qname);
			ptr = strtok(ptr, "."); // .으로 확장자 무시
			if(!strcmp(ptr, modiName)){
				printf("Current score : %.2f\n", score_table[i].score); // 현재 점수 출력
				printf("New score : ");
				scanf("%lf", &newScore);
				getchar(); // 개행 제거
				score_table[i].score = newScore;
				break; // 다음 문제 입력 대기
			}
		}
	}

    sprintf(filename, "%s/%s", ansDir, "score_table.csv");
	write_scoreTable(filename); // score_table.csv 재작성
	free(ptr);

}

void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];

	sprintf(filename, "%s/%s", ansDir, "score_table.csv"); // 경로 생성

	// check exist
	if(access(filename, F_OK) == 0)
		read_scoreTable(filename); // 기존 파일 파싱
	else{
        if (mOption) { // -m 옵션이 주어졌을 때, 원래부터 파일이 없었으면 에러처리
            printf("given -m option, but doesn't exist score table file.\n");
            exit(1);
        }
		make_scoreTable(ansDir); // 새로운 파일 생성
		write_scoreTable(filename); // 파일 저장
	}
}

void read_scoreTable(char *path)
{
	FILE *fp;
	char qname[FILELEN];
	char score[BUFLEN];
	int idx = 0;

	if((fp = fopen(path, "r")) == NULL){ // read-only로 open
		fprintf(stderr, "file open error for %s\n", path);
		return ;
	}

	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){ // <a>,<b>를 한 줄 단위로 파싱해서 각각 a, b 변수에 대입함
		strcpy(score_table[idx].qname, qname); // 문제 번호
		score_table[idx++].score = atof(score); // 점수
	}

	fclose(fp);
}

void make_scoreTable(char *ansDir)
{
	int type, num;
	double score, bscore, pscore;
	struct dirent *dirp, *c_dirp;
	DIR *dp, *c_dp;
	char *tmp;
	int idx = 0;
	int i;

	num = get_create_type(); // 어떤 유형으로 score_table.csv 초기 세팅할지 받아옴

	if(num == 1) // 단 두 개의 입력으로 일괄 처리
	{
		printf("Input data of blank question : ");
		scanf("%lf", &bscore);
		printf("Input data of program question : ");
		scanf("%lf", &pscore);
	}

	if((dp = opendir(ansDir)) == NULL){
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	}

	while((dirp = readdir(dp)) != NULL){ // ansDir 내부에 있는 파일명으로 하나씩 점수 입력 받음

		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		if((type = get_file_type(dirp->d_name)) < 0) // cfile/txtfile 아니면 처리 안 함
			continue;

		strcpy(score_table[idx].qname, dirp->d_name); // score_table 배열에 먼저 저장

		idx++;
	}

	closedir(dp);
	sort_scoreTable(idx); // 문제 번호 순으로 정렬

	for(i = 0; i < idx; i++)
	{
		type = get_file_type(score_table[i].qname);

		if(num == 1) // 일괄 처리면 바로 점수 설정
		{
			if(type == TEXTFILE)
				score = bscore;
			else if(type == CFILE)
				score = pscore;
		}
		else if(num == 2)
		{
			printf("Input of %s: ", score_table[i].qname); // 아니면 하나씩 입력 받음
			scanf("%lf", &score);
		}

		score_table[i].score = score;
	}
}

void write_scoreTable(char *filename)
{
	int fd;
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);

	if((fd = creat(filename, 0666)) < 0){
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for(i = 0; i < num; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		write(fd, tmp, strlen(tmp)); // 문제별로 한줄씩 <문제 번호>,<점수> 형태로 작성
	}

	close(fd);
}


void set_idTable(char *stuDir) // 학생 폴더로부터 학번을 불러온다.
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	if((dp = opendir(stuDir)) == NULL){
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){ // STU_DIR 하위 파일 명 하나씩 가져옴
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		stat(tmp, &statbuf);

		if(S_ISDIR(statbuf.st_mode)) // dir일 때만 처리
			strcpy(id_table[num++], dirp->d_name); // 디렉토리 명으로 id_table에 값 넣음
		else
			continue;
	}
	closedir(dp);

	sort_idTable(num);
}

void sort_idTable(int size) // 학번으로 정렬
{
	int i, j;
	char tmp[10];

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){ // strcmp < 0이 아닌 경우, 즉 > 0일 때 swap함
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

void sort_scoreTable(int size) // 문제 번호로 정렬
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){
            // i번째, j번째의 문제 이름을 가져옴
			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){ // i번째 문제 번호 > j번째 문제 번호라면 swap함

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];

	strncpy(dup, qname, strlen(qname));
	*num1 = atoi(strtok(dup, "-.")); // - 또는 . 기준으로 그 앞의 문자열을 먼저 변환

	p = strtok(NULL, "-."); // -로 구분되어 그 다음 문제 번호가 더 남아있는지 확인
	if(p == NULL)
		*num2 = 0;
	else
		*num2 = atoi(p); // 만약 그렇다면 계속해서 숫자 변환
}

int get_create_type() // 문제 배점 생성 방법 입력 받음
{
	int num;
    char tmp[BUFLEN];

	while(1)
	{
		printf("score_table.csv file doesn't exist in \"%s\"!\n", realpath(ansDir, tmp));
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input data of 1-1: 0.1\n");
		printf("select type >> ");
		if (scanf("%d", &num) == EOF) exit(1);

		if(num != 1 && num != 2)
			printf("not correct number!\n"); // 제대로 입력 안 하면 다시 시도
		else
			break;
	}

	return num;
}

void score_students()
{
	double score = 0;
	int num;
	int fd;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);

	if((fd = creat(scoreDir, 0666)) < 0){ // score.csv 생성
		fprintf(stderr, "creat error for %s\n", scoreDir);
		return;
	}
	write_first_row(fd); // 첫번째 행(header) 작성

    root_sort = NULL; // 채점 후 정렬을 위한 linked list node

    int scored_num = 0;
	for(num = 0; num < size; num++)
	{
		if(!strcmp(id_table[num], ""))
			break;

//		sprintf(tmp, "%s,", id_table[num]); move to score_student
//		write(fd, tmp, strlen(tmp));

        int ok = cOptionForAll; // ok=1이면 점수를 출력해야 하는 학생임 (+평균에 반영해야 하는 학생)
        if (cOption) {
            for (int j = 0; j < ARGNUM; j++) {
                if (!strcmp(printScoreStudent[j], id_table[num])) { // 하나씩 순회하며 확인
                    ok = 1;
                }
            }
        }

        double tmp_score = score_student(fd, id_table[num]); // score_student 호출해서 채점 진행 후 학생의 총점 받아옴
        if (ok) {
            scored_num++; // 개수 증가
            score += tmp_score; // 총점 증가
        }
	}

    if (sOption) { // sort when sOption was given.
        int length = 0;
        struct Node_sort *node = root_sort;
        while (node) ++length, node = node->next; // 먼저 링크드 리스트의 길이를 잼
        node = root_sort;
        struct Node_sort **arr = malloc(length * sizeof(struct Node_sort*)); // 단방향 연결 리스트임으로 단순 정렬은 불가능하므로 동적 할당하여 배열로 처리
        for (int i = 0; i < length; i++) {
            arr[i] = node; // 포인터만 저장함
            node = node->next;
        }

        qsort(arr, length, sizeof (struct Node_sort**), cmp_sort_node); // qsort 사용하여 정렬

        root_sort = arr[0];
        node = root_sort;
        for (int i = 1; i < length; i++) {
            node->next = arr[i]; // 다시 루트 노드부터 돌면서 arr의 포인터를 순서대로 연결 리스트에 넣어줌
            node = node->next;
        }
        node->next = 0; // 마지막 값은 NULL로 해줘야 함

        free(arr);
    }

    { // write
        struct Node_sort *node = root_sort;
        while (node) {
            write(fd, node->data, strlen(node->data)); // 하나씩 순회하면서 해당 학생으로부터 생성되는 파일 내용을 작성함(한 줄 단위임)
            node = node->next;
        }

        clear_linked_sort_problems(root_sort);
        root_sort = NULL;
    }

    if (cOption) printf("Total average : %.2f\n", score / scored_num); // -c 옵션일 때만, 해당 학생들의 평균 출력

    printf("result saved.. (%s)\n", realpath(scoreDir, tmp)); // 절대 경로로 변경하여 출력
	close(fd);
}

double score_student(int fd, char *id) // 한명씩 채점 진행함
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

    struct Node_wrong_problems *root = NULL, *end = NULL;
    struct Node_sort *node_sort = create_linked_list_sort_node(); // 전역으로 관리되는 학생 채점 결과 연결 리스트에 삽입할 노드 생성

    if (root_sort == NULL) root_sort = node_sort; // 해당 노드를 루트 노드로 지정
    else {
        struct Node_sort *tmp = root_sort;
        while (tmp->next) tmp = tmp->next;
        tmp->next = node_sort; // 연결 리스트의 가장 끝에 노드 삽입
    }

    strcpy(node_sort->name, id); // 학번도 노드에 저장

    sprintf(tmp, "%s,", id);
    strcat(node_sort->data, tmp); // 학번을 작성함

	for(i = 0; i < size ; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname); // 제출 답안이 존재하는지 확인

		if(access(tmp, F_OK) < 0)
			result = false; // 존재 안 하면 0점 처리
		else
		{
			if((type = get_file_type(score_table[i].qname)) < 0) // 올바른 파일 타입 아니면 pass
				continue;

			if(type == TEXTFILE) // txt 파일이면 빈칸 채점 진행
				result = score_blank(id, score_table[i].qname);
			else if(type == CFILE) // c 파일이면 프로그램 채점 진행
				result = score_program(id, score_table[i].qname);
		}

		if(result == false) {
            // 틀린 문제 목록을 만듦
            sprintf(tmp, "%s(%.2g)", score_table[i].qname, score_table[i].score); // 문제번호(점수) 형태로 함
            struct Node_wrong_problems *curr = create_linked_list_wrong_problems_node(tmp); // 각각은 연결 리스트 형태로 저장됨
            if (end == NULL) root = end = curr;
            else end = end->next = curr;

            sprintf(tmp, "0,"); // 0점
        }else{
			if(result == true){
				score += score_table[i].score;
				sprintf(tmp, "%.2f,", score_table[i].score); // 점수
			}
			else if(result < 0){
				score = score + score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result); // 음수인 경우 별도로 처리함
			}
		}

        strcat(node_sort->data, tmp); // 현재 문제로 생성된 채점 결과를 데이터에 추가함
	}

    int print_score = 0; // 점수를 출력해야 하는 학생인지 확인
    if (cOptionForAll) print_score = 1;
    else if (cOption) {
        for (int j = 0; j < ARGNUM; j++) {
            if (!strcmp(printScoreStudent[j], id)) { // 하나씩 보면서 확인
                print_score = true;
            }
        }
    }

    int print_wrong = 0; // 틀린 문제를 출력해야 하는지 확인
    if (pOptionForAll) print_wrong = 1;
    else if (pOption) {
        for (int j = 0; j < ARGNUM; j++) {
            if (!strcmp(printWrongStudent[j], id)) { // 하나씩 보면서 확인
                print_wrong = true;
            }
        }
    }

	printf("%s is finished..", id);
    if (print_score) printf(" score : %.2f", score); // 점수 출력
    if (print_wrong) { // 틀린 문제 출력
        if (print_score) printf(","); // 점수 출력했으면 ,로 구분
        printf(" wrong problem : ");
        struct Node_wrong_problems *node = root;
        while (node) { // 틀린 문제 출력
            fputs(node->value, stdout);
            if (node->next) printf(", ");
            node = node->next;
        }
    }
    printf("\n");

    node_sort->score = score; // 노드에 학생 점수도 넣음

	sprintf(tmp, "%.2f\n", score); // 여기서 파일에 작성하지 않고 전체 줄 정보랑 점수를 Linked List에 넣음
    strcat(node_sort->data, tmp);

    clear_linked_list_wrong_problems(root);

	return score;
}

void write_first_row(int fd) // score.csv 파일의 header를 작성함
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	write(fd, ",", 1); // 첫 번째 column은 생략

	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,", score_table[i].qname); // 두번째 칼럼부터 하나씩 문제 번호 넣음
		write(fd, tmp, strlen(tmp));
	}
	write(fd, "sum\n", 4); // 총점
}

char *get_answer(int fd, char *result) // txt 파일의 답안을 :로 구분하여 리턴
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN);
	while(read(fd, &c, 1) > 0) // 하나씩 입력
	{
		if(c == ':')
			break;

		result[idx++] = c; // 저장
	}
	if(result[strlen(result) - 1] == '\n')
		result[strlen(result) - 1] = '\0'; // 마지막이 \n이면 null 문자로 대체

	return result;
}

int score_blank(char *id, char *filename) // 빈칸 문제 채점
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL; // 구문 트리 루트
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	fd_std = open(tmp, O_RDONLY); // 학생 파일 열기
	strcpy(s_answer, get_answer(fd_std, s_answer)); // 학생 답안은 하나만 가져옴

	if(!strcmp(s_answer, "")){ // 답안이 비어있으면 0점 처리
		close(fd_std);
		return false;
	}

	if(!check_brackets(s_answer)){ // 괄호 syntax 확인
		close(fd_std);
		return false;
	}

	strcpy(s_answer, ltrim(rtrim(s_answer))); // 양쪽 공백 문자 제거

	if(s_answer[strlen(s_answer) - 1] == ';'){ // 세미콜론 여부 확인 후 제거
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	}

	if(!make_tokens(s_answer, tokens)){ // 학생 답안으로부터 token 생성
		close(fd_std);
		return false; // 오류 발생하면 0점 처리
	}

	idx = 0;
	std_root = make_tree(std_root, tokens, &idx, 0); // 트리 생성

	sprintf(tmp, "%s/%s", ansDir, filename); // 답안 파일 열기
	fd_ans = open(tmp, O_RDONLY);

	while(1)
	{
		ans_root = NULL;
		result = true;

		for(idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		strcpy(a_answer, get_answer(fd_ans, a_answer)); // : 기준으로 하나씩 입력 받음

		if(!strcmp(a_answer, "")) // 더이상 받을 문자 없으면 종료함
			break;

        // 여기서부터는 학생 답안에 했던 작업을 그대로 진행함
		strcpy(a_answer, ltrim(rtrim(a_answer)));

		if(has_semicolon == false){
			if(a_answer[strlen(a_answer) -1] == ';')
				continue;
		}

		else if(has_semicolon == true)
		{
			if(a_answer[strlen(a_answer) - 1] != ';')
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';
		}

		if(!make_tokens(a_answer, tokens))
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0);

        // 학생 답안과 모범 답안의 트리를 비교함
		compare_tree(std_root, ans_root, &result);

		if(result == true){ // 똑같으면 정답 처리
			close(fd_std);
			close(fd_ans);

			if(std_root != NULL)
				free_node(std_root);
			if(ans_root != NULL)
				free_node(ans_root);
			return true;

		}
	}

	close(fd_std);
	close(fd_ans);

	if(std_root != NULL)
		free_node(std_root);
	if(ans_root != NULL)
		free_node(ans_root);

	return false;
}

double score_program(char *id, char *filename) // 프로그램 파일 채점
{
	double compile;
	int result;

	compile = compile_program(id, filename); // 프로그램 컴파일

	if(compile == ERROR || compile == false) // 오류 나면 0점 처리
		return false;

	result = execute_program(id, filename); // 프로그램 실행

	if(!result)
		return false; // 실패하면 0점 처리

	if(compile < 0)
		return compile; // 컴파일에서 0점 미만 받으면 그 점수 그대로 리턴

	return true; // 정답 처리
}

int is_thread(char *qname) // -t에 의한 -lpthread 필요한지 확인하는 함수
{
    if (tOptionForAll) return true; // 가변인자가 없으면 모든 문제에 대해 -lpthread를 넣음

	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname)) // 하나씩 돌아보며 일치하는지 확인
			return true;
	}
	return false;
}

double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	isthread = is_thread(qname); // -lpthread 여부

	sprintf(tmp_f, "%s/%s", ansDir, filename); // 소스 코드
	sprintf(tmp_e, "%s/%s.exe", ansDir, qname); // 실행 파일

    // 프로그램 컴파일 명령어 생성
	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);
	fd = creat(tmp_e, 0666);

    // 표준 에러를 파일로 redirection 해서 빌드 진행
	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	unlink(tmp_e);

	if(size > 0)
		return false; // 모범 답안이 컴파일 에러 시 채점 중단

        // 학생 파일에 대해서도 똑같이 빌드 진행함.
	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

    // 임시 에러 파일 생성
	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	fd = creat(tmp_f, 0666);
    if (fd < 0) {
        printf("creating error file(%s) failed.\n", tmp_f);
        exit(1);
    }

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if(size > 0){ // 표준 에러가 안 비어있으면,
		if(eOption) // 우선 error/ 폴더로 이동함
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			if(access(tmp_e, F_OK) < 0) // mkdir
				mkdir(tmp_e, 0755);

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname); // 경로 생성
			if (rename(tmp_f, tmp_e) < 0) { // 오류 파일 생성 실패 시 프로그램 종료
                printf("moving error file('%s' -> '%s') failed.\n", tmp_f, tmp_e);
                exit(1);
            }

			result = check_error_warning(tmp_e); // 워닝 수 체크
		}
		else{
			result = check_error_warning(tmp_f); // 워닝 수 체크
			unlink(tmp_f); // 에러 파일 삭제함
		}

		return result;
	}

	unlink(tmp_f); // 에러 파일 삭제함
	return true;
}

double check_error_warning(char *filename) // 워닝 수 체크
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while(fscanf(fp, "%s", tmp) > 0){
		if(!strcmp(tmp, "error:")) // error 있으면 0점 처리
			return ERROR;
		else if(!strcmp(tmp, "warning:"))
			warning += WARNING; // 워닝은 매크로 상수 더해서 감점 처리함
	}

	return warning;
}

int execute_program(char *id, char *filename) // 프로그램 실행
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.'))); // 문제 번호 가져옴

	sprintf(ans_fname, "%s/%s.stdout", ansDir, qname); // 표준 출력 저장
	fd = creat(ans_fname, 0666);

	sprintf(tmp, "%s/%s.exe", ansDir, qname); // 실행 파일 경로
	redirection(tmp, fd, STDOUT); // 실행 후 표준 출력 redirection
	close(fd);

    // 학생 답안에 대해서도 위 과정을 똑같이 진행
	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);
	fd = creat(std_fname, 0666);

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname); //  2> /dev/null

    // 시간 측정
	start = time(NULL);
	redirection(tmp, fd, STDOUT);

	sprintf(tmp, "%s.stdexe", qname);
	while((pid = inBackground(tmp)) > 0){ // 백그라운드에서 실행 종료되는지 확인
		end = time(NULL);

		if(difftime(end, start) > OVER){ // OVER 초과 시 강제 종료 후 0점 처리
			kill(pid, SIGKILL);
			close(fd);
			return false;
		}
	}

	close(fd);

	return compare_resultfile(std_fname, ans_fname); // 실행 후 출력 결과 비교
}

pid_t inBackground(char *name) // 백그라운드에서 실행 중인지 확인
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;

	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666); // 임시 파일 생성

	sprintf(command, "ps | grep %s", name); // name으로 실행되는 프로세스가 존재하는지 확인하는 명령어 생성
	redirection(command, fd, STDOUT); // 명령어 실행

	lseek(fd, 0, SEEK_SET);
	read(fd, tmp, sizeof(tmp));

	if(!strcmp(tmp, "")){ // 임시 파일이 비어있으면 0 리턴
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt"); // 파일 제거
	return pid; // 실행 중인 pid 리턴
}

int compare_resultfile(char *file1, char *file2) // 채점 결과 비교
{
	int fd1, fd2;
	char c1, c2;
	int len1, len2;

	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);

	while(1)
	{
        // 둘 다 하나씩 가져옴, 공백은 무시
		while((len1 = read(fd1, &c1, 1)) > 0){
			if(c1 == ' ')
				continue;
			else
				break;
		}
		while((len2 = read(fd2, &c2, 1)) > 0){
			if(c2 == ' ')
				continue;
			else
				break;
		}

		if(len1 == 0 && len2 == 0) // 동시에 0이 되면 종료
			break;

        // 둘 다 소문자로 변경
		to_lower_case(&c1);
		to_lower_case(&c2);

		if(c1 != c2){ // 다르면 0점 처리
			close(fd1);
			close(fd2);
			return false;
		}
	}
	close(fd1);
	close(fd2);
	return true; // accepted
}

void redirection(char *command, int new, int old) // 특정 fd를 redirection해서 명령어 실행
{
	int saved;

	saved = dup(old); // 기존 fd 복사해둠
	dup2(new, old); // dup2로 redirection 진행

	system(command); // 명령어 실행

	dup2(saved, old); // 기존 fd 복원
	close(saved);
}

int get_file_type(char *filename) // 확장자로 파일 타입 가져오는 함수
{
	char *extension = strrchr(filename, '.'); // .으로 구분

	if(!strcmp(extension, ".txt")) // txt file
		return TEXTFILE;
	else if (!strcmp(extension, ".c")) // c file
		return CFILE;
	else
		return -1;
}

void rmdirs(const char *path) // 재귀적으로 path/ 내의 모든 파일/폴더 제거
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[50];

	if((dp = opendir(path)) == NULL)
		return;

	while((dirp = readdir(dp)) != NULL) // 하나씩 디렉토리 내 파일 읽음
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		if(lstat(tmp, &statbuf) == -1) // lstat 조회
			continue;

		if(S_ISDIR(statbuf.st_mode)) // dir면 재귀 함수 호출
			rmdirs(tmp);
		else
			unlink(tmp); // 파일 제거
	}

	closedir(dp);
	rmdir(path); // rmdir로 현재 폴더 삭제
}

void to_lower_case(char *c) // 대문자를 소문자로 변경
{
	if(*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

void print_usage() // 도움말 출력
{
    printf("Usage : ssu_score <STD_DIR> <ANS_DIR> [OPTION]\n"
           "Option :\n"
           " -n <CSVFILENAME>\n"
           " -m\n"
           " -c [STUDENTIDS ...]\n"
           " -p [STUDENTIDS ...]\n"
           " -t [QNAMES ...]\n"
           " -s <CATEGORY> <1|-1>\n"
           " -e <DIRNAME>\n"
           " -h\n");
}
