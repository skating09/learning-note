## 一、Linux应用开发

前言：为了更好的使用Linux内核的函数和类比zephyr内核，所以想降维打击。

### 1.1、文件？IO？

**open, write, read, lseek**

**看如下例子：**

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "stdio.h"


int main(){
    int fd1 = -1, fd2 = -1, fd3 = -1, fd4 = -1;
    char buf[1024] = {0};
    int ret = 0;
    fd1 = open("./src_file", O_RDONLY);
    if(fd1 == -1){
        printf("fd1 file open fail\n");
        goto _exit1;
    }
    printf("src_file.txt open succesfull:\n");


    fd2 = open("./dest_file", O_WRONLY | O_EXCL |  O_CREAT , S_IRWXU | S_IRGRP | S_IROTH );
    /* O_EXCL： 如果文件存在，则open返回 -1，并同时设置 errno 为 EEXIST .*/
    if(fd2 == -1){
        printf("fd2 file open and cread fail\n");
        goto _exit1;
    }
    printf("dest_file.txt open and cread succesfull:\n");

    off_t off = lseek(fd1, 500, SEEK_SET);
    if(off == -1){
        printf("lseek fd1 500byte err\n ");
        goto _exit1;
    }

    ssize_t size1 = read(fd1, (void *)buf, sizeof(buf));
    if(size1 < 0){
        printf("read src_file.txt err szie: %ld\n", size1);
        goto _exit1;
    }
    printf("read src_file.txt ok szie: %ld\n", size1);

    /*ret = lseek(fd2, 0, SEEK_SET);
    if(ret == -1){
         printf("fd2 0 SEEK_SET err  : %d\n", ret);
        goto _exit1;
    }*/

    size1 = write(fd2, (const void *)buf, sizeof(buf));
    if(size1 == -1){
        printf("write dest_file.txt err szie: %ld\n", size1);
        goto _exit1;
    }
    printf("write dest_file.txt ok szie: %ld\n", size1);


    fd3 = open("./tesr_file.txt",  O_RDWR);
    if(fd3 == -1){
        printf("no file tesr_file.txt\n");
        close(fd3);
    }

    fd4 = open("./new_file.txt",  O_CREAT | S_IRWXU);
    if(fd4 == -1){
        printf("no cread :file new_file.txt\n");
        goto _exit1;
    }
    _exit1:
    close(fd1);
    close(fd2);
    close(fd3);
    close(fd4);

    printf("close all file \n");

    return 0;
}
```

#### 1.1.2、继续探究文件IO

**打开一个文件，系统内部会将这个过程分为三步：**

1) 系统找到这个文件名所对应的 inode 编号；
2) 通过 inode 编号从 inode table 中找到对应的 inode 结构体；
3) 根据 inode 结构体中记录的信息，确定文件数据所在的 block，并读出数据

```shell
skating@DESKTOP-4FRM7ID:~/linux_c/1_chapter$ ls -i
101046 app   101057 app1.c   27019 dest_file      29400 src_file
101058 app1   99992 app.c    29420 new_file.txt

skating@DESKTOP-4FRM7ID:~/linux_c/1_chapter$ stat app
  文件：app
  大小：16264           块：32         IO 块大小：4096   普通文件
设备：830h/2096d        Inode：101046      硬链接：1
权限：(0755/-rwxr-xr-x)  Uid: ( 1000/ skating)   Gid: ( 1000/ skating)
访问时间：2024-08-28 20:15:53.059582510 +0800
修改时间：2024-08-28 20:15:51.479582512 +0800
变更时间：2024-08-28 20:15:51.479582512 +0800
创建时间：2024-08-28 20:15:51.469582512 +0800
```

**Linux如何管理文件？**
打开某个程序（这个程序就是一个进程），程序会读取文件这些操作（先将磁盘的文件搬运到内存中跑），Linux会为这些文件提供一个进程控制块来管理这些数据结构（包含了文件数量，文件的inode 对应在磁盘的 inode）.如下图所示：
![image-20240829104614537](G:\notes\typora_pic\image-20240829104614537.png)

**err 错误反馈**

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(void){
	int fd = -1;
	fd = open("./test_file", O_RDONLY);
	if(fd == -1){
		// printf("err: %s\n", strerror(errno));
		// return -1;
		perror("open error"); //一般用这个，好处是它内部会自动获取errno的值 可以省略#include <errno.h> #include <string.h> 这两个头文件
		return -1;
	}
	close(fd);
	return 0;
}

运行结果：

skating@DESKTOP-4FRM7ID:~/linux_c/1_chapter$ gcc test_err.c -o test_err
skating@DESKTOP-4FRM7ID:~/linux_c/1_chapter$ ./test_err 
err: No such file or directory
skating@DESKTOP-4FRM7ID:~/linux_c/1_chapter$ gcc test_err.c -o test_err
skating@DESKTOP-4FRM7ID:~/linux_c/1_chapter$ ./test_err 
open error: No such file or directory
```

**_ exit()和_Exit()函数**

**官话：**main 函数中使用 return 后返回， return 执行后把控制权交给调用函数， 结束该进程。调用_exit()函数会清除其使用的内存空间，并销毁其在内核中的各种数据结构， 关闭进程的所有文件描述符， 并结束进程、将控制权交给操作系统 。

**参数：** 0 表示正常结束、若为其它值则表示程序执行过程中检测到有错误发生  

exit()和_Exit()两者等价，用法作用是一样的，这里就不再讲了，需要注意的是这 2 个函数都是系统调
用 。

**exit()函数** ：一般建议用这个
exit()函数 _ exit()函数都是用来终止进程的， exit()是一个标准 C 库函数， 而 _ exit()和 _ Exit()是系统调用。
执行exit()会执行一些清理工作，最后调用_exit()函数终止进程。   

* **空洞文件*：**相当于分断的部分，也占内存，一般用lseek()可以弄出来，作用其实类似多段修公路，最后合起来。

```c
//若使用 read 函数读取文件空洞部分，读取出来 的将会是什么？
/*在 Linux 中，文件空洞（hole）是文件中未分配空间的部分。文件空洞通常出现在稀疏文件（sparse files）中，其中部分内容未实际写入磁盘，只是逻辑上存在。这种设计可以节省磁盘空间，特别是对于大量零值数据的文件。

当使用 read 函数读取文件中的空洞部分时，行为如下：

读取空洞部分的结果是零字节：如果你尝试读取一个空洞部分的数据，read 函数会返回的内容将是由零字节组成的，因为空洞部分在磁盘上没有实际的数据。*/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    int fd = open("sparse_file", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    char buffer[1024];
    ssize_t bytesRead;

    // Seek to the start of the hole
    off_t offset = 1024;
    lseek(fd, offset, SEEK_SET);

    // Read from the hole
    bytesRead = read(fd, buffer, sizeof(buffer));
    if (bytesRead < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    // Print the number of bytes read
    printf("Bytes read: %zd\n", bytesRead);

    // Check if the buffer contains only zeros
    for (ssize_t i = 0; i < bytesRead; i++) {
        if (buffer[i] != 0) {
            printf("Non-zero byte found!\n");
            break;
        }
    }

    close(fd);
    return 0;
}

/******输出结果********/
/*在上面的代码中：

lseek 函数将文件指针移动到空洞的起始位置。
read 函数从该位置开始读取数据。
如果文件的空洞部分大小为 1024 字节，则 read 函数将返回 1024 字节的数据，这些数据将是全零的。*/

/****** 注意事项 *****/
/*性能：读取空洞部分不会实际从磁盘读取数据，而是由操作系统虚拟化出来的零字节数据，通常不会消耗额外的磁盘 I/O。
文件大小：文件的实际物理大小可能与逻辑大小不同。你可以使用 fstat 函数获取文件的实际大小（包括空洞部分）。*/
```

**open函数标志中的O_APPEND 和 O_TRUNC 标志**  

O_TRUNC 这个标志的作用非常简单，如果使用了这个标志， 调用 open 函数打开文件的时候会将文件原本的内容全部丢弃，文件大小变为 0；   

O_APPEND  open 函数携带了 O_APPEND 标志， 调用 open 函数打开文件，
当每次使用 write()函数对文件进行写操作时，都会自动把文件当前位置偏移量移动到文件末尾， 从文件末尾开始写入数据，也就是意味着每次写入数据都是从文件末尾开始。  

细节问题需要大家注意，首先第一点， O_APPEND标志并不会影响读文件，当读取文件时， O_APPEND 标志并不会影响读位置偏移量， 即使使用了 O_APPEND标志，**读文件位置偏移量默认情况下依然是文件头，** 关于这个问题大家可以自己进行测试，编程是一个实践性很强的工作，有什么不能理解的问题，可以自己编写程序进行测试。  

大家可能会想到使用 lseek 函数来改变 write()时的写位置偏移量， 其实这种做法并不会成功，这就是笔者给大家提的第二个细节，**使用了 O_APPEND 标志，即使是通过 lseek 函数也是无法修改写文件时对应的位置偏移量（注意笔者这里说的是写文件，并不包括读），写入数据依然是从文件末尾开始， lseek 并不会该变写位置偏移量。**

**当 open 函数同时携带了 O_APPEND 和 O_TRUNC 两个标志时会有什么作用？**  

**答：**`O_TRUNC` 的效果会使 `O_APPEND` 无效，因为文件内容会被清空，并且后续的写入操作将从文件开头开始。

* **多次打开同一个文件**

**一个进程内多次 open 打开同一个文件，那么会得到多个不同的文件描述符 fd，同理在关闭文件的时候也需要调用 close 依次关闭各个文件描述符。**  

调用 open 函数打开文件使用的是什么权限，则返回的文件描述符就拥有什么权限，文件 IO 操作完成之后，在结束进程之前需要使用 close 关闭各个文件描述符 。

testApp 文件是在 vscode 软件提供的终端下进行的，所以 vscode 可以认为是 testApp 进程的父进程，相反，testApp 进程便是 vscode 进程的子进程，子进程会继承父进程的文件描述符。  

如： 

1. **父进程（VSCode 终端）**：
   - 打开了标准输入（stdin）、标准输出（stdout）和标准错误（stderr）。
   - 这些文件描述符通常分别是 0、1 和 2。
   - 可能还打开了其他文件或流。
2. **子进程（`testApp`）**：
   - 通过 `fork` 系统调用创建子进程。
   - 子进程继承了父进程的所有文件描述符。
   - 这意味着 `testApp` 也可以访问标准输入、标准输出和标准错误，以及其他父进程已经打开的文件和流



**一个进程内多次 open 打开同一个文件，在内存中并不会存在多份动态文件。**  

**一个进程内多次 open 打开同一个文件，不同文件描述符所对应的读写位置偏移量是相互独立的。**  

**会覆盖，除非打开加个O_APPEND 标志，就能顺着连续写了。**

* **复制文件描述符**

在 Linux 系统下， 可以使用 dup 或 dup2 这两个系统调用对文件描述符进行复制 ,复制的新描述符和旧的属性完全一致。

**dup 函数：**  **int dup(int oldfd);   例如 new_fd = dup(old_fd);**

可以在open不使用 O_APPEND(flag 标志)只用lseek()来实现偏移连续写。

**dup2 函数：可以手动指定文件描述符**  

int dup2(int oldfd, int newfd)  
这里的意思是 newfd可以由我来自定义，比如 **new_fd=  dup2(old_fd, 100) ;** 如果函数正确时，new_fd为 100， 否之为 -1.

dup()或者dup2()函数可以复制多几次，得到多个不同的文件描述符。

* **文件共享**

思想就是多次打开同一个文件且在其他fd没close掉的时候，对文件操作。

**常见的三种文件共享的实现方式：**

  (1)同一个进程中多次调用 open 函数打开同一个文件，各数据结构之间的关系如下图所示：  

![image-20240906165625736](G:\notes\typora_pic\image-20240906165625736.png)

(2)不同进程中分别使用 open 函数打开同一个文件，其数据结构关系图如下所示：  
![image-20240906174509302](G:\notes\typora_pic\image-20240906174509302.png)

* **竞争冒险（原子操作，加锁这些等）**
  ![image-20240909104215842](G:\notes\typora_pic\image-20240909104215842.png)

上图介绍的是两个进程对于同一个文件进行文件末尾插入字节的操作，很明显处于共享操作都会有进程（线程）的竞争和冒险情况，上面说的是在使用open函数不带标志 “O_APPEND  ”的操作。那么上图说明A进程做了偏移后时间片耗尽给进程B运行，那么进程B用完后偏移地址到了1600，而进程A本身的fd偏移还是在1500，那么再写的100byte就覆盖掉了B写进去的100BYTE。为了避免这个情况有以下实例介绍：

1、open函数加个 O_APPEND 标志 ：当 open 函数的 flags 参数中包含了 O_APPEND 标志，每次执行 write 写入操作时都会将文件当前写位置偏移量移动到文件末尾，然后再写入数据，这里“移动当前写位置偏移量到文件末尾、写入数据”这两个操作步骤就组成了一个原子操作， 加入 O_APPEND 标志后，不管怎么写入数据都会是从文件末尾写，这样就不会导致出现“进程 A 写入的数据覆盖了进程 B 写入的数据” 这种情况了。  

2、pread()和pwrite() 与read()，write()一样，区别在于是系统调用，且有一个offset的参数用于设计偏移，功能是先偏移再读，或者先偏移再写， 那其实这里说的是偏移和读或者写是绑一块的而不是 说像 lseek ,（任务调度），write 。这样也能避免文件在偏移-->（打断）-->写的。

需要注意的是这个函数不会使文件产生偏移！！！如下例子：

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    unsigned char buffer[100];
    int fd;
    int ret;
    /* 打开文件 test_file */
    fd = open("./test_file", O_RDWR);
    if (-1 == fd) {
    	perror("open error");exit(-1);
    }
    /* 使用 pread 函数读取数据(从偏移文件头 1024 字节处开始读取) */
    ret = pread(fd, buffer, sizeof(buffer), 1024);
    if (-1 == ret) {
    	perror("pread error");
    	goto err;
    }
    /* 获取当前位置偏移量 */
    ret = lseek(fd, 0, SEEK_CUR);
    if (-1 == ret) {
    	perror("lseek error");
    	goto err;
    }
    printf("Current Offset: %d\n", ret);
    ret = 0;
    err:
    /* 关闭文件 */
    close(fd);
    exit(ret);
}

/*****************************************************************/
运行结果：
    Current Offset: 0
```

3、创建一个文件

open 函数的 O_EXCL 标志 ，O_EXCL 可以用于测试一个文件是否存在，如果不存在则创建此文件，如果存在则返回错误，这使得测试和创建两者成为一个原子操作。
![image-20240909110911318](G:\notes\typora_pic\image-20240909110911318.png)

进程 A 和进程 B 都会创建出同一个文件，同一个文件被创建两次这是不允许的，那如何规避这样的问题呢？ 那就是通过使用 O_EXCL 标志， 当 open 函数中同时指定了 O_EXCL 和 O_CREAT 标志，如果要打开的文件已经存在，则 open 返回错误；如果指定的文件不存在，则创建这个文件，这里就提供了一种机制，保证进程是打开文件的创建者，将“判断文件是否存在、创建文件”这两个步骤合成为一个原子操作，有了原子操作，就保证不会出现图 3.9.3 中所示的情况。  

#### 1.1.3、fcntl函数和ioctl函数

**fcntl()**函数可以对一个已经打开的文件描述符执行一系列控制操作，譬如复制一个文件描述符（与 **dup、dup2** 作用相同）、获取/设置文件描述符标志、获取/设置文件状态标志等，类似于一个多功能文件描述符管理工具箱。   

```c
int fcntl(int fd, int cmd, ... /* arg */ )
cmd 操作命令大致可以分为以下 5 种功能：
⚫ 复制文件描述符（cmd=F_DUPFD 或 cmd=F_DUPFD_CLOEXEC）；  
⚫ 获取/设置文件描述符标志（cmd=F_GETFD 或 cmd=F_SETFD）；
⚫ 获取/设置文件状态标志（cmd=F_GETFL 或 cmd=F_SETFL）；
⚫ 获取/设置异步 IO 所有权（cmd=F_GETOWN 或 cmd=F_SETOWN）；
⚫ 获取/设置记录锁（cmd=F_GETLK 或 cmd=F_SETLK）；  
```

1、复制文件描述符：

```c
fd2 = fcntl(fd1, F_DUPFD, 0); //0表示由系统分配，如果写100，那么fd2 = 100; 
```

2、获取/设置文件描述符标志：

可以用open 函数时传入的 flags 标志， 可以指定一个或多个（通过位或 | 运算符组合）， 但是文件权限标志（O_RDONLY、 O_WRONLY、 O_RDWR）以及文件创建标志（O_CREAT、O_EXCL、 O_NOCTTY、 O_TRUNC）不能被设置、会被忽略；在 Linux 系统中，只有 O_APPEND、 O_ASYNC、  O_DIRECT、 O_NOATIME 以及 O_NONBLOCK 这些标志可以被修改。

剩下异步io和记录锁就后面再说！！！

**ioctl()函数 ：**可以认为是一个文件 IO 操作的杂物箱，可以处理的事情非常杂、不统一，**一般用于操作特殊文件或硬件外设，此函数将会在进阶篇中使用到， 譬如可以通过 ioctl 获取 LCD 相关信息等**  

```c
int ioctl(int fd, unsigned long request, ...);
```

#### 截断文件  : 使用系统调用 truncate()或 ftruncate()可将普通文件截断为指定字节长度

```c
int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);
//ftruncate()使用文件描述符 fd 来指定目标文件，而 truncate()则直接使用文件路径 path 来指定目标文件，其功能一样

//文件大小小于截断字节大小则多出来的部分会被扩充，补空字节“\0”，如果大于则多余部分会被丢弃！
//使用 ftruncate()函数进行文件截断操作之前，必须调用 open()函数打开该文件得到文件描述符，并且必须要具有可写权限，也就是调用 open()打开文件时需要指定 O_WRONLY 或 O_RDWR.
//调用这两个函数并不会导致文件读写位置偏移量发生改变，所以截断之后一般需要重新设置文件当前的读写位置偏移量，以免由于之前所指向的位置已经不存在而发生错误（譬如文件长度变短了，文件当前所指向的读写位置已不存在）。
```

### 1.2、标准I/O库介绍

所谓标准 I/O 库则是标准 C 库中用于文件 I/O 操作（譬如读文件、写文件等）相关的一系列库函数的集合，通常标准 I/O 库函数相关的函数定义都在头文件<stdio.h>中，所以我们需要在程序源码中包含**<stdio.h>**头文件。  

标准 I/O 库函数是构建于文件 I/O（open()、 read()、 write()、 lseek()、 close()等）这些系统调用之上的，譬如标准 I/O 库函数 fopen()就利用系统调用 open()来执行打开文件的操作、 fread()利用系统调用 read()来执行读文件操作、 fwrite()则利用系统调用 write()来执行写文件操作等等。  

标准 I/O 和文件 I/O 的区别 :
⚫ 虽然标准 I/O 和文件 I/O 都是 C 语言函数，但是标准 I/O 是标准 C 库函数，而文件 I/O 则是 Linux系统调用；
⚫ 标准 I/O 是由文件 I/O 封装而来，标准 I/O 内部实际上是调用文件 I/O 来完成实际操作的；
⚫ 可移植性：标准 I/O 相比于文件 I/O 具有更好的可移植性，通常对于不同的操作系统，其内核向应用层提供的系统调用往往都是不同，譬如系统调用的定义、功能、参数列表、返回值等往往都是不一样的；而对于标准 I/O 来说，由于很多操作系统都实现了标准 I/O 库，标准 I/O 库在不同的操作系统之间其接口定义几乎是一样的，所以标准 I/O 在不同操作系统之间相比于文件 I/O 具有更好的可移植性。
⚫ 性能、效率： 标准 I/O 库在用户空间维护了自己的 stdio 缓冲区， 所以标准 I/O 是带有缓存的，而文件 I/O 在用户空间是不带有缓存的，所以在性能、效率上，标准 I/O 要优于文件 I/O。  

#### **FILE指针:**

标准 I/O 库函数打开或创建一个文件时，会返回**一个指向 FILE 类型对象的指针（FILE *） ** **stdio.h**   ，使用该 FILE 指针与被打开或创建的文件相关联，然后该 FILE 指针就用于后续的标准 I/O 操作（使用标准 I/O 库函数进行 I/O 操作）；
Linux文件系统操作返回的是**int类型的fd**。

FILE 指针的作用相当于文件描述符，只不过 FILE 指针用于标准 I/O 库函数中、而文件描述符则用于文件I/O 系统调用中 。

#### **标准输入，标准输出和标准错误：**

标准输入设备指的就是计算机系统的标准的输入设备，通常指的是计算机所连接的键盘；

标准输出设备指的是计算机系统中用于输出标准信息的设备，通常指的是计算机所连接的显示器；

标准错误设备则指的是计算机系统中用于显示错误信息的设备，通常也指的是显示器设备。

用户通过标准输入设备与系统进行交互， 进程将从标准输入（stdin）文件中得到输入数据，将正常输出数据（譬如程序中 printf 打印输出的字符串） 输出到标准输出（stdout） 文件，而将错误信息（譬如函数调用报错打印的信息）输出到标准错误（stderr） 文件。  

每个进程启动之后都会默认打开标准输入、标准输出以及标准错误， 得到三个文件描述符， 即 0、 1、 2， 其中 0 代表标准输入、 1 代表标准输出、 2 代表标准错误； 在应用编程中可以使用宏 STDIN_FILENO、STDOUT_FILENO 和 STDERR_FILENO 分别代表 0、 1、 2 。

```c
unistd.h 
/* Standard file descriptors. */
#define STDIN_FILENO 0 /* Standard input. */
#define STDOUT_FILENO1 /* Standard output. */
#define STDERR_FILENO2 /* Standard error output. */
//0、 1、 2 这三个是文件描述符，只能用于文件 I/O（read()、 write()等），那么在标准 I/O 中，自然是无法使用文件描述符来对文件进行 I/O 操作的
```

```c
stdio.h
/* Standard streams. */
extern struct _IO_FILE *stdin; /* Standard input stream. */
extern struct _IO_FILE *stdout; /* Standard output stream. */
extern struct _IO_FILE *stderr; /* Standard error output stream. */
/* C89/C99 say they're macros. Make them happy. */
#define stdin stdin
#define stdout stdout
#define stderr stderr

/*Tips： struct _IO_FILE 结构体就是 FILE 结构体，使用了 typedef 进行了重命名。
所以，在标准 I/O 中，可以使用 stdin、 stdout、 stderr 来表示标准输入、标准输出和标准错误。*/
```

**打开文件 fopen()**

```c
#include <stdio.h>
FILE *fopen(const char *path, const char *mode);
/*
path： 参数 path 指向文件路径，可以是绝对路径、也可以是相对路径。
mode： 参数 mode 指定了对该文件的读写权限，是一个字符串，稍后介绍。
返回值： 调用成功返回一个指向 FILE 类型对象的指针（FILE *），该指针与打开或创建的文件相关联，后续的标准 I/O 操作将围绕 FILE 指针进行。 如果失败则返回 NULL，并设置 errno 以指示错误原因。*/
```

mode字符串类型：其实就是类似于open，read，write中的参数O_XXX，但对于标准I/O来说是其他字符！！！

虽然调用 fopen()函数新建文件时无法手动指定文件的权限，但它有一个默认值：

```c
S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH (0666)
S_IRUSR: 文件所有者的读权限（400）
S_IWUSR: 文件所有者的写权限（200）
S_IRGRP: 同组用户的读权限（40）
S_IWGRP: 同组用户的写权限（20）
S_IROTH: 其他用户的读权限（4）
S_IWOTH: 其他用户的写权限（2
```

**fclose()关闭文件**  

调用 fclose()库函数可以关闭一个由 fopen()打开的文件，  

```c
#include <stdio.h>
int fclose(FILE *stream);

参数 stream 为 FILE 类型指针，调用成功返回 0；失败将返回 EOF（也就是-1），并且会设置 errno 来
指示错误原因。
```

#### **读文件和写文件**  

```c
#include <stdio.h>
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
/*
ptr： fread()将读取到的数据存放在参数 ptr 指向的缓冲区中；
size： fread()从文件读取 nmemb 个数据项，每一个数据项的大小为 size 个字节，所以总共读取的数据大小为 nmemb * size 个字节。
nmemb： 参数 nmemb 指定了读取数据项的个数。
stream： FILE 指针。
返回值： 调用成功时返回读取到的数据项的数目（数据项数目并不等于实际读取的字节数，除非参数size 等于 1）；如果发生错误或到达文件末尾，则 fread()返回的值将小于参数 nmemb，那么到底发生了错误还是到达了文件末尾， fread()不能区分文件结尾和错误， 究竟是哪一种情况，此时可以使用 ferror()或 feof()函数来判断， 具体参考 4.7 小节内容的介绍*/

/*要将一个 struct mystr 结构体数据写入到文件中：*/ fwrite(buf, sizeof(struct mystr), 1, file);
/*也可以是：*/fwrite(buf, 1, sizeof(struct mystr), file)

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
/*
ptr： 将参数 ptr 指向的缓冲区中的数据写入到文件中。
size： 参数 size 指定了每个数据项的字节大小，与 fread()函数的 size 参数意义相同。
nmemb： 参数 nmemb 指定了写入的数据项个数，与 fread()函数的 nmemb 参数意义相同。
stream： FILE 指针。
返回值： 调用成功时返回写入的数据项的数目（数据项数目并不等于实际写入的字节数，除非参数 size等于 1）；如果发生错误，则 fwrite()返回的值将小于参数 nmemb（或者等于 0）*/
```

**fseek 定位，区别lseek()** 系统调用 lseek()， 用于设置文件读写位置偏移量， lseek()用于文件 I/O。库函数 fseek()则用于标准 I/O，  

```c
#include <stdio.h>
int fseek(FILE *stream, long offset, int whence);

函数参数和返回值含义如下：
stream： FILE 指针。
offset： 与 lseek()函数的 offset 参数意义相同。
whence： 与 lseek()函数的 whence 参数意义相同。
返回值： 成功返回 0；发生错误将返回-1，并且会设置 errno 以指示错误原因； 与 lseek()函数的返回值
意义不同，这里要注意！
    
调用库函数 fread()、 fwrite()读写文件时，文件的读写位置偏移量会自动递增，使用 fseek()可手动设置
文件当前的读写位置偏移量。
```

譬如将文件的读写位置移动到文件开头处  ： fseek(file, 0, SEEK_SET);  

将文件的读写位置移动到文件末尾：  fseek(file, 0, SEEK_END);  

将文件的读写位置移动到 100 个字节偏移量处：  fseek(file, 100, SEEK_SET);  

**需要注意的是fwrite()写入数据后，文件会自动偏移写入的内容，要想读的话要么重新打开文件要么就是偏移文件到开头再读，这时候就需要使用这个fseek了。**

**ftell()函数：**可用于获取文件当前的读写位置偏移量。

```c
#include <stdio.h>
long ftell(FILE *stream);

参数 stream 指向对应的文件，函数调用成功将返回当前读写位置偏移量；调用失败将返回-1，并会设置
errno 以指示错误原因。
```

#### **检查或复位状态**  

调用 fread()读取数据时，如果返回值小于参数 nmemb 所指定的值，表示发生了错误或者已经到了文件末尾（文件结束 end-of-file），但 fread()无法具体确定是哪一种情况； 在这种情况下，可以通过判断错误标志或 end-of-file 标志来确定具体的情况。

**feof()函数**  {end-of-file} 库函数 feof()用于测试参数 stream 所指文件的 end-of-file 标志，如果 end-of-file 标志被设置了，则调用feof()函数将返回一个非零值，如果 end-of-file 标志没有被设置，则返回 0。  

```c
#include <stdio.h>
int feof(FILE *stream);

if (feof(file)) {
/* 到达文件末尾 */
}else {
/* 未到达文件末尾 */
}
```

**ferror()函数**  用于测试参数 stream 所指文件的错误标志，如果错误标志被设置了，则调用 ferror()函数将返回一个非零值，如果错误标志没有被设置，则返回 0。  

```c
#include <stdio.h>
int ferror(FILE *stream);

if (ferror(file)) {
/* 发生错误 */
}
else {
/* 未发生错误 */
}
```

**clearerr()函数**  用于清除 end-of-file 标志和错误标志，当调用 feof()或 ferror()校验这些标志后，通常需要清除这些标志，避免下次校验时使用到的是上一次设置的值，此时可以手动调用 clearerr()函数清除标志。

```c
#include <stdio.h>
void clearerr(FILE *stream); //此函数没有返回值，调用将总是会成功！
/*对于 end-of-file 标志，除了使用 clearerr()显式清除之外，当调用 fseek()成功时也会清除文件的 end-offile 标志*/

/*调用 ferror() 只能用来检测最近一次的I/O操作是否出现了错误。
在进行新的I/O操作之前，最好先使用 clearerr() 清除流的状态标志，否则可能会误报之前的错误。
ferror() 不会告诉你具体的错误是什么，只告诉你是否有一个错误。*/
```

#### **格式化 I/O**  

printf()函数可将格式化数据写入到标准输出，所以通常称为格式化输出。除了 printf()之外，格式化输出还包括： fprintf()、dprintf()、 sprintf()、 snprintf()这 4 个库函数。

```c
#include <stdio.h>
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int dprintf(int fd, const char *format, ...);
int sprintf(char *buf, const char *format, ...);
int snprintf(char *buf, size_t size, const char *format, ...);

printf() 函数用于将格式化数据写入到标准输出
dprintf() 和 fprintf()函数用于将格式化数据写入到指定的文件中，两者不同之处在于， fprintf()使用 FILE 指针指定对应的文件、而 dprintf()则使用文件描述符 fd 指定对应的文件 
sprintf()、 snprintf()函数可将格式化的数据存储在用户指定的缓冲区 buf 中。
```

```c
fprintf()//函数
fprintf(stderr, "Hello World!\n");
fprintf(stderr, "%d\n", 5);
//函数调用成功返回写入到文件中的字符数；失败将返回一个负值！

/***********************************************************************/
dprintf()//函数 可将格式化数据写入到由文件描述符 fd 指定的文件中，譬如将字符串“Hello World”写入到标准错误
dprintf(STDERR_FILENO, "Hello World!\n");
dprintf(STDERR_FILENO, "%d\n", 5);//函数调用成功返回写入到文件中的字符数；失败将返回一个负值

/**********************************************************************/
sprintf()//函数 sprintf()函数将格式化数据存储在由参数 buf 所指定的缓冲区中， 譬如将字符串“Hello World”存放在缓冲区中
char buf[100];
sprintf(buf, "Hello World!\n");

//我们一般会使用这个函数进行格式化转换，并将转换后的字符串存放在缓冲区中:
char buf[20] = {0};
sprintf(buf, "%d", 100);//sprintf()函数会在字符串尾端自动加上一个字符串终止字符'\0'。
/*sprintf()函数可能会造成由参数 buf 指定的缓冲区溢出，调用者有责任确保该缓冲区足够大，因为缓冲区溢出会造成程序不稳定甚至安全隐患！函数调用成功返回写入到 buf 中的字节数; 失败将返回一个负值！*/

snprintf()//sprintf()函数可能会发生缓冲区溢出的问题，存在安全隐患，为了解决这个问题，引入了 snprintf()函数；在该函数中，使用参数 size 显式的指定缓冲区的大小，如果写入到缓冲区的字节数大于参数 size 指定的大小，超出的部分将会被丢弃！如果缓冲区空间足够大， snprintf()函数就会返回写入到缓冲区的字符数，与sprintf()函数相同，也会在字符串末尾自动添加终止字符'\0'。若发生错误， snprintf()将返回一个负值！
```

**格式控制字符串 format**  

```c
%[flags][width][.precision][length]type

flags： 标志，可包含 0 个或多个标志;
width： 输出最小宽度，表示转换后输出字符串的最小宽度;
precision： 精度，前面有一个点号" . ";
length： 长度修饰符;
type： 转换类型，指定待转换数据的类型;
可以看到，只有%和 type 字段是必须的，其余都是可选的.
```

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(void)
{
    char buf[50] = {0};
    printf("%d (%s) %d (%s)\n", 520, "我爱你", 1314, "一生一世");
    fprintf(stdout, "%d (%s) %d (%s)\n", 520, "我爱你", 1314, "一生一世");
    dprintf(STDOUT_FILENO, "%d (%s) %d (%s)\n", 520, "我爱你", 1314, "一生一世");
    sprintf(buf, "%d (%s) %d (%s)\n", 520, "我爱你", 1314, "一生一世");
    printf("%s", buf);
    memset(buf, 0x00, sizeof(buf));
    snprintf(buf, sizeof(buf), "%d (%s) %d (%s)\n", 520, "我爱你", 1314, "一生一世");
    printf("%s", buf);
    exit(0);
}
以上运行结果都是：520 （我爱你）1314（一生一世）
```

#### **格式化输入**  

**scanf()、 fscanf()、 sscanf()**  

```c
#include <stdio.h>
int scanf(const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int sscanf(const char *str, const char *format, ...);

/*scanf()函数可将用户输入（标准输入）的数据进行格式化转换;fscanf()函数从 FILE 指针指定文件中读取数据,并将数据进行格式化转换; sscanf()函数从参数 str 所指向的字符串中读取数据，并将数据进行格式化转换.*/
```

```c
int a, b, c;
scanf("%d %d %d", &a, &b, &c);
//当程序中调用 scanf()的时候，终端会被阻塞，等待用户输入数据，此时我们可以通过键盘输入一些字符，譬如数字、字母或者其它字符， 输入完成按回车即可！
/********************************************************************************************/
int a, b, c;
fscanf(stdin, "%d %d %d", &a, &b, &c);//可以看出stdin和之前的fprintf的 stdout是固定的。

/*********************************************************************************************/
char *str = "5454 hello";
char buf[10];
int a;
sscanf(str, "%d %s", &a, buf);
```

而这个标准输入的 format与之前的标准输出的有些许差异。

```c
%[*][width][length]type
%[m][width][length]type
```

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void)
{
    int a;
    float b;
    char *str;
    printf("请输入一个整数:\n");
    scanf("%d", &a);
    printf("你输入的整数为: %d\n", a);
    printf("请输入一个浮点数:\n");
    scanf("%f", &b);
    printf("你输入的浮点数为: %f\n", b);
    printf("请输入一个字符串:\n");
    scanf("%ms", &str);  //不需要一开始指定内存，但用完后一定要手动释放内存
    printf("你输入的字符串为: %s\n", str);
    free(str); //释放字符串占用的内存空间
    exit(0);
}
```

#### **I/O 缓冲**  

出于速度和效率的考虑，系统 I/O 调用（即文件 I/O， open、 read、 write 等）和标准 C 语言库 I/O 函数
（即标准 I/O 函数）在操作磁盘文件时会对数据进行缓冲。

**文件 I/O 的内核缓冲：**  read()和 write()系统调用在进行文件读写操作的时候并不会直接访问磁盘设备，而是仅仅在用户空间缓冲区和内核缓冲区（kernel buffer cache）之间复制数据。

后面某个时刻，内核再将缓存区的值写入到磁盘中。这说明是异步的处理方式。write()函数不会等待数据正在写入到磁盘才返回，如果再此时，其他进程调用read()函数读取该文件的几个字节数据，那么内核就将该缓冲区的数据返回给应用程序（一个应用程序就一个进程，而应用程序（APP）里面包含多个线程用来处理多个数据比如鼠标，键盘，音乐，显示这些等等。）与此同理的read()也是，内核先将数据读到内核缓冲区，read()去读时，内核直接返回给函数。读完后内核再将下一段内容读入内核缓冲区进行缓寸。**所以内核缓冲区就称为文件 I/O 的内核缓冲。**  

**目的是为了提高文件 I/O 的速度和效率，使得系统调用 read()、 write()的操作更为快速，不需要等待磁盘操作（将数据写入到磁盘或从磁盘读取出数据），磁盘操作通常是比较缓慢的。同时这一设计也更为高效，减少了内核操作磁盘的次数。**

譬如线程1 调用 write()向文件写入数据"abcd"，线程 2 也调用 write()向文件写入数据"1234"，这样的话，数据"abcd"和"1234"都被缓存在了内核的缓冲区中，在稍后内核会将它们一起写入到磁盘中，只发起一次磁盘操作请求；假如没有内核缓冲区，那么每一次调用 write()，内核就会执行一次磁盘操作。 如果操作文件过大，那么需要的内核缓冲区也越大，一般系统对内核缓存区没有上限要求，但被限制于物理缓存大小。 

**刷新文件 I/O 的内核缓冲区：**  

强制将文件 I/O 内核缓冲区中缓存的数据写入（刷新）到磁盘设备中，对于某些应用程序来说，可能是很有必要的， 例如，应用程序在进行某操作之前， 必须要确保前面步骤调用 write()写入到文件的数据已经真正写入到了磁盘中， 诸如一些数据库的日志进程。  

比如sync命令，在拔出u盘之前需要使用sync将内核数据刷新到u盘，不然拔出u盘会导致数据丢失。

**控制文件 I/O 内核缓冲的系统调用**  

Linux 中提供了一些系统调用可用于控制文件 I/O 内核缓冲，包括系统调用 sync()、 syncfs()、 fsync()以及fdatasync() 。

**fsync()函数：**系统调用 fsync()将参数 fd 所指文件的内容数据和元数据写入磁盘， 只有在对磁盘设备的写入操作完成
之后， fsync()函数才会返回。

```c
#include <unistd.h>
int fsync(int fd);
//参数 fd 表示文件描述符，函数调用成功将返回 0，失败返回-1 并设置 errno 以指示错误原因。

/*前面提到了元数据这个概念，元数据并不是文件内容本身的数据，而是一些用于记录文件属性相关的数据信息，譬如文件大小、时间戳、权限等等信息，这里统称为文件的元数据，这些信息也是存储在磁盘设备中的，在 3.1 小节中介绍过。*/
```

**fdatasync()函数:**  系统调用 fdatasync()与 fsync()类似，不同之处在于 fdatasync()仅将参数 fd 所指文件的内容数据写入磁盘，并不包括文件的元数据(不包含 文件的索引 inode, 但包含了文件大小，时间等信息)

```c
#include <unistd.h>
int fdatasync(int fd);
```

**sync()函数：**  系统调用 sync()会将所有文件 I/O 内核缓冲区中的文件内容数据和元数据全部更新到磁盘设备中，该函
数没有参数、也无返回值，意味着它不是对某一个指定的文件进行数据更新，而是刷新所有文件 I/O 内核缓冲区。  

```c
#include <unistd.h>
void sync(void);

