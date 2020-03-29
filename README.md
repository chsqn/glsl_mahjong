# glsl_mahjong
着色器穷举日本麻将听牌和牌

用穷举法计算麻将配牌的向听数，在判断五向听的时候，有很巨大的运算量。
在着色器中将运算量分担到1024个计算单元，通过显卡进行并行计算，可以显著降低穷举所需时间。

## OpenGL环境

要运行着色器程序，需要：

用freeglut库或者glfw库，创建渲染环境，也就是创建一个隐藏窗口；
用glew库或者gl3w库，解析OpenGL的函数名。

OpenGL的蓝宝书5和红宝书8，使用的是freeglut+glew；而蓝宝书6和红宝书9则都改为了使用glfw+gl3w。
因此本程序使用的库是glfw+gl3w。

用freeglut创建隐藏窗口，可以使用glutCreateMenu函数，用一个空的菜单代替隐藏窗口。
用glfw创建隐藏窗口，需要在创建窗口之前，调用glfwWindowHint (GLFW_VISIBLE, GLFW_FALSE);

CPU程序负责编译、链接着色器程序，并在运行着色器程序后与之交互。
CPU程序将数据传递给着色器程序，着色器程序运算并汇总结果，再由CPU程序将结果读出。

## 显卡要求

默认使用OpenGL版本4.6，由于计算着色器是从版本4.3开始出现的，
若显卡版本在4.3以上，但小于版本4.6，则需要根据具体情况，修改着色器代码。
测试时发现，NVIDIA显卡兼容性更好，速度也更快。
Intel显卡则存在shared变量无法在定义时初始化的问题，因此需要加入屏障，并在0号工作组中负责初始化。

## 牌面编码

为了使用编码掩码判断牌的类型，首先将牌分为数牌和字牌。
数牌的范围是0x01～0x3F，字牌的范围是0x41～0x7F，这样就可以用bit6区分数牌和字牌了。

为了便于判断顺子，各个字牌使用0x01～0x09、0x11～0x19、0x21～0x29、0x31～0x39。
这样调试时便于将数字对应上，并且用bit5和bit4共同区分数牌的花色。
使用十六进制而不是十进制，是因为每种数牌的1和9正好相差8，这样六个老头牌构成等差数列了。

为了将老头牌的等差数列推广到字牌，达成十三个幺九牌都是等差数列的目的，数牌不使用0x01～0x09，而用另外三组。
这样，数牌最大的0x39和字牌最小的0x41相差8，后边的字牌也依次相差8，将字牌孤立开，天然可以避免字牌构成顺子。
幺九牌都是等差数列，可以简化特殊役种国士无双的判断，并且可以用“模8余1”来区分幺九牌和中张牌。

另外，参考天凤对麻将牌的字符编码，将数牌花色的顺序定义为“万筒索”，最后的数值编码如下：

    一万（1m）～九万（9m）：0x11～0x19
    一筒（1p）～九筒（9p）：0x21～0x29
    一索（1s）～九索（9s）：0x31～0x39
    东（1z）南（2z）西（3z）北（4z）：0x41、0x49、0x51、0x59
    白（5z）发（6z）中（7z）：0x61、0x69、0x71

这样编码在无形中实现了另一项便利：对字牌来说，可以用bit5区分风牌和三元牌。

## 和牌判断

计算麻将配牌的向听数，离不开最基本的和牌判断。
通过递归移除手牌中的刻子、对子、顺子，来进行和牌判断。
着色器程序的语法不允许递归，考虑到一般形的刻子、对子、顺子加起来不会超过5组，因此这里展开了递归，
把1面子、2面子、3面子、4面子等情况各自写出独立的函数，以此模拟递归。
和牌判断的着色器代码见“shaders/hule.cs”，负责运行该着色器的C++语言的代码见“src/hule.cc”。
后续会使用状态机彻底解除递归。

## 配牌向听数

这里的向听数，暂时不考虑宝牌指示牌、牌河、副露、暗杠等公开情报，这些公开情报会降低摸到某些牌的概率。
不考虑公开情报时，由于有七对子保底六向听，因此向听数不会大于6。

