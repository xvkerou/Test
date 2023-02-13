//Q1:realloc if oldly is needed?
//Q2:if ** can turn into *[]

#include "ramfs.h"
#include<string.h>
#include<stdlib.h>
/* modify this file freely */

typedef struct node {
	enum { FILE_NODE, DIR_NODE } type;
	struct node **dirents;      // if it's a dir, there's subentries
	char *content;                   // if it's a file, there's data content
	int nrde;                        // number of subentries for dir
	int size;                        // size of file
	char *name;                      // it's short name
} node;

node root;// = malloc(sizeof(node));how to free？？？？？？

typedef struct FD_ {
	int offset;
	int flags;
	node *f;
} FD_;

FD_ FD[4096];
int MAXfd = 0;

int ropen(const char *pathname, int flags) {
	//along the path to find
	int lenth = strlen(pathname);
	int FirstLetter = 0;//name读入完毕后就移至下一个字符
	node *find = &root;
	char name_of_find[32];
	char clear[32];
	for (int a = 0; a < lenth; a++)
	{
		if (find->type == 0) return -1;
		if (pathname[a] != 47)
		{
			name_of_find[a - FirstLetter] = pathname[a];
			if (pathname[a + 1] == 47||a == lenth)
			{
				FirstLetter = a + 1;
				for (int b = 0; b < find->size; b++)
				{
					if (!strcmp(find->dirents[b]->name, name_of_find))
					{
						find = find->dirents[b];
						strcpy(find->name, name_of_find);
						strcpy(name_of_find,clear);//如果找到了子文件，则name_of_find为clear
						break;
					}
					else if (b == find->size -1)//找不到匹配的
					{
						if (FirstLetter != lenth) return -1;
						else if (flags - 64 != 1024 && flags - 64 != 512 && flags - 64 > 2) return -1;
					}
				}
			}
		}
		else FirstLetter++;
	}

	int num = 0;//返回值
	if (MAXfd <= 4096)
	{
		num = MAXfd; MAXfd++;
	}
	else
	{
		while (FD[num].f != NULL) num++;
	}

	//if is a file , judge flags, 赋值给结构体内对应的量，返回下标，MAXfd++; 如果已经都用过了，遍历node找空闲的
	if (find->type == 0)
	{
		if (name_of_find[0] != 0)//need to creat
		{
			node **oldly = find->dirents;
			node **newly = realloc(oldly, (find->size + 1) * sizeof(node*));/////////////////////////////////////////////////
			find->dirents = newly;
			int last = find->size;
			strcpy(find->dirents[last]->name, name_of_find);
			find->dirents[last]->type = 0;
			find->size++;
			flags = flags - 64;
			find = find->dirents[last];
			free(newly);
			free(find);
		}
		
		else if (flags == 513 || flags == 514)
		{
			memset(find->content, 0, sizeof(find->content)-1);//////////
		}

		FD[num].f = find;
		FD[num].flags = flags;
		if (flags > 1000) FD[num].offset = strlen(find->content);
		else FD[num].offset = 0;
	}

	//if is a dir ,if create, realloc dirents, return
	else FD[num].f = find;
	return num;
}

int rclose(int fd) {
  // 通过下标找，如果f已经是NULL，return -1；
	if (FD[fd].f == NULL) return -1;
	else
	{
		FD[fd].f = NULL;
		FD[fd].flags = 0;
		FD[fd].offset = 0;
	}
}

ssize_t rwrite(int fd, const void *buf, size_t count) {
	char *buf1 = malloc(sizeof(buf));
	strcpy(buf1, buf);
	int offset = FD[fd].offset;
  // 判断fd[fd]中的flag，若不符合直接返回-1
	if (FD[fd].flags == 1024 || FD[fd].flags == 64 || FD[fd].flags == 512) return -1;
  //找到fd文件的偏移量，循环将buf中count个字符改写，若容量不够，realloc file扩容，偏移量后移（是否要调用函数）？
	int b = sizeof(FD[fd].f->content);
	if (offset + count > b)
	{
		void *oldly = FD[fd].f->content;
		void *newly = realloc(oldly, sizeof(oldly) + FD[fd].offset + count - b);
		FD[fd].f->content = newly;
	}
	for (int a = 1; a <= count; a++)
	{
		FD[fd].f->content[FD[fd].offset] = buf1[a - 1];//////
		FD[fd].offset++;
	}
	free(buf1);
	return count;
}