/*在 Linux实现中，调用 sync()函数仅在所有数据已经写入到磁盘设备之后才会返回；然后在其它系统中，sync()实现只是简单调度一下 I/O 传递，在动作未完成之后即可返回*/
```

**控制文件 I/O 内核缓冲的标志:**  调用 open()函数时指定一些标志也可以影响到文件 I/O 内核缓冲，譬如 O_DSYNC 标志和 O_SYNC 标志。

**O_DSYNC 标志：** 在调用 open()函数时，指定 O_DSYNC 标志，其效果类似于在每个 write()调用之后调用 fdatasync()函数进行数据同步。  

```c
fd = open(filepath, O_WRONLY | O_DSYNC);  
```

**O_SYNC 标志 ：**在调用 open()函数时，指定 O_SYNC 标志，使得每个 write()调用都会自动将文件内容数据和元数据刷新到磁盘设备中，其效果类似于在每个 write()调用之后调用 fsync()函数进行数据同步。

```c
fd = open(filepath, O_WRONLY | O_SYNC);
```

在程序中频繁调用 fsync()、 fdatasync()、 sync()（或者调用 open 时指定 O_DSYNC 或 O_SYNC 标志）对性能的影响极大，大部分的应用程序是没有这种需求的，所以在大部分应用程序当中基本不会使用到。  

**直接 I/O：绕过内核缓冲：**  从 Linux 内核 2.4 版本开始， Linux 允许应用程序在执行文件 I/O 操作时绕过内核缓冲区，从用户空间直接将数据传递到文件或磁盘设备，把这种操作也称为直接 I/O（direct I/O）或裸 I/O（raw I/O）。  

```c
fd = open(filepath, O_WRONLY | O_DIRECT);
```

**直接 I/O 的对齐限制：**  因为直接 I/O 涉及到对磁盘设备的直接访问，所以在执行直接 I/O 时，必须要遵守以下三个对齐限制要求：
⚫ 应用程序中用于存放数据的缓冲区，其内存起始地址必须以块大小的整数倍进行对齐；
⚫ 写文件时，文件的位置偏移量必须是块大小的整数倍；
⚫ 写入到文件的数据大小必须是块大小的整数倍。  

如果不满足以上任何一个要求，调用 write()均为以错误返回 Invalid argument。以上所说的块大小指的是磁盘设备的物理块大小（block size） ，常见的块大小包括 512 字节、 1024 字节、 2048 以及 4096 字节。

```shell
tune2fs -l /dev/sda1 | grep "Block size" #查看磁盘分区的块大小
```

用 df -h 命令查看 Ubuntu 系统的根文件系统所挂载的磁盘分区：  

![image-20240913170837353](G:\notes\typora_pic\image-20240913170837353.png)

![image-20240913170849285](G:\notes\typora_pic\image-20240913170849285.png)

首先我们需要在程序开头处定义一个宏定义_GNU_SOURCE，原因在于后面 open()函数需要指定 O_DIRECT 标志，这个宏需要我们在程序中定义了O_DIRECT 宏之后才能使用，否则编译程序就会报错提示： O_DIRECT 未定义。  

两种方案：

直接在源文件中定义： 

1、在c文件中添加这个：#define _GNU_SOURCE
2、使用编译器时指定 gcc 编译时使用 -D 选项定义 _GNU_SOURCE 宏，这个选项启用后是全局应用宏定义。

```c
//示例代码 4.9.2 直接 I/O 示例程序
/** 使用宏定义 O_DIRECT 需要在程序中定义宏_GNU_SOURCE
** 不然提示 O_DIRECT 找不到 **/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/** 定义一个用于存放数据的 buf，起始地址以 4096 字节进行对其 **/
static char buf[8192] __attribute((aligned (4096)));
int main(void)
{
    int fd;
    int count;
    /* 打开文件 */
    fd = open("./test_file", O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT,0664);
     if (0 > fd) {
        perror("open error");
        exit(-1);
    }
    /* 写文件 */
    count = 10000;
    while(count--) {
        if (4096 != write(fd, buf, 4096)) {
            perror("write error");
            exit(-1);
        }
    }
    /* 关闭文件退出程序 */
    close(fd);
    exit(0);
}
```

一般还是用内核缓存方式，即文件打开不带O_DIRECT这个标志。这个直接操作磁盘一般使用在磁盘测试上，而且使用内核缓存区的方式用以上代码来运行的话，速度起码是写入的20倍（写40M）。

#### stdio 缓冲

标准 I/O（fopen、 fread、 fwrite、 fclose、 fseek 等）是 C 语言标准库函数， 而文件 I/O（open、 read、 write、
close、 lseek 等）是系统调用，虽然标准 I/O 是在文件 I/O 基础上进行封装而实现（譬如 fopen 内部实际上调用了 open、 fread 内部调用了 read 等）， 但在效率、性能上标准 I/O 要优于文件 I/O，其原因在于标准 I/O 实现维护了自己的缓冲区， 我们把这个缓冲区称为 stdio 缓冲区 。

譬如 fopen 内部实际上调用了 open、 fread 内部调用了 read 等 。

**留下疑问：为什么驱动那里是用open, write来写入数据控制模块如led.ko?**

/*******************************************************************************************************************************************/

当应用程序中通过标准 I/O 操作磁盘文件时，为了减少调用系统调用的次数，标准 I/O 函数会将用户写入或读取文件的数据缓存在 stdio 缓冲区，然后再一次性将 stdio 缓冲区中缓存的数据通过调用系统调用 I/O（文件 I/O）写入到文件 I/O 内核缓冲区或者拷贝到应用程序的 buf 中。  

**对 stdio 缓冲进行设置**  

C 语言提供了一些库函数可用于对标准 I/O 的 stdio 缓冲区进行相关的一些设置， 包括 **setbuf()、setbuffer()**以及 **setvbuf()**。  

```c
#include <stdio.h>
int setvbuf(FILE *stream, char *buf, int mode, size_t size);
/*调用 setvbuf()库函数可以对文件的 stdio 缓冲区进行设置，譬如缓冲区的缓冲模式、 缓冲区的大小、起始地址等。*/

/*
stream： FILE 指针，用于指定对应的文件， 每一个文件都可以设置它对应的 stdio 缓冲区。
    
buf： 如果参数 buf 不为 NULL，那么 buf 指向 size 大小的内存区域将作为该文件的 stdio 缓冲区，因为stdio 库会使用 buf 指向的缓冲区，所以应该以动态（分配在堆内存，譬如 malloc，在 7.6 小节介绍）或静态的方式在堆中为该缓冲区分配一块空间，而不是分配在栈上的函数内的自动变量（局部变量）。如果 buf 等于 NULL，那么 stdio 库会自动分配一块空间作为该文件的 stdio 缓冲区（除非参数 mode 配置为非缓冲模式）。

mode： 参数 mode 用于指定缓冲区的缓冲类型，可取值如下：
⚫ _IONBF： 不对 I/O 进行缓冲（无缓冲） 。意味着每个标准 I/O 函数将立即调用 write()或者 read()，并且忽略 buf 和 size 参数，可以分别指定两个参数为 NULL 和 0。标准错误 stderr 默认属于这一种类型，从而保证错误信息能够立即输出。
⚫ _IOLBF： 采用行缓冲 I/O。 在这种情况下，当在输入或输出中遇到换行符"\n"时，标准 I/O 才会执行文件 I/O 操作。对于输出流，在输出一个换行符前将数据缓存（除非缓冲区已经被填满）， 当输出换行符时，再将这一行数据通过文件 I/O write()函数刷入到内核缓冲区中；对于输入流， 每次读取一行数据。 对于终端设备默认采用的就是行缓冲模式，譬如标准输入和标准输出。
⚫ _IOFBF： 采用全缓冲 I/O。 在这种情况下，在填满 stdio 缓冲区后才进行文件 I/O 操作（read、 write）。
对于输出流，当 fwrite 写入文件的数据填满缓冲区时，才调用 write()将 stdio 缓冲区中的数据刷入内核缓冲区；对于输入流， 每次读取 stdio 缓冲区大小个字节数据。 默认普通磁盘上的常规文件默认常用这种缓冲模式。

size： 指定缓冲区的大小。

返回值： 成功返回 0，失败将返回一个非 0 值，并且会设置 errno 来指示错误原因。

需要注意的是，当 stdio 缓冲区中的数据被刷入到内核缓冲区或被读取之后，这些数据就不会存在于缓冲区中了，数据被刷入了内核缓冲区或被读走了。
*/
```

```c
#include <stdio.h>
void setbuf(FILE *stream, char *buf);
/*
setbuf()调用除了不返回函数结果（void）外，就相当于： 
setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ); BUFSIZ 定义于头文件<stdio.h>中，该值通常为 8192
*/
//这里的大小是被写死： 8192byte
```

```c
#include <stdio.h>
void setbuffer(FILE *stream, char *buf, size_t size);
/*
setvbuf(stream, buf, buf ? _IOFBF : _IONBF, size);
这里允许指定大小
*/
```

```c
#include <stdio.h>
int fflush(FILE *stream);

/*
无论我们采取何种缓冲模式，在任何时候都可以使用库函数 fflush()来强制刷新（将输出到 stdio 缓冲区中的数据写入到内核缓冲区，通过 write()函数） stdio 缓冲区， 该函数会刷新指定文件的 stdio 输出缓冲区）。

参数 stream 指定需要进行强制刷新的文件，如果该参数设置为 NULL，则表示刷新所有的 stdio 缓冲区。
函数调用成功返回 0，否则将返回-1，并设置 errno 以指示错误原因。
*/
```

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void){
    printf("Hello World!\n");
    printf("Hello World!");
    fflush(stdout); //刷新标准输出 stdio 缓冲区
    for ( ; ; )
        sleep(1);
}
/*
此例子说明的是：第二个printf("Hello World!");如果不加上换行情况或者立即刷新，不设置缓存情况下时，是不会打印的，因为标准输入和标准输出是使用行缓存模式，只有接收到换行指令时，这个标准IO（带缓存如：fopen等）才会执行文件操作（open等）刷新到内核缓存。
*/
/*那么上面的可以替换成关闭标准输出， 终止程序来让自动刷新*/
int main(void){
    printf("Hello World!\n");
    printf("Hello World!");
    // fflush(stdout); //刷新标准输出 stdio 缓冲区 能刷新缓存
    //fclose(stdout); //关闭标准输出    能刷新缓存

    // exit(0);  //能刷新缓存  原因是c库里面的，会注册终止出来的程序，刷新所有的标准输出流，执行必要的文件和资源清理
    // return 0;	//能刷新缓存
    // _exit(0);//不能刷新缓存 底层调用，不会调用任何清理刷新函数
    // _Exit(0);//不能刷新缓存  也是c标准宏，但不会调用注册终止处理程序，也不会刷新标准I/O流缓存区
}

关于刷新 stdio 缓冲区相关内容，最后进行一个总结：
⚫ 调用 fflush()库函数可强制刷新指定文件的 stdio 缓冲区;
⚫ 调用 fclose()关闭文件时会自动刷新文件的 stdio 缓冲区;
⚫ 程序退出时会自动刷新 stdio 缓冲区（注意区分不同的情况）.
```

![image-20240918095910433](G:\notes\typora_pic\image-20240918095910433.png)

#### 文件描述符与FILE指针互转

在同一个文件上执行 I/O 操作时，还可以将文件 I/O（系统调用 I/O）与标准 I/O 混合使用，这个时候我们就需要将文件描述符和 FILE 指针对象之间进行转换。此时可以借助于库函数 **fdopen()、fileno()**来完成。  

库函数 fileno()可以将标准 I/O 中使用的 FILE 指针转换为文件 I/O 中所使用的文件描述符，而 fdopen()则进行着相反的操作 。

```c
#include <stdio.h>
int fileno(FILE *stream);
FILE *fdopen(int fd, const char *mode);//这里的mode与fopen()函数中的mode参数含义相同。
/*当混合使用文件 I/O 和标准 I/O 时，需要特别注意缓冲的问题，文件 I/O 会直接将数据写入到内核缓冲
区进行高速缓存，而标准 I/O 则会将数据写入到 stdio 缓冲区，之后再调用 write()将 stdio 缓冲区中的数据写
入到内核缓冲区。*/

/*如下的例子：*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    printf("print");//有缓存先stdio再到文件io
    write(STDOUT_FILENO, "write\n", 6);//无缓存直接到文件io
    exit(0);
}
//执行结果：先输出 write\n 再输出 print。
```

### 1.3、文件属性和目录

#### 文件系统中的文件类型

**普通文件：** **分为文本文件和二进制文件**  

文本文件： 文件中的内容是由文本构成的，所谓文本指的是 ASCII 码字符。文件中的内容其本质上都是数字（因为计算机本身只有 0 和 1，存储在磁盘上的文件内容也都是由 0 和 1 所构成），而文本文件中的数字应该被理解为这个数字所对应的 ASCII 字符码；譬如常见的.c、 .h、 .sh、 .txt 等这些都是文本文件，文本文件的好处就是方便人阅读、浏览以及编写。

二进制文件： 二进制文件中存储的本质上也是数字，只不过对于二进制文件来说， 这些数字并不是文本字符编码，而是真正的数字。譬如 Linux 系统下的可执行文件、 C 代码编译之后得到的.o 文件、 .bin 文件等都是二进制文件。

**stat 命令非常友好，会直观把文件类型显示出来；对于 ls 命令来说，并没有直观的显示出文件的类型，而是通过符号表示出来，**   

 ' - '：普通文件
 ' d '：目录文件
 ' c '：字符设备文件
 ' b '：块设备文件
 ' l '：符号链接文件
 ' s '：套接字文件
 ' p '：管道文件

**目录文件：目录（directory） 就是文件夹，文件夹在 Linux 系统中也是一种文件，是一种特殊文件。**

**字符设备文件和块设备文件：设备文件（字符设备文件、块设备文件）对应的是硬件设备，在 Linux 系统中，硬件设备会对应到一个设备文件，应用程序通过对设备文件的读写来操控、使用硬件设备。**

设备文件并不存在于磁盘中，而是由文件系统虚拟出来的，一般是由内存来维护， 当系统关机时，设备文件都会消失；   字符设备文件一般存放在 Linux 系统/dev/目录下，所以/dev 也称为虚拟文件系统 devfs。  

**符号链接文件：**  

符号链接文件（link） 类似于 Windows 系统中的快捷方式文件，是一种特殊文件，它的内容指向的是另一个文件路径，当对符号链接文件进行操作时，系统根据情况会对这个操作转移到它指向的文件上去，而不是对它本身进行操作，譬如，读取一个符号链接文件内容时，实际上读到的是它指向的文件的内容。  

**管道文件(pipe): 主要用于进程间通信**  

**套接字文件(socket)也是一种进程间通信的方式，与管道文件不同的是，它们可以在不同主机上的进程间通信，实际上就是网络通信**，当学习到网络编程相关知识内容再给大家介绍。  

7 种文件类型，包括：普通文件、目录、字符设备文件、块设备文件、符号链接文件、管道文件以及套接字文件。

#### stat函数：

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
int stat(const char *pathname, struct stat *buf);
/*
函数参数及返回值含义如下：
pathname： 用于指定一个需要查看属性的文件路径。
buf： struct stat 类型指针，用于指向一个 struct stat 结构体变量。调用 stat 函数的时候需要传入一个 struct
stat 变量的指针， 获取到的文件属性信息就记录在 struct stat 结构体中，稍后给大家介绍 struct stat 结构体中有记录了哪些信息。
返回值： 成功返回 0；失败返回-1，并设置 error。*/

struct stat
{
    dev_t st_dev; /* 文件所在设备的 ID */
    ino_t st_ino; /* 文件对应 inode 节点编号 */
    mode_t st_mode; /* 文件对应的模式 */
    nlink_t st_nlink; /* 文件的链接数 */
    uid_t st_uid; /* 文件所有者的用户 ID */
    gid_t st_gid; /* 文件所有者的组 ID */
    dev_t st_rdev; /* 设备号（指针对设备文件） */
    off_t st_size; /* 文件大小（以字节为单位） */
    blksize_t st_blksize; /* 文件内容存储的块大小 */
    blkcnt_t st_blocks; /* 文件内容所占块数 */
    struct timespec st_atim; /* 文件最后被访问的时间 */
    struct timespec st_mtim; /* 文件内容最后被修改的时间 */
    struct timespec st_ctim; /* 文件状态最后被改变的时间 */
};
st_mode：以下数字使用的是八进制方式表示！！！
S_IRWXU 00700 owner has read, write, and execute permission
S_IRUSR 00400 owner has read permission
S_IWUSR 00200 owner has write permission
S_IXUSR 00100 owner has execute permission
S_IRWXG 00070 group has read, write, and execute permission
S_IRGRP 00040 group has read permission
S_IWGRP 00020 group has write permission
S_IXGRP 00010 group has execute permission
S_IRWXO 00007 others (not in group) have read, write, and execute permission
S_IROTH 00004 others have read permission
S_IWOTH 00002 others have write permission
S_IXOTH 00001 others have execute permission

//如想知道文件所有者对该文件是否有可执行权限：
if (st.st_mode & S_IXUSR) {
//有权限
} else {
//无权限
}

/****************************************************************************/
“文件类型”这 4 个 bit 位， 这 4 个 bit 位用于描述该文件的类型，譬如该文件是普通文件、还是链接文件、亦或者是一个目录等。在 C 语言中，八进制方式表示一个数字需要在数字前面添加一个 0（零） 
S_IFSOCK 0140000 socket（套接字文件）
S_IFLNK 0120000 symbolic link（链接文件）
S_IFREG 0100000 regular file（普通文件）
S_IFBLK 0060000 block device（块设备文件）
S_IFDIR 0040000 directory（目录）
S_IFCHR 0020000 character device（字符设备文件）
S_IFIFO 0010000 FIFO（管道文件）
/* 判断是不是普通文件 */
if ((st.st_mode & S_IFMT) == S_IFREG) {
/* 是 */
}
/* 判断是不是链接文件 */
if ((st.st_mode & S_IFMT) == S_IFLNK) {
/* 是 */
}
S_IFMT 宏是文件类型字段位掩码: S_IFMT 0170000
/*****************************************************************************/
除了这样判断之外，我们还可以使用 Linux 系统封装好的宏来进行判断

S_ISREG(m) #判断是不是普通文件，如果是返回 true，否则返回 false
S_ISDIR(m) #判断是不是目录，如果是返回 true，否则返回 false
S_ISCHR(m) #判断是不是字符设备文件，如果是返回 true，否则返回 false
S_ISBLK(m) #判断是不是块设备文件，如果是返回 true，否则返回 false
S_ISFIFO(m) #判断是不是管道文件，如果是返回 true，否则返回 false
S_ISLNK(m) #判断是不是链接文件，如果是返回 true，否则返回 false
S_ISSOCK(m) #判断是不是套接字文件，如果是返回 true，否则返回 false
/* 判断是不是普通文件 */
if (S_ISREG(st.st_mode)) {
/* 是 */
}
/* 判断是不是目录 */
if (S_ISDIR(st.st_mode)) {
/* 是 */
}
```

**fstat 和 lstat 函数：**  **fstat、 lstat 与 stat 的作用一样，但是参数、细节方面有些许不同。**  

fstat 与 stat 区别在于， stat 是从文件名出发得到文件属性信息，不需要先打开文件；而 fstat 函数则是从文件描述符出发得到文件属性信息，所以使用 fstat 函数之前需要先打开文件得到文件描述符。

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
int fstat(int fd, struct stat *buf);

int main(void)
{
    struct stat file_stat;
    int fd;
    int ret;
    /* 打开文件 */
    fd = open("./test_file", O_RDONLY);
    if (-1 == fd) {
    	perror("open error");
    	exit(-1);
    }
    /* 获取文件属性 */
    ret = fstat(fd, &file_stat);
    if (-1 == ret)
    	perror("fstat error");
    close(fd);
    exit(ret);
}
```

**lstat()**与 stat、 fstat 的区别在于，对于符号链接文件， stat、 fstat 查阅的是符号链接文件所指向的文件对应的文件属性信息，而 lstat 查阅的是符号链接文件本身的属性信息。  函数参数列表、返回值与 stat 函数一样，使用方法也一样  

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
int lstat(const char *pathname, struct stat *buf);
```

#### 文件属主  

一般来说文件在创建时，其所有者就是创建该文件的那个用户。  

文件所属组则表示该文件属于哪一个用户组。在 Linux 中，系统并不是通过用户名或用户组名来识别不同的用户和用户组，而是通过 ID。 ID 就是一个编号， Linux 系统会为每一个用户或用户组分配一个 ID， 将用户名或用户组名与对应的 ID 关联起来， 所以系统通过用户 ID（UID） 或组 ID（GID） 就可以识别出不同的用户和用户组。  

Tips： 用户 ID 简称 UID、用户组 ID 简称 GID。 这些都是 Linux 操作系统的基础知识，如果对用户和用户组的概念尚不熟悉，建议先自行学习这些基础知识。

**有效用户 ID 和有效组 ID：**用于给操作系统判断当前执行该进程的用户在当前环境下对某个文件是否拥有相应的权限。  

**chown 函数**  

chown 是一个系统调用，该系统调用可用于改变文件的所有者（用户 ID）和所属组（组 ID） 。

其实在Linux 系统下也有一个 chown 命令，该命令的作用也是用于改变文件的所有者和所属组  

```c
/*如将 testApp.c
文件的所有者和所属组修改为 root：*/
sudo chown root:root testApp.c
```

![image-20240918142136416](G:\notes\typora_pic\image-20240918142136416.png)

通过该命令确实可以改变文件的所有者和所属组，这个命令内部其实就是调用了 chown 函数来实现功能的， chown 函数原型如下所示 ：

```c
#include <unistd.h>
int chown(const char *pathname, uid_t owner, gid_t group);

/*
pathname： 用于指定一个需要修改所有者和所属组的文件路径。
owner： 将文件的所有者修改为该参数指定的用户（以用户 ID 的形式描述） ；
group： 将文件的所属组修改为该参数指定的用户组（以用户组 ID 的形式描述）；
返回值： 成功返回 0；失败将返回-1，兵并且会设置 errno。

注意：
1、只有超级用户进程能更改文件的用户 ID；
2、普通用户进程可以将文件的组 ID 修改为其所从属的任意附属组 ID，前提条件是该进程的有效用户 ID 等于文件的用户 ID；而超级用户进程可以将文件的组 ID 修改为任意值。
*/


```

在 Linux 系统下，可以使用 getuid 和 getgid 两个系统调用分别用于获取当前进程的用户 ID 和用户组ID，这里说的进程的用户 ID 和用户组 ID 指的就是进程的实际用户 ID 和实际组 ID，这两个系统调用函数原型如下所示：  

```c
#include <unistd.h>
#include <sys/types.h>
uid_t getuid(void);
gid_t getgid(void);
```

fchown 和 lchown 函数  这两个同样也是系统调用，作用与 chown 函数相同，只是参数、细节方面有些许不同。 fchown()、 lchown()这两个函数与 chown()的区别就像是 fstat()、 lstat()与 stat 的区别。  

**文件访问权限**

普通权限和特殊权限  ：普通权限包括对文件的读、写以及执行，而特殊权限则包括一些对文件的附加权限，譬如Set-User-ID、Set-Group-ID以及Sticky。  

**普通权限：**即文件所有者，用户组，其他用户的权限，用 ls -l 列出来的9个权限：-r-w-x-r-w-x-r-w-x 

对于进程来说，参与文件权限检查的是进程的有效用户、有效用户组以及进程的附属组用户。

```c
/*如何判断权限，首先要搞清楚该进程对于需要进行操作的文件来说是属于哪一类“角色”  */
1\如果进程的有效用户 ID 等于文件所有者 ID（st_uid） ，意味着该进程以文件所有者的角色存在；
2\如果进程的有效用户 ID 并不等于文件所有者 ID，意味着该进程并不是文件所有者身份；但是进程的有效用户组 ID 或进程的附属组 ID 之一等于文件的组 ID（st_gid），那么意味着该进程以文件所属组成员的角色存在，也就是文件所属组的同组用户成员。
3\如果进程的有效用户 ID 不等于文件所有者 ID、并且进程的有效用户组 ID 或进程的所有附属组 ID均不等于文件的组 ID（st_gid） ，那么意味着该进程以其它用户的角色存在。
4\如果进程的有效用户 ID 等于 0（root 用户），则无需进行权限检查，直接对该文件拥有最高权限.

```

**特殊权限：**  

从高位到低位依次表示文件的 set-user-ID 位权限、 set-groupID 位权限以及 sticky 位权限  三种权限分别使用 S_ISUID、 S_ISGID 和 S_ISVTX 表示。

| S_ISUID | 04000 | set-user-ID bit              |
| ------- | ----- | ---------------------------- |
| S_ISGID | 02000 | set-group-ID bit (see below) |
| S_ISVTX | 01000 | sticky bit (see below)       |

**三个权限位作用：**

当进程对文件进行操作的时候、将进行权限检查，如果文件的 set-user-ID 位权限被设置，内核会将进程的有效 ID 设置为该文件的用户 ID（文件所有者 ID） ，意味着该进程直接获取了文件所有者的权限、以文件所有者的身份操作该文件。  

当进程对文件进行操作的时候、将进行权限检查，如果文件的 set-group-ID 位权限被设置，内核会将进程的有效用户组 ID 设置为该文件的用户组 ID（文件所属组 ID） ，意味着该进程直接获取了文件所属组成员的权限、以文件所属组成员的身份操作该文件。  

**目录权限：**删除文件、创建文件这些操作也是需要相应权限的，这些权限来自于目录！  需要对文件进行读、写或执行等操作，不光是需要拥有该文件本身的读、写或执行权限，还需要拥有文件所在目录的执行权限。  

**检查文件权限 access：**程序当中对文件进行相关操作之前，需要先检查执行进程的用户是否对该文件拥有相应的权限。

```c
#include <unistd.h>
int access(const char *pathname, int mode);
pathname： 需要进行权限检查的文件路径。
mode： 该参数可以取以下值：
⚫ F_OK：检查文件是否存在
⚫ R_OK：检查是否拥有读权限
⚫ W_OK：检查是否拥有写权限
⚫ X_OK：检查是否拥有执行权限
除了可以单独使用之外，还可以通过按位或运算符" | "组合在一起。
返回值： 检查项通过则返回 0，表示拥有相应的权限并且文件存在；否则返回-1，如果多个检查项组合在一起，只要其中任何一项不通过都会返回-1。
```

**修改文件权限 chmod：**  在 Linux 系统下，可以使用 chmod 命令修改文件权限，该命令内部实现方法其实是调用了chmod 函数。

```c
#include <sys/stat.h>
int chmod(const char *pathname, mode_t mode);
pathname： 需要进行权限修改的文件路径，若该参数所指为符号链接，实际改变权限的文件是符号链接所指向的文件，而不是符号链接文件本身。
mode： 该参数用于描述文件权限，与 open 函数的第三个参数一样，这里不再重述，可以直接使用八进制数据来描述，也可以使用相应的权限宏（单个或通过位或运算符" | "组合）。
返回值： 成功返回 0；失败返回-1，并设置 errno。
文件权限对于文件来说是非常重要的属性，是不能随随便便被任何用户所修改的， 要想更改文件权限，要么是超级用户（root） 进程、要么进程有效用户 ID 与文件的用户 ID（文件所有者）相匹配。
    
#include <stdlib.h>
int main(void)
{
    int ret;
    ret = chmod("./test_file", 0777);
    if (-1 == ret) {
    	perror("chmod error");
    	exit(-1);
    }
    exit(0);
}
```

**fchmod 函数：**fchmod()与 chmod()的区别在于使用了文件描述符来代替文件路径，就像是 fstat 与 stat 的区别。 

```c
#include <sys/stat.h>
int fchmod(int fd, mode_t mode);
```

**umask 函数：**  

umask命令用于查看/设置权限掩码， 权限掩码主要用于对新建文件的权限进行屏蔽。权限掩码的表示方式与文件权限的表示方式相同， 但是需要去除特殊权限位， umask 不能对特殊权限位进行屏蔽。  

```c
/*当新建文件时，文件实际的权限并不等于我们所设置的权限，譬如：调用 open 函数新建文件时，文件实际的权限并不等于 mode 参数所描述的权限，而是通过如下关系得到实际权限：*/
mode & ~umask
/*譬如调用 open 函数新建文件时， mode 参数指定为 0777， 假设 umask 为 0002，那么实际权限为*/
0777 & (~0002) = 0775
//umask 权限掩码是进程的一种属性，用于指明该进程新建文件或目录时，应屏蔽哪些权限位。
/*
也就是说比如我们创建个文件: 
1、chmod 777 1.c  但如果umask为0002时， 这个文件实际的权限还是为0775（也就是-rwx-rwx-r-x），因为这个umask是对于open函数来说的，既然fopen也会调用open，估计也是差不多和open一致.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void){
    int ret;
    ret = open("./2", O_CREAT|O_RDWR, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);
    if (-1 == ret) {
    	perror("open error");
    	exit(-1);
    }
    close(ret);
    return 0;
}
/*输出结果：*/
skating@DESKTOP-4FRM7ID:~/linux_c/3_stdio$ ls -l
总计 20
-rwxrwxrwx 1 skating skating     0  9月 19 15:44 1.c
-rwxr-xr-x 1 skating skating     0  9月 19 15:55 2      //这个 2是我创建的，umask是022！！！
-rwxr-xr-x 1 skating skating 16088  9月 19 15:55 test1
-rw-r--r-- 1 skating skating  1280  9月 19 15:55 test1.c
```

umask 权限掩码是进程的一种属性，用于指明该进程新建文件或目录时，应屏蔽哪些权限位。 进程的umask 通常继承至其父进程（关于父、 子进程相关的内容将会在后面章节给大家介绍） ，譬如在 Ubuntu shell终端下执行的应用程序，它的 umask 继承至该 shell 进程。

Linux 系统提供了 umask 函数用于设置进程的权限掩码，该函数是一个系统调用。

```c
#include <sys/types.h>
#include <sys/stat.h>
mode_t umask(mode_t mask);
/*
mask： 需要设置的权限掩码值，可以发现 make 参数的类型与 open 函数、 chmod 函数中的 mode 参数
对应的类型一样，所以其表示方式也是一样的，前面也给大家介绍了，既可以使用数字表示（譬如八进制数）
也可以直接使用宏（S_IRUSR、 S_IWUSR 等）。
返回值： 返回设置之前的 umask 值，也就是旧的 umask。 */

/*这里很有必要说一下概念，首先umask肯定先继承于shell终端，比如我shell终端是0002, 现在我写一个c应用修改下我这个应用我需要修改的umask值比如为0003，请看下面例子：*/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    mode_t old_mask;
    old_mask = umask(0003);
    printf("old mask: %04o\n", old_mask);
    exit(0);
}
/*终端打印是 0002 ， 这个值其实是从当前 vscode 的 shell 终端继承下来的，如果没有修改进程的 umask 值，默认就是从父进程继承下来的 umask， 但是我的进程是 这个c文件编译后的 app哦！*/
/*umask 是进程自身的一种属性、 A 进程的 umask 与 B 进程的 umask 无关（父子进程关系除外）。在 shell 终端下可以使用 umask 命令设置 shell 终端的 umask 值，但是该 shell 终端关闭之后、再次打开一个终端，新打开的终端将与之前关闭的终端并无任何瓜葛！*/
 
```

#### 文件的时间属性  

文件最后被访问的时间、 文件内容最后被修改的时间以及文件状态最后被改变的时间，分别记录在 struct stat 结构体的 st_atim、 st_mtim 以及 st_ctim 变量中。

**utime()、 utimes()修改时间属性：使用 utime()和 utimes()函数来修改文件的时间属性  **只能显式修改文件的最后一次访问时间和文件内容最后被修改的时间，不能显式修改文件状态最后被改变的时间。

**tips:**文件的状态最后被改变的时间（ctime）是由文件系统自动管理的，主要用于跟踪文件元数据的变化，比如权限或所有者的改变。使用`utime()`和`utimes()`函数时，它们只修改访问时间和修改时间，而不影响ctime，因为ctime反映的是文件的元数据状态，只有文件系统或相关操作才会更新它。这种设计确保了文件的完整性和状态的一致性。

**utime()函数：**  

```c
#include <sys/types.h>
#include <utime.h>
int utime(const char *filename, const struct utimbuf *times);
/*
filename： 需要修改时间属性的文件路径。
times： 将时间属性修改为该参数所指定的时间值， times 是一个 struct utimbuf 结构体类型的指针，稍后给大家介绍，如果将 times 参数设置为 NULL，则会将文件的访问时间和修改时间设置为系统当前时间。
返回值： 成功返回值 0；失败将返回-1，并会设置 errno。*/
```

```c
struct utimbuf {
    time_t actime; /* 访问时间 */
    time_t modtime; /* 内容修改时间 */
};//以秒为单位的

/*对文件时间属性的修改也不是任何用户都可以随便修改的， 只有以下两种进程可对其进行修改。*/
⚫ 超级用户进程（以 root 身份运行的进程） 。
⚫ 有效用户 ID 与该文件用户 ID（文件所有者）相匹配的进程。
⚫ 在参数 times 等于 NULL 的情况下，对文件拥有写权限的进程。
除以上三种情况之外的用户进程将无法对文件时间戳进行修改

```

**utimes()函数：**utimes()与 utime()最大的区别在于前者可以以微秒级精度来指定时间值 。

```c
#include <sys/time.h>
int utimes(const char *filename, const struct timeval times[2]);  
/*
filename： 需要修改时间属性的文件路径。
times： 将时间属性修改为该参数所指定的时间值， times 是一个 struct timeval 结构体类型的数组， 数组共有两个元素， 第一个元素用于指定访问时间，第二个元素用于指定内容修改时间， 稍后给大家介绍，如果times 参数为 NULL，则会将文件的访问时间和修改时间设置为当前时间。
返回值： 成功返回 0；失败返回-1，并且会设置 errno。*/
struct timeval {
    long tv_sec; /* 秒 */
    long tv_usec; /* 微秒 */
};
```

**futimens()、 utimensat()修改时间属性：**  功能与 utime()和 utimes()函数功能一样，用于显式修改文件时间戳。

这两个系统调用相对于 utime 和 utimes 函数有以下三个优点： 

⚫ 可按纳秒级精度设置时间戳。 相对于提供微秒级精度的 utimes()，这是重大改进！
⚫ 可单独设置某一时间戳。譬如，只设置访问时间、而修改时间保持不变，如果要使用 utime()或 utimes()来实现此功能，则需要首先使用 stat()获取另一个时间戳的值，然后再将获取值与打算变更的时间戳一同指定。
⚫ 可独立将任一时间戳设置为当前时间。使用 utime()或 utimes()函数虽然也可以通过将 times 参数设置为 NULL 来达到将时间戳设置为当前时间的效果，但是不能单独指定某一个时间戳，必须全部设置为当前时间(不考虑使用额外函数获取当前时间的方式，譬如 time())。

```c
#include <fcntl.h>
#include <sys/stat.h>
int futimens(int fd, const struct timespec times[2]);
/*
该函数的时间戳可以按下列 4 种方式之一进行指定：
⚫ 如果 times 参数是一个空指针，也就是 NULL，则表示将访问时间和修改时间都设置为当前时间。
⚫ 如果 times 参数指向两个 struct timespec 结构体类型变量的数组，任一数组元素的 tv_nsec 字段的值设置为UTIME_NOW，则表示相应的时间戳设置为当前时间，此时忽略相应的 tv_sec 字段。
⚫ 如果 times 参数指向两个 struct timespec 结构体类型变量的数组，任一数组元素的 tv_nsec 字段的值设置为UTIME_OMIT，则表示相应的时间戳保持不变，此时忽略 tv_sec 字段。
⚫ 如果 times 参数指向两个 struct timespec 结构体类型变量的数组，且 tv_nsec 字段的值既不是UTIME_NOW 也不是 UTIME_OMIT，在这种情况下，相应的时间戳设置为相应的 tv_sec 和 tv_nsec字段指定的值。
Tips： UTIME_NOW 和 UTIME_OMIT 是两个宏定义。
*/
/*使用 futimens()函数只有以下进程，可对文件时间戳进行修改：
⚫ 超级用户进程。
⚫ 在参数 times 等于 NULL 的情况下，对文件拥有写权限的进程。
⚫ 有效用户 ID 与该文件用户 ID（文件所有者）相匹配的进程。*/
```

**utimensat()函数：**  与 futimens()函数在功能上是一样的，同样可以实现纳秒级精度设置时间戳、单独设置某一时间戳、独立将任一时间戳设置为当前时间， 与 futimens()在参数以及细节上存在一些差异， 使用 futimens()函数，需要先将文件打开，通过文件描述符进行操作， utimensat()可以直接使用文件路径方式进行操作。

```c
#include <fcntl.h>
#include <sys/stat.h>
int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags);
/*
函数参数和返回值含义如下：
dirfd： 该参数可以是一个目录的文件描述符，也可以是特殊值 AT_FDCWD；如果 pathname 参数指定的是文件的绝对路径，则此参数会被忽略。
pathname： 指定文件路径。如果 pathname 参数指定的是一个相对路径、并且 dirfd 参数不等于特殊值AT_FDCWD，则实际操作的文件路径是相对于文件描述符 dirfd 指向的目录进行解析。如果 pathname 参数指定的是一个相对路径、并且 dirfd 参数等于特殊值 AT_FDCWD， 则实际操作的文件路径是相对于调用进程的当前工作目录进行解析，关于进程的工作目录在 5.7 小节中有介绍。   
tips:如果 dirfd 是一个打开的目录文件描述符，且 pathname 是 file.txt，则操作会在该目录下查找file.txt。
如果 dirfd 是 AT_FDCWD，那么 file.txt 会相对于当前工作目录进行查找。
times： 与 futimens()的 times 参数含义相同。
flags ： 此参数可 以为 0，也可以设置为AT_SYMLINK_NOFOLLOW，如 果 设 置 为AT_SYMLINK_NOFOLLOW，当 pathname 参数指定的文件是符号链接， 则修改的是该符号链接的时间戳，而不是它所指向的文件。
返回值： 成功返回 0；失败返回-1、并会设置时间戳
*/
```

**符号链接（软链接）与硬链接：**  

在 Linux 系统中有两种链接文件，分为软链接（也叫符号链接）文件和硬链接文件，**软链接文件**也就是前面给大家的 Linux 系统下的七种文件类型之一，**其作用类似于 Windows 下的快捷方式。**  

从使用角度来讲，两者没有任何区别，都与正常的文件访问方式一样，支持读、写以及执行。 

**Tips：**使用 ln 命令可以为一个文件创建软链接文件或硬链接文件，用法如下：
硬链接： ln 源文件 链接文件
软链接： ln -s 源文件 链接文件  

如果删除了硬链接文件或源文件其中之一，那文件所对应的 inode 以及文件内容在磁盘中的数据块会被文件系统回收吗？  

事实上并不会这样，因为 inode 数据结结构中会记录文件的链接数，这个链接数指的就是硬链接数， struct stat 结构体中的st_nlink 成员变量就记录了文件的链接数。（软链接不记录文件链接数）

当为文件每创建一个硬链接， inode 节点上的链接数就会加一，每删除一个硬链接， inode 节点上的链接数就会减一，直到为 0， inode 节点和对应的数据块才会被文件系统所回收，也就意味着文件已经从文件系统中被删除了。  

软链接文件与源文件有着不同的 inode 号， 所以也就是意味着它们之间有着不同的数据块，但是软链接文件的数据块中存储的是源文件的路径名，链接文件可以通过这个路径找到被链接的源文件，它们之间类似于一种“主从”关系，当源文件被删除之后，软链接文件依然存在，但此时它指向的是一个无效的文件路径， 这种链接文件被称为悬空链接。

```
比如我在目录A下有一个1.txt和2.txt的文件，我分别为他们创建符号链接。1.txt用命令 ln 1.txt hard1 生成一个硬链接，而2.txt用命令 ln -s 2.txt hard2生成软链接。那么请问如果我删掉1.txt, 2.txt那么这两个链接会怎样呢？我是否还可以继续通过链接去修改和访问这两文件之前的存储区域呢？