配牌可以处理庄家的14张，可以处理闲家的13张。计算向听数的步骤如下：

    对庄家判断天和，若天和则返回-1表示向听数不存在；
    穷举法向配牌中任意加入1张牌，然后任意剔除庄家1张/闲家0张，判断剩余14张牌是否和牌，若能和牌，则是听牌，向听数是0；
    穷举法向配牌中任意加入2张牌，然后任意剔除庄家2张/闲家1张，判断剩余14张牌是否和牌，若能和牌，则是一向听；
    穷举法向配牌中任意加入3张牌，然后任意剔除庄家3张/闲家2张，判断剩余14张牌是否和牌，若能和牌，则是二向听；
    穷举法向配牌中任意加入4张牌，然后任意剔除庄家4张/闲家3张，判断剩余14张牌是否和牌，若能和牌，则是三向听；
    穷举法向配牌中任意加入5张牌，然后任意剔除庄家5张/闲家4张，判断剩余14张牌是否和牌，若能和牌，则是四向听；
    穷举法向配牌中任意加入6张牌，然后任意剔除庄家6张/闲家5张，判断剩余14张牌是否和牌，若能和牌，则是五向听；
    穷举法向配牌中任意加入7张牌，然后任意剔除庄家7张/闲家6张，判断剩余14张牌是否和牌，若能和牌，则是六向听；
    考虑七对子保底六向听，若六向听的判断失败，则表示前面的算法有bug。

## 恶调的十三向听

考虑公开情报时，若手牌的待牌全部被公开，导致绝对摸不到待牌，则会达到“十三向听”。例如南家这样的配牌：
124578m 1234567z，乍一看是国士无双五向听，但是：

东家四连暗杠369m3p，打掉的牌被北家捡走；北家从东家碰了147m北，打掉4张牌；西家从东家碰了258m西，打掉4张牌，北家跟打4张牌；
东家共计打掉9张牌，但被西家北家捡走8张，因此剩下1张。
牌河13张牌，加上5张宝牌指示牌，共计18张牌，这18张牌分别是2p东南白发中各3张。

此时南家第一巡摸到2p。从公开情报来看，南家不论打哪张，剩下的十三张牌都是废牌，因此南家是从五向听瞬间掉到十三向听。
十三向听的手牌，需要用穷举法，在14张手牌中任意加入14张牌，然后任意剔除14张，判断剩余14张牌是否和牌，若仍然不能和牌，则是无穷向听。

## 重复组合数

听牌，从34种牌中，任选1张牌，选法有34种。

一向听，从34种牌中，任选2张牌，由于可以重复选择，这就要涉及到重复组合数的概念了。
数学上大家熟悉的是不重复组合数，从34张牌选出2张不同的牌，选法有(34×33)÷2=561种。
若选择2张牌相同的牌，则选法有34种，因此重复组合数共计595种。

二向听，从34种牌中，任选3张牌，这里的重复组合数就比一向听复杂多了。
首先是从34张牌选出3张不同的牌，组合数有(34×33×32)÷(2×3)=5984种；
然后从34张牌选出2张不同的牌，一个取对子，另一个取单牌，因此这里不用组合，而使用排列，排列数有34×33=1122种；
最后从34张牌选出3张相同的牌，则选法有34种，因此重复组合数共计7140种。

三向听以上，重复组合数的分支计算变得很多，因此有必要整理出重复组合数的计算公式。
其实观察一下就能发现，对任意一种组合，进行排序之后，每个元素都加各自所在的下标，就可以等效转换为普通组合数了。
例如，34选2的重复组合数，由于数字是0～33，下标是0～1，因此加下标后相当于0～34，也就是35选2的普通组合数了，(35×34)÷2=595。
34选3的重复组合数，由于数字是0～33，下标是0～2，因此加下标后相当于0～36，也就是36选3的普通组合数了，(36×35×34)÷(2×3)=7140。

依次类推：
- 34选4的重复组合数是(37×36×35×34)÷(2×3×4)=66045；
- 34选5的重复组合数是(38×37×36×35×34)÷(2×3×4×5)=501942；
- 34选6的重复组合数是(39×38×37×36×35×34)÷(2×3×4×5×6)=3262623；
- 34选7的重复组合数是(40×39×38×37×36×35×34)÷(2×3×4×5×6×7)=18643560。