ssize_t rread(int fd, void *buf, size_t count) {
	char *buf1 = malloc(sizeof(buf));
	strcpy(buf1, buf);
  //  判断fd[fd]中的flag，若不符合直接返回-1
	int offset = FD[fd].offset;
	if (FD[fd].f->type == 1) return -1;
	if (FD[fd].flags - 1== 1024 || FD[fd].flags - 1== 64 || FD[fd].flags - 1== 512) return -1;
  //  找到fd偏移量，判断到文末是否够count个字节，确定返回值，循环赋值给buf(or memcpy)，偏移量后移，return
	int num;
	int b = sizeof(FD[fd].f->content);
	if (offset + count > b) num = b - offset;
	else num = count;
	for (int a = 1; a <= num; a++)
	{
	   buf1 [a - 1] = FD[fd].f->content[offset];
		FD[fd].offset++;
		offset++;
	}
	return num;
}

off_t rseek(int fd, off_t offset, int whence) {//off_t = long，表示需要移到offset位置
  // 修改偏移量
	int last;
	if (whence == 0) last = offset;
	else if (whence == 1) last = FD[fd].offset + offset;
	else if (whence == 2) last = sizeof(FD[fd].f->content) + offset;
	if (last >= 0) { FD[fd].offset = last; return last; }
	else return -1;
}

int rmkdir(const char *pathname) {
  // find,创建目录
	int FirstLetter = 0;//name读入完毕后就移至下一个字符
	node *find = &root;
	char name_of_find[32];
	char clear[32];
	int last = strlen(pathname) - 1;
	while (pathname[last] == 47) last--;
	for (int a = 0; a <= last; a++)
	{
		if (find->type == 0) return -1;
		if (pathname[a] != 47)
		{
			name_of_find[a - FirstLetter] = pathname[a];
			if (pathname[a + 1] == 47 || a == last)
			{
				FirstLetter = a + 1;
				for (int b = 0; b < find->size; b++)
				{
					if (!strcmp(find->dirents[b]->name, name_of_find))
					{
						if (a == last) return -1;
						find = find->dirents[b];
						strcpy(name_of_find, clear);//如果找到了子文件，则name_of_find为clear
						break;
					}
				}
			}
		}
		else FirstLetter++;
	}
	node **oldly = find->dirents;
	node **newly = realloc(oldly, (find->size + 1) * sizeof(node*));
	find->dirents = newly;
	find->size++;
	int latest = find->size;
	find->dirents[latest]->type = 1;
	strcpy(find->dirents[latest]->name, name_of_find);
	free(newly);
	free(find);
	return 0;
}

int rrmdir(const char *pathname) {
  // find，删除空目录
	int FirstLetter = 0;//name读入完毕后就移至下一个字符
	node *header;
	node *find = &root;
	int num;
	char name_of_find[32];
	char clear[32];
	int last = strlen(pathname) - 1;
	while (pathname[last] == 47) last--;
	for (int a = 0; a <= last; a++)
	{
		if (find->type == 0) return -1;
		if (pathname[a] != 47)
		{
			name_of_find[a - FirstLetter] = pathname[a];
			if (pathname[a + 1] == 47 || a == last)
			{
				FirstLetter = a + 1;
				for (int b = 0; b < find->size; b++)
				{
					if (!strcmp(find->dirents[b]->name, name_of_find))
					{
						header = find;
						find = find->dirents[b];
						strcpy(name_of_find, clear);//如果找到了子文件，则name_of_find为clear
						num = b;
						break;
					}
					else return -1;
				}
			}
		}
		else FirstLetter++;
	}
	if (find->type == 0 || find->size != 0) return -1;
	else
	{
		header->dirents[num] = NULL;
		header->size--;
	}
	free(header);
	free(find);
	return 0;
}

int runlink(const char *pathname) {
  // 删除文件
	int FirstLetter = 0;//name读入完毕后就移至下一个字符
	node *header;
	node *find = &root;
	int num;
	char name_of_find[32];
	char clear[32];
	int last = strlen(pathname) - 1;
	for (int a = 0; a <= last; a++)
	{
		if (find->type == 0) return -1;
		if (pathname[a] != 47)
		{
			name_of_find[a - FirstLetter] = pathname[a];
			if (pathname[a + 1] == 47 || a == last)
			{
				FirstLetter = a + 1;
				for (int b = 0; b < find->size; b++)
				{
					if (!strcmp(find->dirents[b]->name, name_of_find))
					{
						header = find;
						find = find->dirents[b];
						strcpy(name_of_find, clear);//如果找到了子文件，则name_of_find为clear
						num = b;
						break;
					}
					else return -1;
				}
			}
		}
		else FirstLetter++;
	}
	if (find->type == 1) return -1;
	else 
	{
		header->dirents[num] = NULL;
		header->size--;
	}
	free(header);
	free(find);
}

void init_ramfs() {
	root.type = 1;
	root.name = "/";
}
