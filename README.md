# ESP RELAY
**开关模块个人固件.**

> ### 作者声明
>
> 注意: 本项目主要目的为作者本人自己学习及使用而开发，本着开源精神及造福网友而开源，仅个人开发，可能无法做到完整的测试，所以不承担他人使用本项目照成的所有后果。
>
> **严禁他人将本项目用户用于任何商业活动。个人在非盈利情况下可以自己使用，严禁收费代刷等任何盈利服务、**
> 
> 有需要请联系作者：qlwz@qq.com

# 开关模块

本开关模块为自己家装修使用，只支持零火。没有单火需求所以也没有单火版。

![image](https://github.com/qlwz/esp_relay/blob/master/file/images/正面.png)
![image](https://github.com/qlwz/esp_relay/blob/master/file/images/反面.png)
![image](https://github.com/qlwz/esp_relay/blob/master/file/images/成品.png)
![image](https://github.com/qlwz/esp_relay/blob/master/file/images/LED状态.png)

开关模块以ESP-01M为基础实现了一路、二路、三路并附带面板LED指示灯及射频功能

- 一路：[一路开关模块详细资料](https://github.com/qlwz/esp_relay/tree/master/file/1%E8%B7%AF%E5%BC%80%E5%85%B3%E6%A8%A1%E5%9D%97)
- 二路：[二路开关模块详细资料](https://github.com/qlwz/esp_relay/tree/master/file/2%E8%B7%AF%E5%BC%80%E5%85%B3%E6%A8%A1%E5%9D%97)
- 三路：[三路开关模块详细资料](https://github.com/qlwz/esp_relay/tree/master/file/3%E8%B7%AF%E5%BC%80%E5%85%B3%E6%A8%A1%E5%9D%97)
- 插座：待开源

## IO对应：  
按键：  
L1：GPIO4  
L2：GPIO9  
L3：GPIO10  
面板指示灯：  
S1：GPIO0  
S2：GPIO2  
S3：GPIO15  

## 射频
为什么需要射频？  
不想因为WIFI或者HA等设备出故障的时候影响日常生活。  
射频为软件解码支持433MHz和315MHz  

# 固件

## 特性

本固件使用开关模块硬件为基础，实现以下功能:

- [x] 单独控制开关
- [x] 支持普通开关、自复位开关
- [x] 支持三档筒灯
- [x] 支持射频
- [x] OTA在线升级
- [x] WEB配置页面
- [x] MQTT服务器连接控制
- [x] 通过MQTT连入Home Assistant

## 如何配网

1、第一次使用自动进入配网模式

## 如何编译
Visual Studio Code + PlatformIO ID 开发  [安装](https://www.jianshu.com/p/c36f8be8c87f)

## 已支持接入的开源智能家居平台
以下排序随机，不分优劣。合适自己的就好。

### 1、Home Assistant
Home Assistant 是一款基于 Python 的智能家居开源系统，支持众多品牌的智能家居设备，可以轻松实现设备的语音控制、自动化等。
- [官方网站](https://www.home-assistant.io/)
- [国内论坛](https://bbs.hassbian.com/)

#### 接入方法
WEB页面开启**MQTT自动发现**  

### 2、ioBroker
ioBroker是基于nodejs的物联网的集成平台，为物联网设备提供核心服务、系统管理和统一操作方式。
- [官方网站](http://www.iobroker.net)
- [中文资料可以参考这里](https://doc.iobroker.cn/#/_zh-cn/)
- [国内论坛](https://bbs.iobroker.cn)
#### 接入方法
ioBroker相关接入问题可以加QQ群776817275咨询

### 3、其他支持mqtt的平台
理论上来说，只要是支持MQTT的平台都可以实现接入。

#### 接入方法
添加对应的topic

# 固件截图

![image](https://github.com/qlwz/esp_relay/blob/master/file/images/tab1.png)
![image](https://github.com/qlwz/esp_relay/blob/master/file/images/tab2.png)
![image](https://github.com/qlwz/esp_relay/blob/master/file/images/tab3.png)
![image](https://github.com/qlwz/esp_relay/blob/master/file/images/tab4.png)


感谢各位使用本方法的玩家，欢迎加入QQ群776817275

## 免责申明
以上纯属个人爱好，因为使用上述方法造成的任何问题，不承担任何责任。

部分图片来源于网络，如果涉及版权，请通知删除。