这是一个很好的问题！让我们来看看硬链接和软链接在这种情况下的行为：
1. **硬链接 (hard link)**：
   - 当你创建一个硬链接 `ln 1.txt hard1` 时，`hard1` 和 `1.txt` 实际上是指向同一个文件数据块。
   - 如果你删除 `1.txt`，`hard1` 仍然可以访问和修改原来的文件内容，因为文件数据块仍然存在，直到所有指向它的硬链接都被删除。

2. **软链接 (symbolic link)**：
   - 当你创建一个软链接 `ln -s 2.txt hard2` 时，`hard2` 只是一个指向 `2.txt` 的路径引用。
   - 如果你删除 `2.txt`，`hard2` 将变成一个“悬空链接”（dangling link），因为它指向的目标文件已经不存在。此时，通过 `hard2` 将无法访问或修改原来的文件内容。

总结一下：
- 删除 `1.txt` 后，你仍然可以通过 `hard1` 访问和修改原来的文件内容。
- 删除 `2.txt` 后，你将无法通过 `hard2` 访问或修改原来的文件内容，因为 `hard2` 变成了一个无效的链接。
```

```
对于硬链接来说， 存在一些限制情况，如下：
⚫ 不能对目录创建硬链接（超级用户可以创建，但必须在底层文件系统支持的情况下）。
⚫ 硬链接通常要求链接文件和源文件位于同一文件系统中。

而软链接文件的使用并没有上述限制条件，优点如下所示：
⚫ 可以对目录创建软链接；
⚫ 可以跨越不同文件系统；
⚫ 可以对不存在的文件创建软链接。（比如 ln -s /path/to/nonexistent/file linkname，这个软链接 linkname 会指向 /path/to/nonexistent/file。当你以后在 /path/to/nonexistent/ 路径下创建了 file 文件或目录时，linkname 会自动指向这个新创建的文件或目录。你不需要对软链接做任何额外的操作，它会自动生效。）
```

**创建链接文件：**  

在 Linux 系统下，可以使用系统调用创建硬链接文件或软链接文件。

**创建硬链接 link()**

```c
#include <unistd.h>
int link(const char *oldpath, const char *newpath);

函数原型和返回值含义如下：
oldpath： 用于指定被链接的源文件路径，应避免 oldpath 参数指定为软链接文件，为软链接文件创建硬链接没有意义，虽然并不会报错。
newpath： 用于指定硬链接文件路径，如果 newpath 指定的文件路径已存在，则会产生错误。
返回值： 成功返回 0；失败将返回-1，并且会设置 errno。
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    int ret;
    ret = link("./test_file", "./hard");
    if (-1 == ret) {
        perror("link error");
        exit(-1);
    }
    exit(0);
}
```

**创建软链接 symlink()**  

```c
#include <unistd.h>
int symlink(const char *target, const char *linkpath);

target： 用于指定被链接的源文件路径， target 参数指定的也可以是一个软链接文件。
linkpath： 用于指定硬链接文件路径，如果 newpath 指定的文件路径已存在，则会产生错误。
返回值： 成功返回 0；失败将返回-1，并会设置 errno。
创建软链接时，并不要求 target 参数指定的文件路径已经存在，如果文件不存在，那么创建的软链接将成为“悬空链接”。
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    int ret;
    ret = symlink("./test_file", "./soft");
    if (-1 == ret) {
        perror("symlink error");
        exit(-1);
    }
    exit(0);
}
```

**读取软链接文件**

```c
#include <unistd.h>
ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);

pathname： 需要读取的软链接文件路径。 只能是软链接文件路径，不能是其它类型文件，否则调用函数将报错。
buf： 用于存放路径信息的缓冲区。
bufsiz： 读取大小，一般读取的大小需要大于链接文件数据块中存储的文件路径信息字节大小。
返回值： 失败将返回-1，并会设置 errno；成功将返回读取到的字节数。
    
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(void)
{
    char buf[50];
    int ret;
    memset(buf, 0x0, sizeof(buf));
    ret = readlink("./soft", buf, sizeof(buf));
    if (-1 == ret) {
        perror("readlink error");
        exit(-1);
    }
    printf("%s\n", buf);
    exit(0);
}
```

![image-20240920103708563](G:\notes\typora_pic\image-20240920103708563.png)

这里可以看出，读到的是某个目录下的文件。那可以利用这个字符串用标准io或者系统io来进行文件操作了。

#### 目录  

在 Linux 系统下，会有一些专门的系统调用或 C 库函数用于对文件夹进行操作，譬如：打开、创建文件夹、删除文件夹、读取文件夹以及遍历文件夹中的文件等。

**目录存储形式**

常规文件包括了 inode 节点以及文件内容数据存储块（block）：

 ![image-20240920104510895](G:\notes\typora_pic\image-20240920104510895.png)

但对于目录来说， 其存储形式则是由 inode 节点和目录块所构成，目录块当中记录了有哪些文件组织在这个目录下，记录它们的文件名以及对应的 inode 编号，如下所示。  

![image-20240920104637923](G:\notes\typora_pic\image-20240920104637923.png)

目录块当中有多个目录项（或叫目录条目） ，每一个目录项（或目录条目） 都会对应到该目录下的某一个文件，目录项当中记录了该文件的文件名以及它的 inode 节点编号，所以通过目录的目录块便可以遍历找到该目录下的所有文件以及所对应的 inode 节点。

所以对此总结如下：
⚫ 普通文件由 inode 节点和数据块构成
⚫ 目录由 inode 节点和目录块构成  

**创建和删除目录**  

使用 open 函数可以创建一个普通文件，但不能用于创建目录文件，在 Linux 系统下，提供了专门用于创建目录 mkdir()以及删除目录 rmdir 相关的系统调用。  

**mkdir 函数**  

```c
#include <sys/stat.h>
#include <sys/types.h>
int mkdir(const char *pathname, mode_t mode);

pathname： 需要创建的目录路径。
mode： 新建目录的权限设置，设置方式与 open 函数的 mode 参数一样，最终权限为（mode & ~umask）。
返回值： 成功返回 0；失败将返回-1，并会设置 errno。
pathname 参数指定的新建目录的路径，该路径名可以是相对路径，也可以是绝对路径，若指定的路径名已经存在，则调用 mkdir()将会失败。
    
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
int main(void)
{
    int ret;
    ret = mkdir("./new_dir", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (-1 == ret) {
        perror("mkdir error");
        exit(-1);
    }
    exit(0);
}
/* 我们通过 mkdir 函数在当前目录下创建了一个目录 new_dir，并将其权限设置为 0755（八进制） */
```

**rmdir 函数：用于删除一个目录。**  

```c
#include <unistd.h>
int rmdir(const char *pathname);

pathname： 需要删除的目录对应的路径名，并且该目录必须是一个空目录，也就是该目录下只有.和..这两个目录项； pathname 指定的路径名不能是软链接文件，即使该链接文件指向了一个空目录。
返回值： 成功返回 0；失败将返回-1，并会设置 errno。
```

**打开、 读取以及关闭目录**  

打开、读取、关闭一个普通文件可以使用 open()、 read()、 close()，而对于目录来说，可以使用 opendir()、readdir()和 closedir()来打开、读取以及关闭目录。

```c
#include <sys/types.h>
#include <dirent.h>
DIR *opendir(const char *name);
/************
name： 指定需要打开的目录路径名，可以是绝对路径，也可以是相对路径。
返回值： 成功将返回指向该目录的句柄，一个 DIR 指针（其实质是一个结构体指针），其作用类似于open函数返回的文件描述符fd，后续对该目录的操作需要使用该DIR指针变量；若调用失败，则返回NULL*///////////


/*读取目录 readdir： readdir()用于读取目录，获取目录下所有文件的名称以及对应 inode 号。*/
#include <dirent.h>
struct dirent *readdir(DIR *dirp);
dirp： 目录句柄 DIR 指针。
/*返回值： 返回一个指向 struct dirent 结构体的指针， 该结构体表示 dirp 指向的目录流中的下一个目录条目。在到达目录流的末尾或发生错误时，它返回 NULL。*/
struct dirent {
	ino_t d_ino; /* inode 编号 */
	off_t d_off; /* not an offset; see NOTES */
	unsigned short d_reclen; /* length of this record */
	unsigned char d_type; /* type of file; not supported by all filesystem types */
	char d_name[256]; /* 文件名 */
};
/*
我们只需要关注 d_ino 和 d_name 两个字段即可，分别记录了文件的 inode 编号和文件名，其余字段并不是所有系统都支持。
每调用一次 readdir()，就会从 drip 所指向的目录流中读取下一条目录项（目录条目），并返回 struct dirent结构体指针，指向经静态分配而得的 struct dirent 类型结构，每次调用 readdir()都会覆盖该结构。 一旦遇到目录结尾或是出错， readdir()将返回 NULL，针对后一种情况，还会设置 errno 以示具体错误。
*/
/*那如何区别究竟是到了目录末尾还是出错了呢，可通过如下代码进行判断：*/
error = 0;
direntp = readdir(dirp);
if (NULL == direntp) {
    if (0 != error) {
    /* 出现了错误 */
    } else {
    /* 已经到了目录末尾 */
    }
}
使用 readdir()返回时并未对文件名进行排序，而是按照文件在目录中出现的天然次序（这取决于文件系统向目录添加文件时所遵循的次序，及其在删除文件后对目录列表中空隙的填补方式）。
```

**rewinddir 函数： **  rewinddir()是 C 库函数，可将目录流重置为目录起点， 以便对 readdir()的下一次调用将从目录列表中的第一个文件开始。  

```c
#include <sys/types.h>
#include <dirent.h>
void rewinddir(DIR *dirp);

dirp： 目录句柄。
返回值： 无返回值。
```

**关闭目录 closedir 函数 ：**closedir()函数用于关闭处于打开状态的目录，同时释放它所使用的资源  

```c
#include <sys/types.h>
#include <dirent.h>
int closedir(DIR *dirp);

dirp： 目录句柄。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
```

```c
//打开一个目录、并将目录下的所有文件的名称以及其对应 inode 编号打印出来。
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>
int main(void)
{
    struct dirent *dir;
    DIR *dirp;
    int ret = 0;
    /* 打开目录 */
    dirp = opendir("./my_dir");
    if (NULL == dirp) {
        perror("opendir error");
        exit(-1);
    }
    /* 循环读取目录流中的所有目录条目 */
    errno = 0;
    while (NULL != (dir = readdir(dirp))) printf("%s %ld\n", dir->d_name, dir->d_ino);
    if (0 != errno) {
        perror("readdir error");
        ret = -1;
        goto err;
    } else
        printf("End of directory!\n");
    err:
    closedir(dirp);
    exit(ret);
}

```

**进程的当前工作目录**  

Linux 下的每一个进程都有自己的当前工作目录（current working directory），当前工作目录是该进程解析、搜索相对路径名的起点（不是以" / "斜杆开头的绝对路径）。  

运行一个进程时、其父进程的当前工作目录将被该进程所继承，成为该进程的当前工作目录。可通过 getcwd 函数来获取进程的当前工作目录。

```c
#include <unistd.h>
char *getcwd(char *buf, size_t size);
/*
buf： getcwd()将内含当前工作目录绝对路径的字符串存放在 buf 缓冲区中。
size： 缓冲区的大小，分配的缓冲区大小必须要大于字符串长度，否则调用将会失败。
返回值： 如果调用成功将返回指向 buf 的指针，失败将返回 NULL，并设置 errno。*/

//Tips：若传入的 buf 为 NULL，且 size 为 0，则 getcwd()内部会按需分配一个缓冲区，并将指向该缓冲区的指针作为函数的返回值，为了避免内存泄漏，调用者使用完之后必须调用 free()来释放这一缓冲区所占内存空间。

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(void)
{
    char buf[100];
    char *ptr;
    memset(buf, 0x0, sizeof(buf));
    ptr = getcwd(buf, sizeof(buf));
    if (NULL == ptr) {
    	perror("getcwd error");
    	exit(-1);
    }
    printf("Current working directory: %s\n", buf);
    exit(0);
}
```

**改变当前工作目录：**  系统调用 chdir()和 fchdir()可以用于更改进程的当前工作目录。

```c
#include <unistd.h>
int chdir(const char *path);
int fchdir(int fd);
/*
path： 将进程的当前工作目录更改为 path 参数指定的目录，可以是绝对路径、也可以是相对路径，指定的目录必须要存在，否则会报错。
fd： 将进程的当前工作目录更改为 fd 文件描述符所指定的目录（譬如使用 open 函数打开一个目录）。
返回值： 成功均返回 0；失败均返回-1，并设置 errno。*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(void)
{
    char buf[100];
    char *ptr;
    int ret;
    /* 获取更改前的工作目录 */
    memset(buf, 0x0, sizeof(buf));
    ptr = getcwd(buf, sizeof(buf));
    if (NULL == ptr) {
        perror("getcwd error");
        exit(-1);
    }
    printf("Before the change: %s\n", buf);
    /* 更改进程的当前工作目录 */
    ret = chdir("./new_dir");
    if (-1 == ret) {
        perror("chdir error");
        exit(-1);
    }
    /* 获取更改后的工作目录 */
    memset(buf, 0x0, sizeof(buf));
    ptr = getcwd(buf, sizeof(buf));
    if (NULL == ptr) {
        perror("getcwd error");
        exit(-1);
    }
    printf("After the change: %s\n", buf);
    exit(0);
}
```

#### 删除文件  

删除一个目录，使用 rmdir()函数即可，显然该函数并不能删除一个普通文件，那如何删除一个普通文件呢？方法就是通过系统调用 unlink()或使用 C 库函数 remove()。  

```c
#include <unistd.h>
int unlink(const char *pathname);

pathname： 需要删除的文件路径，可使用相对路径、也可使用绝对路径，如果 pathname 参数指定的文件不存在，则调用 unlink()失败。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
    
一个条件也会阻止删除文件的内容---只要有进程打开了该文件，其内容也不能被删除。关闭一个文件时，内核会检查打开该文件的进程个数，如果这个计数达到 0，内核再去检查其链接计数，如果链接计数也是 0，那么就删除该文件对应的内容（也就是文件对应的 inode 以及数据块被回收，如果一个文件存在多个硬链接，删除其中任何一个硬链接，其inode 和数据块并没有被回收，还可通过其它硬链接访问文件的数据）。

unlink()系统调用并不会对软链接进行解引用操作，若 pathname 指定的文件为软链接文件，则删除软链接文件本身，而非软链接所指定的文件。
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    int ret;
    ret = unlink("./test_file");
    if (-1 == ret) {
        perror("unlink error");
        exit(-1);
    }
    exit(0);
}
```

**使用 remove 函数删除文件**  remove()是一个 C 库函数，用于移除一个文件或空目录。

```c
#include <stdio.h>
int remove(const char *pathname);

pathname： 需要删除的文件或目录路径，可以是相对路径、也可是绝对路径。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
/*
pathname 参数指定的是一个非目录文件，那么 remove()去调用 unlink()，如果 pathname 参数指定的是一个目录，那么 remove()去调用 rmdir()。
与 unlink()、 rmdir()一样， remove()不对软链接进行解引用操作，若 pathname 参数指定的是一个软链接文件，则 remove()会删除链接文件本身、而非所指向的文件。
*/
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
	int ret;
	ret = remove("./test_file");
	if (-1 == ret) {
    	perror("remove error");
    	exit(-1);
	}
    exit(0);
}
```

#### 文件重命名  

借助于 rename()既可以对文件进行重命名，又可以将文件移至同一文件系统中的另一个目录下 。

```c
#include <stdio.h>
int rename(const char *oldpath, const char *newpath);

函数参数和返回值含义如下：
oldpath： 原文件路径。
newpath： 新文件路径。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
```

### 1.4、字符串处理

#### 字符串输入/输出

与 printf()一样， putchar()、 puts()、 fputc()、 fputs()这些函数也是标准 I/O 函数， 属于标准 C 库函数， 所以需要包含头文件<stdio.h>，并且它们也使用 stdio 缓冲。  

puts()函数用来向标准输出设备（屏幕、显示器）输出字符串并自行换行。 把字符串输出到标准输出设备，将' \0 '转换为换行符' \n '  

```c
#include <stdio.h>
int puts(const char *s);
s： 需要进行输出的字符串。
返回值： 成功返回一个非负数；失败将返回 EOF， EOF 其实就是-1。

tips: 使用 puts()函数连换行符' \n '都省了， 函数内部会自动在其后添加一个换行符。所以，如果只是单纯输
出字符串到标准输出设备，而不包含数字格式化转换操作，那么使用 puts()会更加方便、简洁；
```

**putchar()函数**可以把参数 c 指定的字符（一个无符号字符）输出到标准输出设备， 其输出可以是一个字符，可以是介于 0~127 之间的一个十进制整型数（包含 0 和 127，输出其对应的 ASCII 码字符），也可以是用 char 类型定义好的一个字符型变量。  

```c
#include <stdio.h>
int putchar(int c);

函数参数和返回值含义如下：
c： 需要进行输出的字符。
返回值： 出错将返回 EOF。
    
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    putchar('A');
    putchar('B');
    putchar('C');
    putchar('D');
    putchar('\n');
    exit(0);
}
```

fputc()与 putchar()类似，也用于输出参数 c 指定的字符（一个无符号字符），与 putchar()区别在于，putchar()只能输出到标准输出设备，而 fputc()可把字符输出到指定的文件中，既可以是标准输出、标准错误设备，也可以是一个普通文件。  

```c
#include <stdio.h>
int fputc(int c, FILE *stream);

c： 需要进行输出的字符。
stream： 文件指针。
返回值： 成功时返回输出的字符； 出错将返回 EOF。
//如果stream是stdout 这不就是变成输入到标准输出设备了吗？就能实现 putchar()的功能了。
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    fputc('A', stdout);
    fputc('B', stdout);
    fputc('C', stdout);
    fputc('D', stdout);
    fputc('\n', stdout);
    exit(0);
}

//使用 fputc 函数将字符输出到一个普通文件
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    FILE *fp = NULL;
    /* 创建一个文件 */
    fp = fopen("./new_file", "a");
    if (NULL == fp) {
    	perror("fopen error");
    	exit(-1);
    }
    /* 输入字符到文件 */
    fputc('A', fp);
    fputc('B', fp);
    fputc('C', fp);
    fputc('D', fp);
    fputc('\n', fp);
    /* 关闭文件 */
    fclose(fp);
    exit(0);
}
```

**fputs 函数：**同理， fputs()与 puts()类似，也用于输出一条字符串，与 puts()区别在于， puts()只能输出到标准输出设
备，而 fputs()可把字符串输出到指定的文件中，既可以是标准输出、标准错误设备，也可以是一个普通文件 。可以参考上面的例子。

```c
#include <stdio.h>
int fputs(const char *s, FILE *stream);

s： 需要输出的字符串。
stream： 文件指针。
返回值： 成功返回非负数；失败将返回 EOF。

//1、
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    fputs("Hello World! 1\n", stdout);
    fputs("Hello World! 2\n", stdout);
    exit(0);
}
//2、
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    FILE *fp = NULL;
    /* 创建一个文件 */
    fp = fopen("./new_file", "a");
    if (NULL == fp) {
        perror("fopen error");
        exit(-1);
    }
    fputs("Hello World! 1\n", fp);
    fputs("Hello World! 2\n", fp);
    /* 关闭文件 */
    fclose(fp);
    exit(0);
}
```

**字符串输入：**  

常用的字符串输入函数有 gets()、 getchar()、 fgetc()、 fgets()。与 printf()对应， 在 C 库函数中同样也提供了格式化输入函数 scanf()。 scanf()与 gets()、 getchar()、 fgetc()、 fgets()这些函数相比，在功能上确实有它的优势，但是在使用上不如它们方便、简单、更易于使用。  

与 scanf()一样， gets()、 getchar()、 fgetc()、 fgets()这些函数也是标准 I/O 函数， 属于标准 C 库函数， 所以需要包含头文件<stdio.h>，并且它们也使用 stdio 缓冲。  

**gets()函数**用于从标准输入设备（譬如键盘）中获取用户输入的字符串。

```c
#include <stdio.h>
char *gets(char *s);

s： 指向字符数组的指针， 用于存储字符串。
返回值： 如果成功， 该函数返回指向 s 的指针； 如果发生错误或者到达末尾时还未读取任何字符，则返回 NULL。
/*
用户从键盘输入的字符串数据首先会存放在一个输入缓冲区中， gets()函数会从输入缓冲区中读取字符串存储到字符指针变量 s 所指向的内存空间，当从输入缓冲区中读走字符后，相应的字符便不存在于缓冲区了。

警告信息，其实是建议我们不要使用 gets()函数，因为程序中使用 gets()函数是非常不安全的，可能会出现 bug、出现不可靠性， gets()在某些意外情况下会导致程序陷入不可控状态，所以一般建议大家不要使用这个函数，可以使用后面将给大家介绍的 fgets()代替。*/
/****************************************************************
gets()与 scanf()的区别
gets()除了在功能上不及 scanf 之外，它们在一些细节上也存在着不同：
⚫ gets()函数不仅比 scanf 简洁，而且，就算输入的字符串中有空格也可以， 因为 gets()函数允许输入的字符串带有空格、制表符，输入的空格和制表符也是字符串的一部分，仅以回车换行符作为字符串的分割符； 而对于 scanf 以%s 格式输入的时候， 空格、换行符、 TAB 制表符等都是作为字符串分割符存在， 即分隔符前后是两个字符串， 读取字符串时并不会将分隔符读取出来作为字符串的组成部分， 一个%s 只能读取一个字符串，若要多去多个字符串，则需要使用多个%s、并且需要使用
多个字符数组存储。
⚫ gets()会将回车换行符从输入缓冲区中取出来，然后将其丢弃，所以使用 gets()读走缓冲区中的字符串数据之后，缓冲区中将不会遗留下回车换行符；而对于 scanf()来说，使用 scanf()读走缓冲区中的字符串数据时，并不会将分隔符（空格、 TAB 制表符、 回车换行符等） 读走将其丢弃，所以使用 scanf()读走缓冲区中的字符串数据之后，缓冲区中依然还存在用户输入的分隔符。*/
```

**getchar 函数：**  getchar()函数用于从标准输入设备中读取一个字符（一个无符号字符）；

```c
#include <stdio.h>
int getchar(void);/*
返回值： 该函数以无符号 char 强制转换为 int 的形式返回读取的字符，如果到达文件末尾或发生读错误，则返回 EOF。
同样 getchar()函数也是从输入缓冲区读取字符数据，但只读取一个字符，包括空格、 TAB 制表符、换行回车符等。*/
```

**fgets 函数：fgets()与 gets()一样用于获取输入的字符串。**  

```c
#include <stdio.h>
char *fgets(char *s, int size, FILE *stream);
/*
s： 指向字符数组的指针，用于存储字符串。
size： 这是要读取的最大字符数。
stream： 文件指针*/

fgets()与 gets()的区别主要是三点：
⚫ gets()只能从标准输入设备中获取输入字符串，而 fgets()既可以从标准输入设备获取字符串、也可以从一个普通文件中获取输入字符串。
⚫ fgets()可以设置获取字符串的最大字符数。
⚫ gets()会将缓冲区中的换行符'\n'读取出来、将其丢弃、将'\n'替换为字符串结束符'\0'； fgets()也会将缓冲区中的换行符读取出来，但并不丢弃，而是作为字符串组成字符存在，读取完成之后自动在最后添加字符串结束字符'\0'。
    
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    char str[100] = {0};
    printf("请输入字符串: ");
    fgets(str, sizeof(str), stdin);
    printf("%s", str);
    exit(0);
}
```

**![image-20240921084119061](G:\notes\typora_pic\image-20240921084119061.png)**

```c
/*
注意这里的读取文件会设置文件偏移。从0开始，如读取100字节大小是0-99， 然后是99开始，到99+1024 = 1123
*/
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char str[100] = {0};
    FILE *fp = NULL;

    /* 打开文件 */
    fp = fopen("./test_file", "r");
    if (NULL == fp) {
        perror("fopen error");
        exit(-1);
    }

    /* 从文件中输入字符串 */
    fgets(str, sizeof(str), fp);
    printf("First read: %s\n", str);

    /* 打印文件指针位置 */
    printf("File pointer position after first read: %ld\n", ftell(fp));

    /* 再次读取 */
    char str2[1024] = {0};
    fgets(str2, sizeof(str2), fp);
    printf("Second read: %s\n", str2);

    /* 打印文件指针位置 */
    printf("File pointer position after second read: %ld\n", ftell(fp));

    /* 关闭文件 */
    fclose(fp);
    exit(0);
}
```

**fgetc 函数：fgetc()与 getchar()一样，用于读取一个输入字符。**  

```c
#include <stdio.h>
int fgetc(FILE *stream);

/*
stream： 文件指针。
返回值： 该函数以无符号 char 强制转换为 int 的形式返回读取的字符，如果到达文件末尾或发生读错误，则返回 EOF。

fgetc()与 getchar()的区别在于， fgetc 可以指定输入字符的文件，既可以从标准输入设备输入字符，也可以从一个普通文件中输入字符，其它方面与 getchar 函数相同。
*/
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    int ch;
    ch = fgetc(stdin);
    printf("%c\n", ch);
    exit(0);
}

/*********************************************/
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    int ch;
    FILE *fp = NULL;
    /* 打开文件 */
    fp = fopen("./test_file", "r");
    if (NULL == fp) {
    	perror("fopen error");
    	exit(-1);
    }
    /* 从文件中获取一个字符 在文件重新打开后偏移为0， 否之可以一直读取一直累加文件偏移*/
    ch = fgetc(fp);
    printf("%c\n", ch)
    /*
    for(int i = 0; i < 86; i++){//获取前86个字符
        ch = fgetc(fp);
        printf("%c ", ch);
    }
    */
    /* 关闭文件 */
    fclose(fp);
    exit(0);
}
```

#### 字符串长度  

C 语言函数库中提供了一个用于计算字符串长度的函数 strlen()；

```c
#include <string.h>
size_t strlen(const char *s);
/*
s： 需要进行长度计算的字符串，字符串必须包含结束字符' \0 '。
返回值： 返回字符串长度（以字节为单位），字符串结束字符' \0 '不计算在内。*/
```

sizeof 和 strlen 的区别

⚫ sizeof 是 C 语言内置的操作符关键字，而 strlen 是 C 语言库函数；
⚫ sizeof 仅用于计算数据类型的大小或者变量的大小，而 strlen 只能以结尾为' \0 '的字符串作为参数；
⚫ 编译器在编译时就计算出了 sizeof 的结果，而 strlen 必须在运行时才能计算出来；
⚫ sizeof 计算数据类型或变量会占用内存的大小， strlen 计算字符串实际长度。

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void)
{
    char str[50] = "Linux app strlen test!";
    char *ptr = str;
    printf("sizeof: %ld\n", sizeof(str));
    printf("strlen: %ld\n", strlen(str));
    puts("~~~~~~~~~~");
    printf("sizeof: %ld\n", sizeof(ptr));//64位系统，指针是8字节，32位系统是4字节
    printf("strlen: %ld\n", strlen(ptr));
    exit(0);
}
```

#### 字符串拼接  

C 语言函数库中提供了 strcat()函数或 strncat()函数用于将两个字符串连接（拼接）起来。

```c
#include <string.h>
char *strcat(char *dest, const char *src);
/*****************************************
dest： 目标字符串。
src： 源字符串。
返回值： 返回指向目标字符串 dest 的指针。

strcat()函数会把 src 所指向的字符串追加到 dest 所指向的字符串末尾，所以必须要保证 dest 有足够的存储空间来容纳两个字符串，否则会导致溢出错误； dest 末尾的' \0 '结束字符会被覆盖， src 末尾的结束字符'\0 '会一起被复制过去，最终的字符串只有一个' \0 '。*/
```

```c
/*
strncat()与 strcat()的区别在于， strncat 可以指定源字符串追加到目标字符串的字符数量.
*/
#include <string.h>
char *strncat(char *dest, const char *src, size_t n);
/*
dest： 目标字符串。
src： 源字符串。
n： 要追加的最大字符数。
返回值： 返回指向目标字符串 dest 的指针。
*/
```

#### 字符串拷贝  

C 语言函数库中提供了 strcpy()函数和 strncpy()函数用于实现字符串拷贝.

```c
#include <string.h>
char *strcpy(char *dest, const char *src);
/*dest： 目标字符串。
src： 源字符串。
返回值： 返回指向目标字符串 dest 的指针。
strcpy()会把 src（必须包含结束字符' \0 '） 指向的字符串复制（包括字符串结束字符' \0 '） 到 dest，所以必须保证 dest 指向的内存空间足够大，能够容纳下 src 字符串，否则会导致溢出错误。
*/

/*strncpy()与 strcpy()的区别在于， strncpy()可以指定从源字符串 src 复制到目标字符串 dest 的字符数量*/
#include <string.h>
char *strncpy(char *dest, const char *src, size_t n);
/*
dest： 目标字符串。
src： 源字符串。
n： 从 src 中复制的最大字符数。
返回值： 返回指向目标字符串 dest 的指针。

把 src 所指向的字符串复制到 dest，最多复制 n 个字符。当 n 小于或等于 src 字符串长度（不包括结束字符的长度）时， 则复制过去的字符串中没有包含结束字符' \0 '；当 n 大于 src 字符串长度时，则会将 src 字符串的结束字符' \0 '也一并拷贝过去， 必须保证 dest 指向的内存空间足够大，能够容纳下拷贝过来的字符串，否则会导致溢出错误。
*/

/*
除了 strcpy()和 strncpy()之外，其实还可以使用 memcpy()、 memmove()以及 bcopy()这些库函数实现拷贝操作，字符串拷贝本质上也只是内存数据的拷贝，所以这些库函数同样也是适用的，在实际的编程当中，这些库函数也是很常用的.
*/
```

#### 内存填充  

```c
#include <string.h>
void *memset(void *s, int c, size_t n);
/*
s： 需要进行数据填充的内存空间起始地址。
c： 要被设置的值，该值以 int 类型传递。
n： 填充的字节数。
返回值： 返回指向内存空间 s 的指针。

参数 c 虽然是以 int 类型传递，但 memset()函数在填充内存块时是使用该值的无符号字符形式，也就是函数内部会将该值转换为 unsigned char 类型的数据，以字节为单位进行数据填充。
*/

#include <strings.h>
void bzero(void *s, size_t n);
/*
s： 内存空间的起始地址。
n： 填充的字节数。
返回值： 无返回值。
*/
int main(void)
{
    char str[100];
    bzero(str, sizeof(str));
    exit(0);
}
/*********************************************************************/

```

#### 字符串比较  

```c
#include <string.h>
int strcmp(const char *s1, const char *s2);

/*
函数参数和返回值含义如下：
s1： 进行比较的字符串 1。
s2： 进行比较的字符串 2。
返回值：
⚫ 如果返回值小于 0，则表示 str1 小于 str2
⚫ 如果返回值大于 0，则表示 str1 大于 str2
⚫ 如果返回值等于 0，则表示字符串 str1 等于字符串 str2

strcmp 进行字符串比较，主要是通过比较字符串中的字符对应的 ASCII 码值， strcmp 会根据 ASCII 编码依次比较 str1 和 str2 的每一个字符，直到出现了不同的字符，或者某一字符串已经到达末尾（遇见了字符串结束字符' \0 '）。
*/
/***************************************************************************************/
/*strncmp()与 strcmp()函数一样，也用于对字符串进行比较操作，但最多比较前 n 个字符*/
#include <string.h>
int strncmp(const char *s1, const char *s2, size_t n);
/*
s1： 参与比较的第一个字符串。
s2： 参与比较的第二个字符串。
n： 最多比较前 n 个字符。
返回值： 返回值含义与 strcmp()函数相同。*/
```

#### 字符串查找  

C 语言函数库中也提供了一些用于字符串查找的函数，包括 strchr()、strrchr()、 strstr()、 strpbrk()、 index()以及 rindex()等.  

```c
//strchr()函数可以查找到给定字符串当中的某一个字符.
#include <string.h>
char *strchr(const char *s, int c);

/*
s： 给定的目标字符串。
c： 需要查找的字符。
返回值： 返回字符 c 第一次在字符串 s 中出现的位置，如果未找到字符 c，则返回 NULL。
*/
```

```c
/*strrchr()与 strchr()函数一样，它同样表示在字符串中查找某一个字符，返回字符第一次在字符串中出现的位置，如果没找到该字符，则返回值 NULL，但两者唯一不同的是， strrchr()函数在字符串中是从后到前（或者称为从右向左）查找字符，找到字符第一次出现的位置就返回，返回值指向这个位置.*/
#include <string.h>
char *strrchr(const char *s, int c);//函数参数和返回值含义与 strchr()函数相同。
```

```c
/*与 strchr()函数不同的是， strstr()可在给定的字符串 haystack 中查找第一次出现子字符串 needle 的位置，不包含结束字符' \0 '*/
#include <string.h>
char *strstr(const char *haystack, const char *needle);
/*
haystack： 目标字符串。
needle： 需要查找的子字符串。
返回值： 如果目标字符串 haystack 中包含了子字符串 needle，则返回该字符串首次出现的位置；如果未能找到子字符串 needle，则返回 NULL。*/
```

#### 字符串与数字互转

**字符串转整形数据：** C 函数库中提供了一系列函数用于实现将一个字符串转为整形数据，主要包括 **atoi()、 atol()、 atoll()**以及**strtol()、 strtoll()、 strtoul()、 strtoull()** ， 它们之间的区别主要包括以下两个方面：  

⚫ 数据类型（int、 long int、 unsigned long 等）。
⚫ 不同进制方式表示的数字字符串（八进制、十六进制、十进制）。  

atoi()、 atol()、 atoll()三个函数可用于将字符串分别转换为 int、 long int 以及 long long 类型的数据。

```c
#include <stdlib.h>
int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
/*
nptr： 需要进行转换的字符串。
返回值： 分别返回转换之后得到的 int 类型数据、 long int 类型数据以及 long long 类型数据。

目标字符串 nptr 中可以包含非数字字符， 转换时跳过前面的空格字符（如果目标字符串开头存在空格字符） ，直到遇上数字字符或正负符号才开始做转换，而再遇到非数字或字符串结束时(' /0 ')才结束转换，并将结果返回。
tips: 使用 atoi()、 atol()、 atoll()函数只能转换十进制表示的数字字符串，即 0~9
*/
printf("atoi: %d\n", atoi("500"));
printf("atol: %ld\n", atol("500"));
printf("atoll: %lld\n", atoll("500"));
```

**strtol、 strtoll 函数**  

strtol()、 strtoll()两个函数可分别将字符串转为 long int 类型数据和 long long ing 类型数据，与 atol()、atoll()之间的区别在于， strtol()、 strtoll()可以实现将多种不同进制数（譬如二进制表示的数字字符串、八进制表示的数字字符串、十六进制表示的数数字符串） 表示的字符串转换为整形数据。 

```c
#include <stdlib.h>
long int strtol(const char *nptr, char **endptr, int base);
long long int strtoll(const char *nptr, char **endptr, int base);

/*
nptr： 需要进行转换的目标字符串。
endptr： char **类型的指针， 如果 endptr 不为 NULL，则 strtol()或 strtoll()会将字符串中第一个无效字符的地址存储在*endptr 中。如果根本没有数字， strtol()或 strtoll()会将 nptr 的原始值存储在*endptr 中（并返回 0）。 也可将参数 endptr 设置为 NULL，表示不接收相应信息。
base： 数字基数， 参数 base 必须介于 2 和 36（包含）之间，或者是特殊值 0。 参数 base 决定了字符串转换为整数时合法字符的取值范围，譬如，当 base=2 时，合法字符为' 0 '、 ' 1 '（表示是一个二进制表示的数字字符串） ；当 base=8 时，合法字符为' 0 '、 ' 1 '、 ' 2 '、 ' 3 '……' 7 '（表示是一个八进制表示的数字字符串） ；当 base=16 时，合法字符为' 0 ' 、 ' 1 '、 ' 2 '、 ' 3 '……' 9 '、 ' a '……' f '（表示是一个十六进制表示的数字字符串） ；当 base 大于 10 的时候， ' a '代表 10、 ' b '代表 11、 ' c '代表 12，依次类推， ' z '代表 35（不区分大小写） 。
返回值： 分别返回转换之后得到的 long int 类型数据以及 long long int 类型数据。
需要进行转换的目标字符串可以以任意数量的空格或者 0 开头， 转换时跳过前面的空格字符，直到遇上数字字符或正负符号（' + '或' - '） 才开始做转换，而再遇到非数字或字符串结束时(' /0 ')才结束转换，并将结果返回。
*/
```

#### 字符串转浮点型数据  

C 函数库中用于字符串转浮点型数据的函数有 atof()、 strtod()、 strtof()、 strtold()。  

```c
#include <stdlib.h>
double atof(const char *nptr);
/*
nptr： 需要进行转换的字符串。
返回值： 返回转换得到的 double 类型数据。*/
printf("atof: %lf\n", atof("0.123"));
printf("atof: %lf\n", atof("-1.1185"));
printf("atof: %lf\n", atof("100.0123"));

/*
0.123000
-1.118500
100.012300
*/
    
```

**strtof()、 strtod()以及 strtold()三个库函数可分别将字符串转换为 float 类型数据、 double 类型数据、 long double 类型数据**  

```c
#include <stdlib.h>
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
```

**数字转换为字符串推荐大家使用前面介绍的格式化 IO 相关库函数，譬如使用 printf()将数字转字符串、并将其输出到标准输出设备或者使用 sprintf()或 snprintf()将数字转换为字符串并存储在缓冲区中。**

#### 给应用程序传参  

如果在执行应用程序时， 需要向应用程序传递参数，则写法如下：  

```c
int main(int argc, char **argv)
{
/* 代码 */
}
//或
int main(int argc, char *argv[])
{
/* 代码 */
}
/*
传递进来的参数以字符串的形式存在，字符串的起始地址存储在 argv 数组中，参数 argc 表示传递进来的参数个数，包括应用程序自身路径名，多个不同的参数之间使用空格分隔开来，如果参数本身带有空格、则可以使用双引号" "或者单引号' '的形式来表示。
*/

#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
	int i = 0;
    printf("Number of parameters: %d\n", argc);
    for (i = 0; i < argc; i++)
    	printf(" %s\n", argv[i]);
    exit(0);
}
```

