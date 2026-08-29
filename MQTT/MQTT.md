# MQTT学习

## 一、原理和实操

**B站超子物联网up主视频读后实操。**

### 1.1、环境准备

**注：mqtt版本3.1.1协议； 使用工具：PC， EMQX服务器和客户端，网络调试助手。**

**emqx服务器**：[Directory listing for EMQX: /v5.3.2/ | EMQ](https://www.emqx.com/zh/downloads/broker/v5.3.2)

**服务器依赖 erlang支持**： [Downloads - Erlang/OTP](https://www.erlang.org/downloads)

**emqx客户端**：[MQTTX：全功能 MQTT 客户端工具](https://mqttx.app/zh)

**emqx服务器安装（管理员）：**

![image-20260707094555389](./../Image/image-20260707094555389.png)

```powershell
#查看ip和端口
.\bin\emqx console

#开启服务
.\bin\emqx start

#关闭服务
.\bin\emqx stop
```

接下进入管理界面：

```shell
在浏览器输入以下地址回车
127.0.0.1:18083 
#or
localhost:18083

#账号/密码
admin/public
```

![image-20260707095929451](./../Image/image-20260707095929451.png)

![image-20260707095950197](./../Image/image-20260707095950197.png)

```
#规划两个客户端
用户1( 网络串口调试助手 )
ClientID(USER001), 用户名(USER001), 密码(USER001), Topic(USER001)

用户2( MQTTX 客户端 )
ClientID(USER002), 用户名(USER002), 密码(USER002), Topic(USER002)

```

![image-20260707100500950](./../Image/image-20260707100500950.png)

### 1.2、按下列表格以此验证

![image-20260707102920609](./../Image/image-20260707102920609.png)

![image-20260707102008929](./../Image/image-20260707102008929.png)

**剩余长度计算：**

```c
#include <stdio.h>
#include <stdint.h>

// 编码剩余长度
int encode_remaining_length(int length, uint8_t *encoded) {
    int i = 0;
    do {
        uint8_t byte = length % 128;
        length /= 128;
        // 如果还有数据，设置最高位
        if (length > 0) {
            byte |= 0x80;
        }
        encoded[i++] = byte;
    } while (length > 0 && i < 4); // 最多 4 字节
    return i; // 返回编码字节数
}

// 解码剩余长度
int decode_remaining_length(const uint8_t *encoded, int *value) {
    int multiplier = 1;
    *value = 0;
    int i = 0;
    uint8_t byte;
    do {
        byte = encoded[i++];
        *value += (byte & 127) * multiplier;
        multiplier *= 128;
        if (multiplier > 128*128*128) {
            // 超过最大长度，报错
            return -1;
        }
    } while ((byte & 128) != 0 && i < 4);
    return i; // 返回解码消耗的字节数
}

int main() {
    uint8_t buffer[4];
    int len, value, used;

    // 示例 1：长度 = 64
    len = encode_remaining_length(64, buffer);
    printf("Encode 64: ");
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
    used = decode_remaining_length(buffer, &value);
    printf("Decode: %d (used %d bytes)\n\n", value, used);

    // 示例 2：长度 = 321
    len = encode_remaining_length(321, buffer);
    printf("Encode 321: ");
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
    used = decode_remaining_length(buffer, &value);
    printf("Decode: %d (used %d bytes)\n\n", value, used);

    // 示例 3：长度 = 16384
    len = encode_remaining_length(16384, buffer);
    printf("Encode 16384: ");
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
    used = decode_remaining_length(buffer, &value);
    printf("Decode: %d (used %d bytes)\n", value, used);

    return 0;
}

```



```c
//规划两个客户端
/*用户1( 网络串口调试助手 )
ClientID(USER001), 用户名(USER001), 密码(USER001), Topic(USER001)

用户2( MQTTX 客户端 )
ClientID(USER002), 用户名(USER002), 密码(USER002), Topic(USER002)
*/

/******************************/
//CONNECT,CONACK,DISCONNECT
CONNECT;

10 25
00 04 4D 51 54 54 04 C2 00 64
00 07 55 53 45 52 30 30 31 
00 07 55 53 45 52 30 30 31 
00 07 55 53 45 52 30 30 31 
;

CONACK;
20 02 00 00 
    
DISCONNECT;
E0 00
;

//PING,PINGRSP
PINGREQ:
C0 00

PINGRSP:
D0 00

;
//SUBSCRIBE,UNSUBSCRIBE, SUBACK, UNSUBACK
SUBSCRIBE :
82 0C
00 0A
00 07 55 53 45 52 30 30 32 02

//00 0A: 标识符可以任意写
//这个订阅可以订阅任意topic， 我目前用的mqtt服务器，反正写了不同的topic后可以任意订阅，即使我的mqtt只注册了两个topic:USER001,USER002
    
SUBACK
90 03 00 0A 02 
// 00 0A 是订阅时的标识符是多少就会返回多少，02是订阅的Qos最大是多少，返回多少就是最大多少

UNSUBSCRIBE;
A2 0B
30 32
00 07 55 53 45 52 30 30 32
/*
A2 0B 
30 32 :报文标识
00 07 55 53 45 52 30 30 32 ：需求取消订阅的topic
*/
UNSUBACK;
B0 02 30 32

//PUBLISH, PUBACK(QOS1), PUBREC(QOS2), PUBREL(QOS2), PUBCOMP(QOS2)
PUBLISH:

Qos0:无报文标识，不使用 RETAIN（只要我发了服务器保留最近1条信息，如果有客户端订阅了我，就发给此客户端。类似b站关注up后台发一条信息，取消关注再关注也发一天信息）

30 12
00 07 55 53 45 52 30 30 31
30 31 32 33 34 35 36 37 38

无回应    
/****************************************/

Qos1:有报文标识，不使用 RETAIN， DUP在重复发的时候，不是第一次的信息都为1.
first:
32 14
00 07 55 53 45 52 30 30 31
30 31
30 31 32 33 34 35 36 37 38
later:
3A 14
00 07 55 53 45 52 30 30 31
30 31
30 31 32 33 34 35 36 37 38

PUBACK:(Q1)
40 02 00 01 
//网络调试助手需要回应此信息，否则触发重传 DUP bit
/******************************************/

    
Q2
PUBLISH:(Q2)
first:
34 0E 
00 07 55 53 45 52 30 30 32 
00 01 
34 35 36

later:
3C 0E 
00 07 55 53 45 52 30 30 32 
00 01 
34 35 36


PUBREC:(Q2)

50 02 XX XX 
XX XX 标识

PUBREL:(Q2)

62 02 XX XX
XX XX 标识

PUBCOMP:(Q2)
70 02 XX XX

```

订阅列表

![image-20260707163709755](./../Image/image-20260707163709755.png)

取消订阅

![image-20260707163734228](./../Image/image-20260707163734228.png)

Qos1发布

![image-20260707170825722](./../Image/image-20260707170825722.png)

Qos1超时
![image-20260707171346648](./../Image/image-20260707171346648.png)

重传时间一般由客户端自行设计，没有硬性条件需求。

Qos2 重传机制时保证4路报文的标识必须一致。

![image-20260707174635037](./../Image/image-20260707174635037.png)

### 1.3、其他bit位的详细介绍

#### 1.3.1、CONNECTE -- Clean Session.bit 和 CONACK -- SP.bit

![image-20260708080413054](./../Image/image-20260708080413054.png)

Clean Session ==1 : 0xC0
第一次连接服务器，超时时间为5s.
![image-20260708081105963](./../Image/image-20260708081105963.png)

![image-20260708081629606](./../Image/image-20260708081629606.png)

![image-20260708081521209](./../Image/image-20260708081521209.png)

超时抵达。服务器断开 USER001的tcp连接，网络调试助手显示网络断开。
![image-20260708081154301](./../Image/image-20260708081154301.png)

再次发Clean Session ==1 : 0xC0

和第一次一样，无订阅USER001. 不保留会话如此。
![image-20260708081413862](./../Image/image-20260708081413862.png)

![image-20260708082037098](./../Image/image-20260708082037098.png)

#############################

如果发一次Clean Session ==1，订阅后再发Clean Session ==0的情况：
 ![image-20260708083231476](./../Image/image-20260708083231476.png)

![image-20260708083449174](./../Image/image-20260708083449174.png)

**说明在0xC2(clear session =1)时，会清楚连接到断开连接的会话，且不保存。**

**CONNECT**发0xC0超时后(clear session =0)：

![image-20260708083808841](./../Image/image-20260708083808841.png)

超时后重新连接服务器发0xC0(clear session =0)：

![image-20260708084011277](./../Image/image-20260708084011277.png)

我们先订阅一下再超时再重新不清除会话情况再次连接：
![image-20260708084422427](./../Image/image-20260708084422427.png)

![image-20260708084458866](./../Image/image-20260708084458866.png)

![image-20260708084748869](./../Image/image-20260708084748869.png)

若下次用0xC0则继续 CONACK: SP(bit) = 1；否则使用 0xC1的话，SP 一直为零，除非连续开始0xC0,因为SP只在连续的第二次0xC0,才开始为1.需要注意也只有上次也为0xC0,的时候才能保存上次会话，如果有0xC2(Clean Session = 1), 会清除以往的会话。

**会话清理和保留，同ClientID的关系**

1. 有记忆的，但是必须保证相同的ClientID, (相当于同一游戏中不同的账号)。

2. ```
   0xC0    0xC0     0xC0     0xC0    0xC0     0xC0      0xC0
   USE001   USE001  USE002   USE001  USE001  USE002    USE001
   
   那么USE002只拥有USE002的记忆， USE001也只能拥有前面USE001的记忆。
   ```

#### 1.3.2、PUBLISH -- RETAIN(bit)

![image-20260708164036769](./../Image/image-20260708164036769.png)

##### 示例 1：温度传感器

- 设备发布消息：`topic = "home/temperature"`，payload = `25℃`，QoS=1，RETAIN=1。
- 服务端保存这条消息为保留消息。
- 新客户端订阅 `"home/temperature"` 时，立刻收到 `25℃`，即使它订阅之前没有在线。 👉 这样新订阅者一上来就能知道最新温度。

##### 示例 2：状态开关

- 设备发布消息：`topic = "light/status"`，payload = `ON`，RETAIN=1。
- 服务端保存这条消息。
- 新客户端订阅 `"light/status"` 时，立刻收到 `ON`，知道灯当前是开着的。 👉 Retain 消息常用于这种“状态类”主题。

##### 示例 3：清除保留消息

- 客户端发布消息：`topic = "light/status"`，payload = 空字符串，RETAIN=1。
- 服务端会删除这个主题的保留消息。
- 以后新订阅者订阅 `"light/status"` 时，不会再收到任何保留消息。 👉 这是协议里规定的“清除保留消息”的方法。

##### 示例 4：RETAIN=0 的情况

- 设备发布消息：`topic = "sensor/data"`，payload = `123`，RETAIN=0。
- 服务端只转发给当前在线的订阅者，不会保存。
- 新订阅者以后订阅 `"sensor/data"` 时，不会收到这条历史消息。 👉 适合“实时数据流”，不需要保留。

**RETAIN=1** → 服务端保存最新消息，新订阅者能立即收到。

**RETAIN=0** → 服务端不保存，只发给当前订阅者。

**空消息 + RETAIN=1** → 删除该主题的保留消息。

![image-20260708171132702](./../Image/image-20260708171132702.png)

**总结下：**

```
QOS 等级问题，使能了RETAIN标志后，按照QOS等级进行回应。
注意细节：
1、注册RETAIN报文时，需要携带负载内容，删除时不需要携带负载内容，尤其注意QOS>0的publish报文需要带标识。
2、发送带RETAIN标志报文时（不管是取消RETAIN还是注册RETAIN），服务器都会将报文去掉RETAIN标志的报文发送给订阅方。订阅方只要第一次连接服务器且订阅后会收到带RETAIN报文的数据。应答也需要按Qos等级进行应答流程。
3、被订阅方的RETAIN报文设计和清除可以在，订阅方不在线情况下随意更改。

```



#### 1.3.3、遗嘱功能

属于报警功能的一环，在非正常断线（不发14号报文(DISCONNETE)， 直接断开TCP，被踢下线（心跳超时））时，服务器会向订阅了遗嘱功能TOPIC的客户端推送遗嘱PUBLISH报文。
遗嘱功能设定包含在connect报文中。

**遗嘱消息**

实验：在EMQX客户端中的CONNECT补齐遗嘱功能（WILLTOPIC： WILL002），没开遗嘱保留标志，后面测试！
WILLQos =0，WILLQos =1， WILLQos =2，用关闭EMQX来除非遗嘱，使用网络串口调试助手订阅WILL002后测试如下。
![image-20260709102541279](./../Image/image-20260709102541279.png)

**WILLQos0**：

![image-20260709105152930](./../Image/image-20260709105152930.png)

**WILLQos1：**

![image-20260709121538703](./../Image/image-20260709121538703.png)

**WILLQos2：**

![image-20260709122554478](./../Image/image-20260709122554478.png)

**小结：**
订阅方（WILL002）活着的时候是不会发送任何的遗嘱信息的。

**遗嘱保留标志 : WILLRETAIN**

那遗嘱的会话保留和发布的会话保留有啥区别？

🔹 **遗嘱消息的会话保留**

- **定义**：遗嘱消息是客户端在连接时告诉 Broker：“如果我非正常断开（比如掉线），请帮我自动发布这条消息。”
- **会话保留选项**：遗嘱消息也可以设置 `retain=1`。这样 Broker 在发布遗嘱时，会把它保存为该主题的保留消息。
- **作用**：新订阅者加入时，也能立即看到这个遗嘱消息。
- **典型场景**：设备上线时设置遗嘱为 `"status/offline"`，retain=1。这样如果设备掉线，Broker 会自动发布遗嘱并保留，后续新订阅者一看就知道设备已经离线。

**🔹 普通 PUBLISH 的会话保留**

- **定义**：客户端主动发布消息时，可以设置 `retain=1`。Broker 会保存这条消息为该主题的最新保留消息。
- **作用**：新订阅者加入时，立即收到这条保留消息。
- **典型场景**：传感器定期发布温度数据，最后一条温度消息被保留，新订阅者一上来就能看到最新温度。

| 特性                  | **遗嘱消息保留**                                   | **普通 PUBLISH 保留**                |
| --------------------- | -------------------------------------------------- | ------------------------------------ |
| **触发时机**          | 客户端异常断开时 Broker 自动发布                   | 客户端主动发送时立即发布             |
| **是否可设置 retain** | 可以，遗嘱消息里有 retain 标志                     | 可以，PUBLISH 报文里有 retain 标志   |
| **作用**              | 表示客户端状态（如离线），可被保留供后续订阅者查看 | 表示主题的最新数据，供后续订阅者查看 |
| **典型用途**          | 设备上下线状态通知                                 | 传感器最新数据、设备状态值           |

**小结**

- **遗嘱保留**：强调“客户端掉线时 Broker 自动发布并保留”，常用于设备状态。
- **普通保留**：强调“客户端主动发布并保留”，常用于最新数据。
- 两者的 retain 标志作用相同，区别在于 **触发时机** 和 **应用场景**。

细节问题：被订阅方是否在线。

```
🔹 普通 PUBLISH 的保留 (retain=1)
谁触发：客户端主动发布。

什么时候发：只要客户端发了，Broker 就立即转发给当前订阅者，并保存为保留消息。

新订阅者：以后订阅同一主题时，Broker 会把这条保留消息立即推送给它。
👉 即使发布者还在线，保留消息也会存在，后来的订阅者也能收到。

🔹 遗嘱消息的保留 (will + retain=1)
谁触发：Broker，而不是客户端。客户端在连接时告诉 Broker：“如果我掉线，请帮我发这条消息”。

什么时候发：只有当客户端 非正常断开（比如网络掉线、心跳超时），Broker 才会自动发布遗嘱消息。

新订阅者：如果遗嘱设置了 retain=1，Broker 会保存这条遗嘱消息，后来的订阅者也能收到。
👉 如果客户端一直在线、正常断开连接，Broker 不会发布遗嘱消息。
```

**细节：**遗嘱的生效动作是自己被踢了或者断开tcp连接了。这里的WILLTARIN很细节！要清楚遗嘱保留报文需要和发布的PUBLISH的RETAIN清楚一样。也就是置位RETAIN，然后负载信息为空。但是发布是点发布到服务器就可以清楚RETAIN了，但是遗嘱就必须让他遗嘱生效，不能有DISCON的发送，应该是勾线WILLRETAIN，之后遗嘱信息为空，然后直接关EMQX客户端！不能有DISCON。然后就可以清楚掉WILLRETAIN.的功能信息了。而且WILLREATAIN被新上来的订阅者会发送一条信息，此发布信息是PUBLISH，且bit RETAIN为1. 发布等级看WILLQos等级。和发布的REATAIN一样动作。

发布WILLREATAIN的客户端不管自己是否在线都会发送PUBLISH(with RETAIN).

```
[2026-07-09 15:21:29.495]# Connecting to server...

[2026-07-09 15:21:29.498]# Server connected from local 192.168.61.1 :7420

[2026-07-09 15:21:46.905]# Connecting to server...

[2026-07-09 15:21:46.909]# Server connected from local 192.168.61.1 :7439

[2026-07-09 15:21:49.071]# SEND HEX/39 >>>
10 25 00 04 4D 51 54 54 04 C2 11 64 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 

[2026-07-09 15:21:49.097]# RECV HEX/4 from SERVER <<<
20 02 00 00 

[2026-07-09 15:21:51.061]# SEND HEX/14 >>>
82 0C 00 0A 00 07 55 53 45 52 30 30 32 02 

[2026-07-09 15:21:51.089]# RECV HEX/5 from SERVER <<<
90 03 00 0A 02 

[2026-07-09 15:21:51.592]# SEND HEX/14 >>>
82 0C 00 01 00 07 57 49 4C 4C 30 30 32 02 

[2026-07-09 15:21:51.593]# RECV HEX/22 from SERVER <<<
90 03 00 01 02 31 0F 00 07 57 49 4C 4C 30 30 32 30 31 32 33 34 35 
31: 0x01:REATAIN , Qos=0
```

EMQX断线后的遗嘱信息本质还是PUBLISH. 下面是Qos1的遗嘱断线信息。

```
[2026-07-09 15:24:52.431]# Server disconnected

[2026-07-09 15:24:53.300]# Connecting to server...

[2026-07-09 15:24:53.303]# Server connected from local 192.168.61.1 :7538

[2026-07-09 15:24:55.227]# SEND HEX/39 >>>
10 25 00 04 4D 51 54 54 04 C2 11 64 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 

[2026-07-09 15:24:55.245]# RECV HEX/4 from SERVER <<<
20 02 00 00 

[2026-07-09 15:24:58.290]# SEND HEX/14 >>>
82 0C 00 0A 00 07 55 53 45 52 30 30 32 02 

[2026-07-09 15:24:58.291]# RECV HEX/5 from SERVER <<<
90 03 00 0A 02 

[2026-07-09 15:24:58.971]# SEND HEX/14 >>>
82 0C 00 01 00 07 57 49 4C 4C 30 30 32 02 

[2026-07-09 15:24:58.972]# RECV HEX/22 from SERVER <<<
90 03 00 01 02 31 0F 00 07 57 49 4C 4C 30 30 32 30 31 32 33 34 35 
此时的Qos=0,, 0x31, publish with RETAIN.


下面的是临时改了个Qos1的遗嘱，然后关EMQX 模拟非正常断线
[2026-07-09 15:25:10.150]# RECV HEX/19 from SERVER <<<
32 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:25:40.157]# RECV HEX/19 from SERVER <<<
3A 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:26:10.172]# RECV HEX/19 from SERVER <<<
3A 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:26:24.523]# SEND HEX/4 >>>
04 02 00 01 


WILLQos = 1, WILLREATAIN =1:
[2026-07-09 15:28:43.247]# Connecting to server...

[2026-07-09 15:28:43.251]# Server connected from local 192.168.61.1 :7691

[2026-07-09 15:28:45.450]# SEND HEX/39 >>>
10 25 00 04 4D 51 54 54 04 C2 11 64 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 

[2026-07-09 15:28:45.480]# RECV HEX/4 from SERVER <<<
20 02 00 00 

[2026-07-09 15:28:46.875]# SEND HEX/14 >>>
82 0C 00 0A 00 07 55 53 45 52 30 30 32 02 

[2026-07-09 15:28:46.899]# RECV HEX/5 from SERVER <<<
90 03 00 0A 02 

[2026-07-09 15:28:47.669]# SEND HEX/14 >>>
82 0C 00 01 00 07 57 49 4C 4C 30 30 32 02 

[2026-07-09 15:28:47.699]# RECV HEX/24 from SERVER <<<
90 03 00 01 02 33 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:29:17.680]# RECV HEX/19 from SERVER <<<
3B 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:29:47.689]# RECV HEX/19 from SERVER <<<
3B 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:29:50.678]# SEND HEX/4 >>>
04 02 00 01 
```

```

WILLQos = 2, WILLRETAIN = 1:
[2026-07-09 15:33:37.572]# SEND HEX/39 >>>
10 25 00 04 4D 51 54 54 04 C2 11 64 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 00 07 55 53 45 52 30 30 31 

[2026-07-09 15:33:37.587]# RECV HEX/4 from SERVER <<<
20 02 00 00 

[2026-07-09 15:33:40.512]# SEND HEX/14 >>>
82 0C 00 0A 00 07 55 53 45 52 30 30 32 02 

[2026-07-09 15:33:40.512]# RECV HEX/5 from SERVER <<<
90 03 00 0A 02 

[2026-07-09 15:33:41.046]# SEND HEX/14 >>>
82 0C 00 01 00 07 57 49 4C 4C 30 30 32 02 

[2026-07-09 15:33:41.068]# RECV HEX/24 from SERVER <<<
90 03 00 01 02 35 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:34:11.052]# RECV HEX/19 from SERVER <<<
3D 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:34:41.058]# RECV HEX/19 from SERVER <<<
3D 11 00 07 57 49 4C 4C 30 30 32 00 01 30 31 32 33 34 35 

[2026-07-09 15:34:42.473]# SEND HEX/4 >>>
50 02 00 01 

[2026-07-09 15:34:42.495]# RECV HEX/4 from SERVER <<<
62 02 00 01 

[2026-07-09 15:34:47.450]# SEND HEX/4 >>>
70 02 00 01 


//非正常断线遗嘱 WILLQos = 2
[2026-07-09 15:35:52.483]# RECV HEX/19 from SERVER <<<
34 11 00 07 57 49 4C 4C 30 30 32 00 02 30 31 32 33 34 35 

[2026-07-09 15:36:22.491]# RECV HEX/19 from SERVER <<<
3C 11 00 07 57 49 4C 4C 30 30 32 00 02 30 31 32 33 34 35 

[2026-07-09 15:36:52.499]# RECV HEX/19 from SERVER <<<
3C 11 00 07 57 49 4C 4C 30 30 32 00 02 30 31 32 33 34 35 

[2026-07-09 15:36:58.919]# SEND HEX/4 >>>
50 02 00 02 

[2026-07-09 15:36:58.944]# RECV HEX/4 from SERVER <<<
62 02 00 02 

[2026-07-09 15:37:02.985]# SEND HEX/4 >>>
70 02 00 02 

```

```
如果我需要取消遗嘱保留会话功能，那么必须设置WILLREATAIN = 1. 负载为空，然后非正常断线后。
服务器会下方到订阅方：
34 0B 00 07 57 49 4C 4C 30 30 32 00 03 
此时的publish是Qos2等级，因为我没有改变WILLQos等级，还是维持的2.
完整如下：取消WILLRETAIN设计, WILLREATIAN =1 ,负载 =NULL

[2026-07-09 15:45:41.282]# RECV HEX/13 from SERVER <<<
34 0B 00 07 57 49 4C 4C 30 30 32 00 03 

[2026-07-09 15:46:11.285]# RECV HEX/13 from SERVER <<<
3C 0B 00 07 57 49 4C 4C 30 30 32 00 03 

[2026-07-09 15:46:41.298]# RECV HEX/13 from SERVER <<<
3C 0B 00 07 57 49 4C 4C 30 30 32 00 03 

[2026-07-09 15:46:52.808]# SEND HEX/4 >>>
50 02 00 04 

[2026-07-09 15:46:52.832]# RECV HEX/4 from SERVER <<<
62 02 00 04 

[2026-07-09 15:46:58.119]# SEND HEX/4 >>>
70 02 00 04 

[2026-07-09 15:47:11.308]# RECV HEX/13 from SERVER <<<
3C 0B 00 07 57 49 4C 4C 30 30 32 00 03 

[2026-07-09 15:47:12.844]# SEND HEX/4 >>>
50 02 00 03 

[2026-07-09 15:47:12.866]# RECV HEX/4 from SERVER <<<
62 02 00 03 

[2026-07-09 15:47:16.808]# SEND HEX/4 >>>
70 02 00 03
```

如果PUBLISH(RETAIN =1), WILLRETAIN = 1.如下。![image-20260709164616783](./../Image/image-20260709164616783.png)

图片最后清空遗嘱保留会话功能时也会发一个空信息过来。但此时的PUBLISH不会有RETAIN位。这和PUBLISH的RETAIN的功能实验一样的，新订阅时收到信息的REATIAN位表现也是一样。

那其实如果要去清除遗嘱保留的话得先把遗嘱信息清除并且保持WILLRETAIN, 然后异常断线。这样的话遗嘱也会掉。所以有需要重新设计遗嘱信息不要勾选WILLREATIN, 设计遗嘱报文。


### 1.4、回路细节（两个网络调试助手测试下Qos1, Qos2的应答测试下）

一样的，没啥区别。

## 二、STM32例子

AT指令的结合模块连接到onenet, 注意的点是以json格式发送，然后做了个发送链表来作为数据发送设计挺好的。

```c

sendfun(){
    if(timInfo.timer3Out - list_time >= 20)	
    {
        list_time = timInfo.timer3Out;

        if(NET_DEVICE_CheckListHead())
        {
            NET_DEVICE_SendData(NET_DEVICE_GetListHeadBuf(),NET_DEVICE_GetListHeadLen());
            NET_DEVICE_DeleteDataSendList();
        }
    }
}
void NET_DEVICE_SendData(unsigned char *data, unsigned short len)
{

    char cmdBuf[24];

    DelayXms(50);						
    sprintf(cmdBuf, "AT+CIPSEND=0,%d\r\n", len); 
    if(!NET_DEVICE_SendCmd(cmdBuf, ">"))			
    {
        NET_IO_Send(data, len);  					
    }

    DelayXms(50);									
}

```

