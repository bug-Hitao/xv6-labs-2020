#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *target){
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    /*
    struct dirent {
    ushort inum;
    char name[DIRSIZ];
    };
    用于在读取目录内容时代表目录中的一个条目
    inum:代表文件的 inode 号码,inode 是文件系统中用于存储文件元数据的数据结构，如文件权限、所有者信息、文件大小、文件数据块的位置等。
    name：文件名，DIRSIZ 是一个宏定义，用于指定目录条目中文件名的最大长度。
    */
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch(st.type){
        case T_FILE:
            // 如果文件名结尾匹配 `/target`，则视为匹配
            if(strcmp(path + strlen(path) - strlen(target), target) == 0){
                fprintf(1, "%s\n", path);
            }
            break;
        case T_DIR:
            //DIRSIZ是目录名长度的上限为16，该段判定是查看已有的路径长度加上'/'加上最大目录名长度加上文件终止符'\0'是否大于缓存区长度
            //strlen(path)的长度是不包括文件终止符的
            //sizeof后面跟明确的变量名而不是类型名的话，可以省略括号
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
			    printf("find: path too long\n");
			    break;
		    }
            strcpy(buf, path);  //将当前目录复制给缓冲区
            p = buf + strlen(buf);  //移动至指针到路径末尾（有字符的后一位）
            *p++ = '/';  //添加路径分隔符，p++到路径末尾处
            while(read(fd, &de, sizeof(de)) == sizeof(de)){  //读取目录项
                if(de.inum == 0)
                    continue;   //如果节点号为0，跳过（空目录项）
                //memmove：从源内存区域复制字节到目标内存区域，即使这两个内存区域有重叠也能正确处理。
                memmove(p, de.name, DIRSIZ);  //将目录项名称复制到路径后
                p[DIRSIZ] = 0;  //确保字符串结束
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);  //错误处理：无法获取状态
                    continue;
			    }
			// 不要进入 `.` 和 `..`
                if(strcmp(buf+strlen(buf)-2, "/.") != 0 && strcmp(buf+strlen(buf)-3, "/..") != 0) {
                    find(buf, target); // 递归查找
                }
		    }
		    break;
	}
	close(fd);  //关闭文件描述符
    
}

int
main(int argc, char *argv[])
{
    if(argc != 3){
        fprintf(2,"Usage: find [path] [filename]\n");
        exit(-1);
    }else{

        char target[512];
        target[0] = '/'; // 为查找的文件名添加 / 在开头
        strcpy(target+1, argv[2]);
        find(argv[1], target);
        exit(0);
    }
}