![image-20240921160510341](G:\notes\typora_pic\image-20240921160510341.png)

#### 正则表达式  

正则表达式，又称为规则表达式（英语: Regular Expression），正则表达式通常被用来检索、替换那些符合某个模式（规则）的字符串，正则表达式描述了一种字符串的匹配模式（pattern），可以用来检查一个给定的字符串中是否含有某种子字符串、将匹配的字符串替换或者从某个字符串中取出符合某个条件的子字符串。  

在 Linux 系统下运行命令的时候，相信大家都使用过?或* 通配符来查找硬盘上的文件或者文本中的某个字符串， ?通配符匹配 0 个或 1 个字符，而*通配符匹配 0 个或多个字符，譬如"data?.txt"这样的匹配模式可以将下列文件查找出来：  

```c
data.dat
data1.dat
data2.dat
datax.dat
dataN.dat
```

正则表达式这个概念最初是由 Unix 中的工具软件（例如 sed 和 grep）普及开的，使用过 sed 命令的朋友想必对正则表达式并不陌生。同样，在 C 语言函数库中也提供了用于处理正则表达式的接口供程序员使用。  

编译正则表达式
匹配正则表达式
释放正则表达式

```sh
匹配 URL 的正则表达式：
^((ht|f)tps?)://[-A-Za-z0-9_]+(\.[-A-Za-z0-9_]+)+([-A-Za-z0-9_.,@?^=%&:/~+#]*[-A-Za-z0-9_@?^=%&/~+#])?$
```

~~~shell
当然可以！正则表达式（Regular Expression，简称regex）是一种用于匹配字符串中字符模式的工具。它在文本处理、数据验证、搜索和替换等方面非常有用。下面是一些基本概念和常用的正则表达式语法：

### 基本概念

1. **字符**：普通字符（如 `a`, `b`, `c`）匹配自身。
2. **元字符**：特殊字符具有特殊含义，如 `.`（匹配任意单个字符），`*`（匹配前面的字符零次或多次），`^`（匹配字符串的开始），`$`（匹配字符串的结束）。

### 常用语法

1. **字符类**：
    - `[abc]`：匹配 `a`, `b`, 或 `c` 中的任意一个字符。
    - `[a-z]`：匹配任意小写字母。
    - `[^abc]`：匹配除 `a`, `b`, `c` 之外的任意字符。

2. **预定义字符类**：
    - `\d`：匹配任意数字，相当于 `[0-9]`。
    - `\w`：匹配任意字母、数字或下划线，相当于 `[a-zA-Z0-9_]`。
    - `\s`：匹配任意空白字符（空格、制表符等）。

3. **量词**：
    - `*`：匹配前面的字符零次或多次。
    - `+`：匹配前面的字符一次或多次。
    - `?`：匹配前面的字符零次或一次。
    - `{n}`：匹配前面的字符恰好 `n` 次。
    - `{n,}`：匹配前面的字符至少 `n` 次。
    - `{n,m}`：匹配前面的字符至少 `n` 次，至多 `m` 次。

4. **位置匹配**：
    - `^`：匹配字符串的开始。
    - `$`：匹配字符串的结束。
    - `\b`：匹配单词边界。

5. **分组和捕获**：
    - `()`：将括号内的表达式作为一个组。
    - `|`：表示“或”操作，例如 `(a|b)` 匹配 `a` 或 `b`。

### 示例

1. **匹配邮箱地址**：
    ```regex
    [a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}
    ```
    - 解释：匹配一个邮箱地址，`[a-zA-Z0-9._%+-]+` 匹配邮箱用户名部分，`@[a-zA-Z0-9.-]+` 匹配域名部分，`\.[a-zA-Z]{2,}` 匹配顶级域名部分。

2. **匹配电话号码**：
    ```regex
    \d{3}-\d{3}-\d{4}
    ```
    - 解释：匹配格式为 `123-456-7890` 的电话号码。

3. **匹配日期**：
    ```regex
    \d{4}-\d{2}-\d{2}
    ```
    - 解释：匹配格式为 `YYYY-MM-DD` 的日期。

### 使用示例

在C语言中使用正则表达式的示例已经在你提供的代码中展示了。通过 `regcomp` 编译正则表达式，使用 `regexec` 进行匹配，并通过 `regfree` 释放资源。

希望这些解释能帮助你更好地理解正则表达式！如果你有具体的正则表达式问题或需要更多示例，请告诉我。

#####################################################################################
[a-zA-Z0-9._%+-]这里说的._%+-是啥意思？

在正则表达式 `[a-zA-Z0-9._%+-]` 中，方括号 `[]` 内的内容表示一个字符类，匹配其中任意一个字符。具体来说：

- `a-z`：匹配任意小写字母（从 `a` 到 `z`）。
- `A-Z`：匹配任意大写字母（从 `A` 到 `Z`）。
- `0-9`：匹配任意数字（从 `0` 到 `9`）。
- `.`：匹配一个点（`.`）。
- `_`：匹配一个下划线（`_`）。
- `%`：匹配一个百分号（`%`）。
- `+`：匹配一个加号（`+`）。
- `-`：匹配一个减号（`-`）。

因此，`[a-zA-Z0-9._%+-]` 表示可以匹配一个字母（无论大小写）、一个数字、一个点、一个下划线、一个百分号、一个加号或一个减号中的任意一个字符。

这个字符类通常用于匹配电子邮件地址中的用户名部分，因为这些字符都是合法的用户名字符。希望这个解释对你有帮助！如果你有更多问题，随时告诉我。
~~~

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <string.h>
int main(int argc, char *argv[])
{
    regmatch_t pmatch = {0};
    regex_t reg;
    char errbuf[64];
    int ret;
    char *sptr;
    int length;
    int nmatch; //最多匹配出的结果
    if (4 != argc) {
        /**********************************
        * 执行程序时需要传入两个参数:
        * arg1: 正则表达式
        * arg2: 待测试的字符串
        * arg3: 最多匹配出多少个结果
        **********************************/
        fprintf(stderr, "usage: %s <regex> <string> <nmatch>\n", argv[0]);
        exit(0);
    }
    /* 编译正则表达式 */
    if(ret = regcomp(&reg, argv[1], REG_EXTENDED)) {
        regerror(ret, &reg, errbuf, sizeof(errbuf));
        fprintf(stderr, "regcomp error: %s\n", errbuf);
        exit(0);
    }
    /* 赋值操作 */
    sptr = argv[2]; //待测试的字符串
    length = strlen(argv[2]);//获取字符串长度
    nmatch = atoi(argv[3]); //获取最大匹配数
    /* 匹配正则表达式 */
    for (int j = 0; j < nmatch; j++) {
        char temp_str[100];
        /* 调用 regexec 匹配正则表达式 */
        if(ret = regexec(&reg, sptr, 1, &pmatch, 0)) {
            regerror(ret, &reg, errbuf, sizeof(errbuf));
            fprintf(stderr, "regexec error: %s\n", errbuf);
            goto out;
        }
        if(-1 != pmatch.rm_so) {
                if (pmatch.rm_so == pmatch.rm_eo) {//空字符串
                sptr += 1;
                length -= 1;
                printf("\n"); //打印出空字符串
                if (0 >= length)//如果已经移动到字符串末尾、则退出
                    break;
                continue; //从 for 循环开始执行
            }
            memset(temp_str, 0x00, sizeof(temp_str));//清零缓冲区
            memcpy(temp_str, sptr + pmatch.rm_so,
            pmatch.rm_eo - pmatch.rm_so);//将匹配出来的子字符串拷贝到缓冲区
            printf("%s\n", temp_str); //打印字符串
            sptr += pmatch.rm_eo;
            length -= pmatch.rm_eo;
            if (0 >= length)
                break;
        }
    }
    /* 释放正则表达式 */
    out:
    regfree(&reg);
    exit(0);
}
#endif
//运行：
skating@DESKTOP-4FRM7ID:~/linux_c/4_string$ ./fgets_verifiy "^[a-zA-Z]+$" "HelloWorld" 5
HelloWorld

^[a-zA-Z]+$ : ^开始匹配， [a-zA-z]表示匹配字符是大小写字母，+表示多次匹配，$表示结束匹配
```

### 1.5、系统信息与系统资源

#### 系统信息

系统调用 **uname()**用于获取有关当前操作系统内核的名称和信息。

```c
#include <sys/utsname.h>
int uname(struct utsname *buf);

/*
buf： struct utsname 结构体类型指针， 指向一个 struct utsname 结构体类型对象。
返回值： 成功返回 0；失败将返回-1，并设置 errno。

uname()函数用法非常简单，先定义一个 struct utsname 结构体变量，调用 uname()函数时传入变量的地址即可，
*/
struct utsname {
    char sysname[]; /* 当前操作系统的名称 */
    char nodename[]; /* 网络上的名称（主机名） */
    char release[]; /* 操作系统内核版本 */
    char version[]; /* 操作系统发行版本 */
    char machine[]; /* 硬件架构类型 */
#ifdef _GNU_SOURCE
    char domainname[];/* 当前域名 */
#endif
};

/*编写一个简单地程序，获取并打印出当前操作系统名称、主机名、内核版本、操作系统发行版本以及处理器硬件架构类型等信息*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
int main(void)
{
    struct utsname os_info;
    int ret;
    /* 获取信息 */
    ret = uname(&os_info);
    if (-1 == ret) {
        perror("uname error");
    	exit(-1);
    }
    /* 打印信息 */
    printf("操作系统名称: %s\n", os_info.sysname);
    printf("主机名: %s\n", os_info.nodename);
    printf("内核版本: %s\n", os_info.release);
    printf("发行版本: %s\n", os_info.version);
    printf("硬件架构: %s\n", os_info.machine);
    exit(0);
}
```

![image-20240921171816816](G:\notes\typora_pic\image-20240921171816816.png)

**sysinfo() 函数：**  sysinfo 系统调用可用于获取一些系统统计信息 。

```c
#include <sys/sysinfo.h>
int sysinfo(struct sysinfo *info);
/*
info： struct sysinfo 结构体类型指针，指向一个 struct sysinfo 结构体类型对象。
返回值： 成功返回 0；失败将返回-1，并设置 errno。*/
struct sysinfo {
    long uptime; /* 自系统启动之后所经过的时间（以秒为单位） */
    unsigned long loads[3]; /* 1, 5, and 15 minute load averages */
    unsigned long totalram; /* 总的可用内存大小 */
    unsigned long freeram; /* 还未被使用的内存大小 */
    unsigned long sharedram; /* Amount of shared memory */
    unsigned long bufferram; /* Memory used by buffers */
    unsigned long totalswap; /* Total swap space size */
    unsigned long freeswap; /* swap space still available */
    unsigned short procs; /* 系统当前进程数量 */
    unsigned long totalhigh; /* Total high memory size */
    unsigned long freehigh; /* Available high memory size */
    unsigned int mem_unit; /* 内存单元大小（以字节为单位） */
    char _f[20-2*sizeof(long)-sizeof(int)]; /* Padding to 64 bytes */
};

#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
int main(void)
{
    struct sysinfo sys_info;
    int ret;
    /* 获取信息 */
    ret = sysinfo(&sys_info);
    if (-1 == ret) {
    	perror("sysinfo error");
    	exit(-1);
    }
    /* 打印信息 */
    printf("uptime: %ld\n", sys_info.uptime);
    printf("totalram: %lu\n", sys_info.totalram);
    printf("freeram: %lu\n", sys_info.freeram);
    printf("procs: %u\n", sys_info.procs);
    exit(0);
}
```

**gethostname() 函数**

此函数可用于单独获取 Linux 系统主机名， 与 struct utsname 数据结构体中的 nodename 变量一样。

```c
#include <unistd.h>
int gethostname(char *name, size_t len);

/**
name： 指向用于存放主机名字符串的缓冲区。
len： 缓冲区长度。
返回值： 成功返回 0, 失败将返回-1，并会设置 errno。
**/
//使用 gethostname 函数获取系统主机名:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
int main(void)
{
    char hostname[20];
    int ret;
    memset(hostname, 0x0, sizeof(hostname));
    ret = gethostname(hostname, sizeof(hostname));
    if (-1 == ret) {
   	 	perror("gethostname error");
    	exit(ret);
    }
    puts(hostname);
    exit(0);
}
```

**sysconf()函数**

sysconf()函数是一个库函数，可在运行时获取系统的一些配置信息，譬如页大小（page size）、主机名的最大长度、进程可以打开的最大文件数、 每个用户 ID 的最大并发进程数等。

```c
#include <unistd.h>
long sysconf(int name);
/*调用 sysconf()函数获取系统的配置信息， 参数 name 指定了要获取哪个配置信息*/
⚫ _SC_ARG_MAX： exec 族函数的参数的最大长度， exec 族函数后面会介绍，这里先不管！
⚫ _SC_CHILD_MAX： 每个用户的最大并发进程数，也就是同一个用户可以同时运行的最大进程数。
⚫ _SC_HOST_NAME_MAX： 主机名的最大长度。
⚫ _SC_LOGIN_NAME_MAX： 登录名的最大长度。
⚫ _SC_CLK_TCK： 每秒时钟滴答数，也就是系统节拍率。  用的比较多的
⚫ _SC_OPEN_MAX： 一个进程可以打开的最大文件数。
⚫ _SC_PAGESIZE： 系统页大小（page size）。  		用的比较多的
⚫ _SC_TTY_NAME_MAX： 终端设备名称的最大长度。
 /* 获取每个用户的最大并发进程数、系统节拍率和系统页大小。 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    printf("每个用户的最大并发进程数: %ld\n", sysconf(_SC_CHILD_MAX));
    printf("系统节拍率: %ld\n", sysconf(_SC_CLK_TCK));
    printf("系统页大小: %ld\n", sysconf(_SC_PAGESIZE));
    exit(0);
}
```

#### 时间、日期

GMT 12:00 对应的北京时间是 20:00  

GMT 与 UTC 这两者几乎是同一概念，它们都是指格林威治标准时间，也就是国际标准时间，只不过UTC 时间比 GMT 时间更加精准，所以在我们的编程当中不用刻意去区分它们之间的区别。  

在 Ubuntu 系统下， 可以使用"date -u"命令查看到当前的 UTC 时间

```shell
hhb@DESKTOP-4FRM7ID:/usr/share/zoneinfo$ ls -l /etc/localtime
lrwxrwxrwx 1 root root 33 Sep 21 14:35 /etc/localtime -> /usr/share/zoneinfo/Asia/Shanghai
#可以通过修改/etc/localtime的指向来修改系统时间。

#如果我们要修改 Ubuntu 系统本地时间的时区信息，可以直接将/etc/localtime 链接到/usr/share/zoneinfo目录下的任意一个时区配置文件，譬如 EST（美国东部标准时间）
#进入到/etc/目录下
sudo rm -rf localtime #删除原有链接文件
sudo ln -s /usr/share/zoneinfo/EST localtime #重新建立链接文件
```

操作系统中一般会有两个时钟，一个系统时钟（system clock），一个实时时钟（Real time clock），也叫 RTC；  

使用 date 命令查看到的就是系统时钟， 所以在系统关机情况下是不存在的；而实时时钟一般由 RTC 时钟芯片提供， RTC 芯片有相应的电池为其供电，以保证系统在关机情况下 RTC 能够继续工作、继续计时。  

操作系统使用 jiffies 这个全局变量来记录当前时间，当我们需要获取到系统当前时间点时，就可以使用 jiffies 变量去计算。

Linux 系统提供了相应的系统调用或 C库函数用于获取当前时间，譬如系统调用 time()、 gettimeofday()，其实质上就是通过 jiffies 变量换算得到。  

**获取时间 time/gettimeofday**

**time()函数**  系统调用 time()用于获取当前时间，以秒为单位，返回得到的值是自 1970-01-01 00:00:00 +0000 (UTC)以来的秒数。

```c
#include <time.h>
time_t time(time_t *tloc);
/*
tloc： 如果 tloc 参数不是 NULL，则返回值也存储在 tloc 指向的内存中。
返回值： 成功则返回自 1970-01-01 00:00:00 +0000 (UTC)以来的时间值（以秒为单位）；失败则返回-1，并会设置 errno
你要计算现在这个时间点，只需要使用 time()得到的秒数加 1970-01-01 00:00:00即可。
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    time_t t;
    t = time(NULL);
    if (-1 == t) {
   	 	perror("time error");
    	exit(-1);
    }
    printf("时间值: %ld\n", t);//单位是秒
    exit(0);
}

```

**gettimeofday()函数**，获取时间是微秒为单位的函数。  

```c
#include <sys/time.h>
int gettimeofday(struct timeval *tv, struct timezone *tz);

/*
tv： 参数 tv 是一个 struct timeval 结构体指针变量;
tz： 参数 tz 是个历史产物，早期实现用其来获取系统的时区信息，目前已遭废弃，在调用 gettimeofday()函数时应将参数 tz 设置为 NULL。
返回值： 成功返回 0；失败将返回-1，并设置 errno。

获取得到的时间值存储在参数 tv 所指向的 struct timeval 结构体变量中，该结构体包含了两个成员变量tv_sec 和 tv_usec，分别用于表示秒和微秒，所以获取得到的时间值就是 tv_sec（秒） +tv_usec（微秒） ，同样获取得到的秒数与 time()函数一样，也是自 1970-01-01 00:00:00 +0000 (UTC)到现在这段时间所经过的秒数， 也就是日历时间， 所以由此可知 time()返回得到的值和函数 gettimeofday()所返回的 tv 参数中 tv_sec 字段的数值相同。
*/
```

#### 时间转换函数

**ctime()函数: **  ctime()是一个 C 库函数， 可以将日历时间转换为可打印输出的字符串形式.

```c
#include <time.h>
char *ctime(const time_t *timep);
char *ctime_r(const time_t *timep, char *buf);

/*
timep： time_t 时间变量指针。
返回值： 成功将返回一个 char *类型指针，指向转换后得到的字符串；失败将返回 NULL。

但是 ctime()是一个不可重入函数， 存在一些安全上面的隐患， ctime_r()是 ctime()的可重入版本，一般推荐大家使用可重入函数 ctime_r()，可重入函数 ctime_r()多了一个参数 buf，也就是缓冲区首地址，所以 ctime_r()函数需要调用者提供用于存放字符串的缓冲区。

ctime （或ctime_r）转换得到的时间是计算机所在地对应的本地时间（譬如在中国对应的便是北京时间），并不是 UTC 时间
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    char tm_str[100] = {0};
    time_t tm;
    /* 获取时间 */
    tm = time(NULL);
    if (-1 == tm) {
        perror("time error");
        exit(-1);
    }
    /* 将时间转换为字符串形式 */
    ctime_r(&tm, tm_str);
    /* 打印输出 */
    printf("当前时间: %s", tm_str);
    exit(0);
}
```

**localtime()函数**可以把 **time()或 gettimeofday()**得到的秒数（time_t 时间或日历时间） 变成一个 **struct tm**结构体所表示的时间， 该时间对应的是本地时间。  

```c
#include <time.h>
struct tm *localtime(const time_t *timep);
struct tm *localtime_r(const time_t *timep, struct tm *result);

/*
使用该函数需要包含头文件<time.h>， localtime()的可重入版本为 localtime_r()。

timep： 需要进行转换的 time_t 时间变量对应的指针，可通过 time()或 gettimeofday()获取得到。
result： 是一个 struct tm 结构体类型指针， 稍后给大家介绍 struct tm 结构体，参数 result 是可重入函数
localtime_r()需要额外提供的参数

返回值： 对于不可重入版本 localtime()来说，成功则返回一个有效的 struct tm 结构体指针;
而对于可重入版本 localtime_r()来说， 成功执行情况下，返回值将会等于参数 result；失败则返回 NULL。

使用不可重入函数 localtime()并不需要调用者提供 struct tm 变量，而是它会直接返回出来一个 struct tm结构体指针，然后直接通过该指针访问里边的成员变量即可！虽然很方便，但是存在一些安全隐患，所以一般不推荐使用不可重入版本。

使用可重入版本 localtime_r()调用者需要自己定义 struct tm 结构体变量、并将该变量指针赋值给参数result，在函数内部会对该结构体变量进行赋值操作。
*/
struct tm {
    int tm_sec; /* 秒(0-60) */
    int tm_min; /* 分(0-59) */
    int tm_hour; /* 时(0-23) */
    int tm_mday; /* 日(1-31) */
    int tm_mon; /* 月(0-11) */
    int tm_year; /* 年(这个值表示的是自 1900 年到现在经过的年数) */
    int tm_wday; /* 星期(0-6, 星期日 Sunday = 0、星期一=1…) */
    int tm_yday; /* 一年里的第几天(0-365, 1 Jan = 0) */
    int tm_isdst; /* 夏令时 */
};
/*
从 struct tm 结构体内容可知，该结构体中包含了年月日时分秒星期等信息， 使用 localtime/localtime_r()便可以将 time_t 时间总秒数分解成了各个独立的时间信息， 易于我们查看和理解。*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    struct tm t;
    time_t sec;
    /* 获取时间 */
    sec = time(NULL);
    if (-1 == sec) {
        perror("time error");
        exit(-1);
    }
	/* 转换得到本地时间 */
    localtime_r(&sec, &t);
    /* 打印输出 */
    printf("当前时间: %d 年%d 月%d 日 %d:%d:%d\n",
    	t.tm_year + 1900, t.tm_mon, t.tm_mday,
    	t.tm_hour, t.tm_min, t.tm_sec);
    exit(0);
}
//运行结果： 当前时间: 2021 年 1 月 22 日 18:38:32
```

**gmtime()函数**也可以把 time_t 时间变成一个 struct tm 结构体所表示的时间，与 localtime()所不同的是，gmtime()函数所得到的是 UTC 国际标准时间，并不是计算机的本地时间，这是它们之间的唯一区别。  

```c
#include <time.h>
struct tm *gmtime(const time_t *timep);
struct tm *gmtime_r(const time_t *timep, struct tm *result);
/*
gmtime_r()是 gmtime()的可重入版本， 同样也是推荐大家使用可重入版本函数 gmtime_r。 关于该函数的参数和返回值，这里便不再介绍，与 localtime()是一样的。*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    struct tm local_t;
    struct tm utc_t;
    time_t sec;
    /* 获取时间 */
    sec = time(NULL);
    if (-1 == sec) {
        perror("time error");
        exit(-1);
    }
    /* 转换得到本地时间 */
    localtime_r(&sec, &local_t);
    /* 转换得到国际标准时间 */
    gmtime_r(&sec, &utc_t);
    /* 打印输出 */
    printf("本地时间: %d 年%d 月%d 日 %d:%d:%d\n",
        local_t.tm_year + 1900, local_t.tm_mon, local_t.tm_mday,
        local_t.tm_hour, local_t.tm_min, local_t.tm_sec);
    printf("UTC 时间: %d 年%d 月%d 日 %d:%d:%d\n",
        utc_t.tm_year + 1900, utc_t.tm_mon, utc_t.tm_mday,
        utc_t.tm_hour, utc_t.tm_min, utc_t.tm_sec);
        exit(0);
}
/* 本地时间与 UTC 时间（国际标准时间）相差 8 个小时，因为笔者使用的计算机其对应的本地时间指的便是北京时间，而北京时间要早于国际标准时间 8 个小时（东八区）。 */
```

**mktime()函数**与 **localtime()函数**相反， mktime()可以将使用 struct tm 结构体表示的分解时间转换为 time_t 时间（日历时间） ，同样这也是一个 C 库函数  

```c
#include <time.h>
time_t mktime(struct tm *tm);
/*
函数参数和返回值含义如下：
tm： 需要进行转换的 struct tm 结构体变量对应的指针。
返回值： 成功返回转换得到 time_t 时间值；失败返回-1。
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    struct tm local_t;
    time_t sec;
    /* 获取时间 */
    sec = time(NULL);
    if (-1 == sec) {
        perror("time error");
        exit(-1);
    }
    printf("获取得到的秒数: %ld\n", sec);
    localtime_r(&sec, &local_t);
    printf("转换得到的秒数: %ld\n", mktime(&local_t));
	exit(0);
}
```

**asctime()函数**与 **ctime()函数的作用一样**，也可将时间转换为可打印输出的字符串形式，与 ctime()函数的区别在于， **ctime()是将 time_t 时间转换为固定格式字符串**、**而 asctime()则是将 struct tm 表示的分解时间转换为固定格式的字符串**  

```c
#include <time.h>
char *asctime(const struct tm *tm);
char *asctime_r(const struct tm *tm, char *buf);

/*
tm： 需要进行转换的 struct tm 表示的时间。
buf： 可重入版本函数 asctime_r 需要额外提供的参数 buf，指向一个缓冲区，用于存放转换得到的字符串。
返回值： 转换失败将返回 NULL；成功将返回一个 char *类型指针，指向转换后得到的时间字符串，对于 asctime_r 函数来说，返回值就等于参数 buf。
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    struct tm local_t;
    char tm_str[100] = {0};
    time_t sec;
    /* 获取时间 */
    sec = time(NULL);
    if (-1 == sec) {
        perror("time error");
    	exit(-1);
    }
    localtime_r(&sec, &local_t);
    asctime_r(&local_t, tm_str);
    printf("本地时间: %s", tm_str);
    exit(0);
}
```

**strftime 函数 : **  除了 asctime()函数之外，这里再给大家介绍一个 C 库函数 strftime()，此函数也可以将一个 struct tm 变
量表示的分解时间转换为为格式化字符串，并且在功能上比 asctime()和 ctime()更加强大，它可以根据自己的喜好自定义时间的显示格式，而 asctime()和 ctime()转换得到的字符串时间格式的固定的。  

```c
#include <time.h>
size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);

/*
s： 指向一个缓存区的指针，该缓冲区用于存放生成的字符串。
max： 字符串的最大字节数。
format： 这是一个用字符串表示的字段， 包含了普通字符和特殊格式说明符，可以是这两种字符的任意组合。 特殊格式说明符将会被替换为 struct tm 结构体对象所指时间的相应值，

tm： 指向 struct tm 结构体对象的指针。
返回值： 如果转换得到的目标字符串不超过最大字节数（也就是 max），则返回放置到 s 数组中的字节数；如果超过了最大字节数，则返回 0。*/

/*譬如我要想输出"2021-01-14 16:1:25<PM> January Thursday"这样一种形式表示的时间日期，那么就可以这样来设置 format 参数：*/
"%Y-%m-%d %H:%M:%S<%p> %B %A"
    
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void)
{
    struct tm local_t;
    char tm_str[100] = {0};
    time_t sec;
    /* 获取时间 */
    sec = time(NULL);
    if (-1 == sec) {
        perror("time error");
        exit(-1);
    }
    localtime_r(&sec, &local_t);
    strftime(tm_str, sizeof(tm_str), "%Y-%m-%d %A %H:%M:%S", &local_t);
    printf("本地时间: %s\n", tm_str);
    exit(0);
}
```

**设置时间 settimeofday()：**使用 settimeofday()函数可以设置时间， 也就是设置系统的本地时间。

```c
#include <sys/time.h>
int settimeofday(const struct timeval *tv, const struct timezone *tz);

/*
tv： 参数 tv 是一个 struct timeval 结构体指针变量， struct timeval 结构体在前面章节内容中已经给大家介绍了，需要设置的时间便通过参数 tv 指向的 struct timeval 结构体变量传递进去。
tz： 参数 tz 是个历史产物，早期实现用其来设置系统的时区信息，目前已遭废弃，在调用 settimeofday()函数时应将参数 tz 设置为 NULL。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
使用 settimeofday 设置系统时间时内核会进行权限检查，只有超级用户（root）才可以设置系统时间，普通用户将无操作权限。
*/
```

总结： time/ctime/localtime/gmtime/mktime/asctime/strftime/gettimeofday/settimeofday  

![image-20241015164548742](G:\notes\typora_pic\image-20241015164548742.png)

#### 进程时间  

进程时间指的是进程从创建后（也就是程序运行后）到目前为止这段时间内使用 CPU 资源的时间总数，出于记录的目的，内核把 CPU 时间（进程时间） 分为以下两个部分：  

⚫ 用户 CPU 时间：进程在用户空间（用户态）下运行所花费的 CPU 时间。有时也成为虚拟时间（virtual time）。
⚫ 系统 CPU 时间：进程在内核空间（内核态）下运行所花费的 CPU 时间。这是内核执行系统调用或代表进程执行的其它任务（譬如，服务页错误）所花费的时间。  

一般来说，进程时间指的是用户 CPU 时间和系统 CPU 时间的总和，也就是总的 CPU 时间。  

Tips：进程时间不等于程序的整个生命周期所消耗的时间， 如果进程一直处于休眠状态（进程被挂起、不会得到系统调度），那么它并不会使用 CPU 资源，所以休眠的这段时间并不计算在进程时间中。  

**times()函数**用于获取当前进程时间  

```c
#include <sys/times.h>
clock_t times(struct tms *buf);

/*
buf： times()会将当前进程时间信息存在一个 struct tms 结构体数据中，所以我们需要提供 struct tms 变量，使用参数 buf 指向该变量。
返回值： 返回值类型为 clock_t（实质是 long 类型）， 调用成功情况下，将返回从过去任意的一个时间点（譬如系统启动时间） 所经过的时钟滴答数（其实就是系统节拍数）， 将(节拍数 / 节拍率)便可得到秒数，返回值可能会超过 clock_t 所能表示的范围（溢出）； 调用失败返回-1，并设置 errno。
*/

struct tms {
    clock_t tms_utime; /* user time, 进程的用户 CPU 时间, tms_utime 个系统节拍数 */
    clock_t tms_stime; /* system time, 进程的系统 CPU 时间, tms_stime 个系统节拍数 */
    clock_t tms_cutime; /* user time of children, 已死掉子进程的 tms_utime + tms_cutime 时间总和 */
    clock_t tms_cstime; /* system time of children, 已死掉子进程的 tms_stime + tms_cstime 时间总和 */
};
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
    struct tms t_buf_start;
    struct tms t_buf_end;
    clock_t t_start;
    clock_t t_end;
    long tck;
    int i, j;
    /* 获取系统的节拍率 */
    tck = sysconf(_SC_CLK_TCK);
    /* 开始时间 */
    t_start = times(&t_buf_start);
    if (-1 == t_start) {
    	perror("times error");
    	exit(-1);
    }
    /* *****需要进行测试的代码段***** */
    for (i = 0; i < 20000; i++)
        for (j = 0; j < 20000; j++);
    	sleep(1); //休眠挂起
    /* *************end************** */
    /* 结束时间 */
    t_end = times(&t_buf_end);
    if (-1 == t_end) {
    	perror("times error");
    	exit(-1);
    }
    /* 打印时间 */
    printf("时间总和: %f 秒\n", (t_end - t_start) / (double)tck);
    printf("用户 CPU 时间: %f 秒\n", (t_buf_end.tms_utime - t_buf_start.tms_utime) / (double)tck);
    printf("系统 CPU 时间: %f 秒\n", (t_buf_end.tms_stime - t_buf_start.tms_stime) / (double)tck);
    exit(0);
}
结果： 
"时间总和: 2.91 秒"
"用户 CPU 时间: 1.9 秒"  /*sleep不计入cpu使用时间*/
"系统 CPU 时间: 0 秒"
```

**库函数 clock()**提供了一个更为简单的方式用于进程时间，它的返回值描述了进程使用的总的 CPU 时间（也就是进程时间，包括用户 CPU 时间和系统 CPU 时间）。

```c
#include <time.h>
clock_t clock(void);
/*
无参数。
返回值： 返回值是到目前为止程序的进程时间，为 clock_t 类型，注意 clock()的返回值并不是系统节拍数，如果想要获得秒数，请除以 CLOCKS_PER_SEC（这是一个宏）。 如果返回的进程时间不可用或其值无法表示，则该返回值是-1。
clock()函数虽然可以很方便的获取总的进程时间，但并不能获取到单独的用户 CPU 时间和系统 CPU 时间，在实际编程当中，根据自己的需要选择。
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[])
{
    clock_t t_start;
    clock_t t_end;
    int i, j;
    /* 开始时间 */
    t_start = clock();
    if (-1 == t_start)
    	exit(-1);
    /* *****需要进行测试的代码段***** */
    for (i = 0; i < 20000; i++)
    	for (j = 0; j < 20000; j++);
    /* *************end************** */
    /* 结束时间 */
    t_end = clock();
    if (-1 == t_end)
    exit(-1);
    /* 打印时间 */
    printf("总的 CPU 时间: %f\n", (t_end - t_start) / (double)CLOCKS_PER_SEC);
    exit(0);
}
/*
运行结果：
	总的 CPU 时间: 1.806213
*/
```

#### 产生随机数

随机数与伪随机数 ：随机数是随机出现，没有任何规律的一组数列。在我们编程当中，是没有办法获得真正意义上的随机数列的， 这是一种理想的情况，在我们的程序当中想要使用随机数列，只能通过算法得到一个伪随机数序列，那在编程当中说到的随机数，基本都是指伪随机数。  

C 语言函数库中提供了很多函数用于产生伪随机数，其中最常用的是通过 **rand()**和 **srand()**产生随机数。

**rand 函数： rand()函数用于获取随机数，多次调用 rand()可得到一组随机数序列**。

```c
#include <stdlib.h>
int rand(void);

/*
返回值： 返回一个介于 0 到 RAND_MAX（包含）之间的值，也就是数学上的[0, RAND_MAX]。
程度当中调用 rand()可以得到[0, RAND_MAX]之间的伪随机数，多次调用 rand()便可以生成一组伪随机树序列，但是这里有个问题，就是每一次运行程序所得到的随机数序列都是相同的，那如何使得每一次启动应用程序所得到的随机数序列是不一样的呢？那就通过设置不同的随机数种子，可通过 srand()设置随机数种子。

如果没有调用 srand()设置随机数种子的情况下， rand()会将 1 作为随机数种子，如果随机数种子相同，那么每一次启动应用程序所得到的随机数序列就是一样的，所以每次启动应用程序需要设置不同的随机数种子，这样就可以使得程序每次运行所得到随机数序列不同。

*/
```

**srand 函数：使用 srand()函数为 rand()设置随机数种子。**

```c
#include <stdlib.h>
void srand(unsigned int seed);
/*
seed： 指定一个随机数中， int 类型的数据，一般尝尝将当前时间作为随机数种子赋值给参数 seed，譬如 time(NULL)，因为每次启动应用程序时间上是一样的，所以就能够使得程序中设置的随机数种子在每次启动程序时是不一样的。
返回值： void
常用的用法 srand(time(NULL));
*/

//使用 rand()和 srand()产生一组伪随机数，数值范围为[0~100]，将其打印出来：
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc, char *argv[])
{
    int random_number_arr[8];
    int count;
    
    /* 设置随机数种子 */
    srand(time(NULL));
    
    /* 生成伪随机数 */
    for (count = 0; count < 8; count++)
    	random_number_arr[count] = rand() % 100;
    
    /* 打印随机数数组 */
    printf("[");
    for (count = 0; count < 8; count++) {
        printf("%d", random_number_arr[count]);
        if (count != 8 - 1)
        printf(", ");
    }
    printf("]\n");
    exit(0);
}
/*产生随机数的 API 函数并不仅仅只有这些，除此之外，譬如还有 random()、 srandom()、 initstate()、 setstate()等。*/
```

#### 休眠  

有时需要将进程暂停或休眠一段时间， 进入休眠状态之后，程序将暂停运行，直到休眠结束。 常用的系统调用和 C 库函数有 sleep()、 usleep()以及 nanosleep()， 这些函数在应用程序当中通常作为延时使用。

```c
//秒级休眠: sleep
#include <unistd.h>
unsigned int sleep(unsigned int seconds);

/***
seconds： 休眠时长，以秒为单位。
返回值： 如果休眠时长为参数 seconds 所指定的秒数，则返回 0；若被信号中断则返回剩余的秒数。
sleep()是一个秒级别休眠函数，程序在休眠过程中，是可以被其它信号所打断的，
**/

```

```c
//微秒级休眠: usleep  
#include <unistd.h>
int usleep(useconds_t usec);

/*
usec： 休眠时长，以微秒为单位。
返回值： 成功返回 0；失败返回-1，并设置 errno。
*/
```

```c
//高精度休眠: nanosleep
#include <time.h>
int nanosleep(const struct timespec *req, struct timespec *rem);

/**
req： 一个 struct timespec 结构体指针，指向一个 struct timespec 变量，用于设置休眠时间长度，可精确
到纳秒级别。
rem： 也是一个 struct timespec 结构体指针，指向一个 struct timespec 变量，也可设置 NULL。
返回值： 在成功休眠达到请求的时间间隔后， nanosleep()返回 0； 如果中途被信号中断或遇到错误，则返回-1， 并将剩余时间记录在参数 rem 指向的 struct timespec 结构体变量中（参数 rem 不为 NULL 的情况下，如果为 NULL 表示不接收剩余时间），还会设置 errno 标识错误类型。
*/
```

#### 申请堆内存  

在操作系统下，内存资源是由操作系统进行管理、分配的，**当应用程序想要内存时（这里指的是堆内存），可以向操作系统申请内存，然后使用内存；当不再需要时，将申请的内存释放、归还给操作系统；** 在许多的应用程序当中，往往都会有这种需求，譬如为一些数据结构动态分配/释放内存空间  

Linux C 程序当中一般使用 malloc()函数为程序分配一段堆内存，而使用 free()函数来释放这段内存。

```c
//**在堆上分配内存： malloc 和 free** 
#include <stdlib.h>
void *malloc(size_t size);
/*
size： 需要分配的内存大小，以字节为单位。

返回值： 返回值为 void *类型，如果申请分配内存成功，将返回一个指向该段内存的指针， void *并不是说没有返回值或者返回空指针，而是返回的指针类型未知,所以在调用 malloc()时通常需要进行强制类型转换，将 void *指针类型转换成我们希望的类型；如果分配内存失败（譬如系统堆内存不足） 将返回 NULL，如果参数 size 为 0，返回值也是 NULL。

malloc()在堆区分配一块指定大小的内存空间，用来存放数据。这块内存空间在函数执行完成后不会被
初始化，它们的值是未知的，所以通常需要程序员对 malloc()分配的堆内存进行初始化操作。
*/

//在堆上分配的内存，需要开发者自己手动释放掉，通常使用 free()函数释放堆内存.
#include <stdlib.h>
void free(void *ptr);
/*
ptr： 指向需要被释放的堆内存对应的指针。
返回值： 无返回值
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MALLOC_MEM_SIZE (1 * 1024 * 1024)
int main(int argc, char *argv[])
{
    char *base = NULL;
    /* 申请堆内存 */
    base = (char *)malloc(MALLOC_MEM_SIZE);
    if (NULL == base) {
        printf("malloc error\n");
        exit(-1);
    }
    /* 初始化申请到的堆内存 */
    memset(base, 0x0, MALLOC_MEM_SIZE);
    /* 使用内存 */
    /* ...... */
    /* 释放内存 */
    free(base);
    exit(0);
}