六向听的18643560是个很大的数了，如果在剔牌时，也把全部可能性穷举一遍，那么庄家剔除7张牌的普通组合数是14选7，也就是3432；
闲家剔除6张牌的普通组合数是13选7，也就是1716。就算用着色器将工作量分担到1024个计算单元中，每个工作单元的计算量仍然很巨大。
因此需要改进和牌算法，将剔除手牌和判断和牌放在一起。

## 新的和牌判断方法

定义一个新的判断和牌的函数（has_hule），这个函数允许手牌中有用不到的牌。同时为了避免递归，这里用状态机彻底解除递归。
判断一般形，为了降低状态机的规模，先排序并扫描剔除孤立牌。判断七对子，可以使用剔除后的牌，但是判断国士无双时，就必须使用原始牌了。
孤立牌有以下2种可能：

- 绝对孤立的数牌和字牌，例如单张2m且没有13m，则2m就是孤立牌；
- 半靠的单张数牌，例如233p且没有14p，则2p就是孤立牌；223s且没有14s，则3s就是孤立牌。

剔除之后，就可以用10个变量作为状态机，使用状态机进行穷举了。

第一对变量是第一次跳过的牌数和移除的组合。
牌数可以是0、1、2……只要剩余牌不够用就可以结束递归。
移除的组合可以是刻子、对子、顺子；移除之后，进行后续步骤。

第二对变量是第二次跳过的牌数和移除的组合。
牌数可以是0、1、2……只要剩余牌不够用就可以结束递归。
移除的组合可以是刻子、对子、顺子；但是若之前已经移除了对子，则本次不能移除对子；移除之后，进行后续步骤。

第三对变量是第三次跳过的牌数和移除的组合。
牌数可以是0、1、2……只要剩余牌不够用就可以结束递归。
移除的组合可以是刻子、对子、顺子；但是若之前已经移除了对子，则本次不能移除对子；移除之后，进行后续步骤。

第四对变量是第四次跳过的牌数和移除的组合。
牌数可以是0、1、2……只要剩余牌不够用就可以结束递归。
移除的组合可以是刻子、对子、顺子；但是若之前已经移除了对子，则本次不能移除对子；移除之后，进行后续步骤。

第五对变量是第五次跳过的牌数和移除的组合。
牌数可以是0、1、2……只要剩余牌不够用就可以结束递归。
移除的组合可以是刻子、对子、顺子；但是若之前已经移除了对子，则本次不能移除对子；若之前没有移除对子，则本次只能移除对子；
移除之后，和牌成立。

若和牌不成立，则进行状态机的更新，并重新来过。直到最后都无法和牌时，状态机结束，进行七对子的判断。

判断七对子，只需统计数量在2以上的牌有多少种，达到7种就是七对子和牌成立，否则进行国士无双的判断。

判断国士无双，只需统计幺九牌的存在和雀头的存在，使用掩码在一个变量上统计所有幺九牌的数量，雀头也使用掩码记录。掩码判断成功则国士无双和牌成立。

若以上三类全部判断失败，则表示未和牌。

新的和牌判断函数暂时命名为“含有和了”，其着色器代码见“shaders/has_hule.cs”，负责运行该着色器的C++语言的代码见“src/has_hule.cc”。

## 计算配牌向听数

通过“含有和了”的判断，就可以用穷举法进行向听数的计算了。计算向听数需要一个公共模块和若干个独立的着色器程序，如下：

    shaders/common.cs
    shaders/tianhu.cs
    shaders/tingpai.cs
    shaders/xiangting1.cs
    shaders/xiangting2.cs
    shaders/xiangting3.cs
    shaders/xiangting4.cs
    shaders/xiangting5.cs
    shaders/xiangting6.cs

其中，shaders/common.cs是公共模块；
shaders/tianhu.cs需要1个工作组，shaders/tingpai.cs需要34个工作组，shaders/xiangting1.cs需要595个工作组；
二向听以上，就需要将所有的重复组合数分担到1024个工作组中了。
负责运行这些着色器的C++语言的代码见“src/xiangting.cc”。
