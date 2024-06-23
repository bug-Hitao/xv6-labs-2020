#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
/*
实现：echo hello too | xargs echo bye
     bye hello too
    命令的执行过程如下：
echo hello too 生成 "hello too"。
这个输出通过管道传递给 xargs。
xargs 读取输入，得到 "hello" 和 "too"。
xargs 将这些参数依次添加到 echo bye 命令后，形成新的命令 echo bye hello too。
最终输出为 "bye hello too"。
*/

void run(char *program, char **args) {
	if(fork() == 0) { // child exec
		exec(program, args);
		exit(0);
	}
	return; // parent return
}

int
main(int argc, char *argv[])
{
    char buf[2048]; //读入时的使用的内存池
    char *p = buf, *last_p = buf; // 当前参数的结束、开始指针
    char *argsbuf[128]; // 全部参数列表，字符串指针数组，包含 argv 传进来的参数和 stdin 读入的参数
    char **args = argsbuf; // char **等价于字符串数组，每个元素代表的不同字符串的首地址，指向 argsbuf 中第一个从 stdin 读入的参数
    for(int i = 1; i < argc; i++) {
		// 将 argv 提供的参数加入到最终的参数列表中
		*args = argv[i]; //*args代表字符串
		args++; //加1代表数组内下一个字符串的首地址
	}
    //目前argsbuf数组里存放了所有argv 提供的参数，args指向argsbuf数组下一维空字符串
    char **pa = args; // 开始读入参数
	while(read(0, p, 1) != 0) {  //从标准输入中读取参数
        char cur = *p;
		if(*p == ' ' || *p == '\n') {
			// 读入一个参数完成（以空格分隔，如 `echo hello world`，则 hello 和 world 各为一个参数）
			*p = '\0';	// 将空格替换为 \0 分割开各个参数，这样可以直接使用内存池中的字符串作为参数字符串
						// 而不用额外开辟空间
			*(pa++) = last_p; //将last_p中的每个字符地址都给pa，即给argsbuf的参数表
			last_p = p+1; //last_p代表了stdin中的单个元素的首地址，而p则一个一个把last_p填满，当遇到' '、'\n'，则代表一个元素读取结束，将last_p赋值给argsbuf的最后一维

			if(cur == '\n') {
				// 读入一行完成
				*pa = 0; // 参数列表末尾用 null 标识列表结束
				run(argv[1], argsbuf); // 执行最后一行指令
				pa = args; // 重置读入参数指针，准备读入下一行
			}
		}
		p++;
	}
    if(pa != args) { // 如果最后一行不是空行
		// 收尾最后一个参数
		*p = '\0';
		*(pa++) = last_p;
		// 收尾最后一行
		*pa = 0; // 参数列表末尾用 null 标识列表结束
		// 执行最后一行指令
		run(argv[1], argsbuf);
	}
	while(wait(0) != -1) {}; // 循环等待所有子进程完成，每一次 wait(0) 等待一个
    exit(0);
}