//调用 free()还是不调用 free() ??

/*Linux 系统中，当一个进程终止时，内核会自动关闭它没有关闭的所有文件（该进程打开的文件，但是在进程终止时未调用 close()关闭它）。同样，对于内存来说，也是如此！当进程终止时，内核会将其占用的所有内存都返还给操作系统，这包括在堆内存中由 malloc()函数所分配的内存空间。基于内存的这一自动释放机制，很多应用程序通常会省略对 free()函数的调用。

这在程序中分配了多块内存的情况下可能会特别有用，因为加入多次对 free()的调用不但会消耗品大量的 CPU 时间，而且可能会使代码趋于复杂。
**/
```

```c
//在堆上分配内存的其它方法
//用 calloc()分配内存
#include <stdlib.h>
void *calloc(size_t nmemb, size_t size);

/*
calloc()在堆中动态地分配 nmemb 个长度为 size 的连续空间，并将每一个字节都初始化为 0。所以它的结果是分配了 nmemb * size 个字节长度的内存空间，并且每个字节的值都是 0。
返回值： 分配成功返回指向该内存的地址，失败则返回 NULL。

calloc()与 malloc()的一个重要区别是： calloc()在动态分配完内存后，自动初始化该内存空间为零，而malloc()不初始化，里边数据是未知的垃圾数据。
*/

// calloc()分配内存空间并初始化
char *buf1 = (char *)calloc(10, 2);

// malloc()分配内存空间并用 memset()初始化
char *buf2 = (char *)malloc(10 * 2);
memset(buf2, 0, 20);
```

**分配对其内存**  C 函数库中还提供了一系列在堆上分配对齐内存的函数，对齐内存在某些应用场合非常有必要，常用于
分配对其内存的库函数有： posix_memalign()、 aligned_alloc()、 memalign()、 valloc()、 pvalloc()。

```c
/*
前面介绍的 malloc()、 calloc()分配内存返回的地址其实也是对齐的，但是它俩的对齐都是固定的，并且对其的字节边界比较小，譬如在 32 位系统中，通常是以 8 字节为边界进行对其，在 64 位系统中是以 16 字节进行对其。

如果想实现更大字节的对齐，则需要使用本小节介绍的函数
*/

#include <stdlib.h>
int posix_memalign(void **memptr, size_t alignment, size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void *valloc(size_t size);

#include <malloc.h>
void *memalign(size_t alignment, size_t size);
void *pvalloc(size_t size);

/*******************************************************************/

```

**posix_memalign()**函数用于在堆上分配 size 个字节大小的对齐内存空间， 将*memptr 指向分配的空间，分配的内存地址将是参数 alignment 的整数倍。 参数 alignment 表示对齐字节数， alignment 必须是 2 的幂次方（譬如 2^4、 2^5、 2^8 等），同时也要是 sizeof(void *)的整数倍，对于 32 位系统来说， sizeof(void *)等于4，如果是 64 位系统 sizeof(void *)等于 8。  

```c
#include <stdlib.h>
int posix_memalign(void **memptr, size_t alignment, size_t size);
/*
memptr： void **类型的指针，内存申请成功后会将分配的内存地址存放在*memptr 中。
alignment： 设置内存对其的字节数， alignment 必须是 2 的幂次方（譬如 2^4、 2^5、 2^8 等），同时也要是 sizeof(void *)的整数倍。
size： 设置分配的内存大小，以字节为单位，如果参数 size 等于 0，那么*memptr 中的值是 NULL。
返回值： 成功将返回 0；失败返回非 0 值。*/

#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    int *base = NULL;
    int ret;
    /* 申请内存: 256 字节对齐 */
    ret = posix_memalign((void **)&base, 256, 1024);
    if (0 != ret) {
        printf("posix_memalign error\n");
        exit(-1);
    }
    /* 使用内存 */
    // base[0] = 0;
    // base[1] = 1;
    // base[2] = 2;
    // base[3] = 3;
    /* 释放内存 */
    free(base);
    exit(0);
}
```

**aligned_alloc()函数**用于分配 size 个字节大小的内存空间， 返回指向该空间的指针。  

```c
#include <stdlib.h>
void *aligned_alloc(size_t alignment, size_t size);

/*
alignment： 用于设置对齐字节大小， alignment 必须是 2 的幂次方（譬如 2^4、 2^5、 2^8 等）。
size： 设置分配的内存大小，以字节为单位。参数 size 必须是参数 alignment 的整数倍。
返回值： 成功将返回内存空间的指针，内存空间的起始地址是参数 alignment 的整数倍；失败返回 NULL。
*/

#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    int *base = NULL;
    /* 申请内存: 256 字节对齐 */
    base = (int *)aligned_alloc(256, 256 * 4);
    if (base == NULL) {
        printf("aligned_alloc error\n");
        exit(-1);
    }
    /* 使用内存 */
    // base[0] = 0;
    // base[1] = 1;
    // base[2] = 2;
    // base[3] = 3;
    /* 释放内存 */
    free(base);
    exit(0);
}

```

**memalign()与 aligned_alloc()参数是一样的**，它们之间的区别在于：对于参数 size 必须是参数 alignment的整数倍这个限制条件， memalign()并没有这个限制条件。  **Tips： memalign()函数已经过时了，并不提倡使用！**  

```c
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
int main(int argc, char *argv[])
{
    int *base = NULL;
    /* 申请内存: 256 字节对齐 */
    base = (int *)memalign(256, 1024);
    if (base == NULL) {
        printf("memalign error\n");
        exit(-1);
    }
    /* 使用内存 */
    // base[0] = 0;
    // base[1] = 1;
    // base[2] = 2;
    // base[3] = 3;
    /* 释放内存 */
    free(base);
    exit(0);
}
```

**valloc()**分配 size 个字节大小的内存空间，返回指向该内存空间的指针， 内存空间的地址是页大小（pagesize） 的倍数。  **valloc()**与 **memalign()**类似，只不过 **valloc()**函数内部实现中，使用了页大小作为对齐的长度，在程序当中，可以通过系统调用 **getpagesize()**来获取内存的页大小。  **Tips： valloc()函数已经过时了，并不提倡使用！**  

#### proc 文件系统

**proc 文件系统是一个虚拟文件系统， 它以文件系统的方式为应用层访问系统内核数据提供了接口， 用户和应用程序可以通过 proc 文件系统得到系统信息和进程相关信息，对 proc 文件系统的读写作为与内核进行通信的一种手段**。 

但是与普通文件不同的是， proc 文件系统是动态创建的，文件本身并不存在于磁盘当中， 只存在于内存当中，与 devfs 一样，都被称为虚拟文件系统。   

proc 文件系统挂载在系统的/proc 目录下， 对于内核开发者（譬如驱动开发工程师）来说， proc 文件系统给了开发者一种调试内核的方法：**通过查看/proc/xxx 文件来获取到内核特定数据结构的值，在添加了新功能前后进行对比，就可以判断此功能所产生的影响是否合理。**  

**/proc 目录下中包含了一些目录和虚拟文件**  

/proc 目录下除了文件夹之外，还有很多的虚拟文件，譬如 buddyinfo、 cgroups、 cmdline、 version 等等，不同的文件记录了不同信息， 关于这些文件记录的信息和意思如下：  

cmdline： 内核启动参数；
cpuinfo： CPU 相关信息；
iomem： IO 设备的内存使用情况；
interrupts：显示被占用的中断号和占用者相关的信息；
ioports： IO 端口的使用情况；
kcore：系统物理内存映像，不可读取；
loadavg：系统平均负载；
meminfo：物理内存和交换分区使用情况；
modules：加载的模块列表；
mounts：挂载的文件系统列表；
partitions：系统识别的分区表；
swaps：交换分区的利用情况；
version：内核版本信息；
uptime：系统运行时间；  

```c
//使用 read()函数读取
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
    char buf[512] = {0};
    int fd;
    int ret;
    /* 打开文件 */
    fd = open("/proc/version", O_RDONLY);
    if (-1 == fd) {
        perror("open error");
        exit(-1);
    }
    /* 读取文件 */
    ret = read(fd, buf, sizeof(buf));
    if (-1 == ret) {
        perror("read error");
        exit(-1);
    }
    /* 打印信息 */
    puts(buf);
    /* 关闭文件 */
    close(fd);
    exit(0);
}
```

### 1.6、信号

#### 基本概念  

信号是事件发生时对进程的通知机制，也可以把它称为**软件中断**。**信号与硬件中断的相似之处在于能够打断程序当前执行的正常流程， 其实是在软件层次上对中断机制的一种模拟。** 大多数情况下，是无法预测信号达到的准确时间，所以，信号提供了一种处理异步事件的方法。  

##### 信号的目的是用来通信的

一个具有合适权限的进程能够向另一个进程发送信号，信号的这一用法可作为一种同步技术，甚至是进程间通信（IPC）的原始形式。 信号可以由“谁”发出呢？ 以下列举的很多情况均可以产生信号：  

⚫ 硬件发生异常，即硬件检测到错误条件并通知内核，随即再由内核发送相应的信号给相关进程。硬件检测到异常的例子包括执行一条异常的机器语言指令，诸如，除数为 0、数组访问越界导致引用了无法访问的内存区域等，这些异常情况都会被硬件检测到，并通知内核、然后内核为该异常情况发生时正在运行的进程发送适当的信号以通知进程。
⚫ 用于在终端下输入了能够产生信号的特殊字符。 **譬如在终端上按下 CTRL + C 组合按键可以产生中断信号(SIGINT)，通过这个方法可以终止在前台运行的进程；按下 CTRL + Z 组合按键可以产生暂停信号（SIGCONT），通过这个方法可以暂停当前前台运行的进程。**
⚫ **进程调用 kill()系统调用可将任意信号发送给另一个进程或进程组。** **当然对此是有所限制的，接收信号的进程和发送信号的进程的所有者必须相同，亦或者发送信号的进程的所有者是 root 超级用户。**
⚫ 用户可以通过 **kill 命令将信号发送给其它进程**。 kill 命令想必大家都会使用，通常我们会通过 kill命令来“杀死”（终止）一个进程，**譬如在终端下执行"kill -9 xxx"来杀死 PID 为 xxx 的进程**。 **kill命令其内部的实现原理便是通过 kill()系统调用来完成的。**
⚫ 发生了**软件事件**，即当检测到某种软件条件已经发生。 这里指的不是硬件产生的条件（如除数为 0、引用无法访问的内存区域等），而是软件的触发条件、触发了某种软件条件（**进程所设置的定时器已经超时、进程执行的 CPU 时间超限、进程的某个子进程退出**等等情况）。  

进程同样也可以向自身发送信号，然而发送给进程的诸多信号中，大多数都是来自于内核。总的来看，信号的目的都是用于通信的，当发生某种情况下，通过信号将情况“告知”相应的进程，从而达到同步、通信的目的。  

##### 信号由谁处理、怎么处理  

信号通常是发送给对应的进程，当信号到达后， 该进程需要做出相应的处理措施，通常进程会视具体信号执行以下操作之一：  

⚫ 忽略信号。也就是说，当信号到达进程后，该进程并不会去理会它、直接忽略，就好像是没有出该信号，信号对该进程不会产生任何影响。事实上，大多数信号都可以使用这种方式进行处理，但有两种信号却决不能被忽略，它们是 SIGKILL 和 SIGSTOP，这两种信号不能被忽略的原因是：它们向内核和超级用户提供了使进程终止或停止的可靠方法。另外，如果忽略某些由硬件异常产生的信号，则进程的运行行为是未定义的。
⚫ 捕获信号。 当信号到达进程后，执行预先绑定好的信号处理函数。为了做到这一点，要通知内核在某种信号发生时，执行用户自定义的处理函数，该处理函数中将会对该信号事件作出相应的处理，**Linux 系统提供了 signal()系统调用可用于注册信号的处理函数**，将会在后面向大家介绍。
⚫ 执行系统默认操作。 进程不对该信号事件作出处理，而是交由系统进行处理，每一种信号都会有其对应的系统默认的处理方式， 8.3 小节中对此有进行介绍。需要注意的是，**对大多数信号来说，系默认的处理方式就是终止该进程。**

##### 信号是异步的    

信号是异步事件的经典实例，产生信号的事件对进程而言是随机出现的，进程无法预测该事件产生的准确时间，进程不能够通过简单地测试一个变量或使用系统调用来判断是否产生了一个信号，这就如同硬件中断事件，程序是无法得知中断事件产生的具体时间，只有当产生中断事件时，才会告知程序、然后打断当前程序的正常执行流程、跳转去执行中断服务函数，这就是异步处理方式。  

##### 信号本质上是 int 类型数字编号  

信号本质上是 int 类型的数字编号，这就好比硬件中断所对应的中断号。内核针对每个信号，都给其定义了一个唯一的整数编号，从数字 1 开始顺序展开。并且每一个信号都有其对应的名字（其实就是一个宏），信号名字与信号编号乃是一一对应关系，但是由于每个信号的实际编号随着系统的不同可能会不一样，所以在程序当中一般都使用信号的符号名（也就是宏定义）。  

#include <signum.h>头文件中定义  

```c
/* Signals. */
#define SIGHUP 1 /* Hangup (POSIX). */
#define SIGINT 2 /* Interrupt (ANSI). */
#define SIGQUIT 3 /* Quit (POSIX). */
#define SIGILL 4 /* Illegal instruction (ANSI). */
#define SIGTRAP 5 /* Trace trap (POSIX). */
#define SIGABRT 6 /* Abort (ANSI). */
#define SIGIOT 6 /* IOT trap (4.2 BSD). */
#define SIGBUS 7 /* BUS error (4.2 BSD). */
#define SIGFPE 8 /* Floating-point exception (ANSI). */
#define SIGKILL 9 /* Kill, unblockable (POSIX). */
#define SIGUSR1 10 /* User-defined signal 1 (POSIX). */
#define SIGSEGV 11 /* Segmentation violation (ANSI). */
#define SIGUSR2 12 /* User-defined signal 2 (POSIX). */
#define SIGPIPE 13 /* Broken pipe (POSIX). */
#define SIGALRM 14 /* Alarm clock (POSIX). */
#define SIGTERM 15 /* Termination (ANSI). */
#define SIGSTKFLT 16 /* Stack fault. */
#define SIGCLD SIGCHLD /* Same as SIGCHLD (System V). */
#define SIGCHLD 17 /* Child status has changed (POSIX). */
#define SIGCONT 18 /* Continue (POSIX). */
#define SIGSTOP 19 /* Stop, unblockable (POSIX). */
#define SIGTSTP 20 /* Keyboard stop (POSIX). */
#define SIGTTIN 21 /* Background read from tty (POSIX). */
#define SIGTTOU 22 /* Background write to tty (POSIX). */
#define SIGURG 23 /* Urgent condition on socket (4.2 BSD). */
#define SIGXCPU 24 /* CPU limit exceeded (4.2 BSD). */
#define SIGXFSZ 25 /* File size limit exceeded (4.2 BSD). */
#define SIGVTALRM 26 /* Virtual alarm clock (4.2 BSD). */
#define SIGPROF 27 /* Profiling alarm clock (4.2 BSD). */
#define SIGWINCH 28 /* Window size change (4.3 BSD, Sun). */
#define SIGPOLL SIGIO /* Pollable event occurred (System V). */
#define SIGIO 29 /* I/O now possible (4.2 BSD). */
#define SIGPWR 1 /* Power failure restart (System V). */
#define SIGSYS 31 /* Bad system call. */
#define SIGUNUSED 31
```

#### 信号的分类  

Linux 系统下可对信号从两个不同的角度进行分类，从可靠性方面将信号分为可靠信号与不可靠信号；而从实时性方面将信号分为实时信号与非实时信号。

##### 可靠信号与不可靠信号

Linux 信号机制基本上是从 UNIX 系统中继承过来的， 早期 UNIX 系统中的信号机制比较简单和原始，后来在实践中暴露出一些问题，它的主要问题是：  

1、进程每次处理信号后，就将对信号的响应设置为系统默认操作。在某些情况下，将导致对信号的错误处理；因此，用户如果不希望这样的操作，那么就要在信号处理函数结尾再一次调用 signal()，重新为该信号绑定相应的处理函数。

2、  因此导致， 早期 UNIX 下的不可靠信号主要指的是进程可能对信号做出错误的反应以及信号可能丢失（处理信号时又来了新的信号，则导致信号丢失）。  

Linux 支持不可靠信号，但是对不可靠信号机制做了改进： 在调用完信号处理函数后，不必重新调用signal()。因此， Linux 下的不可靠信号问题主要指的是信号可能丢失。在 Linux 系统下，信号值小于 SIGRTMIN（34） 的信号都是不可靠信号， 这就是"不可靠信号"的来源。

**可靠信号支持排队，不会丢失， 同时，信号的发送和绑定也出现了新版本， 信号发送函数 sigqueue()及信号绑定函数 sigaction()。**  

##### 实时信号与非实时信号  

实时信号与非实时信号其实是从时间关系上进行的分类，与可靠信号与不可靠信号是相互对应的， 非实时信号都不支持排队，都是不可靠信号；  实时信号都支持排队，都是可靠信号。 实时信号保证了发送的多个信号都能被接收， 实时信号是 POSIX 标准的一部分，可用于应用进程。  

一般我们也把非实时信号（不可靠信号）称为标准信号，如果文档中用到了这个词，那么大家要知道，这里指的就是非实时信号（不可靠信号）。  

#### 常见信号与默认行为  

看文档吧！

#### 进程对信号的处理  

当进程接收到内核或用户发送过来的信号之后，根据具体信号可以采取不同的处理方式：忽略信号、捕获信号或者执行系统默认操作。 **Linux 系统提供了系统调用 signal()和 sigaction()两个函数用于设置信号的处理方式。**

##### signal()函数

```c
#include <signal.h>
typedef void (*sig_t)(int);
sig_t signal(int signum, sig_t handler);

/*
signum： 此参数指定需要进行设置的信号，可使用信号名（宏）或信号的数字编号，建议使用信号名。
handler： sig_t 类型的函数指针，指向信号对应的信号处理函数，当进程接收到信号后会自动执行该处理函数；参数 handler 既可以设置为用户自定义的函数，也就是捕获信号时需要执行的处理函数，也可以设置为 SIG_IGN 或 SIG_DFL， SIG_IGN 表示此进程需要忽略该信号， SIG_DFL 则表示设置为系统默认操作。
sig_t 函数指针的 int 类型参数指的是，当前触发该函数的信号，可将多个信号绑定到同一个信号处理函数上，此时就可通过此参数来判断当前触发的是哪个信号。

Tips： SIG_IGN、 SIG_DFL 分别取值如下：
/* Fake signal functions. */
#define SIG_ERR ((sig_t) -1) /* Error return. */
#define SIG_DFL ((sig_t) 0) /* Default action. */
#define SIG_IGN ((sig_t) 1) /* Ignore signal. */
/*
返回值： 此函数的返回值也是一个 sig_t 类型的函数指针，成功情况下的返回值则是指向在此之前的信号处理函数；如果出错则返回 SIG_ERR，并会设置 errno。*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
static void sig_handler(int sig)
{
	printf("Received signal: %d\n", sig);
}
int main(int argc, char *argv[])
{
    sig_t ret = NULL;
    ret = signal(SIGINT, (sig_t)sig_handler);
    if (SIG_ERR == ret) {
        perror("signal error");
    	exit(-1);
    }
    /* 死循环 */
    for ( ; ; ) { }
    exit(0);
}

/*
当运行程序之后，程序会占用终端称为一个前台进程，此时按下中断符便会打印出信息（^C 表示按下了中断符）。平时大家使用 CTRL + C 可以终止一个进程，而这里却不能通过这种方式来终止这个测试程序，原因在于测试程序中捕获了该信号，而对应的处理方式仅仅只是打印一条语句、而并不终止进程。
*/

/*
那此时该怎么关闭这个测试程序呢？前面给大家介绍了“一击必杀”信号 SIGKILL（编号为 9），可向该进程发送 SIGKILL 暴力终止该进程，当然一般不推荐大家这样使用，如果实在没办法才采取这种措施。
*/
shell: ps -aux | grep testApp | grep -v grep
skating       85 99.7  0.0   2776  1032 pts/0    R+   14:59   0:22 ./testApp
shell: kill -9 85
log:
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./testApp
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
^CReceived signal: 2
已杀死  ---->用了kill命令

/*Tips：普通用户只能杀死该用户自己的进程，无权限杀死其它用户的进程。*/
/*
我们再执行一次测试程序，这里将测试程序放在后台运行，然后再按下中断符：
*/
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./testApp &
[1] 100
/*
按下中断符发现进程并没有收到 SIGINT 信号，原因很简单，因为进程并不是前台进程、而是一个后台进程，按下中断符时系统并不会给后台进程发送 SIGINT 信号。 可以使用 kill 命令手动发送信号给我们的进程：*/
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ps
    PID TTY          TIME CMD
     15 pts/0    00:00:00 bash
    100 pts/0    00:01:03 testApp
    101 pts/0    00:00:00 ps
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ kill -2 100		//这里的2 是SIGINT 键盘的ctrl+c
Received signal: 2
/******************/
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ kill -9 100
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ps
    PID TTY          TIME CMD
     15 pts/0    00:00:00 bash
    102 pts/0    00:00:00 ps
[1]+  已杀死               ./testApp
        
        
/*********************注册多个信号***********************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// 定义信号处理函数
void handle_signal(int signum) {
    printf("Caught signal %d\n", signum);

    // 根据不同的信号执行不同的操作
    switch (signum) {
        case SIGINT:
            printf("SIGINT received. Exiting...\n");
            exit(0);
            break;
        case SIGQUIT:
            printf("SIGQUIT received. Generating core dump...\n");
            abort();  // 生成核心转储
            break;
    }
}

int main() {
    // 注册信号处理函数
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);

    printf("Program is running. Press Ctrl+C or Ctrl+\\ to send signals.\n");

    // 模拟长时间运行的程序
    while (1) {
        sleep(1);
    }

    return 0;
}
```

##### 两种不同状态下信号的处理方式  

如果程序中没有调用 signal()函数为信号设置相应的处理方式，亦或者程序刚启动起来并未运行到 signal()处，那么这时进程接收到一个信号后是如何处理的呢？  

⚫ 程序启动
当一个应用程序刚启动的时候（或者程序中没有调用 signal()函数） ， 通常情况下， 进程对所有信号的处理方式都设置为系统默认操作。所以如果在我们的程序当中，没有调用 signal()为信号设置处理方式，则默认的处理方式便是系统默认操作。
所以为什么大家平时都可以使用 CTRL + C 中断符来终止一个进程，因为大部分情况下，应用程序中并不会为 SIGINT 信号设置处理方式，所以该信号的处理方式便是系统默认操作，当接收到信号之后便执行系统默认操作，而 SIGINT 信号的系统默认操作便是终止进程。
⚫ 进程创建
当一个进程调用 fork()创建子进程时，其子进程将会继承父进程的信号处理方式，因为子进程在开始时复制了父进程的内存映像，所以信号捕获函数的地址在子进程中是有意义的。  

##### sigaction()函数

除了signal()之外， sigaction()系统调用是设置信号处理方式的另一选择， 事实上，推荐大家使用sigaction()函数。虽然 signal()函数简单好用，而 sigaction()更为复杂，但作为回报， sigaction()也更具灵活性以及移植性。  

**sigaction()**允许单独获取信号的处理函数而不是设置，并且还可以设置各种属性对调用信号处理函数时的行为施以更加精准的控制。

```c
#include <signal.h>
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
/*
signum： 需要设置的信号，除了 SIGKILL 信号和 SIGSTOP 信号之外的任何信号。
act： act 参数是一个 struct sigaction 类型指针，指向一个 struct sigaction 数据结构，该数据结构描述了信
号的处理方式，稍后介绍该数据结构；如果参数 act 不为 NULL，则表示需要为信号设置新的处理方式；如果参数 act 为 NULL，则表示无需改变信号当前的处理方式。
oldact： oldact 参数也是一个 struct sigaction 类型指针，指向一个 struct sigaction 数据结构。如果参数oldact 不为 NULL， 则会将信号之前的处理方式等信息通过参数 oldact 返回出来；如果无意获取此类信息，那么可将该参数设置为 NULL。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
*/
//struct sigaction 结构体
struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);/*过时不用了*/
};
/*
⚫ sa_handler：指定信号处理函数，与 signal()函数的 handler 参数相同。
⚫ sa_sigaction：也用于指定信号处理函数，这是一个替代的信号处理函数，他提供了更多的参数，可以通过该函数获取到更多信息，这些信号通过 siginfo_t 参数获取，稍后介绍该数据结构； 
sa_handler 和sa_sigaction 是互斥的，不能同时设置， 对于标准信号来说， 使用 sa_handler 就可以了，可通过SA_SIGINFO 标志进行选择。
⚫ sa_mask： 参数 sa_mask 定义了一组信号， 当进程在执行由 sa_handler 所定义的信号处理函数之前，会先将这组信号添加到进程的信号掩码字段中，当进程执行完处理函数之后再恢复信号掩码，将这组信号从信号掩码字段中删除。 当进程在执行信号处理函数期间， 可能又收到了同样的信号或其它信号，从而打断当前信号处理函数的执行，这就好点像中断嵌套；通常我们在执行信号处理函数期间不希望被另一个信号所打断，那么怎么做呢？那么就是通过信号掩码来实现， 如果进程接收到了信号掩码中的这些信号，那么这个信号将会被阻塞暂时不能得到处理，直到这些信号从进程的信号掩码中移除。 在信号处理函数调用时，进程会自动将当前处理的信号添加到信号掩码字段中，这样保证了在处理一个给定的信号时，如果此信号再次发生，那么它将会被阻塞。如果用户还需要在阻塞其它的信号，则可以通过设置参数 sa_mask 来完成（此参数是 sigset_t 类型变量，关于该类型的介绍信息请看 8.6.1 小节内容，关于信号掩码还会在 8.7.1 小节中进一步介绍） ，信号掩码可以避免一些信号之间的竞争状态（也称为竞态）。
⚫ sa_restorer：该成员已过时，不要再使用了。
⚫ sa_flags： 参数 sa_flags 指定了一组标志，这些标志用于控制信号的处理过程，可设置为如下这些标志（多个标志使用位或" | "组合）：
SA_NOCLDSTOP
如果signum为SIGCHLD， 则子进程停止时（即当它们接收到SIGSTOP、SIGTSTP、SIGTTIN 或SIGTTOU中的一种时）或恢复（即它们接收到 SIGCONT）时不会收到 SIGCHLD 信号。
SA_NOCLDWAIT
如果 signum 是 SIGCHLD，则在子进程终止时不要将其转变为僵尸进程。
SA_NODEFER
不要阻塞从某个信号自身的信号处理函数中接收此信号。 也就是说当进程此时正在执行某个信号的处理函数，默认情况下，进程会自动将该信号添加到进程的信号掩码字段中，从而在执行信号处理函数期间阻塞该信号， 默认情况下，我们期望进程在处理一个信号时阻塞同种信号，否则引起一些竞态条件；如果设置了 SA_NODEFER 标志，则表示不对它进行阻塞。
SA_RESETHAND
执行完信号处理函数之后，将信号的处理方式设置为系统默认操作。
SA_RESTART
被信号中断的系统调用，在信号处理完成之后将自动重新发起。
SA_SIGINFO
如果设置了该标志，则表示使用 sa_sigaction 作为信号处理函数、而不是 sa_handler，关于 sa_sigaction信号处理函数的参数信息。
*/
siginfo_t {
    int si_signo; /* Signal number */
    int si_errno; /* An errno value */
    int si_code; /* Signal code */
    int si_trapno; /* Trap number that caused hardware-generated signal(unused on most    architectures) */
    pid_t si_pid; /* Sending process ID */
    uid_t si_uid; /* Real user ID of sending process */
    int si_status; /* Exit value or signal */
    clock_t si_utime; /* User time consumed */
    clock_t si_stime; /* System time consumed */
    sigval_t si_value; /* Signal value */
    int si_int; /* POSIX.1b signal */
    void *si_ptr; /* POSIX.1b signal */
    int si_overrun; /* Timer overrun count; POSIX.1b timers */
    int si_timerid; /* Timer ID; POSIX.1b timers */
    void *si_addr; /* Memory location which caused fault */
    long si_band; /* Band event (was int in glibc 2.3.2 and earlier) */
    int si_fd; /* File descriptor */
    short si_addr_lsb; /* Least significant bit of address(since Linux 2.6.32) */
    void *si_call_addr; /* Address of system call instruction(since Linux 3.5) */
    int si_syscall; /* Number of attempted system call(since Linux 3.5) */
    unsigned int si_arch; /* Architecture of attempted system call(since Linux 3.5) */
}

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
static void sig_handler(int sig)
{
	printf("Received signal: %d\n", sig);
}
int main(int argc, char *argv[])
{
    struct sigaction sig = {0};
    int ret;
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    ret = sigaction(SIGINT, &sig, NULL);
    if (-1 == ret) {
        perror("sigaction error");
        exit(-1);
    }
    /* 死循环 */
    for ( ; ; ) { }
    exit(0);
}

/***
关于信号处理函数说明
一般而言，将信号处理函数设计越简单越好，这就好比中断处理函数，越快越好，不要在处理函数中做大量消耗 CPU 时间的事情，这一个重要的原因在于，设计的越简单这将降低引发信号竞争条件的风险。
*/

/*****************************************/
//紧接着说下 sigset_t sa_mask;//信号掩码如何使用，其实这个是设计者来设置的，如下例子：
//展示了如何使用 sa_mask 来阻塞 SIGUSR1 信号，直到 SIGINT 信号处理函数执行完毕：
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("Caught signal %d\n", sig);
    sleep(5); // 模拟长时间处理
    printf("Signal %d handling finished\n", sig);
}

int main() {
    struct sigaction act;
    sigset_t mask;

    // 初始化信号集
    sigemptyset(&mask);
    // 添加 SIGUSR1 到信号集中
    sigaddset(&mask, SIGUSR1);

    // 设置信号处理函数
    act.sa_handler = handler;
    // 设置信号掩码
    act.sa_mask = mask;
    // 设置标志位
    act.sa_flags = 0;

    // 应用信号处理设置
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (1) {
        pause(); // 等待信号
    }

    return 0;
}
```

#### 向进程发送信号  

与 kill 命令相类似， Linux 系统提供了 kill()系统调用，一个进程可通过 kill()向另一个进程发送信号；
除了 kill()系统调用之外， Linux 系统还提供了系统调用 **killpg()**以及库函数 **raise()**，也可用于实现发送信号的功能，  

##### kill()函数  

kill()系统调用可将信号发送给指定的进程或进程组中的每一个进程；

```c
#include <sys/types.h>
#include <signal.h>
int kill(pid_t pid, int sig);

/*
pid： 参数 pid 为正数的情况下，用于指定接收此信号的进程 pid；除此之外，参数 pid 也可设置为 0 或-1 以及小于-1 等不同值，稍后给说明。
sig： 参数 sig 指定需要发送的信号，也可设置为 0，如果参数 sig 设置为 0 则表示不发送信号，但任执行错误检查，这通常可用于检查参数 pid 指定的进程是否存在。
返回值： 成功返回 0；失败将返回-1，并设置 errno。

参数 pid 不同取值含义：
⚫ 如果 pid 为正，则信号 sig 将发送到 pid 指定的进程。
⚫ 如果 pid 等于 0，则将 sig 发送到当前进程的进程组中的每个进程。
⚫ 如果 pid 等于-1，则将 sig 发送到当前进程有权发送信号的每个进程，但进程 1（init）除外。
⚫ 如果 pid 小于-1，则将 sig 发送到 ID 为-pid 的进程组中的每个进程

进程中将信号发送给另一个进程是需要权限的，并不是可以随便给任何一个进程发送信号，超级用户root 进程可以将信号发送给任何进程，但对于非超级用户（普通用户）进程来说，其基本规则是发送者进程的实际用户 ID 或有效用户 ID 必须等于接收者进程的实际用户 ID 或有效用户 ID。

从上面介绍可知，当 sig 为 0 时，仍可进行正常执行的错误检查，但不会发送信号，这通常可用于确定一个特定的进程是否存在，如果向一个不存在的进程发送信号， kill()将会返回-1， errno 将被设置为 ESRCH，表示进程不存在。
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    int pid;
    /* 判断传参个数 */
    if (2 > argc)
    	exit(-1);
    /* 将传入的字符串转为整形数字 */
    pid = atoi(argv[1]);
    printf("pid: %d\n", pid);
    /* 向 pid 指定的进程发送信号 */
    if (-1 == kill(pid, SIGINT)) {
    	perror("kill error");
    	exit(-1);
    }
    exit(0);
}

/*
Terminal 1：
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./testApp &
[1] 103
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ps
    PID TTY          TIME CMD
     15 pts/0    00:00:00 bash
    103 pts/0    00:00:02 testApp
    104 pts/0    00:00:00 ps
    
Terminal 2：    
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./sendsem 103
pid: 103
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./sendsem 103
pid: 103
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./sendsem 103
pid: 103
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./sendsem 103
pid: 103

Terminal 1：
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ Received signal: 2
Received signal: 2
Received signal: 2
Received signal: 2

如果已经杀死PID:103进程，那么会如下提示：
Terminal 1：
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ kill -9 103
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ls
sendsem  sendsem.c  testApp  testApp.c
[1]+  已杀死               ./testApp

Terminal 2：
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./sendsem 103
pid: 103
kill error: No such process
*/
```

##### raise()  

**有时进程需要向自身发送信号， raise()函数可用于实现这一要求**， raise()函数原型如下所示（此函数为 C库函数） ：  

```c
#include <signal.h>
int raise(int sig);
/*
函数参数和返回值含义如下：
sig： 需要发送的信号。
返回值： 成功返回 0；失败将返回非零值。

raise()其实等价于： kill(getpid(), sig);
Tips： getpid()函数用于获取进程自身的 pid。
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
static void sig_handler(int sig)
{
	printf("Received signal: %d\n", sig);
}
int main(int argc, char *argv[])
{
    struct sigaction sig = {0};
    int ret;
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    ret = sigaction(SIGINT, &sig, NULL);
    if (-1 == ret) {
    	perror("sigaction error");
    	exit(-1);
    }
    for ( ; ; ) {
    /* 向自身发送 SIGINT 信号 */
    	if (0 != raise(SIGINT)) {
    		printf("raise error\n");
    		exit(-1);
    	}
    	sleep(3); // 每隔 3 秒发送一次
    }
    exit(0);
}
```

#### alarm()和 pause()函数  

##### alarm()函数  

使用 alarm()函数可以设置一个定时器（闹钟） ，当定时器定时时间到时，内核会向进程发送 SIGALRM信号 。

```c
#include <unistd.h>
unsigned int alarm(unsigned int seconds);

/*
seconds： 设置定时时间，以秒为单位；如果参数 seconds 等于 0，则表示取消之前设置的 alarm 闹钟。
返回值： 如果在调用 alarm()时，之前已经为该进程设置了 alarm 闹钟还没有超时，则该闹钟的剩余值作为本次 alarm()函数调用的返回值，之前设置的闹钟则被新的替代；否则返回 0。
参数 seconds 的值是产生 SIGALRM 信号需要经过的时钟秒数，当这一刻到达时，由内核产生该信号，每个进程只能设置一个 alarm 闹钟； 虽然 SIGALRM 信号的系统默认操作是终止进程，但是如果程序当中设置了 alarm 闹钟，但大多数使用闹钟的进程都会捕获此信号。
需要注意的是 alarm 闹钟并不能循环触发，只能触发一次，若想要实现循环触发，可以在 SIGALRM 信号处理函数中再次调用 alarm()函数设置定时器。
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
static void sig_handler(int sig)
{
    puts("Alarm timeout");
    exit(0);
}
int main(int argc, char *argv[])
{
    struct sigaction sig = {0};
    int second;
    /* 检验传参个数 */
    if (2 > argc)
    	exit(-1);
    
    /* 为 SIGALRM 信号绑定处理函数 */
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    if (-1 == sigaction(SIGALRM, &sig, NULL)) {
        perror("sigaction error");
    	exit(-1);
    }
    
    /* 启动 alarm 定时器 */
    second = atoi(argv[1]);
    printf("定时时长: %d 秒\n", second);
    alarm(second);
    /* 循环 */
    for ( ; ; )
    	sleep(1);
    exit(0);
}
```

##### pause()函数  

**pause()**系统调用可以使得进程暂停运行、进入休眠状态，直到进程捕获到一个信号为止，只有执行了信号处理函数并从其返回时， pause()才返回，在这种情况下， pause()返回-1，并且将 errno 设置为 EINTR。  

```c
#include <unistd.h>
int pause(void);

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
static void sig_handler(int sig)
{
    puts("Alarm timeout");
}
int main(int argc, char *argv[])
{
    struct sigaction sig = {0};
    int second;	
    /* 检验传参个数 */
    if (2 > argc)
    	exit(-1);
    
    /* 为 SIGALRM 信号绑定处理函数 */
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    if (-1 == sigaction(SIGALRM, &sig, NULL)) {
    	perror("sigaction error");
    	exit(-1);
    }
    
    /* 启动 alarm 定时器 */
    second = atoi(argv[1]);
    printf("定时时长: %d 秒\n", second);
    alarm(second);
    
    /* 进入休眠状态 */
    pause();
    puts("休眠结束");
    exit(0);
}

/*
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ ./testfun 5
定时时长: 5 秒
Alarm timeout
休眠结束
*/
```

#### 信号集  

通常我们需要有一个能表示多个信号（一组信号）的数据类型---信号集（signal set），很多系统调用都使用到了信号集这种数据类型来作为参数传递，**譬如 sigaction()函数、 sigprocmask()函数、 sigpending()函数  这些函数本小节没说明，但都在后面另外开几节说明了**等。  

```c
#define _SIGSET_NWORDS (1024 / (8 * sizeof (unsigned long int)))
typedef struct
{
	unsigned long int __val[_SIGSET_NWORDS];
} sigset_t;
/*使用这个结构体可以表示一组信号，将多个信号添加到该数据结构中， 当然 Linux 系统了用于操作sigset_t 信号集的API，譬如 sigemptyset()、 sigfillset()、 sigaddset()、 sigdelset()、 sigismember()。*/
```

##### 初始化信号集  

**sigemptyset()**和 **sigfillset()**用于初始化信号集。 **sigemptyset()**初始化信号集，使其不包含任何信号；而**sigfillset()**函数初始化信号集，使其包含所有信号（包括所有实时信号）。

```c
#include <signal.h>
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
/*
set： 指向需要进行初始化的信号集变量。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
*/

/**** 初始化为空信号集：*****/
sigset_t sig_set;
sigemptyset(&sig_set);

/**** 初始化信号集，使其包含所有信号：*****/
sigset_t sig_set;
sigfillset(&sig_set);
```

##### 向信号集中添加/删除信号  

分别使用 sigaddset()和 sigdelset()函数向信号集中添加或移除一个信号。

```c
#include <signal.h>
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);

