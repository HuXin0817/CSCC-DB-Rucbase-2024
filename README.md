<div align="center">
<img src="RMDB.jpg"  width=25%  /> 
</div>

全国大学生计算机系统能力大赛数据库管理系统赛道，以培养学生“数据库管理系统内核实现”能力为目标。本次比赛为参赛队伍提供数据库管理系统代码框架 RMDB，参赛队伍在 RMDB 的基础上，设计和实现一个完整的关系型数据库管理系统，该系统要求具备运行 TPC-C 基准测试（TPC-C 是一个面向联机事务处理的测试基准）常用负载的能力。

RMDB 由中国人民大学数据库教学团队开发，同时得到教育部-华为”智能基座”项目的支持，平台、赛题和测试用例等得到了全国大学生计算机系统能力大赛数据库管理系统赛道技术委员会的支持和审核。系统能力大赛专家组和[101 计划数据库系统课程](http://101.pku.edu.cn/courseDetails?id=DC767C683D697417E0555943CA7634DE)工作组给予了指导。

## 作品简介

队伍学校：成都理工大学

队伍名称：RushDB

队伍成员：明泰，胡鑫

TPC-C 测试结果：**32,820** txns/min

<img src="image.png" width=50% />

## 实验环境：

- 操作系统：Ubuntu 18.04 及以上(64 位)
- 编译器：GCC
- 编程语言：C++17
- 管理工具：cmake
- 推荐编辑器：VScode

### 依赖环境库配置：

- gcc 7.1 及以上版本（要求完全支持 C++17）
- cmake 3.16 及以上版本
- flex
- bison
- readline

欲查看有关依赖运行库和编译工具的更多信息，以及如何运行的说明，请查阅[RMDB 使用文档](RMDB使用文档.pdf)

欲了解如何在非 Linux 系统 PC 上部署实验环境的指导，请查阅[RMDB 环境配置文档](RMDB环境配置文档.pdf)

### 项目说明文档

- [RMDB 环境配置文档](RMDB环境配置文档.pdf)
- [RMDB 使用文档](RMDB使用文档.pdf)
- [RMDB 项目结构](RMDB项目结构.pdf)
- [测试说明文档](测试说明文档.pdf)

## 推荐参考资料

- [**Database System Concepts** (**_Seventh Edition_**)](https://db-book.com/)
- [PostgreSQL 数据库内核分析](https://book.douban.com/subject/6971366//)
- [数据库系统实现](https://book.douban.com/subject/4838430/)
- [数据库系统概论(第 5 版)](http://chinadb.ruc.edu.cn/second/url/2)

## License

RMDB 采用[木兰宽松许可证，第 2 版](https://license.coscl.org.cn/MulanPSL2)，可以自由拷贝和使用源码, 当做修改或分发时, 请遵守[木兰宽松许可证，第 2 版](https://license.coscl.org.cn/MulanPSL2).

[![Visitors](https://api.visitorbadge.io/api/visitors?path=https://github.com/Kosthi/CSCC-DB-Rucbase-2024&label=visitors&countColor=%23263759)](https://visitorbadge.io/status?path=https://github.com/Kosthi/CSCC-DB-Rucbase-2024)