/*
set： 指向信号集。
signum： 需要添加/删除的信号。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
*/

/***** 向信号集中添加信号 ****/
sigset_t sig_set;
sigemptyset(&sig_set);
sigaddset(&sig_set, SIGINT);

/*****从信号集中移除信号*******/
sigset_t sig_set;
sigfillset(&sig_set);
sigdelset(&sig_set, SIGINT);
```

##### 测试信号是否在信号集中  

```c
#include <signal.h>
int sigismember(const sigset_t *set, int signum);

/*
set： 指定信号集。
signum： 需要进行测试的信号。
返回值： 如果信号 signum 在信号集 set 中，则返回 1；如果不在信号集 set 中，则返回 0；失败则返回-1，并设置 errno。
*/

//示例
sigset_t sig_set;
......
if (1 == sigismember(&sig_set, SIGINT))
	puts("信号集中包含 SIGINT 信号");
else if (!sigismember(&sig_set, SIGINT))
	puts("信号集中不包含 SIGINT 信号");
```

#### 获取信号的描述信息  

在 Linux 下，每个信号都有一串与之相对应的字符串描述信息，用于对该信号进行相应的描述。  这些字符串位于 sys_siglist 数组中， sys_siglist 数组是一个 char *类型的数组，数组中的每一个元素存放的是一个字符串指针，指向一个信号描述信息。  sys_siglist[SIGINT]来获取对 SIGINT 信号的描述。

```c
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
    printf("SIGINT 描述信息: %s\n", sys_siglist[SIGINT]);
    printf("SIGQUIT 描述信息: %s\n", sys_siglist[SIGQUIT]);
    printf("SIGBUS 描述信息: %s\n", sys_siglist[SIGBUS]);
    exit(0);
}
```

##### strsignal()函数  

除了直接使用 sys_siglist 数组获取描述信息之外，还可以使用 strsignal()函数。较之于直接引用 sys_siglist数组，更推荐使用 strsignal()函数。

```c
#include <string.h>
char *strsignal(int sig);
/*调用 strsignal()函数将会获取到参数 sig 指定的信号对应的描述信息，返回该描述信息字符串的指针；
函数会对参数 sig 进行检查，若传入的 sig 无效，则会返回"Unknown signal"信息。*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void)
{
	printf("SIGINT 描述信息: %s\n", strsignal(SIGINT));
    printf("SIGQUIT 描述信息: %s\n", strsignal(SIGQUIT));
    printf("SIGBUS 描述信息: %s\n", strsignal(SIGBUS));
    printf("编号为 1000 的描述信息: %s\n", strsignal(1000));
    exit(0);
}
```

##### psignal()函数  

psignal()可以在标准错误（stderr）上输出信号描述信息。 

```c
#include <signal.h>
void psignal(int sig, const char *s);
/*
调用 psignal()函数会将参数 sig 指定的信号对应的描述信息输出到标准错误，并且还允许调用者添加一些输出信息，由参数 s 指定；所以整个输出信息由字符串 s、冒号、空格、描述信号编号 sig 的字符串和尾随的换行符组成。
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void)
{
    psignal(SIGINT, "SIGINT 信号描述信息");
    psignal(SIGQUIT, "SIGQUIT 信号描述信息");
    psignal(SIGBUS, "SIGBUS 信号描述信息");
    exit(0);
}
```

#### 信号掩码(阻塞信号传递)  

内核为每一个进程维护了一个信号掩码（其实就是一个信号集） ，即一组信号。当进程接收到一个属于信号掩码中定义的信号时，该信号将会被阻塞、无法传递给进程进行处理， 那么内核会将其阻塞，直到该信号从信号掩码中移除，内核才会把该信号传递给进程从而得到处理。

向信号掩码中添加一个信号，通常有如下几种方式：
⚫ 当应用程序调用 signal()或 sigaction()函数为某一个信号设置处理方式时，进程会自动将该信号添加到信号掩码中， 这样保证了在处理一个给定的信号时，如果此信号再次发生，那么它将会被阻塞；
当然对于 sigaction()而言，是否会如此，需要根据 sigaction()函数是否设置了 SA_NODEFER 标志而定；当信号处理函数结束返回后，会自动将该信号从信号掩码中移除。
⚫ 使用 sigaction()函数为信号设置处理方式时，可以额外指定一组信号，当调用信号处理函数时将该组信号自动添加到信号掩码中， 当信号处理函数结束返回后，再将这组信号从信号掩码中移除； 通过 sa_mask 参数进行设置，参考 8.4.2 小节内容。
⚫ 除了以上两种方式之外，还可以使用 sigprocmask()系统调用，随时可以显式地向信号掩码中添加或移除信号。  

```c
#include <signal.h>
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);

/*
how： 参数 how 指定了调用函数时的一些行为。
set： 将参数 set 指向的信号集内的所有信号添加到信号掩码中或者从信号掩码中移除；如果参数 set 为NULL，则表示无需对当前信号掩码作出改动。
oldset： 如果参数 oldset 不为 NULL，在向信号掩码中添加新的信号之前，获取到进程当前的信号掩码，存放在 oldset 所指定的信号集中；如果为 NULL 则表示不获取当前的信号掩码。
返回值： 成功返回 0；失败将返回-1，并设置 errno。

参数 how 可以设置为以下宏：
⚫ SIG_BLOCK：将参数 set 所指向的信号集内的所有信号添加到进程的信号掩码中。换言之，将信号掩码设置为当前值与 set 的并集。
⚫ SIG_UNBLOCK：将参数 set 指向的信号集内的所有信号从进程信号掩码中移除。
⚫ SIG_SETMASK：进程信号掩码直接设置为参数 set 指向的信号集。
*/


/**********************************************************/
/* 将信号 SIGINT 添加到进程的信号掩码中： */
int ret;
/* 定义信号集 */
sigset_t sig_set;
/* 将信号集初始化为空 */
sigemptyset(&sig_set);
/* 向信号集中添加 SIGINT 信号 */
sigaddset(&sig_set, SIGINT);
/* 向进程的信号掩码中添加信号 */
ret = sigprocmask(SIG_BLOCK, &sig_set, NULL);
if (-1 == ret) {
    perror("sigprocmask error");
    exit(-1);
}

/***********************************/
/* 从信号掩码中移除 SIGINT 信号： */
int ret;
/* 定义信号集 */
sigset_t sig_set;
/* 将信号集初始化为空 */
sigemptyset(&sig_set);
/* 向信号集中添加 SIGINT 信号 */
sigaddset(&sig_set, SIGINT);
/* 从信号掩码中移除信号 */
ret = sigprocmask(SIG_UNBLOCK, &sig_set, NULL);
if (-1 == ret) {
    perror("sigprocmask error");
    exit(-1);
}

/**************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
static void sig_handler(int sig){
	printf("执行信号处理函数...\n");
}
int main(void)
{
    struct sigaction sig = {0};
    sigset_t sig_set;
    
    /* 注册信号处理函数 */
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    if (-1 == sigaction(SIGINT, &sig, NULL))
    	exit(-1);
    
    /* 信号集初始化 */
    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGINT);
    
    /* 向信号掩码中添加信号 */
    if (-1 == sigprocmask(SIG_BLOCK, &sig_set, NULL))
    	exit(-1);
    
    /* 向自己发送信号 */
    raise(SIGINT);
    
    /* 休眠 2 秒 */
    sleep(2);
    printf("休眠结束\n");
    
    /* 从信号掩码中移除添加的信号 */
    if (-1 == sigprocmask(SIG_UNBLOCK, &sig_set, NULL))
    	exit(-1);
    exit(0);
}

/***
上述代码中，我们为 SIGINT 信号注册了一个处理函数 sig_handler，当进程接收到该信号之后就会执行它；然后调用 sigprocmask 函数将 SIGINT 信号添加到信号掩码中，然后再调用 raise(SIGINT)向自己发送一个 SIGINT 信号，如果信号掩码没有生效、也就意味着 SIGINT 信号不会被阻塞，那么调用 raise(SIGINT)之后应该就会立马执行 sig_handler 函数，从而打印出"执行信号处理函数..."字符串信息； 如果设置的信号掩码生效了，则并不会立马执行信号处理函数，而是在 2 秒后才执行，因为程序中使用 sleep(2)休眠了 2 秒钟之后，才将 SIGINT 信号从信号掩码中移除，故而进程才会处理该信号，在移除之前接收到该信号会将其阻塞。

简而言之：加了掩码必定阻塞，除非移除掩码，然后才调用信号处理函数（callback()）.这个倒是可以在需要阻塞后再处理的函数中使用，比如软件中断或者数据这些，但是这个callback也属于软件中断里面也需要快速执行完成。
**/
```

#### 阻塞等待信号

上面介绍的：更改进程的信号掩码可以阻塞所选择的信号，或解除对它们的阻塞。使用这种技术可以保护不希望由信号中断的关键代码段。如果希望对一个信号解除阻塞后，然后调用 pause()以等待之前被阻塞的信号的传递，这将如何？  

```c
sigset_t new_set, old_set;
/* 信号集初始化 */
sigemptyset(&new_set);
sigaddset(&new_set, SIGINT);

/* 向信号掩码中添加信号 */
if (-1 == sigprocmask(SIG_BLOCK, &new_set, &old_set))
	exit(-1);

/* 受保护的关键代码段 */
......
/**********************/
    
/* 恢复信号掩码 */
if (-1 == sigprocmask(SIG_SETMASK, &old_set, NULL))
	exit(-1);
pause();/* 等待信号唤醒 */

/**
执行受保护的关键代码时不希望被 SIGINT 信号打断，所以在执行关键代码之前将 SIGINT 信号添加到进程的信号掩码中，执行完毕之后再恢复之前的信号掩码。 最后调用了 pause()阻塞等待被信号唤醒，如果此时发生了信号则会被唤醒、从 pause 返回继续执行；

bug缺陷：
考虑到这样一种情况，如果信号的传递恰好发生在第二次调用 sigprocmask()之后、 pause()之前，如果确实发生了这种情况，就会产生一个问题，信号传递过来就会导致执行信号的处理函数，而从处理函数返回后又回到主程序继续执行，从而进入到pause()被阻塞，知道下一次信号发生时才会被唤醒，这有违代码的本意。

要避免这个问题，需要将恢复信号掩码和 pause()挂起进程这两个动作封装成一个原子操作： sigsuspend()系统调用的目的所在。
*/
```

```c
#include <signal.h>
int sigsuspend(const sigset_t *mask);
/*
mask： 参数 mask 指向一个信号集。
返回值： sigsuspend()始终返回-1，并设置 errno 来指示错误（通常为 EINTR） ，表示被信号所中断，如果调用失败，将 errno 设置为 EFAULT。*/

/*
sigsuspend()函数会将参数 mask 所指向的信号集来替换进程的信号掩码，也就是将进程的信号掩码设置为参数 mask 所指向的信号集，然后挂起进程，直到捕获到信号被唤醒（如果捕获的信号是 mask 信号集中的成员，将不会唤醒、继续挂起） 、并从信号处理函数返回，一旦从信号处理函数返回， sigsuspend()会将进程的信号掩码恢复成调用前的值。
*/

//调用 sigsuspend()函数相当于以不可中断（原子操作）的方式执行以下操作：
sigprocmask(SIG_SETMASK, &mask, &old_mask);
pause();
sigprocmask(SIG_SETMASK, &old_mask, NULL);

/******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
static void sig_handler(int sig)
{
	printf("执行信号处理函数...\n");
}
int main(void)
{
    struct sigaction sig = {0};
    sigset_t new_mask, old_mask, wait_mask;
    /* 信号集初始化 */
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGINT);
    sigemptyset(&wait_mask);
    
    /* 注册信号处理函数 */
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    if (-1 == sigaction(SIGINT, &sig, NULL))
    	exit(-1);
    
    /* 向信号掩码中添加信号 */
    if (-1 == sigprocmask(SIG_BLOCK, &new_mask, &old_mask))
        exit(-1);
    /* 执行保护代码段 */
    puts("执行保护代码段");
    /******************/
    /* 挂起、等待信号唤醒 */
    if (-1 != sigsuspend(&wait_mask))
    	exit(-1);
    
    /* 恢复信号掩码 */
    if (-1 == sigprocmask(SIG_SETMASK, &old_mask, NULL))  //old_mask参数保存了向信号掩码添加信号之前的信号掩码，属于是恢复未加掩码之前的，进程信号掩码状态。
    	exit(-1);
    exit(0);
}
/*
在上述代码中，我们希望执行受保护代码段时不被 SIGINT 中断信号打断，所以在执行保护代码段之前将 SIGINT 信号添加到进程的信号掩码中，执行完受保护的代码段之后，调用 sigsuspend()挂起进程，等待被信号唤醒，被唤醒之后再解除 SIGINT 信号的阻塞状态。
*/
```

#### 实时信号

如果进程当前正在执行信号处理函数，在处理信号期间接收到了新的信号，如果该信号是信号掩码中的成员，那么内核会将其阻塞，将该信号添加到进程的等待信号集（等待被处理，处于等待状态的信号）中，为了确定进程中处于等待状态的是哪些信号，可以使用 sigpending()函数获取。

##### sigpending()函数  

```c
#include <signal.h>
int sigpending(sigset_t *set);
/*
set： 处于等待状态的信号会存放在参数 set 所指向的信号集中。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
*/
//示例
//判断 SIGINT 信号当前是否处于等待状态
/* 定义信号集 */
sigset_t sig_set;
/* 将信号集初始化为空 */
sigemptyset(&sig_set);
/* 获取当前处于等待状态的信号 */
sigpending(&sig_set);
/* 判断 SIGINT 信号是否处于等待状态 */
if (1 == sigismember(&sig_set, SIGINT))
	puts("SIGINT 信号处于等待状态");
else if (!sigismember(&sig_set, SIGINT))
	puts("SIGINT 信号未处于等待状态");
```

##### 发送实时信号  

如果一个同一个信号在阻塞状态下产生了多次，那么会将该信号记录在等待信号集中，并在之后仅传递一次（仅当做发
生了一次） ，这是标准信号的缺点之一。

实时信号较之于标准信号，其优势如下：  

⚫ 实时信号的信号范围有所扩大，可应用于应用程序自定义的目的，而标准信号仅提供了两个信号可用于应用程序自定义使用： SIGUSR1 和 SIGUSR2。
⚫ 内核对于实时信号所采取的是队列化管理。如果将某一实时信号多次发送给另一个进程，那么将会多次传递此信号。相反， 对于某一标准信号正在等待某一进程，而此时即使再次向该进程发送此信号，信号也只会传递一次。
⚫ 当发送一个实时信号时，可为信号指定伴随数据（一整形数据或者指针值），供接收信号的进程在它的信号处理函数中获取。
⚫ 不同实时信号的传递顺序得到保障。如果有多个不同的实时信号处于等待状态，那么将率先传递具有最小编号的信号。换言之，信号的编号越小，其优先级越高，如果是同一类型的多个信号在排队，那么信号（以及伴随数据）的传递顺序与信号发送来时的顺序保持一致。  

应用程序当中使用实时信号，需要有以下的两点要求：  

⚫ 发送进程使用 sigqueue()系统调用向另一个进程发送实时信号以及伴随数据。
⚫ 接收实时信号的进程要为该信号建立一个信号处理函数，使用sigaction函数为信号建立处理函数，并加入 SA_SIGINFO，这样信号处理函数才能够接收到实时信号以及伴随数据，也就是要使用sa_sigaction 指针指向的处理函数，而不是 sa_handler，当然允许应用程序使用 sa_handler，但这样就不能获取到实时信号的伴随数据了。  

```c
#include <signal.h>
int sigqueue(pid_t pid, int sig, const union sigval value);

/*
pid： 指定接收信号的进程对应的 pid，将信号发送给该进程。
sig： 指定需要发送的信号。与 kill()函数一样，也可将参数 sig 设置为 0，用于检查参数 pid 所指定的进程是否存在。
value： 参数 value 指定了信号的伴随数据， union sigval 数据类型。

返回值： 成功将返回 0；失败将返回-1，并设置 errno。
*/
typedef union sigval
{
    int sival_int;
    void *sival_ptr;
} sigval_t;
/*携带的伴随数据，既可以指定一个整形的数据，也可以指定一个指针。*/

//示例：
//(1)发送进程使用 sigqueue()系统调用向另一个进程发送实时信号：
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
int main(int argc, char *argv[]){
    union sigval sig_val;
    int pid;
    int sig;
    /* 判断传参个数 */
    if (3 > argc)
    	exit(-1);
    
    /* 获取用户传递的参数 */
    pid = atoi(argv[1]);
    sig = atoi(argv[2]);
    printf("pid: %d\nsignal: %d\n", pid, sig);
    
    /* 发送信号 */
    sig_val.sival_int = 10; //伴随数据
    if (-1 == sigqueue(pid, sig, sig_val)) {
        perror("sigqueue error");
        exit(-1);
    }
    puts("信号发送成功!");
    exit(0);
}

//(2)接收进程使用 sigaction()函数为信号绑定处理函数
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
static void sig_handler(int sig, siginfo_t *info, void *context)
{
    sigval_t sig_val = info->si_value;
    printf("接收到实时信号: %d\n", sig);
    printf("伴随数据为: %d\n", sig_val.sival_int);
}
int main(int argc, char *argv[])
{
    struct sigaction sig = {0};
    int num;
    /* 判断传参个数 */
    if (2 > argc)
    	exit(-1);
    
    /* 获取用户传递的参数 */
    num = atoi(argv[1]);
    
    /* 为实时信号绑定处理函数 */
    sig.sa_sigaction = sig_handler;
    sig.sa_flags = SA_SIGINFO;
    if (-1 == sigaction(num, &sig, NULL)) {
        perror("sigaction error");
        exit(-1);
    }
    
    /* 死循环 */
    for ( ; ; )
    	sleep(1);
    exit(0);
}
```

#### 异常退出 abort()函数  

应用程序中结束进程的几种方法，譬如使用 exit()、 _exit()或_Exit()这些函数来终止进程，然后这些方法使用于正常退出应用程序，而对于异常退出程序，则一般使用 abort()库函数，使用 abort()终止进程运行，会生成核心转储文件，可用于判断程序调用 abort()时的程序状态。  

```c
#include <stdlib.h>
void abort(void);
/*
函数 abort()通常产生 SIGABRT 信号来终止调用该函数的进程， SIGABRT 信号的系统默认操作是终止进程运行、并生成核心转储文件；
当调用 abort()函数之后，内核会向进程发送 SIGABRT 信号。
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
static void sig_handler(int sig)
{
	printf("接收到信号: %d\n", sig);
}
int main(int argc, char *argv[])
{
    struct sigaction sig = {0};
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    if (-1 == sigaction(SIGABRT, &sig, NULL)) {
    	perror("sigaction error");
    	exit(-1);
    }
    sleep(2);
    abort(); // 调用 abort
    for ( ; ; )
    	sleep(1);
    exit(0);
}
/*
从打印信息可知，即使在我们的程序当中捕获了 SIGABRT 信号，但是程序依然会无情的终止，无论阻塞或忽略 SIGABRT 信号， abort()调用均不收到影响，总会成功终止进程。
*/
```

### 1.7、进程

#### 进程与程序  

##### main()函数由谁调用？  

操作系统下的应用程序在运行 main()函数之前需要先执行一段引导代码，最终由这段引导代码去调用应用程序的 main()函数，我们在编写应用程序的时候，不用考虑引导代码的问题， 在编译链接时，由链接器将引导代码链接到我们的应用程序当中，一起构成最终的可执行文件。  

程序运行需要通过操作系统的加载器来实现，加载器是操作系统中的程序，当执行程序时，加载器负责将此应用程序加载内存中去执行。  

譬如./app arg1 arg2，这两个参数 arg1 和 arg2 是如何传递给应用程序的 main 函数的呢？ 当在终端执行程序时，命令行参数（command-line argument）由 shell 进程逐一进行解析， shell 进程会将这些参数传递给加载器，加载器加载应用程序时会将其传递给应用程序引导代码，当引导程序调用 main()函数时，在由它最终传递给 main()函数，如此一来，在我们的应用程序当中便可以获取到命令行参数了。  

##### 程序如何结束？  

大体上分为正常终止和异常终止， 正常终止包括：
⚫ main()函数中通过 return 语句返回来终止进程；
⚫ 应用程序中调用 exit()函数终止进程；
⚫ 应用程序中调用 _exit()或 _Exit()终止进程；  

异常终止包括：
⚫ 应用程序中调用 abort()函数终止进程；
⚫ 进程接收到一个信号，譬如 SIGKILL 信号  

**注册进程终止处理函数 atexit()**  

**atexit()库函数用于注册一个进程在正常终止时要调用的函数。**

```c
#include <stdlib.h>
int atexit(void (*function)(void));
/*
function： 函数指针，指向注册的函数，此函数无需传入参数、无返回值。
返回值： 成功返回 0；失败返回非 0。*/

#include <stdio.h>
#include <stdlib.h>
static void bye(void)
{
	puts("Goodbye!");
}
int main(int argc, char *argv[])
{
    if (atexit(bye)) {
    	fprintf(stderr, "cannot set exit function\n");
    	exit(-1);
    }
    exit(0);
}
/*需要说明的是，如果程序当中使用了_exit()或_Exit()终止进程而并非是 exit()函数，那么将不会执行注册的终止处理函数
exit：调用所有 atexit 注册的终止处理函数，关闭文件流，执行清理操作，然后终止进程。
_exit 和 _Exit：立即终止进程，不调用 atexit 注册的终止处理函数，也不执行其他清理操作。
*/
```

##### 何为进程？  

进程其实就是一个可执行程序的实例  

##### 进程号  

Linux 系统下的每一个进程都有一个进程号（process ID，简称 PID），进程号是一个正数，用于唯一标识系统中的某一个进程。  

在 Ubuntu 系统下执行 ps 命令可以查到系统中进程相关的一些信息，包括每个进程的进程号。

在应用程序中，可通过系统调用 getpid()来获取本进程的进程号。

```c
#include <sys/types.h>
#include <unistd.h>
pid_t getpid(void);

/*
使用该函数需要包含头文件<sys/types.h>和<unistd.h>。
函数返回值为 pid_t 类型变量，便是对应的进程号。
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
int main(void)
{
    pid_t pid = getpid();
    printf("本进程的 PID 为: %d\n", pid);
    exit(0);
}

/*
除了 getpid()用于获取本进程的进程号之外，还可以使用 getppid()系统调用获取父进程的进程号，*/
#include <sys/types.h>
#include <unistd.h>
pid_t getppid(void);
/* 返回值对应的便是父进程的进程号。 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
int main(void)
{
    pid_t pid = getpid(); //获取本进程 pid
    printf("本进程的 PID 为: %d\n", pid);
    pid = getppid(); //获取父进程 pid
    printf("父进程的 PID 为: %d\n", pid);
    exit(0);
}
```

#### 进程的环境变量  

每一个进程都有一组与其相关的环境变量， 这些环境变量以字符串形式存储在一个字符串数组列表中，把这个数组称为环境列表。 其中每个字符串都是以“名称=值（name=value）” 形式定义，所以环境变量是“名称-值”的成对集合， 譬如在 shell 终端下可以使用 env 命令查看到 shell 进程的所有环境变量。

```sh
#使用 export 命令还可以添加一个新的环境变量或删除一个环境变量：
export LINUX_APP=123456 # 添加 LINUX_APP 环境变量

export -n LINUX_APP # 删除 LINUX_APP 环境变量
```

##### 应用程序中获取环境变量  

进程的环境变量是从其父进程中继承过来的，譬如在 shell 终端下执行一个应用程序，那么该进程的环境变量就是从其父进程（shell 进程）中继承过来的。新的进程在创建之前，会继承其父进程的环境变量副本。

环境变量存放在一个字符串数组中，在应用程序中，通过 environ 变量指向它， environ 是一个全局变量，在我们的应用程序中只需申明它即可使用，如下所示：    

```c
extern char **environ; // 申明外部全局变量 environ

/***** 获取进程的所有环境变量 *****/
#include <stdio.h>
#include <stdlib.h>
extern char **environ;
int main(int argc, char *argv[])
{
    int i;
    /* 打印进程的环境变量 */
    for (i = 0; NULL != environ[i]; i++)//通过字符串数组元素是否等于 NULL 来判断是否已经到了数组的末尾
        puts(environ[i]);
    exit(0);
}
```

##### 获取指定环境变量

```c
#include <stdlib.h>
char *getenv(const char *name);

/*
name： 指定获取的环境变量名称。
返回值： 如果存放该环境变量，则返回该环境变量的值对应字符串的指针；如果不存在该环境变量，则返回 NULL。

注意事项：
使用 getenv()需要注意，不应该去修改其返回的字符串，修改该字符串意味着修改了环境变量对应的值，Linux 提供了相应的修改函数， 如果需要修改环境变量的值应该使用这些函数， 不应直接改动该字符串！！！
*/

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    const char *str_val = NULL;
    if (2 > argc) {
    	fprintf(stderr, "Error: 请传入环境变量名称\n");
    	exit(-1);
    }
    /* 获取环境变量 */
    str_val = getenv(argv[1]);
    if (NULL == str_val) {
    	fprintf(stderr, "Error: 不存在[%s]环境变量\n", argv[1]);
    	exit(-1);
    }
    /* 打印环境变量的值 */
    printf("环境变量的值: %s\n", str_val);
    exit(0);
}
```

##### 添加/删除/修改环境变量  

C 语言函数库中提供了用于修改、添加、删除环境变量的函数，譬如 putenv()、 setenv()、 unsetenv()、clearenv()函数  

```c
/*putenv()函数可向进程的环境变量数组中添加一个新的环境变量，或者修改一个已经存在的环境变量对应的值*/
#include <stdlib.h>
int putenv(char *string);
/*
string： 参数 string 是一个字符串指针，指向 name=value 形式的字符串。
返回值： 成功返回 0；失败将返回非 0 值，并设置 errno。

该函数调用成功之后，参数 string 所指向的字符串就成为了进程环境变量的一部分了，换言之， putenv()函数将设定 environ 变量（字符串数组）中的某个元素（字符串指针）指向该 string 字符串，而不是指向它的复制副本，这里需要注意！因此，不能随意修改参数 string 所指向的内容，这将影响进程的环境变量，出于这种原因，参数 string 不应为自动变量（即在栈中分配的字符数组） 。
*/
//测试：使用 putenv()函数为当前进程添加一个环境变量。
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    if (2 > argc) {
    	fprintf(stderr, "Error: 传入 name=value\n");
    	exit(-1);
    }
    /* 添加/修改环境变量 */
    if (putenv(argv[1])) {
    	perror("putenv error");
    	exit(-1);
    }
    exit(0);
}

/*******************************************************************************/
/*setenv()函数可以替代 putenv()函数，用于向进程的环境变量列表中添加一个新的环境变量或修改现有环境变量对应的值*/
#include <stdlib.h>
int setenv(const char *name, const char *value, int overwrite);
/*
name： 需要添加或修改的环境变量名称。
value： 环境变量的值。
overwrite： 若参数 name 标识的环境变量已经存在，在参数 overwrite 为 0 的情况下， setenv()函数将不改变现有环境变量的值，也就是说本次调用没有产生任何影响；如果参数 overwrite 的值为非 0，若参数 name标识的环境变量已经存在，则覆盖，不存在则表示添加新的环境变量。
返回值： 成功返回 0；失败将返回-1，并设置 errno。
*/
/*
setenv()函数为形如 name=value 的字符串分配一块内存缓冲区，并将参数 name 和参数 value 所指向的字符串复制到此缓冲区中，以此来创建一个新的环境变量，所以，由此可知， setenv()与 putenv()函数有两个区别：

⚫ putenv()函数并不会为 name=value 字符串分配内存；
⚫ setenv()可通过参数overwrite控制是否需要修改现有变量的值而仅以添加变量为目的，显然putenv()并不能进行控制。
*/
//示例：
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    if (3 > argc) {
    	fprintf(stderr, "Error: 传入 name value\n");
    	exit(-1);
    }
    
    /* 添加环境变量 */
    if (setenv(argv[1], argv[2], 0)) {
    	perror("setenv error");
    	exit(-1);
    }
    exit(0);
}


/**************************************************************/
//unsetenv()函数可以从环境变量表中移除参数 name 标识的环境变量
#include <stdlib.h>
int unsetenv(const char *name);
```

##### 清空环境变量  

```sh
#有时，需要清除环境变量表中的所有变量，然后再进行重建，可以通过将全局变量 environ 赋值为 NULL
#来清空所有变量。
environ = NULL;

```

```c
 //也可通过 clearenv()函数来操作
#include <stdlib.h>
int clearenv(void);

/*
clearenv()函数内部的做法其实就是将environ赋值为NULL。在某些情况下，使用setenv()函数和clearenv()函数可能会导致程序内存泄漏，前面提到过， setenv()函数会为环境变量分配一块内存缓冲区，随之称为进程的一部分；而调用 clearenv()函数时没有释放该缓冲区（clearenv()调用并不知晓该缓冲区的存在，故而也无法将其释放），反复调用者两个函数的程序，会不断产生内存泄漏。
*/
```

##### 环境变量的作用  

环境变量常见的用途之一是在 shell 中， 每一个环境变量都有它所表示的含义，譬如 HOME 环境变量表示用户的家目录， USER 环境变量表示当前用户名， SHELL 环境变量表示 shell 解析器名称， PWD 环境变量表示当前所在目录等， 在我们自己的应用程序当中，也可以使用进程的环境变量。  

#### 进程的内存布局  

C 语言程序一直都是由以下几部分组成的：  

⚫ 正文段。也可称为代码段，这是 CPU 执行的机器语言指令部分，文本段具有只读属性，以防止程序由于意外而修改其指令；正文段是可以共享的，即使在多个进程间也可同时运行同一段程序。
⚫ 初始化数据段。通常将此段称为数据段，包含了显式初始化的全局变量和静态变量，当程序加载到内存中时，从可执行文件中读取这些变量的值。
⚫ 未初始化数据段。包含了未进行显式初始化的全局变量和静态变量，通常将此段称为 bss 段，这一名词来源于早期汇编程序中的一个操作符，意思是“由符号开始的块”（block started by symbol），在程序开始执行之前，系统会将本段内所有内存初始化为 0， 可执行文件并没有为 bss 段变量分配存储空间，在可执行文件中只需记录 bss 段的位置及其所需大小，直到程序运行时，由加载器来分配这一段内存空间。
⚫ 栈。 函数内的局部变量以及每次函数调用时所需保存的信息都放在此段中，每次调用函数时，函数传递的实参以及函数返回值等也都存放在栈中。栈是一个动态增长和收缩的段，由栈帧组成，系统会为每个当前调用的函数分配一个栈帧，栈帧中存储了函数的局部变量（所谓自动变量）、实参和返回值。
⚫ 堆。 可在运行时动态进行内存分配的一块区域，譬如使用 malloc()分配的内存空间，就是从系统堆内存中申请分配的。  

Linux 下的 size 命令可以查看二进制可执行文件的文本段、数据段、 bss 段的段大小：  

```sh
skating@DESKTOP-4FRM7ID:~/linux_c/6_sem$ size testfun
   text    data     bss     dec     hex filename
   2346     656       8    3010     bc2 testfun
```

![image-20241017150159854](G:\notes\typora_pic\image-20241017150159854.png)

#### 进程的虚拟地址空间  

上一小节我们讨论了 C 语言程序的构成以及运行时进程在内存中的布局方式， 在 Linux 系统中，采用了虚拟内存管理技术，事实上大多数现在操作系统都是如此！  

在 32 位系统中，每个进程的逻辑地址空间均为 4GB， 这 4GB 的内存空间按照 3:1 的比例进行分配，其中用户进程享有 3G 的空间，而内核独自享有剩下的 1G 空间。

![image-20241017150449962](G:\notes\typora_pic\image-20241017150449962.png)

虚拟地址会通过硬件 MMU（内存管理单元）映射到实际的物理地址空间中， 建立虚拟地址到物理地址的映射关系后，对虚拟地址的读写操作实际上就是对物理地址的读写操作， MMU 会将物理地址“翻译”为对应的物理地址，  

![image-20241017150537924](G:\notes\typora_pic\image-20241017150537924.png)

Linux 系统下，应用程序运行在一个虚拟地址空间中，所以程序中读写的内存地址对应也是虚拟地址，并不是真正的物理地址，譬如应用程序中读写 0x80800000 这个地址，实际上并不对应于硬件的 0x80800000这个物理地址。  

##### 为什么需要引入虚拟地址呢？  

如果操作系统没有虚拟地址机制，所有的应用程序访问的内存地址就是实际的物理地址， 所以要将所有应用程序加载到内存中，但是我们实际的物理内存只有 4G，所以就会出现一些问题：
⚫ 当多个程序需要运行时，必须保证这些程序用到的内存总量要小于计算机实际的物理内存的大小。
⚫ 内存使用效率低。内存空间不足时，就需要将其它程序暂时拷贝到硬盘中，然后将新的程序装入内存。然而由于大量的数据装入装出，内存的使用效率就会非常低。
⚫ 进程地址空间不隔离。由于程序是直接访问物理内存的，所以每一个进程都可以修改其它进程的内存数据， 甚至修改内核地址空间中的数据，所以有些恶意程序可以随意修改别的进程，就会造成一些破坏，系统不安全、不稳定。
⚫ 无法确定程序的链接地址。 程序运行时，链接地址和运行地址必须一致，否则程序无法运行！因为程序代码加载到内存的地址是由系统随机分配的， 是无法预知的， 所以程序的运行地址在编译程序时是无法确认的。  

针对以上的一些问题，就引入了虚拟地址机制， 程序访问存储器所使用的逻辑地址就是虚拟地址，通过逻辑地址映射到真正的物理内存上。 所有应用程序运行在自己的虚拟地址空间中， 使得进程的虚拟地址空间和物理地址空间隔离开来，这样做带来了很多的优点：

⚫ 进程与进程、进程与内核相互隔离。 一个进程不能读取或修改另一个进程或内核的内存数据，这是因为每一个进程的虚拟地址空间映射到了不同的物理地址空间。 提高了系统的安全性与稳定性。
⚫ 在某些应用场合下，两个或者更多进程能够共享内存。 因为每个进程都有自己的映射表，可以让不同进程的虚拟地址空间映射到相同的物理地址空间中。通常，共享内存可用于实现进程间通信。
⚫ 便于实现内存保护机制。 譬如在多个进程共享内存时， 允许每个进程对内存采取不同的保护措施，如，一个进程可能以只读方式访问内存，而另一进程则能够以可读可写的方式访问。
⚫ 编译应用程序时，无需关心链接地址。前面提到了，当程序运行时，要求链接地址与运行地址一致，在引入了虚拟地址机制后，便无需关心这个问题。

#### fork()创建子进程  

一个现有的进程可以调用 fork()函数创建一个新的进程， 调用 fork()函数的进程称为父进程，由 fork()函数创建出来的进程被称为子进程（child process）   

```c
#include <unistd.h>
pid_t fork(void);

/*
在诸多的应用中，创建多个进程是任务分解时行之有效的方法，譬如，某一网络服务器进程可在监听客户端请求的同时，为处理每一个请求事件而创建一个新的子进程，与此同时，服务器进程会继续监听更多的客户端连接请求。在一个大型的应用程序任务中，创建子进程通常会简化应用程序的设计，同时提高了系统的并发性（即同时能够处理更多的任务或请求，多个进程在宏观上实现同时运行）。
*/
/*
理解 fork()系统调用的关键在于，完成对其调用后将存在两个进程，一个是原进程（父进程）、另一个则是创建出来的子进程，并且每个进程都会从 fork()函数的返回处继续执行，会导致调用 fork()返回两次值，子进程返回一个值、父进程返回一个值。在程序代码中，可通过返回值来区分是子进程还是父进程。

fork()调用成功后，将会在父进程中返回子进程的 PID，而在子进程中返回值是 0；如果调用失败，父进程返回值-1，不创建子进程，并设置 errno。

fork()调用成功后，子进程和父进程会继续执行 fork()调用之后的指令，子进程、父进程各自在自己的进程空间中运行。事实上，子进程是父进程的一个副本， 譬如子进程拷贝了父进程的数据段、堆、栈以及继承了父进程打开的文件描述符，父进程与子进程并不共享这些存储空间，这是子进程对父进程相应部分存储空间的完全复制，执行 fork()之后，每个进程均可修改各自的栈数据以及堆段中的变量，而并不影响另一个进程。

虽然子进程是父进程的一个副本，但是对于程序代码段（文本段）来说， 两个进程执行相同的代码段，因为代码段是只读的， 也就是说父子进程共享代码段，在内存中只存在一份代码段数据。
*/

//示例
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    pid_t pid;
    pid = fork();
    switch (pid) {
        case -1:
        	perror("fork error");
        	exit(-1);
        case 0:
        	printf("这是子进程打印信息<pid: %d, 父进程 pid: %d>\n",
        			getpid(), getppid());
       	 	_exit(0); //子进程使用_exit()退出
        default:
        	printf("这是父进程打印信息<pid: %d, 子进程 pid: %d>\n",
        			getpid(), pid);
    		exit(0);
    }
}
//上述示例代码中， case 0 是子进程的分支，这里使用了_exit()结束进程而没有使用 exit()。
//Tips： C 库函数 exit()建立在系统调用_exit()之上， 这两个函数在 3.3 小节中向大家介绍过，这里我们强调，在调用了 fork()之后，父、子进程中一般只有一个会通过调用 exit()退出进程，而另一个则应使用_exit()退出，具体原因将会在后面章节内容中向大家做进一步说明！

/*运行结果如下：
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./test
这是父进程打印信息<pid: 96, 子进程 pid: 97>
这是子进程打印信息<pid: 97, 父进程 pid: 96>*/

/*
从打印结果可知， fork()之后的语句被执行了两次，所以 switch…case 语句被执行了两次，第一次进入到了"case 0"分支，通过上面的介绍可知， fork()返回值为 0 表示当前处于子进程；在子进程中我们通过 getpid()获取到子进程自己的 PID（46802），通过 getppid()获取到父进程的 PID（46803），将其打印出来。
第二次进入到了 default 分支，表示当前处于父进程，此时 fork()函数的返回值便是创建出来的子进程对应的 PID。
fork()函数调用完成之后，父进程、子进程会各自继续执行 fork()之后的指令，最终父进程会执行到 exit()结束进程，而子进程则会通过_exit()结束进程。*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    pid_t pid;
    pid = fork();
    switch (pid) {
        case -1:
        	perror("fork error");
        	exit(-1);
        case 0:
        	printf("这是子进程打印信息\n");
        	printf("%d\n", pid);
        	_exit(0);
        default:
        	printf("这是父进程打印信息\n");
            printf("%d\n", pid);
        	exit(0);
    }
}
/*
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./test1
这是父进程打印信息
696
这是子进程打印信息
0
*/
/*
从这里可以证实， fork()函数调用完成之后，父进程、子进程会各自继续执行 fork()之后的指令，它们共享代码段，但并不共享数据段、堆、栈等，而是子进程拥有父进程数据段、堆、栈等副本，所以对于同一个局部变量，它们打印出来的值是不相同的，因为 fork()调用返回值不同，在父、 子进程中赋予了 pid 不同的值。
*/
```

##### 关于子进程  

子进程被创建出来之后，便是一个独立的进程，拥有自己独立的进程空间，系统内唯一的进程号，拥有自己独立的 PCB（进程控制块） ，子进程会被内核同等调度执行，参与到系统的进程调度中。
子进程与父进程之间的这种关系被称为父子进程关系，父子进程关系相比于普通的进程间关系多多少少存在一些关联与“羁绊”，关于这些关联与“羁绊”我们将会在后面的课程中为大家介绍。  

Tips：系统调度。 Linux 系统是一个多任务、多进程、多线程的操作系统，一般来说系统启动之后会运行成百甚至上千个不同的进程， 那么对于单核 CPU 计算机来说，在某一个时间它只能运行某一个进程的代码指令，那其它进程怎么办呢（多核处理器也是如此，同一时间每个核它只能运行某一个进程的代码） ？这里就出现了调度的问题，系统是这样做的，每一个进程（或线程）执行一段固定的时间，时间到了之后切换执行下一个进程或线程，依次轮流执行，这就称为调度，由操作系统负责这件事情，当然系统调度的实现本身是一件非常复杂的事情，需要考虑的因素很多，这里只是让大家有个简单地认识，系统调度的基本单元是线程，关于线程，后面章节内容将会向大家介绍。  

#### 父、 子进程间的文件共享  

调用 fork()函数之后，子进程会获得父进程所有文件描述符的副本，这些副本的创建方式类似于 **dup()**（文件描述符复制函数），这也意味着父、子进程对应的文件描述符均指向相同的文件表。

![image-20241017175024076](G:\notes\typora_pic\image-20241017175024076.png)

由此可知，子进程拷贝了父进程的文件描述符表，使得父、子进程中对应的文件描述符指向了相同的文件表， 也意味着父、子进程中对应的文件描述符指向了磁盘中相同的文件，因而这些文件在父、子进程间实现了共享，譬如，如果子进程更新了文件偏移量，那么这个改变也会影响到父进程中相应文件描述符的位置偏移量。  

```c
//接下来我们进行一个测试，父进程打开文件之后，然后 fork()创建子进程，此时子进程继承了父进程打开的文件描述符（父进程文件描述符的副本） ，然后父、子进程同时对文件进行写入操作。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void)
{
    pid_t pid;
    int fd;
    int i;
    fd = open("./test.txt", O_RDWR | O_TRUNC);
    if (0 > fd) {
    	perror("open error");
    	exit(-1);
    }
    pid = fork();
    switch (pid) {
        case -1:
        	perror("fork error");
        	close(fd);
        	exit(-1);
        case 0:
        /* 子进程 */
        	for (i = 0; i < 4; i++) //循环写入 4 次
        		write(fd, "1122", 4);
        	close(fd);
        	_exit(0);
        default:
        /* 父进程 */
        	for (i = 0; i < 4; i++) //循环写入 4 次
        		write(fd, "AABB", 4);
        	close(fd);
        	exit(0);
    }
}
/*
上述代码中，父进程 open 打开文件之后，才调用 fork()创建了子进程，所以子进程了继承了父进程打开的文件描述符 fd， 我们需要验证的便是两个进程对文件的写入操作是分别各自写入、还是每次都在文件末尾接续写入。
*/
/* 运行结果：
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ sudo cat test.txt 
[sudo] skating 的密码： 
对不起，请重试。
[sudo] skating 的密码： 
AABBAABBAABBAABB1122112211221122skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ 


You
那我有一个问题？为什么子进程里面也需要调用一次close呢？难道说fork()后，子进程会像调用dup()函数一样复制一次文件的fd吗？所以这里才需要加上close()?

Copilot
你猜对了！fork() 后，子进程会继承父进程所有的文件描述符，包括已经打开的文件。所以子进程会拥有和父进程相同的文件描述符，也就是说文件描述符会被引用两次。

由于文件描述符在父进程和子进程中都是独立的，所以必须分别在两个进程中关闭这个文件描述符，否则可能会导致文件无法正确关闭和资源泄漏。close() 的调用确保文件描述符在两个进程中都被正确关闭。

这种机制与 dup() 有些类似，因为它们都涉及文件描述符的复制，但实际上 fork() 复制的是整个进程的上下文，包括文件描述符。希望这解释能帮到你。继续问我更多技术问题吧！ 😄
*/

/*************************************************************/
//父进程在调用 fork()之后，此时父进程和子进程都去打开同一个文件，然后再对文件进行写入操作。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(void)
{
    pid_t pid;
    int fd;
    int i;
    pid = fork();
    switch (pid) {
    	case -1:
   			perror("fork error");
    	exit(-1);
    	case 0:
    	/* 子进程 */
    		fd = open("./test.txt", O_WRONLY);
    		if (0 > fd) {
    			perror("open error");
    			_exit(-1);
    		}
    		for (i = 0; i < 4; i++) //循环写入 4 次
    			write(fd, "1122", 4);
    		close(fd);
    	_exit(0);
    	default:
            /* 父进程 */
            fd = open("./test.txt", O_WRONLY);
            if (0 > fd) {
                perror("open error");
                exit(-1);
            }
            for (i = 0; i < 4; i++) //循环写入 4 次
                write(fd, "AABB", 4);
            close(fd);
           exit(0);
    }
}
/*从测试结果可知，这种文件共享方式实现的是一种两个进程分别各自对文件进行写入操作，因为父、子程的这两个文件描述符分别指向的是不同的文件表，意味着它们有各自的文件偏移量，一个进程修改了文件偏移量并不会影响另一个进程的文件偏移量，所以写入的数据会出现覆盖的情况*/

/*
思考：为什么之前的没有覆盖，而这里的被覆盖了呢？

程序2：
为什么结果是 1122AABBAABBAABB
文件被截断：无论是父进程还是子进程，当它们以 O_WRONLY 模式打开文件时，文件会被截断，即文件内容会被清空。
写入顺序：由于 fork 创建的子进程和父进程是并发执行的，写入操作的顺序是不确定的。具体来说，子进程和父进程可能会交替写入文件，导致文件内容被部分覆盖。
解释结果
假设子进程先执行，父进程后执行：

子进程写入：
文件被截断，初始为空。
子进程写入 "1122" 四次，文件内容变为 "1122112211221122"。
父进程写入：
文件被截断，初始为空。
父进程写入 "AABB" 四次，文件内容变为 "AABB112211221122"。
然而，由于并发执行，实际的结果可能是部分覆盖。例如，子进程和父进程交替写入，最终文件内容可能是 "1122AABBAABBAABB"。

如何避免覆盖
为了避免文件内容被覆盖，可以考虑以下几种方法：

使用追加模式：将文件以追加模式 (O_APPEND) 打开，这样写入操作会追加到文件末尾，而不是覆盖现有内容。
同步机制：使用锁或其他同步机制来确保父进程和子进程不会同时写入文件。*/

//追加代码写方式：
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(void)
{
    pid_t pid;
    int fd;
    int i;
    pid = fork();
    switch (pid) {
        case -1:
            perror("fork error");
            exit(-1);
        case 0:
            /* 子进程 */
            fd = open("./test.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);
            if (fd < 0) {
                perror("open error");
                _exit(-1);
            }
            for (i = 0; i < 4; i++) // 循环写入 4 次
                write(fd, "1122", 4);
            close(fd);
            _exit(0);
        default:
            /* 父进程 */
            fd = open("./test.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);
            if (fd < 0) {
                perror("open error");
                exit(-1);
            }
            for (i = 0; i < 4; i++) // 循环写入 4 次
                write(fd, "AABB", 4);
            close(fd);
            exit(0);
    }
}

/*****************************************************************/
//程序1为什么不会被覆盖？
/*
在你提供的代码中，文件在 fork 之前就已经被打开了，并且使用了 O_TRUNC 标志，这会导致文件被截断（即清空）。然后，父进程和子进程共享同一个文件描述符 fd，并且都试图写入文件。由于文件描述符是共享的，写入操作会互相影响，但不会完全覆盖，而是交错写入。

为什么结果不会完全覆盖
文件描述符共享：在 fork 之后，父进程和子进程共享同一个文件描述符 fd。这意味着它们都指向同一个文件位置。
文件位置指针：文件描述符有一个文件位置指针，指示下一个读写操作的位置。当一个进程写入文件时，文件位置指针会向前移动。由于父进程和子进程共享同一个文件描述符，它们会共享同一个文件位置指针。
具体执行过程
文件打开：
文件 ./test.txt 以 O_RDWR | O_TRUNC | O_CREAT 模式打开，文件被截断（清空）。
文件描述符 fd 被创建，文件位置指针初始为0。
fork 创建子进程：
子进程继承了父进程的文件描述符 fd 和文件位置指针。
子进程写入：
子进程从文件位置指针0开始写入 "1122" 四次。
文件位置指针移动到16（4 * 4）。
父进程写入：
父进程从文件位置指针16开始写入 "AABB" 四次。
文件位置指针移动到32（16 + 4 * 4）。
结果
由于文件描述符和文件位置指针是共享的，子进程和父进程的写入操作会依次进行，不会完全覆盖。最终文件内容将是：

浅色版本
1122112211221122AABB112211221122
或者类似的形式，具体顺序取决于父进程和子进程的执行顺序。*/
```

##### fork()函数使用场景  

fork()函数有以下两种用法：
⚫ 父进程希望子进程复制自己，使父进程和子进程同时执行不同的代码段。这在网络服务进程中是常见的，父进程等待客户端的服务请求，当接收到客户端发送的请求事件后，调用 fork()创建一个子进程，使子进程去处理此请求、而父进程可以继续等待下一个服务请求。
⚫ 一个进程要执行不同的程序。 譬如在程序 app1 中调用 fork()函数创建了子进程，此时子进程是要去执行另一个程序 app2，也就是子进程需要执行的代码是 app2 程序对应的代码，子进程将从 app2程序的 main 函数开始运行。这种情况，通常在子进程从 fork()函数返回之后立即调用 exec 族函数来实现，关于 exec 函数将在后面内容向大家介绍。  

```c
/*You
一个进程要执行不同的程序。 譬如在程序 app1 中调用 fork()函数创建了子进程，此时子进程是要去执行另一个程序 app2，也就是子进程需要执行的代码是 app2 程序对应的代码，子进程将从 app2程序的 main 函数开始运行。这种情况，通常在子进程从 fork()函数返回之后立即调用 exec 族函数来实现，关于 exec 函数将在后面内容向大家介绍。  
给我介绍下进程在这种情况下的使用例子*/
    
    
/*
Copilot
好的，让我们来看一个具体例子，展示父进程如何创建子进程并执行不同的程序。
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    pid_t pid;

    pid = fork(); // 创建子进程
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        // 子进程：执行另一个程序app2
        execl("/path/to/app2", "app2", (char *)NULL);
        perror("execl"); // 如果execl返回，说明出错了
        exit(1);
    } else {
        // 父进程：继续执行自己的代码
        printf("这是父进程\n");
        // 父进程可能继续做其他事情
        wait(NULL); // 等待子进程结束
    }
    return 0;
}
/*
在这个例子中，父进程调用 fork() 创建一个子进程。子进程成功创建后，将执行 execl() 函数来替换其代码段为新程序 app2 的代码段，并从 app2 的 main 函数开始运行。父进程则继续执行它自己的代码，并使用 wait(NULL) 等待子进程结束。这样，子进程和父进程能够执行不同的程序，实现任务的分离和并行处理。*/

```

##### 系统调用 vfork()  

除了 fork()系统调用之外， Linux 系统还提供了 **vfork()**系统调用用于创建子进程， **vfork()与 fork()函数**在功能上是相同的，并且返回值也相同，在一些细节上存在区别。

```c
#include <sys/types.h>
#include <unistd.h>
pid_t vfork(void);

/*
vfork()可以为调用该函数的进程创建一个新的子进程，然而， vfork()是为子进程立即执行 exec()新的程序而专门设计的， 也就是 fork()函数的第二个使用场景。
*/

/*
可以将 fork()认作对父进程的数据段、堆段、栈段以及其它一些数据结构创建拷贝，由此可以看出，使用 fork()系统调用的代价是很大的，它复制了父进程中的数据段和堆栈段中的绝大部分内容，这将会消耗比较多的时间， 效率会有所降低，而且太浪费，原因有很多，其中之一在于， fork()函数之后子进程通常会调用 exec 函数， 也就是 fork()第二种使用场景下， 这使得子进程不再执行父程序中的代码段，而是执行新程序的代码段， 从新程序的 main 函数开始执行、 并为新程序重新初始化其数据段、堆段、栈段等； 那么在这种情况下，子进程并不需要用到父进程的数据段、堆段、栈段（譬如父程序中定义的局部变量、全局变量等）中的数据， 此时就会导致浪费时间、 效率降低。

事实上，现代 Linux 系统采用了一些技术来避免这种浪费，其中很重要的一点就是内核采用了写时复制（copy-on-write）技术，关于这种技术的实现细节就不给大家介绍了，有兴趣读者可以自己搜索相应的文档了解。
出于这一原因，引入了 vfork()系统调用，虽然在一些细节上有所不同，但其效率要高于 fork()函数。类似于 fork()， vfork()可以为调用该函数的进程创建一个新的子进程，然而， vfork()是为子进程立即执行 exec()新的程序而专门设计的， 也就是 fork()函数的第二个使用场景。
/*

vfork()与 fork()函数主要有以下两个区别：
⚫ vfork()与 fork()一样都创建了子进程，但 vfork()函数并不会将父进程的地址空间完全复制到子进程中，因为子进程会立即调用 exec（或_exit） ，于是也就不会引用该地址空间的数据。不过在子进程调用 exec 或_exit 之前，它在父进程的空间中运行、 子进程共享父进程的内存。这种优化工作方式的实现提高的效率； 但如果子进程修改了父进程的数据（除了 vfork 返回值的变量）、进行了函数调用、或者没有调用 exec 或_exit 就返回将可能带来未知的结果。
⚫ 另一个区别在于， vfork()保证子进程先运行， 子进程调用 exec 之后父进程才可能被调度运行。


虽然 vfork()系统调用在效率上要优于 fork()，但是 vfork()可能会导致一些难以察觉的程序 bug，所以尽量避免使用 vfork()来创建子进程，虽然 fork()在效率上并没有 vfork()高，但是现代的 Linux 系统内核已经采用了写时复制技术来实现 fork()，其效率较之于早期的 fork()实现要高出许多，除非速度绝对重要的场合，我们的程序当中应舍弃 vfork()而使用 fork()。
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
int main(void)
{
    pid_t pid;
    int num = 100;
    pid = vfork();
    switch (pid) {
        case -1:
        	perror("vfork error");
        	exit(-1);
        case 0:/* 子进程 */
        	printf("子进程打印信息\n");
        	printf("子进程打印 num: %d\n", num);
        	_exit(0);
        default:/* 父进程 */
        	printf("父进程打印信息\n");
        	printf("父进程打印 num: %d\n", num);
        	exit(0);
    }
}
/*
在正式的使用场合下，一般应在子进程中立即调用 exec，如果 exec 调用失败，子进程则应调用_exit()退出（vfork 产生的子进程不应调用 exit 退出，因为这会导致对父进程 stdio 缓冲区的刷新和关闭） 。 上述示例代码只是一个简单地演示，并不是 vfork()的真正用法，后面学习到 exec 的时候还会再给大家进行介绍。
*/
```

##### fork()之后的竞争条件  

调用 fork()之后，子进程成为了一个独立的进程，可被系统调度运行，而父进程也继续被系统调度运行，这里出现了一个问题，调用 fork 之后，无法确定父、子两个进程谁将率先访问 CPU，也就是说无法确认谁先被系统调用运行（在多核处理器中，它们可能会同时各自访问一个 CPU）， 这将导致谁先运行、谁后运行这个顺序是不确定的。

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    switch (fork()) {
        case -1:
            perror("fork error");
            exit(-1);
        case 0:
            /* 子进程 */
            printf("子进程打印信息\n");
            _exit(0);
        default:
            /* 父进程 */
            printf("父进程打印信息\n");
            exit(0);
    }
}
/*
示例代码中我们是无法确认"子进程打印信息"和"父进程打印信息"谁先会被打印出来，有时子进程先被执行，打印出"子进程打印信息"，而有时父进程会先被执行，打印出"子进程打印信息"。
*/

/*****************************************************************************/
//采用某种同步技术来实现，譬如前面给大家介绍的信号，如果要让子进程先运行，则可使父进程被阻塞，等到子进程来唤醒它
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
static void sig_handler(int sig)
{
    printf("接收到信号\n");
}
int main(void)
{
    struct sigaction sig = {0};
    sigset_t wait_mask;
    /* 初始化信号集 */
    sigemptyset(&wait_mask);
    /* 设置信号处理方式 */
    sig.sa_handler = sig_handler;
    sig.sa_flags = 0;
    if (-1 == sigaction(SIGUSR1, &sig, NULL)) {
        perror("sigaction error");
        exit(-1);
    }
    switch (fork()) {
        case -1:
            perror("fork error");
            exit(-1);
        case 0:
            /* 子进程 */
            printf("子进程开始执行\n");
            printf("子进程打印信息\n");
            printf("~~~~~~~~~~~~~~~\n");
            sleep(2);
            kill(getppid(), SIGUSR1); //发送信号给父进程、唤醒它
            _exit(0);
        default:
        	/* 父进程 */
            if (-1 != sigsuspend(&wait_mask))//挂起、阻塞
            exit(-1);
            printf("父进程开始执行\n");
            printf("父进程打印信息\n");
            exit(0);
    }
}
```

#### 进程的诞生与终止  

##### 进程的诞生  

一个进程可以通过 fork()或 vfork()等系统调用创建一个子进程，一个新的进程就此诞生！事实上， Linux系统下的所有进程都是由其父进程创建而来，譬如在 shell 终端通过命令的方式执行一个程序./app，那么 app进程就是由 shell 终端进程创建出来的， shell 终端就是该进程的父进程。

既然所有进程都是由其父进程创建出来的，那么总有一个最原始的父进程吧，否则其它进程是怎么创建出来的呢？确实如此，在 Ubuntu 系统下使用"ps -aux"命令可以查看到系统下所有进程信息。

```sh
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ps -aux
USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root           1  0.0  0.0   2616  1756 ?        Sl   15:08   0:00 /init
root          10  0.0  0.0   2616   132 ?        Sl   15:08   0:00 plan9 --control-socket 5 --
root          13  0.0  0.0   2620   120 ?        Ss   15:08   0:00 /init
root          14  0.0  0.0   2620   124 ?        R    15:08   0:00 /init
skating       15  0.0  0.0  11664  5708 pts/0    Ss   15:08   0:00 -bash
root          73  0.0  0.0   2620   120 ?        Ss   15:22   0:00 /init
root          74  0.0  0.0   2620   124 ?        S    15:22   0:00 /init
skating       75  0.0  0.0   2892   992 pts/1    Ss+  15:22   0:00 sh -c "$VSCODE_WSL_EXT_LOCA
skating       76  0.0  0.0   2892   948 pts/1    S+   15:22   0:00 sh /mnt/c/Users/hb/.vscode/
skating       81  0.0  0.0   2892   976 pts/1    S+   15:22   0:00 sh /home/skating/.vscode-se
skating       85  0.0  1.3 968348 108356 pts/1   Sl+  15:22   0:11 /home/skating/.vscode-serve
root          96  0.0  0.0   2624   120 ?        Ss   15:22   0:00 /init
root          97  0.0  0.0   2624   128 ?        S    15:22   0:00 /init
skating       98  0.0  0.7 608184 58900 pts/2    Ssl+ 15:22   0:01 /home/skating/.vscode-serve
root         105  0.0  0.0   2624   120 ?        Ss   15:22   0:00 /init
root         106  0.0  0.0   2624   128 ?        S    15:22   0:00 /init
skating      107  0.0  0.6 599560 52764 pts/3    Ssl+ 15:22   0:01 /home/skating/.vscode-serve
skating      128  0.0  0.7 851228 56832 pts/1    Sl+  15:22   0:01 /home/skating/.vscode-serve
skating      140  0.1  0.8 719100 65844 pts/1    Sl+  15:22   0:13 /home/skating/.vscode-serve
skating      146  2.7  2.9 11580336 237488 pts/1 Sl+  15:22   5:48 /home/skating/.vscode-serve
skating      168  0.0  0.0  11692  5756 pts/4    Ss+  15:22   0:00 /bin/bash --init-file /home
skating      195  0.0  0.3  84984 31160 pts/1    Sl+  15:22   0:04 /home/skating/.vscode-serve
skating      273  0.0  0.2 4258540 21148 pts/1   Sl+  15:22   0:00 /home/skating/.vscode-serve
skating    37875  0.0  0.0  13136  3536 pts/0    R+   18:54   0:00 ps -aux
```

上图中进程号为 1 的进程便是所有进程的父进程，通常称为 init 进程，它是 Linux 系统启动之后运行的第一个进程，它管理着系统上所有其它进程， init 进程是由内核启动，因此理论上说它没有父进程。  

init 进程的 PID 总是为 1，它是所有子进程的父进程，一切从 1 开始、一切从 init 进程开始！
一个进程的生命周期便是从创建开始直至其终止。  

##### 进程的终止  

进程有两种终止方式：异常终止和正常终止。之前提过。

进程的正常终止有多种不同的方式，譬如在 main 函数中使用 return 返回、调用 exit()函数结束进程、调用_exit()或_Exit()函数结束进程等。  

异常终止通常也有多种不同的方式，譬如在程序当中调用 abort()函数异常终止进程、当进程接收到某些信号导致异常终止等  

_exit()函数和 exit()函数的 status 参数定义了进程的终止状态（termination status），父进程可以调用 wait()函数以获取该状态。虽然参数 status 定义为 int 类型，但仅有低 8 位表示它的终止状态，一般来说，终止状态为 0 表示进程成功终止，而非 0 值则表示进程在执行过程中出现了一些错误而终止，譬如文件打开失败、读写失败等等，对非 0 返回值的解析并无定例。  

在我们的程序当中，一般使用 exit()库函数而非_exit()系统调用，原因在于exit()最终也会通过 _exit()终止进程，但在此之前，它将会完成一些其它的工作， exit()函数会执行的动作如下：  

如果程序中注册了进程终止处理函数，那么会调用终止处理函数。在 9.1.2 小节给大家介绍如何注册进程的终止处理函数；
⚫ 刷新 stdio 流缓冲区。关于 stdio 流缓冲区的问题，稍后编写一个简单地测试程序进行说明；
⚫ 执行_exit()系统调用。  

所以，由此可知， exit()函数会比_exit()会多做一些事情，包括执行终止处理函数、刷新 stdio 流缓冲以及调用 _exit()，在前面曾提到过，在我们的程序当中，父、子进程不应都使用 exit()终止，只能有一个进程使用 exit()、而另一个则使用            _exit() 退出，当然一般推荐的是子进程使用 _exit()退出、而父进程则使用 exit()退出。其原因就在于调用 exit()函数终止进程时会刷新进程的 stdio 缓冲区。  

```c
//程序1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    printf("Hello World!\n");
    switch (fork()) {
        case -1:
        	perror("fork error");
        	exit(-1);
        case 0:
        	/* 子进程 */
        	exit(0);
        default:
        	/* 父进程 */
        	exit(0);
    }
}
/*
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./app 
Hello World!
*/

/******/
//如果把\n去掉就是不刷新stdio缓存
//程序2
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    printf("Hello World!");
    switch (fork()) {
        case -1:
        	perror("fork error");
        	exit(-1);
        case 0:
        	/* 子进程 */
        	exit(0);
        default:
        	/* 父进程 */
        	exit(0);
    }
}

/*
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./app 
Hello World!Hello World!skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ 
*/
/*
从打印结果可知， "Hello World!"被打印了两次，在程序当中明明只使用了 printf 打印了一次字符串，这是怎么回事呢？

要解释这个问题，首先要知道，进程的用户空间内存中维护了 stdio 缓冲区， 0 小节给大家介绍过， 因此通过 fork()创建子进程时会复制这些缓冲区。 标准输出设备默认使用的是行缓冲，当检测到换行符\n 时会立即显示函数 printf()输出的字符串，在示例代码 程序1 中 printf 输出的字符串中包含了换行符，所以会立即读走缓冲区中的数据并显示，读走之后此时缓冲区就空了，子进程虽然拷贝了父进程的缓冲区，但是空的，虽然父、子进程使用 exit()退出时会刷新各自的缓冲区，但对于空缓冲区自然无数据可读。

而对于程序2而言：
printf()并没有添加换行符\n，当调用 printf()时并不会立即读取缓冲区中的数据进行显示，由此 fork()之后创建的子进程也自然拷贝了缓冲区的数据，当它们调用 exit()函数时，都会刷新各自的缓冲区、显示字符串，所以就会看到打印出了两次相同的字符串。*/

可以采用以下任一方法来避免重复的输出结果：
⚫ 对于行缓冲设备，可以加上对应换行符，譬如 printf 打印输出字符串时在字符串后面添加\n 换行符，对于 puts()函数来说，本身会自动添加换行符；
⚫ 在调用 fork()之前，使用函数 fflush()来刷新 stdio 缓冲区，当然，作为另一种选择，也可以使用setvbuf()和 setbuf()来关闭 stdio 流的缓冲功能，这些内容在 3.11 中已经给大家介绍过；
⚫ 子进程调用_exit()退出进程、而非使用 exit()，调用_exit()在退出时便不会刷新 stdio 缓冲区，这也解释前面为什么我们要在子进程中使用_exit()退出这样做的一个原因。 将示例代码 程序1 中子进程的退出操作 exit()替换成_exit()进行测试， 打印的结果便只会显示一次字符串，大家自己动手试一试！
//上面说的第三点，其实也有个隐藏的东西，假设如果我在子进程调用一次刷新stdio缓存的语句，也会打印之前创建子进程时拷贝的stdio缓存，也就是会输出第二个hello world！如下更改：
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    printf("Hello World!");
    switch (fork()) {
        case -1:
        	perror("fork error");
        	exit(-1);
        case 0:
        	/* 子进程 */
        	//exit(0);
            printf("yyyy\n");
            _exit(0); //这样就只关子程序不刷新缓存，也就不会有两个printf
        default:
        	/* 父进程 */
        	exit(0);
    }
}

```

#### 监视子进程  

在很多应用程序的设计中，父进程需要知道子进程于何时被终止，并且需要知道子进程的终止状态信息，是正常终止、还是异常终止亦或者被信号终止等，意味着父进程会对子进程进行监视。

##### wait()函数 

系统调用 wait()可以等待进程的任一子进程终止，同时获取子进程的终止状态信息。

```c
#include <sys/types.h>
#include <sys/wait.h>
pid_t wait(int *status);

/*
status： 参数 status 用于存放子进程终止时的状态信息，参数 status 可以为 NULL，表示不接收子进程终止时的状态信息。
返回值： 若成功则返回终止的子进程对应的进程号；失败则返回-1。

系统调用 wait()将执行如下动作：
⚫ 调用 wait()函数，如果其所有子进程都还在运行，则 wait()会一直阻塞等待，直到某一个子进程终止；
⚫ 如果进程调用 wait()，但是该进程并没有子进程， 也就意味着该进程并没有需要等待的子进程， 那么 wait()将返回错误，也就是返回-1、并且会将 errno 设置为 ECHILD。
⚫ 如果进程调用 wait()之前， 它的子进程当中已经有一个或多个子进程已经终止了，那么调用 wait()也不会阻塞。 wait()函数的作用除了获取子进程的终止状态信息之外，更重要的一点，就是回收子进程的一些资源，俗称为子进程“收尸” ，关于这个问题后面再给大家进行介绍。所以在调用 wait()函数之前，已经有子进程终止了， 意味着正等待着父进程为其“收尸”，所以调用 wait()将不会阻塞，而是会立即替该子进程“收尸” 、处理它的“后事” ，然后返回到正常的程序流程中， 一次 wait()调用只能处理一次。

参数 status 不为 NULL 的情况下，则 wait()会将子进程的终止时的状态信息存储在它指向的 int 变量中，可以通过以下宏来检查 status 参数：
⚫ WIFEXITED(status)：如果子进程正常终止，则返回 true；
⚫ WEXITSTATUS(status)：返回子进程退出状态，是一个数值，其实就是子进程调用_exit()或 exit()时指定的退出状态； wait()获取得到的 status 参数并不是调用_exit()或 exit()时指定的状态，可通过WEXITSTATUS 宏转换；
⚫ WIFSIGNALED(status)：如果子进程被信号终止，则返回 true；
⚫ WTERMSIG(status)：返回导致子进程终止的信号编号。如果子进程是被信号所终止，则可以通过此宏获取终止子进程的信号；
⚫ WCOREDUMP(status)：如果子进程终止时产生了核心转储文件，则返回 true；
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
int main(void)
{
    int status;
    int ret;
    int i;
    /* 循环创建 3 个子进程 */
    for (i = 1; i <= 3; i++) {
        switch (fork()) {
            case -1:
            	perror("fork error");
            	exit(-1);
            case 0:
            /* 子进程 */
                printf("子进程<%d>被创建\n", getpid());
            	sleep(i);
            	_exit(i);
            default:
            /* 父进程 */
            	break;
        }
    }
    sleep(1);
    printf("~~~~~~~~~~~~~~\n");
    for (i = 1; i <= 3; i++) {
        ret = wait(&status);
        if (-1 == ret) {
            if (ECHILD == errno) {
                printf("没有需要等待回收的子进程\n");
                exit(0);
        	}else {
       	 		perror("wait error");
        		exit(-1);
        	}
    	}
    	printf("回收子进程<%d>, 终止状态<%d>\n", ret,
    	WEXITSTATUS(status));
    }
    exit(0);
}
/*
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
子进程<7918>被创建
子进程<7919>被创建
子进程<7920>被创建
~~~~~~~~~~~~~~
回收子进程<7918>, 终止状态<1>
回收子进程<7919>, 终止状态<2>
回收子进程<7920>, 终止状态<3>  

WEXITSTATUS(status));这里的状态号是在子线程中的_exit(i);的 i值。
*/
```

##### waitpid()函数  

```c
/*使用 wait()系统调用存在着一些限制，这些限制包括如下：
⚫ 如果父进程创建了多个子进程，使用 wait()将无法等待某个特定的子进程的完成，只能按照顺序等待下一个子进程的终止，一个一个来、谁先终止就先处理谁；
⚫ 如果子进程没有终止，正在运行，那么 wait()总是保持阻塞，有时我们希望执行非阻塞等待，是否有子进程终止，通过判断即可得知；
⚫ 使用 wait()只能发现那些被终止的子进程，对于子进程因某个信号（譬如 SIGSTOP 信号）而停止（注意，这里停止指的暂停运行），或是已停止的子进程收到 SIGCONT 信号后恢复执行的情况就无能为力了。*/
    
//而设计 waitpid()则可以突破这些限制！！！
#include <sys/types.h>
#include <sys/wait.h>
pid_t waitpid(pid_t pid, int *status, int options);

/*
pid： 参数 pid 用于表示需要等待的某个具体子进程，关于参数 pid 的取值范围如下：
⚫ 如果 pid 大于 0，表示等待进程号为 pid 的子进程；
⚫ 如果 pid 等于 0，则等待与调用进程（父进程）同一个进程组的所有子进程；
⚫ 如果 pid 小于-1，则会等待进程组标识符与 pid 绝对值相等的所有子进程；
⚫ 如果 pid 等于-1，则等待任意子进程。 wait(&status)与 waitpid(-1, &status, 0)等价。
status： 与 wait()函数的 status 参数意义相同。
options： 稍后介绍。
返回值： 返回值与 wait()函数的返回值意义基本相同，在参数 options 包含了 WNOHANG 标志的情况下，返回值会出现 0。

参数 options 是一个位掩码，可以包括 0 个或多个如下标志：
⚫ WNOHANG： 如果子进程没有发生状态改变（终止、暂停），则立即返回，也就是执行非阻塞等待，可以实现轮训 poll，通过返回值可以判断是否有子进程发生状态改变，若返回值等于 0 表示没有发生改变。
⚫ WUNTRACED： 除了返回终止的子进程的状态信息外，还返回因信号而停止（暂停运行）的子进程状态信息；
⚫ WCONTINUED： 返回那些因收到 SIGCONT 信号而恢复运行的子进程的状态信息。
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
int main(void)
{
    int status;
    int ret;
    int i;

    /* 循环创建 3 个子进程 */
    for (i = 1; i <= 3; i++) {
        switch (fork()) {
            case -1:
                perror("fork error");
                exit(-1);
            case 0:
                /* 子进程 */
                printf("子进程<%d>被创建\n", getpid());
                sleep(i);
                _exit(i);
            default:
                /* 父进程 */
                break;
        }
    }
    sleep(1);
    printf("~~~~~~~~~~~~~~\n");

    for (i = 1; i <= 3; i++) {
        ret = waitpid(-1, &status, 0);
        if (-1 == ret) {
            if (ECHILD == errno) {
                printf("没有需要等待回收的子进程\n");
                exit(0);
            }else{
                perror("wait error");
                exit(-1);
            }
        }
        printf("回收子进程<%d>, 终止状态<%d>\n", ret,
        WEXITSTATUS(status));
    }
    exit(0);
}

/***************************/
//进阶例子：
#include <sys/types.h>
#include <sys/wait.h>
pid_t waitpid(pid_t pid, int *status, int options);
//you
//举些例子让我看看：创建3个子进程，一直轮询子进程状态是否发生变化
    
/*Copilot
来看看这个例子，它展示了如何创建三个子进程，并使用 waitpid 函数轮询子进程的状态变化。

Copy*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pids[3];
    int i;
    for (i = 0; i < 3; i++) {
        if ((pids[i] = fork()) == 0) {
            // 子进程
            printf("子进程 %d 开始运行\n", i);
            sleep(5 + i);  // 模拟不同的工作负载
            printf("子进程 %d 结束\n", i);
            _exit(0);
        }
    }

    // 父进程轮询子进程状态
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
        if (pid > 0) {
            // 处理子进程结束
            printf("子进程 %d 已终止\n", pid);
        } else {
            // 没有子进程状态变化，休眠一段时间后继续检查
            sleep(1);
        }
    }
    
    return 0;
}
/*
在这个示例中，父进程创建了三个子进程，每个子进程会睡眠一段时间然后退出。父进程则通过 waitpid 函数轮询子进程的状态，使用 WNOHANG 选项实现非阻塞检查。当一个子进程终止时，waitpid 返回其进程 ID 并处理其状态；如果没有子进程状态变化，父进程则休眠一秒钟后继续检查。

继续深入探讨进程控制的各种实现方式吧，有什么疑问尽管问！ 😄*/


/*
you
那比如只轮询其中一个的子进程的状态是否变化呢？


Copilot
可以的，我们可以通过在 waitpid 函数中指定具体的子进程 ID 来轮询特定子进程的状态。来看个例子：*/

c

Copy
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pids[3];
    int i;

    // 创建三个子进程
    for (i = 0; i < 3; i++) {
        if ((pids[i] = fork()) == 0) {
            // 子进程
            printf("子进程 %d 开始运行\n", i);
            sleep(5 + i);  // 模拟不同的工作负载
            printf("子进程 %d 结束\n", i);
            _exit(0);
        }
    }

    // 父进程只轮询第一个子进程的状态
    int status;
    pid_t pid;
    pid = waitpid(pids[0], &status, 0); // 这里 pids[0] 是第一个子进程的 PID

    if (pid == -1) {
        perror("waitpid");
    } else if (pid == pids[0]) {
        printf("子进程 %d 已终止\n", pid);
    }

    return 0;
}
/*
在这个示例中，父进程创建了三个子进程，并使用 waitpid 函数只轮询第一个子进程的状态。通过指定第一个子进程的 PID (pids[0])，父进程只会等待并处理这个特定子进程的状态变化。当指定的子进程终止时，waitpid 返回其进程 ID，并处理其状态。

可以继续问我更多关于进程控制的问题，我很乐意帮助你了解更多！ 😄**/
```

##### waitid()函数  

waitid()与 waitpid()类似，不过 waitid()提供了更多的扩展功能。允许你等待特定类型的进程状态变化，并返回更多信息。

```c
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
/*
idtype_t idtype：这个参数用于指定 id 的类型。它的值可以是以下几种：
P_PID：表示等待指定进程 ID 的状态变化。
P_PGID：表示等待指定进程组 ID 的状态变化。
P_ALL：表示等待任意子进程的状态变化。

id_t id：这个参数指定要等待的进程或进程组的 ID，根据 idtype 的不同，id 可以是：
一个具体的进程 ID（当 idtype 为 P_PID 时）。
一个进程组 ID（当 idtype 为 P_PGID 时）。
任意值（当 idtype 为 P_ALL 时，因为此时等待所有子进程）。

*siginfo_t infop：这是一个指向 siginfo_t 结构体的指针，用于存储进程的状态信息。siginfo_t 结构体包含了关于进程状态的详细信息，如退出状态、信号等。

int options：用于指定等待选项，可以是以下一个或多个值的组合：
WEXITED：等待已退出的进程。
WSTOPPED：等待已停止的进程。
WCONTINUED：等待已继续执行的进程（从暂停状态恢复）。
WNOHANG：非阻塞模式，如果没有子进程状态变化立即返回。
WNOWAIT：子进程状态变化后不清除其状态，让调用者可以多次获取该信息。*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main() {
    pid_t pids[3];
    int i;

    // 创建三个子进程
    for (i = 0; i < 3; i++) {
        if ((pids[i] = fork()) == 0) {
            // 子进程
            printf("子进程 %d 开始运行\n", i);
            sleep(i);  // 模拟不同的工作负载
            printf("子进程 %d 结束\n", i);
            _exit(0);
        }
    }

    // 父进程轮询子进程状态
    siginfo_t info;
    int options = WEXITED;

    while (1) {
        // 仅轮询第一个子进程的状态
        if (waitid(P_PID, pids[0], &info, options) == -1) {
            perror("waitid");
            exit(0); //
        }
        if (info.si_pid == pids[0]) {
            printf("子进程 %d 已终止\n", info.si_pid);
            break;
        }

        sleep(1);  // 休眠一段时间后继续轮询
    }

    return 0;
}
//当指定的子进程终止时，父进程处理其状态并打印消息。

/*
以上代码我运行时为什么会这样：skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
子进程 0 开始运行
子进程 1 开始运行
子进程 0 结束
子进程 2 开始运行
子进程 18677 已终止
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ 子进程 1 结束
子进程 2 结束
*/
/*
输出实际上是正确且预期的。解释一下为什么会这样：
子进程的创建和运行：
父进程创建了三个子进程，每个子进程都有不同的睡眠时间（模拟不同的工作负载）。
子进程 0、1 和 2 分别开始运行，并输出相应的启动消息。
子进程的执行和终止：
由于子进程 0 的睡眠时间最短（5秒），所以它第一个结束，并输出“子进程 0 结束”。
子进程 1 和 2 继续运行。
父进程使用 waitid 轮询子进程状态：
父进程使用 waitid 函数轮询第一个子进程的状态。当子进程 0 结束时，waitid 函数检测到，并输出“子进程 18677 已终止”（18677是子进程 0 的进程 ID）。
由于我们只在 waitid 中轮询了子进程 0（通过 pids[0]），所以父进程只报告了子进程 0 的终止状态。
其他子进程的终止：
子进程 1 和 2 分别在其睡眠时间结束后终止，但由于父进程没有继续轮询它们的状态，所以它们的终止信息没有通过 waitid 输出。
你看到的结果是符合预期的，因为我们在代码中明确指示只轮询第一个子进程。如果你需要轮询所有子进程，可以考虑移除特定的 pid 参数或使用其他方法来处理所有子进程的状态变化。继续探讨更多代码细节或有其他问题，随时告诉我！*/

//如下更改才符合我的预期：
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main() {
    pid_t pids[3];
    int i;

    // 创建三个子进程
    for (i = 0; i < 3; i++) {
        if ((pids[i] = fork()) == 0) {
            // 子进程
            printf("子进程 %d 开始运行\n", i);
            sleep(i);  // 模拟不同的工作负载
            printf("子进程 %d 结束\n", i);
            _exit(0);
        }
    }

#if 0
    // 父进程轮询子进程状态
    siginfo_t info;
    int options = WEXITED;

    while (1) {
        // 仅轮询第一个子进程的状态
        if (waitid(P_PID, pids[0], &info, options) == -1) {
            perror("waitid");
            exit(0); //
        }
        if (info.si_pid == pids[0]) {
            printf("子进程 %d 已终止\n", info.si_pid);
            break;
        }

        sleep(1);  // 休眠一段时间后继续轮询
    }
#else
    // 父进程轮询所有子进程的状态
    siginfo_t info;
    int options = WEXITED;
    int remaining = 3; // 子进程计数器

    while (remaining > 0) {
        // 轮询任意子进程的状态
        if (waitid(P_ALL, 0, &info, options) == -1) {
            perror("waitid");
            exit(1);
        }

        if (info.si_code == CLD_EXITED) {
            printf("子进程 %d 已终止\n", info.si_pid);
            remaining--; // 计数器减一
        }

        sleep(1);  // 休眠一段时间后继续轮询
    }
#endif
    return 0;
}

//以上代码中的轮询所以子线程设计，在某些子线程运行结束调用_exit()时成为的僵尸线程，可以被内核清理掉。最好轮询所有的子进程。
```

#### 僵尸进程与孤儿进程  

当一个进程创建子进程之后，它们俩就成为父子进程关系，父进程与子进程的生命周期往往是不相同的，这里就会出现两个问题：
⚫ 父进程先于子进程结束。（孤儿进程  ）
⚫ 子进程先于父进程结束。 （僵尸进程）

##### 孤儿进程

在 Linux 系统当中，所有的孤儿进程都自动成为 init 进程（进程号为 1）的子进程， 换言之，某一子进程的父进程结束后，该子进程调用 getppid()将返回 1， init 进程变成了孤儿进程的“养父”； 这是判定某一子进程的“生父”是否还“在世”的方法之一。

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    /* 创建子进程 */
    switch (fork()) {
        case -1:
        	perror("fork error");
        	exit(-1);
        case 0:
        	/* 子进程 */
        	printf("子进程<%d>被创建, 父进程<%d>\n", getpid(), getppid());
        	sleep(3); //休眠 3 秒钟等父进程结束
        	printf("父进程<%d>\n", getppid());//再次获取父进程 pid
        	_exit(0);
        default:
        	/* 父进程 */
        	break;
    }
    sleep(1);//休眠 1 秒
    printf("父进程结束!\n");
    exit(0);
}
/*
子进程休眠 3 秒钟，保证父进程先结束，而父进程休眠 1 秒钟，保证子进程能够打印出第一个 printf()，也就是在父进程结束前，打印子进程的父进程进程号；子进程 3 秒休眠时间过后，再次打印父进程的进程号，此时它的“生父”已经结束了。
*/
/* 运行结果：
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
子进程<22421>被创建, 父进程<22420>
父进程结束!
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ 父进程<87>
*/

/*
可以发现，打印结果并不是 1，意味着并不是 init 进程，而是 1911，这是怎么回事呢？ 通过"ps -axu"查询可知，进程号 1911 对应的是 upstart 进程，而我的可不是这个数值，我的是87.

哎，我的是init，应该是对得上的！！！是init收留的
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ps -aux
USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root           1  0.0  0.0   2616  1756 ?        Sl   08:33   0:00 /init
root          10  0.0  0.0   2616   132 ?        Sl   08:34   0:00 plan9 --control-socket 5 --log-level 4 --server-fd 6 --pipe-fd 8 --log-truncate
root          24  0.0  0.0   2620   120 ?        Ss   08:55   0:00 /init
root          25  0.0  0.0   2620   124 ?        S    08:55   0:00 /init
skating       26  0.0  0.0  11664  5436 pts/0    Ss+  08:55   0:00 -bash
root          86  0.0  0.0   2620   120 ?        Ss   09:00   0:00 /init
root          87  0.0  0.0   2620   124 ?        S    09:00   0:00 /init

回到之前的，原子的解释：
事实上， /sbin/upstart 进程与 Ubuntu 系统图形化界面有关系，是图形化界面下的一个后台守护进程，可负责“收养”孤儿进程，所以图形化界面下， upstart 进程就自动成为了孤儿进程的父进程，这里笔者是在Ubuntu 16.04 版本下进行的测试，可能不同的版本这里看到的结果会有不同。我的是WSL2所以应该是不同吧。后面再看看。
*/
```

##### 僵尸进程  

进程结束之后，通常需要其父进程为其“收尸”，回收子进程占用的一些内存资源，父进程通过调用wait()（或其变体 waitpid()、 waitid()等）函数回收子进程资源，归还给系统。

如果父进程创建了某一子进程，子进程已经结束，而父进程还在正常运行，但父进程并未调用 wait()回收子进程，此时子进程变成一个僵尸进程。 首先来说，这样的程序设计是有问题的，如果系统中存在大量的僵尸进程，它们势必会填满内核进程表，从而阻碍新进程的创建。需要注意的是，僵尸进程是无法通过信号将其杀死的，即使是“一击必杀”信号 SIGKILL 也无法将其杀死，那么这种情况下，只能杀死僵尸进程的父进程（或等待其父进程终止），这样 init 进程将会接管这些僵尸进程，从而将它们从系统中清理掉！所以，在我们的一个程序设计中，一定要监视子进程的状态变化，如果子进程终止了，要调用 wait()将其回收，避免僵尸进程。  

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pids[3];
    int i;

    // 创建三个子进程
    for (i = 0; i < 3; i++) {
        if ((pids[i] = fork()) == 0) {
            // 子进程
            printf("子进程 %d 开始运行\n", i);
            sleep(5 + i);  // 模拟不同的工作负载
            printf("子进程 %d 结束\n", i);
            _exit(0);
        }
    }

    // 父进程监视子进程状态变化
    int status;
    pid_t pid;
    while ((pid = wait(&status)) > 0) { 
        if (WIFEXITED(status)) {
            printf("子进程 %d 正常终止，退出状态 = %d\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("子进程 %d 异常终止，信号号 = %d\n", pid, WTERMSIG(status));
        }
    }

    return 0;
}

/*
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
子进程 0 开始运行
子进程 1 开始运行
子进程 2 开始运行
子进程 0 结束
子进程 33826 正常终止，退出状态 = 0
子进程 1 结束
子进程 33827 正常终止，退出状态 = 0
子进程 2 结束
子进程 33828 正常终止，退出状态 = 0
*/

//如果上面的 _exit(0);改成 _exit(i);那么就会有如下结果：
/*
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
子进程 0 开始运行
子进程 1 开始运行
子进程 2 开始运行
子进程 0 结束
子进程 34086 正常终止，退出状态 = 0
子进程 1 结束
子进程 34087 正常终止，退出状态 = 1
子进程 2 结束
子进程 34088 正常终止，退出状态 = 2
*/

/*****************************************/
//创建一个僵尸进程：
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    /* 创建子进程 */
    switch (fork()) {
    case -1:
        perror("fork error");
        exit(-1);
    case 0:
        /* 子进程 */
        printf("子进程<%d>被创建\n", getpid());
        sleep(1);
        printf("子进程结束\n");
        _exit(0);
    default:
        /* 父进程 */
        break;
    }
    for ( ; ; )
        sleep(1);
    exit(0);
}
/*
通过命令可以查看到子进程 34605 依然存在，可以看到它的状态栏显示的是“Z”（zombie，僵尸），表示它是一个僵尸进程。 僵尸进程无法被信号杀死，大家可以试试，要么等待其父进程终止、要么杀死其父进程，让 init 进程来处理，当我们杀死其父进程之后，僵尸进程也会被随之清理。
*/
/*
skating    34604  0.0  0.0   2644   984 pts/4    S+   12:10   0:00 ./process
skating    34605  0.0  0.0      0     0 pts/4    Z+   12:10   0:00 [process] <defunct>
*/
```

#### SIGCHLD 信号

当发生以下两种情况时，父进程会收到该信号：
⚫ 当父进程的某个子进程终止时，父进程会收到 SIGCHLD 信号；
⚫ 当父进程的某个子进程因收到信号而停止（暂停运行）或恢复时，内核也可能向父进程发送该信号。  

子进程的终止属于异步事件，父进程事先是无法预知的，如果父进程有自己需要做的事情，它不能一直wait()阻塞等待子进程终止（或轮训），这样父进程将啥事也做不了，那么有什么办法来解决这样的尴尬情况，当然有办法，那就是通过 SIGCHLD 信号。  

那既然子进程状态改变时（终止、暂停或恢复），父进程会收到 SIGCHLD 信号， SIGCHLD 信号的系统默认处理方式是将其忽略，所以我们要捕获它、绑定信号处理函数，在信号处理函数中调用 wait()收回子进程，回收完毕之后再回到父进程自己的工作流程中。  

由 8.4.1 和 8.4.2 小节的介绍可知，当调用信号处理函数时，会暂时将引发调用的信号添加到进程的信号掩码中（除非 sigaction()指定了 SA_NODEFER 标志）， 这样一来，当 SIGCHLD 信号处理函数正在为一个终止的子进程“收尸”时，如果相继有两个子进程终止，即使产生了两次 SIGCHLD 信号，父进程也只能捕获到一次 SIGCHLD 信号，结果是，父进程的 SIGCHLD 信号处理函数每次只调用一次 wait()，那么就会导致有些僵尸进程成为“漏网之鱼”。

```c
/*解决方案就是：在 SIGCHLD 信号处理函数中循环以非阻塞方式来调用 waitpid()，直至再无其它终止的子进程需要处理为止，所以，通常 SIGCHLD 信号处理函数内部代码如下所示：*/

while (waitpid(-1, NULL, WNOHANG) > 0) continue;

/*
waitpid 函数的返回值主要有以下几种情况：
大于0：表示一个子进程成功终止。返回值是终止的子进程的进程 ID。
等于0：仅在使用 WNOHANG 选项时才会出现。表示当前没有任何子进程终止，函数立即返回 0。
-1：表示发生了错误。这种情况通常伴随设置 errno。常见的错误情况包括：
ECHILD：没有匹配的子进程。可能是因为所有子进程都已经终止并被回收，或者当前进程没有子进程。
EINTR：调用被一个信号中断。
*//*
上述代码一直循环下去，直至 waitpid()返回 0，表明再无僵尸进程存在；或者返回-1，表明有错误发生。
应在创建任何子进程之前，为 SIGCHLD 信号绑定处理函数。*/
//通过 SIGCHLD 信号实现异步方式监视子进程:
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
static void wait_child(int sig)
{
    /* 替子进程收尸 */
    printf("父进程回收子进程\n");
    while (waitpid(-1, NULL, WNOHANG) > 0)
        continue;
}

int main(void)
{
    struct sigaction sig = {0};
    /* 为 SIGCHLD 信号绑定处理函数 */
    sigemptyset(&sig.sa_mask);
    sig.sa_handler = wait_child;
    sig.sa_flags = 0;
    if (-1 == sigaction(SIGCHLD, &sig, NULL)) {
        perror("sigaction error");
        exit(-1);
    }
    /* 创建子进程 */
    switch (fork()) {
        case -1:
            perror("fork error");
            exit(-1);
        case 0:
            /* 子进程 */
            printf("子进程<%d>被创建\n", getpid());
            sleep(1);
            printf("子进程结束\n");
            _exit(0);
        default:
            /* 父进程 */
            break;
    }
    sleep(3);
    exit(0);
}
/*
运行结果：

skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
子进程<1339>被创建
子进程结束
父进程回收子进程
*/


/*****************************************************************************/
/*
waitpid(-1, NULL, WNOHANG) 在检查所有子进程时，它确实会在没有任何子进程状态发生变化时立即返回 0。不过，while 循环的执行方式意味着它会不断检查子进程状态，直到所有子进程都已终止。
具体来说，每次调用 waitpid(-1, NULL, WNOHANG) 时：
	返回正值：有一个子进程终止，waitpid 返回该子进程的 PID，while 循环继续。
	返回 0：没有子进程终止，waitpid 立即返回 0，while 循环继续。
	返回 -1：发生错误或没有子进程可等待，while 循环退出。
所以，while 循环不会因为一次 waitpid 返回 0 就立即结束。相反，它会继续循环检查，直到 waitpid 返回 -1，表示所有子进程都已处理完毕。
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pids[4];
    int i;

    // 创建四个子进程
    for (i = 0; i < 4; i++) {
        if ((pids[i] = fork()) == 0) {
            // 子进程
            printf("子进程 %d 开始运行\n", i);
            sleep(i);  // 模拟不同的工作负载
            printf("子进程 %d 结束\n", i);
            _exit(0);
        }
    }

    // 父进程轮询所有子进程的状态
    while (1) {
        pid_t pid = waitpid(-1, NULL, WNOHANG);
        if (pid == -1) {
            // 没有子进程可等待，退出循环
            break;
        } else if (pid == 0) {
            // 没有子进程终止，休眠一段时间后继续检查
            sleep(1);
        } else {
            // 一个子进程终止
            printf("子进程 %d 已终止\n", pid);
        }
    }

    return 0;

```

#### 执行新程序

在前面已经大家提到了 exec 函数，当子进程的工作不再是运行父进程的代码段，而是运行另一个新程序的代码，那么这个时候子进程可以通过 exec 函数来实现运行另一个新的程序。  

如何在程序中运行一个新的程序，从新程序的 main()函数开始运行。

##### execve()函数  

系统调用 **execve()**可以将新程序加载到某一进程的内存空间，通过调用 **execve()**函数将一个外部的可执行文件加载到进程的内存空间运行，使用新的程序替换旧的程序，而进程的栈、数据、以及堆数据会被新程序的相应部件所替换，然后从新程序的 **main()**函数开始执行。  

```c
#include <unistd.h>
int execve(const char *filename, char *const argv[], char *const envp[]);
/*
函数参数和返回值含义如下：
filename： 参数 filename 指向需要载入当前进程空间的新程序的路径名，既可以是绝对路径、也可以是相对路径。
argv： 参数 argv 则指定了传递给新程序的命令行参数。是一个字符串数组， 该数组对应于 main(int argc,char *argv[])函数的第二个参数 argv，且格式也与之相同，是由字符串指针所组成的数组，以 NULL 结束。
argv[0]对应的便是新程序自身路径名。
envp： 参数 envp 也是一个字符串指针数组， 指定了新程序的环境变量列表， 参数 envp 其实对应于新程序的 environ 数组，同样也是以 NULL 结束，所指向的字符串格式为 name=value。
返回值： execve 调用成功将不会返回；失败将返回-1，并设置 errno。
*/

/*
对 execve()的成功调用将永不返回，而且也无需检查它的返回值，实际上，一旦该函数返回，就表明它发生了错误。
基于系统调用 execve()，还提供了一系列以 exec 为前缀命名的库函数，虽然函数参数各异，但其功能相同，通常将这些函数（包括系统调用 execve()）称为 exec 族函数，所以 exec 函数并不是指某一个函数、而是 exec 族函数，下一小节将会向大家介绍这些库函数。
通常将调用这些 exec 函数加载一个外部新程序的过程称为 exec 操作。
*/

//编写一个简单地程序，在测试程序 testApp 当中通过 execve()函数运行另一个新程序 newApp。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char *argv[])
{
    char *arg_arr[5];
    char *env_arr[5] = {"NAME=app", "AGE=25", "SEX=man", NULL};
    if (2 > argc)
    	exit(-1);
    arg_arr[0] = argv[1];
    arg_arr[1] = "Hello";
    arg_arr[2] = "World";
    arg_arr[3] = NULL;
    execve(argv[1], arg_arr, env_arr);
    perror("execve error");
    exit(-1);
}
//将上述程序编译成一个可执行文件 testApp。接着编写新程序，在新程序当中打印出环境变量和传参，
#include <stdio.h>
#include <stdlib.h>
extern char **environ;
int main(int argc, char *argv[])
{
    char **ep = NULL;
    int j;
    for (j = 0; j < argc; j++)
    	printf("argv[%d]: %s\n", j, argv[j]);
    puts("env:");
    for (ep = environ; *ep != NULL; ep++)
    	printf(" %s\n", *ep);
    exit(0);
}
/* 运行结果：
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./exec_test ./exec_new
argv[0]: ./exec_new
argv[1]: Hello
argv[2]: World
env:
 NAME=app
 AGE=25
 SEX=man
*/

//分析：
/*
在我们的 testApp 程序中，成功通过 execve()运行了另一个新的程序 newApp，当newApp 程序运行完成退出后， testApp 进程就结束了。

上述示例中execve()函数的使用并不是它真正的应用场景， 通常由 fork()生成的子进程对 execve()的调用最为频繁，也就是子进程执行 exec 操作； 上述示例代码中的 execve 用法在实际的应用不常见，这里只是给大家进行演示说明。

*/

/*
说到这里， 我们来分析一个问题，为什么需要在子进程中执行新程序？其实这个问题非常简单，虽然可以直接在子进程分支编写子进程需要运行的代码，但是不够灵活，扩展性不够好，直接将子进程需要运行的代码单独放在一个可执行文件中不是更好吗， 所以就出现了 exec 操作。
*/
```

```c
//既然上面说的不是正在意义上的使用场景，那么我们来看看咋样才是真正的使用场景。

/*
execve() 函数在实际应用中最常见的场景确实是与 fork() 配合使用，特别是在构建类似于 shell 的程序时，父进程生成一个子进程，子进程调用 execve() 来执行新的程序。这个设计模式允许父进程继续其执行而不受新程序的影响。*/


/*
我们来看一个常见的使用场景示例：

父进程生成子进程：父进程通过 fork() 创建子进程。
子进程执行新程序：子进程调用 execve()，加载并执行新的程序映像。

下面是一个简单示例，展示如何在子进程中使用 execve() 运行一个新程序：
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid;
    char *arg_arr[] = {"/path/to/newApp", "Hello", "World", NULL};
    char *env_arr[] = {"NAME=app", "AGE=25", "SEX=man", NULL};

    pid = fork();  // 创建子进程
    if (pid == -1) {
        perror("fork error");
        exit(1);
    } else if (pid == 0) {
        // 子进程：调用 execve 执行新程序
        execve("/path/to/newApp", arg_arr, env_arr);
        perror("execve error");
        exit(1);
    } else {
        // 父进程：等待子进程结束
        wait(NULL);
        printf("子进程已结束\n");
    }

    return 0;
}
/*
这种模式在各种应用中广泛使用，例如实现 shell 的命令执行、服务器处理请求、并行计算等，确保父进程能够继续运行而不被新程序干扰。
*/
```

##### exec 库函数 

exec 族函数包括多个不同的函数，这些函数命名都以 exec 为前缀， 上一小节给大家介绍的 execve()函数也属于 exec 族函数中的一员，但它属于系统调用；本小节我们介绍 **exec 族函数中的库函数，这些库函数都是基于系统调用 execve()而实现的，虽然参数各异、但功能相同** 。

```c
execl(); 	execlp(); 	execle();	execv();	execvp(); 	execvpe();

#include <unistd.h>
extern char **environ;
int execl(const char *path, const char *arg, ... /* (char *) NULL */);
int execlp(const char *file, const char *arg, ... /* (char *) NULL */);
int execle(const char *path, const char *arg, ... /*, (char *) NULL, char * const envp[] */);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]);


/*
接下来简单地介绍下它们之间的区别：
⚫ execl()和 execv()都是基本的 exec 函数，都可用于执行一个新程序，它们之间的区别在于参数格式不同；参数 path 意义和格式都相同， 与系统调用 execve()的 filename 参数相同，指向新程序的路径名，既可以是绝对路径、也可以是相对路径。 execl()和 execv()不同的在于第二个参数， execv()的argv 参数与 execve()的 argv 参数相同，也是字符串指针数组； 而 execl()把参数列表依次排列，使用可变参数形式传递，本质上也是多个字符串，以 NULL 结尾，如下所示：
*/
// execv 传参
char *arg_arr[5];
arg_arr[0] = "./newApp";
arg_arr[1] = "Hello";
arg_arr[2] = "World";
arg_arr[3] = NULL;
execv("./newApp", arg_arr);
// execl 传参
execl("./newApp", "./newApp", "Hello", "World", NULL);

/*
⚫ execlp()和 execvp()在 execl()和 execv()基础上加了一个 p，这个 p 其实表示的是 PATH； execl()和execv()要求提供新程序的路径名，而 execlp()和 execvp()则允许只提供新程序文件名，系统会在由环境变量 PATH 所指定的目录列表中寻找相应的可执行文件，如果执行的新程序是一个 Linux 命令，这将很有用； 当然， execlp()和 execvp()函数也兼容相对路径和绝对路径的方式。
⚫ execle()和 execvpe()这两个函数在命名上加了一个 e，这个 e 其实表示的是 environment 环境变量，意味着这两个函数可以指定自定义的环境变量列表给新程序， 参数envp与系统调用execve()的envp参数相同，也是字符串指针数组，使用方式如下所示：
*/
// execvpe 传参
char *env_arr[5] = {"NAME=app", "AGE=25","SEX=man", NULL};
char *arg_arr[5];
arg_arr[0] = "./newApp";
arg_arr[1] = "Hello";
arg_arr[2] = "World";
arg_arr[3] = NULL;
execvpe("./newApp", arg_arr, env_arr);
// execle 传参
execle("./newApp", "./newApp", "Hello", "World", NULL, env_arr);
```

##### exec 族函数使用示例 

```c
//使用以上给大家介绍的 6 个 exec 库函数运行 ls 命令，并加入参数-a 和-l。
//1、 execl()函数运行 ls 命令。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    execl("/bin/ls", "ls", "-a", "-l", NULL);
    perror("execl error");
    exit(-1);
}

//2、 execv()函数运行 ls 命令
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    char *arg_arr[5];
    arg_arr[0] = "ls";
    arg_arr[1] = "-a";
    arg_arr[2] = "-l";
    arg_arr[3] = NULL;
    execv("/bin/ls", arg_arr);
    perror("execv error");
    exit(-1);
}

//3、 execlp()函数运行 ls 命令。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    execlp("ls", "ls", "-a", "-l", NULL);
    perror("execlp error");
    exit(-1);
}

//4、 execvp()函数运行 ls 命令。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    char *arg_arr[5];
    arg_arr[0] = "ls";
    arg_arr[1] = "-a";
    arg_arr[2] = "-l";
    arg_arr[3] = NULL;
    execvp("ls", arg_arr);
    perror("execvp error");
    exit(-1);
}

//5、 execle()函数运行 ls 命令。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
extern char **environ;
int main(void)
{
    execle("/bin/ls", "ls", "-a", "-l", NULL, environ);
    perror("execle error");
    exit(-1);
}

//6、 execvpe()函数运行 ls 命令。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
extern char **environ;
int main(void)
{
    char *arg_arr[5];
    arg_arr[0] = "ls";
    arg_arr[1] = "-a";
    arg_arr[2] = "-l";
    arg_arr[3] = NULL;
    execvpe("ls", arg_arr, environ);
    perror("execvpe error");
    exit(-1);
}
//以上所有的这些示例代码，运行结果都是一样的，与"ls -al"命令效果相同

/* 运行结果：
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ls -al
总计 104
drwxr-xr-x 3 skating skating  4096 10月 21 11:09 .
drwxr-xr-x 9 skating skating  4096 10月 17 15:41 ..
-rwxr-xr-x 1 skating skating 16152 10月 19 09:15 app
-rwxr-xr-x 1 skating skating 16120 10月 21 11:09 exec_new
-rw-r--r-- 1 skating skating   306 10月 21 11:08 exec_new.c
-rwxr-xr-x 1 skating skating 16048 10月 21 11:08 exec_test
-rw-r--r-- 1 skating skating   397 10月 21 11:08 exec_test.c
-rwxr-xr-x 1 skating skating 16416 10月 21 09:1 process
-rw-r--r-- 1 skating skating  6759 10月 21 09:1 process.c
-rw-r--r-- 1 skating skating  4432 10月 19 09:19 test.c
-rwxrwxrwx 1 skating skating     0 10月 18 15:36 test.txt
drwxr-xr-x 2 skating skating  4096 10月 21 09:27 .vscode

*/
```

##### system()函数

使用 system()函数可以很方便地在我们的程序当中执行任意 shell 命令。

```c
#include <stdlib.h>
int system(const char *command);

/*
command： 参数 command 指向需要执行的 shell 命令，以字符串的形式提供，譬如"ls -al"、 "echo HelloWorld"等。
返回值： 关于 system()函数的返回值有多种不同的情况，稍后给大家介绍。*/

/*
system()函数其内部的是通过调用 fork()、execl()以及 waitpid()这三个函数来实现它的功能， 首先 system()会调用 fork()创建一个子进程来运行 shell（可以把这个子进程成为 shell 进程） ，并通过 shell 执行参数command 所指定的命令。譬如：*/
system("ls -la")
system("echo HelloWorld")
/*
system()的返回值如下：
⚫ 当参数 command 为 NULL， 如果 shell 可用则返回一个非 0 值，若不可用则返回 0；针对一些非UNIX 系统，该系统上可能是没有 shell 的，这样就会导致 shell 不可能；如果 command 参数不为NULL，则返回值从以下的各种情况所决定。
⚫ 如果无法创建子进程或无法获取子进程的终止状态，那么 system()返回-1；
⚫ 如果子进程不能执行 shell，则 system()的返回值就好像是子进程通过调用_exit(127)终止了；
⚫ 如果所有的系统调用都成功， system()函数会返回执行 command 的 shell 进程的终止状态。*/
    
/*
system()的主要优点在于使用上方便简单，编程时无需自己处理对 fork()、 exec 函数、 waitpid()以及 exit()等调用细节， system()内部会代为处理； 当然这些优点通常是以牺牲效率为代价的，使用 system()运行 shell命令需要至少创建两个进程，一个进程用于运行 shell、另外一个或多个进程则用于运行参数 command 中解析出来的命令，每一个命令都会调用一次 exec 函数来执行；所以从这里可以看出，使用 system()函数其效率会大打折扣，如果我们的程序对效率或速度有所要求，那么建议大家不是直接使用 system()。
*/
//以下示例代码演示了 system()函数的用法，执行测试程序时，将需要执行的命令通过参数传递给 main()函数，在 main 函数中调用 system()来执行该条命令。
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    int ret;
    if (2 > argc)
    	exit(-1);
    ret = system(argv[1]);
    if (-1 == ret)
    	fputs("system error.\n", stderr);
    else {
    	if (WIFEXITED(ret) && (127 == WEXITSTATUS(ret)))
    		fputs("could not invoke shell.\n", stderr);
    }
    exit(0);
}
/*

if (WIFEXITED(ret) && (127 == WEXITSTATUS(ret)))
    		fputs("could not invoke shell.\n", stderr);
 
system 函数并不返回子进程的 PID，而是返回子进程的终止状态。这是一个编码在 int 类型中的值，用来表示子进程是如何终止的。

具体来说：

system 函数在成功调用后返回的是子进程终止状态的编码值。
WIFEXITED(ret) 宏用于检查子进程是否正常终止。
WEXITSTATUS(ret) 宏用于提取子进程的退出状态。
*/

/*运行结果：
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process ls
app  exec_new  exec_new.c  exec_test  exec_test.c  process  process.c  test.c  test.txt
*/
```

####  进程状态与进程关系  

##### 进程状态  

Linux 系统下进程通常存在 6 种不同的状态，分为：就绪态、运行态、僵尸态、 可中断睡眠状态（浅度睡眠）、不可中断睡眠状态（深度睡眠）以及暂停态。  

⚫ 就绪态（Ready） ： 指该进程满足被 CPU 调度的所有条件但此时并没有被调度执行，只要得到 CPU就能够直接运行；意味着该进程已经准备好被 CPU 执行，当一个进程的时间片到达，操作系统调度程序会从就绪态链表中调度一个进程；
⚫ 运行态： 指该进程当前正在被 CPU 调度运行，处于就绪态的进程得到 CPU 调度就会进入运行态；
⚫ 僵尸态： 僵尸态进程其实指的就是僵尸进程，指该进程已经结束、但其父进程还未给它“收尸”；
⚫ 可中断睡眠状态： 可中断睡眠也称为浅度睡眠，表示睡的不够“死”，还可以被唤醒，一般来说可以通过信号来唤醒；
⚫ 不可中断睡眠状态： 不可中断睡眠称为深度睡眠，深度睡眠无法被信号唤醒，只能等待相应的条件成立才能结束睡眠状态。把浅度睡眠和深度睡眠统称为等待态（或者叫阻塞态） ，表示进程处于一种等待状态，等待某种条件成立之后便会进入到就绪态；所以，处于等待态的进程是无法参与进程系统调度的。
⚫ 暂停态： 暂停并不是进程的终止，表示进程暂停运行，一般可通过信号将进程暂停，譬如 SIGSTOP信号；处于暂停态的进程是可以恢复进入到就绪态的，譬如收到 SIGCONT 信号。  

![image-20241021143028509](G:\notes\typora_pic\image-20241021143028509.png)

##### 进程关系  

在 Linux 系统下， 每个进程都有自己唯一的标识：**进程号（进程 ID、 PID）**，也有自己的生命周期，进程都有自己的父进程、而父进程也有父进程，这就形成了一个**以 init 进程为根的进程家族树**； **当子进程终止时，父进程会得到通知并能取得子进程的退出状态。**  

进程间还存在着其它一些层次关系， 譬如进程组和会话； 所以， 由此可知， 进程间存在着多种不同的关系，主要包括： 无关系（相互独立） 、父子进程关系、进程组以及会话。  

1、 无关系
两个进程间没有任何关系，相互独立。
2、 父子进程关系
两个进程间构成父子进程关系，譬如一个进程 fork()创建出了另一个进程，那么这两个进程间就构成了父子进程关系，调用 fork()的进程称为父进程、而被 fork()创建出来的进程称为子进程；当然，如果“生父”先与子进程结束，那么 init 进程“养父”）就会成为子进程的父进程，它们之间同样也是父子进程关系。
3、 进程组
每个进程除了有一个进程 ID、父进程 ID 之外，还有一个进程组 ID，用于标识该进程属于哪一个进程组，进程组是一个或多个进程的集合，这些进程并不是孤立的，它们彼此之间或者存在父子、兄弟关系，或者在功能上有联系。

Linux 系统设计进程组实质上是为了方便对进程进行管理。假设为了完成一个任务，需要并发运行 100个进程，但当处于某种场景时需要终止这 100 个进程，若没有进程组就需要一个一个去终止，这样非常麻烦且容易出现一些问题；有了进程组的概念之后，就可以将这 100 个进程设置为一个进程组，这些进程共享一个进程组 ID，这样一来，终止这 100 个进程只需要终止该进程组即可。

关于进程组需要注意以下以下内容：
⚫ 每个进程必定属于某一个进程组、且只能属于一个进程组；
⚫ 每一个进程组有一个组长进程，组长进程的 ID 就等于进程组 ID；
⚫ 在组长进程的 ID 前面加上一个负号即是操作进程组；

⚫ 组长进程不能再创建新的进程组；
⚫ 只要进程组中还存在一个进程，则该进程组就存在，这与其组长进程是否终止无关；
⚫ 一个进程组可以包含一个或多个进程，进程组的生命周期从被创建开始，到其内所有进程终止或离开该进程组；
⚫ 默认情况下，新创建的进程会继承父进程的进程组 ID。  

```c
//通过系统调用 getpgrp()或 getpgid()可以获取进程对应的进程组 ID.
#include <unistd.h>
pid_t getpgid(pid_t pid);
pid_t getpgrp(void);

/*
这两个函数都用于获取进程组 ID， getpgrp()没有参数，返回值总是调用者进程对应的进程组 ID；而对于 getpgid()函数来说，可通过参数 pid 指定获取对应进程的进程组 ID，如果参数 pid 为 0 表示获取调用者进程的进程组 ID。
getpgid()函数成功将返回进程组 ID；失败将返回-1、并设置 errno。
所以由此可知， getpgrp()就等价于 getpgid(0)。
*/
/********* 示例代码 9.12.1 获取进程组 ID */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    pid_t pid = getpid();
    printf("进程组 ID<%d>---getpgrp()\n", getpgrp());
    printf("进程组 ID<%d>---getpgid(0)\n", getpgid(0));
    printf("进程组 ID<%d>---getpgid(%d)\n", getpgid(pid), pid);
    exit(0);
}
/*运行结果：

skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
进程组 ID<60192>---getpgrp()
进程组 ID<60192>---getpgid(0)
进程组 ID<60192>---getpgid(60192) */

/* 从上面的结果可以发现，其新创建的进程对应的进程组 ID 等于该进程的 ID。 */

```

```c
/* 调用系统调用 setpgid()或 setpgrp()可以加入一个现有的进程组或创建一个新的进程组 */
#include <unistd.h>
int setpgid(pid_t pid, pid_t pgid);
int setpgrp(void);

/* setpgid()函数将参数 pid 指定的进程的进程组 ID 设置为参数 gpid。如果这两个参数相等(pid==gpid),则由 pid 指定的进程变成为进程组的组长进程，创建了一个新的进程；如果参数 pid 等于 0，则使用调用者的进程 ID；另外，如果参数 gpid 等于 0，则创建一个新的进程组，由参数 pid 指定的进程作为进程组组长进程。
setpgrp()函数等价于 setpgid(0, 0)。
一个进程只能为它自己或它的子进程设置进程组 ID，在它的子进程调用 exec 函数后，它就不能更改该子进程的进程组 ID 了。*/

/********  创建进程组或加入一个现有进程组  ********/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    printf("更改前进程组 ID<%d>\n", getpgrp());
    setpgrp();
    printf("更改后进程组 ID<%d>\n", getpgrp());
    exit(0);
}
/*
skating@DESKTOP-4FRM7ID:~/linux_c/7_program$ ./process 
更改前进程组 ID<61849>
更改后进程组 ID<61849>
*/
```

##### 会话  

介绍完进程组之后，再来看下会话，会话是一个或多个进程组的集合，其与进程组、进程之间的关系如下图所示：  

![image-20241021152302899](G:\notes\typora_pic\image-20241021152302899.png)

一个会话可包含一个或多个进程组，但只能有一个前台进程组，其它的是后台进程组；每个会话都有一个会话首领（leader），即创建会话的进程。一个会话可以有控制终端、也可没有控制终端，在有控制终端  的情况下也只能连接一个控制终端，这通常是登录到其上的终端设备（在终端登录情况下）或伪终端设备（譬如通过 SSH 协议网络登录）， 一个会话中的进程组可被分为一个前台进程组以及一个或多个后台进程组。  

会话的首领进程连接一个终端之后，该终端就成为会话的控制终端，与控制终端建立连接的会话首领进程被称为控制进程；产生在终端上的输入和信号将发送给会话的前台进程组中的所有进程，譬如 Ctrl + C（产生 SIGINT 信号）、 Ctrl + Z（产生 SIGTSTP 信号）、 Ctrl + \（产生 SIGQUIT 信号） 等等这些由控制终端产生的信号。  

当用户在某个终端登录时，一个新的会话就开始了； 当我们在 Linux 系统下打开了多个终端窗口时，实际上就是创建了多个终端会话。  

一个进程组由组长进程的 ID 标识，而对于会话来说，会话的首领进程的进程组 ID 将作为该会话的标识，也就是会话 ID（sid），在默认情况下，新创建的进程会继承父进程的会话 ID。通过系统调用 getsid()可以获取进程的会话 ID。

```c
#include <unistd.h>
pid_t getsid(pid_t pid);
/*
如果参数 pid 为 0，则返回调用者进程的会话 ID；如果参数 pid不为 0，则返回参数 pid 指定的进程对应的会话 ID。成功情况下，该函数返回会话 ID，失败则返回-1、并设置 errno。*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    printf("会话 ID<%d>\n", getsid(0));
    exit(0);
}
/* 运行结果：
会话 ID<187>	*/

/* ps -aux
skating      187  0.0  0.0  11692  5724 pts/4    Ss   09:25   0:00 /bin/bash	*/
//确确实实是 会话的首领进程的进程组 ID 作为了该会话的标识

/*************************************************************/
// 使用系统调用 setsid()可以创建一个会话

#include <unistd.h>
pid_t setsid(void);
/*
如果调用者进程不是进程组的组长进程，调用 setsid()将创建一个新的会话，调用者进程是新会话的首领进程，同样也是一个新的进程组的组长进程，调用 setsid()创建的会话将没有控制终端。
setsid()调用成功将返回新会话的会话 ID；失败将返回-1，并设置 errno。*/

```

##### 守护进程

守护进程（Daemon） 也称为精灵进程，是运行在后台的一种特殊进程，它独立于控制终端并且周期性地执行某种任务或等待处理某些事情的发生， 主要表现为以下两个特点：
⚫ 长期运行。 守护进程是一种生存期很长的一种进程，它们一般在系统启动时开始运行，除非强行终止，否则直到系统关机都会保持运行。 与守护进程相比，普通进程都是在用户登录或运行程序时创建，在运行结束或用户注销时终止，但守护进程不受用户登录注销的影响，它们将会一直运行着、直到系统关机。
⚫ 与控制终端脱离。 在 Linux 中，系统与用户交互的界面称为终端，每一个从终端开始运行的进程都会依附于这个终端，这是上一小节给大家介绍的控制终端，也就是会话的控制终端。当控制终端被关闭的时候， 该会话就会退出， 由控制终端运行的所有进程都会被终止， 这使得普通进程都是和运行该进程的终端相绑定的； 但守护进程能突破这种限制，它脱离终端并且在后台运行， 脱离终端的目的是为了避免进程在运行的过程中的信息在终端显示并且进程也不会被任何终端所产生的信息所打断。  

