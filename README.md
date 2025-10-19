# Database-OPENGAUSS-POSTGRES
INTRODUCTION : 评估和实验设计原则:

1.控制变量的方式设计实验，保持硬件，网络等无关数据库系统性能的条件一致，对于特定的功能的分析查询要使其他的性能在此问题
上尽量保持相等。

2.保持对比的公平性，尽量实现精准的对比，在使用数据库时坚持使用explain analyze等语句来获得更详细精确的数据，深入了解数据库的操作
方式

3.不仅分析简单的时间差别，而是尽量深入数据库的原理，对于原理进行学习，基于原理进行实验设计和分析，原理包含oid加二进制的存储方式
缓存系统管理以及磁盘io等机理，对于数据库的操作：从parser解析，到磁盘读取，再到调用函数进行完整的分析

有关数据库的原理的学习和调查，可以见part2两个DBMS系统对比的部分对于数据库的原理和性能的深入分析。PART 1 中C++与DBMS的对比实验在学习这些原理
之后也做出了一些调整。

4.实验中参与分析的数据可能包括：​内存与I/O： shared_hit,shared_read,buffer_alloc,buffer_backend...​吞吐量： 每秒完成的事务数 (TPS) 或每秒查询数 (QPS)。​延迟：
平均延迟 (ms)
尾部延迟 (如 P95, P99, P999 延迟 - ms)
最大延迟 (ms)
​资源利用率：
​CPU：​​ 用户态%、内核态%、空闲%、等待I/O%。
​内存：​​ 使用量、换页/交换活动。
​磁盘 I/O：​​ 读写吞吐量 (MB/s)、IOPS、平均 I/O 等待时间 (ms)
​网络：​​ 流量 (如果涉及）
​数据库内部指标： pg_stat_database / pg_stat_database (PG) 和 OpenGauss 对应的统计视图 (pg_stat_database, dbe_perf.statement, dbe_perf.session_wait 等)。
缓冲区命中率。

5.报告结构：分为PART1 C++对比DBMS 和 PART2 POSTGRES对比OPENGAUSS
每个part里面的一个大实验包含两个小实验的实验过程，结果对比和实验结论，实验过程中穿插了对数据库原理分析，读者若想快速看到结论可以直接进入一个单元
的底部。

Part1：对比C++和DBMS系统
1.原理解释：DBMS系统对比简单的文件操作拥有以下方面的优势：快速的I/O吞吐，建立并管理的独立的数据库缓存，高速的查询操作，查询指令优化的parser功能，对于csv格式的数据的更完善且安全的解析，回滚机制，多线程同时处理的并发机制

2.实验设计：（1） 选择数据集，选择数据量相对大的数据集，本文中文件2.4G，数据量大于1000万条，这样才能对于大量数据场景下的操作进行有效的对比
（2） 选择硬件系统和设备：确保C++文件和数据库在同一台电脑，使用相似的运行环境（比如docker里和visualstudio里要分配一样的CPU数量）
（3） 实验部分：对于简单查询，按索引查询，阅读与改写的性能对比
（4）重要！：C++程序的编写为了利于对比有很多值得审视的地方，经过错误的设计尝试，我总结出了本次实验中C++设计遵循的几个principle：
principle1 数据格式 在编写C++程序时我们为了尽量使C++和数据库的性能具有可比性，因此我们存储数据的方式为string，每一个数据被 存储成一个string，然后各列用vector存储，这样可以体现对于数据库存储和使用数据的方式的对比，因为数据库使用了极其高效的数据读写存思路：
数据库存用数据流程图：

解析：识别出42是integer类型（OID=23）
编码：将42转换为内部二进制格式（4字节整数）
存储：写入磁盘时包含
数据头（包含长度、NULL标记等）
类型OID（标识这是integer类型）
实际数据（二进制形式的42）
读取：根据OID找到对应的类型处理函数来解析数据
C++: 存储为string格式，匹配时调用经典的C++函数，如find，这些函数具备简单高效的特征，可以作为baseline和数据库的类型函数较好的对比
principle2 C++程序设计使用chrono设计了timer类，对于整体时间和search（排除读取） 的时间进行分别时间统计，以确保C++和数据库都是在统计搜索这一单项的用时，达到控制变量的效果;

principle3 尽可能采取和数据库一致的基础优化，让大部分的操作尽量做到内存连续等基础优化，保障对比公平性！ 数据库postgres采取了行存，opengauss采取了行列存，因此设计的C++程序仍然也是行列存！

principle4 C++程序使用std标准库进行读取的过程中实际上也经历了从磁盘取数据到页面缓存的过程，因此我们对于C++的函数都实现对于具体功能的单独计时
和对于整体执行过程的总时长计时，采取这样的对比方式更加科学。

MAIN EXP E1.简单查询：

THEORETICALL ANALYZE基于原理的实验设计：C++首先通过loadcsv函数读取文件并把数据库内容存储成vector<vector> 的格式，
在无需通过数值判定进行等值或范围查询的情况下，这种数据存储方式我认为与数据库的较为接近。

数据库原理分析：在postgres源码阅读中，我发现数据库先定义了类型编码oid然后把数据存成了二进制类型
并且创建表之后并非真的拿int,string之类的数据类型存储，而是使用了一个类型码的方式先通过类型码确定了对应的比较和搜索函数。 这意味着真正的比较运算并非判断两个int或string值是否相等，而是调用了oid码对应的比较函数，这点其实也是数据库搜索优于C++的一大原因！

因此在真正的设计中，我们使用string储存数据可以反映数据库在这一方面的优化，根据principle2，这样的设计是合理的。

exp1.相等简单查询：使用C++实现了查询一列中满足x=a的元素的查询程序

exp1.C++实验结果： image.png

数据分析：整体运行时间三次取均值为57884ms,搜索对应数值的索引耗费时间平均34539ms,单纯统计热启动时的平均耗时约28000ms

C++程序的运行无比缓慢，不过这点也可以理解，因为C++为了作为baseline使用的存储方式是vecotr，
不考虑创建结果数组和输出和其他耗时，单纯的搜索到相应索引花费的最低时间就已经达到了21158ms
更耗费时间的是第一次加载CSV时的loadcsv函数，但是这点因为数据库本身就已经把数据存储，我们为了更好的比较性能这一方面
默认忽略loadcsv读取的时间

exp1.DBMS实验设计：为了更好的了解数据库的比较方式（对于oid码不同的对象使用了不同的函数我们将尝试对于不同的类型的属性的查询），
这里面冷启动热启动带来差别： exp1.DBMS实验内容：1.使用语句analyze explain select * from taxi_trips where vendor_name = DDS;重复三次 2.analyze explain select * from taxi_trips where trip_distance = 1.60000001;重复三次

exp1.DBMS实验结果： image.png

可以看到冷启动时数据库的耗时在20000ms以上，热启动时数据库三次实验的平均耗时为3152ms

EXP1CONCLUSION： 有趣的现象是数据库的冷启动操作和C++的热启动操作耗时接近，在启动方式相同的前提下数据库的操作速度远高于C++

原理解释：
数据库使用OID存储对象类型，并且根据OID调用独特设计的比较函数的方式十分的高效，
这点会让数据库在相等比较的情况下性能远高于存储方式都是string的简单设计的baselineC++程序！ 并且，考虑到C++还有额外的读取，并且存储结果的时间，可以看到数据库在查询方面的设计的优越性！

exp2.实验操作：使用C++实现了一个简单查询程序，查询特定类中是否存在形如'%d%"的元素，数据库中也使用同样操作

exp2.CPP实验结果1：可以看到，C++的程序运行了101678ms，换算成秒数是101simage.png 这里我们希望分析以下为什么这个查询会如此缓慢：比较显然的原因是这里的C++模拟实现的是通配功能，要比直接验证两个项是否相等的=查询缓慢许多， ！！！但是，真正导致查询操作缓慢的真实原因是：C++的查询操作中每次检测是否匹配都要将匹配到的result的整个row pushback进入新建立的result数组
这样的个大型数组的逐步创建占据了巨大时间，相比于直接存储了数据的数据库可以直接记录数据指针并且打印来讲慢了很多！！因此，这次的查询主要
证明了数据库对于指针的使用，操作耗时的管理优于粗糙设计的c++文件。

exp2.CPP实验结果2：为了保障实验的公平性和更好的验证C++的查询时间，我们改写C++程序如下，采用先创建index数组
，在匹配过程保存index之后立刻停止search计时的方式进行搜索时间统计：image.png 并且做了优化reserve一个内存给结果数组，防止动态扩容！

这样的设计方法对标了数据库的原理：在查询时并非创建一个新的大块数据出来，而是记录查询到的结果的索引，再在输出时取出来，这样的方式
保障了查询的高效性，记录了C++和DBMS系统比较时数据库更深层的优化（前方的理论分析中提到，通过oid读取存储并调用比较函数）， 这个优化也应用于了exp1，即exp1中的c++程序已经做了记录索引和输出，创建结果数组的分离

经够改写之后，得到新的运行时间如图：image.png

三次平均的运行时间49704ms，在冷启动之后的热启动的平均运行时间为26983ms，速度有显著提升

exp2.DBMS实验结果：image.png
可以看到冷启动的时间是23487ms，image.png
热启动的平均时间是3499ms

exp2 实验对比结论：

可以看到完善C++程序之后仍然是数据库的查询语句更加快速，在冷热启动上数据库系统都在通配类查询上表现更佳。
数据库的查询的主要瓶颈是全表扫描的时间

原理上进行分析，数据库先读取数据，命中数据块之后进入MVCC(多版本并发控制）系统，检查对于当前事务可见的数据 之后更新尚未更新的数据，最后进行模式匹配。和本实验中的C++程序进行对比，可以发现数据库可能更快的优化在于
检查对于当前事务可见的数据这一项，C++操作文件是先取出了数据库中一列的表头，然后再进行检查。

从原理的角度解释这样的现象：第一次查询为冷查询，其耗时主要包括了从磁盘读取数据页并加载到DBMS缓冲池和OS页面缓存的I/O开销。而第二次查询为热查询，所需数据页已在内存中（高缓存命中率），绝大部分时间消耗在CPU计算上，从而导致了XX倍的性能提升。这个实验直观地展示了内存缓存在数据库性能中的决定性作用。

MAIN EXP E2.构建B树的查询：B树是数据库的高效排序数据结构，是一个每个父节点可以有（N>2)个子节点的树，这种数据结构实现了O(nlogn)的时间复杂度 ，在按index查询时这样的操作远快于在普通C++文件通过index进行遍历搜索

E2.1实验过程：在opengaussz之中：
实验语句：我们先使用create index idx_trips on taxi_trips(trip_distance)
image.png
创建索引的过程不算快，但仍然超过了C++的操作速度。
建索引之后对于数据进行查询发现速度并没有想象中那么快：image.png 之后我们重复了几次（前几次，缓慢的原因我们认为可能是数据没有被正确存储到缓存）： image.png 发现时间仍然不够快，但是比起之前的5000多ms的时间快了许多，到了3000左右的水准，这一部分原因暂时不分析，因为真正的搜索时间并非totaltime，而是 分析函数之中显示的真实搜索时间

！注意到时间中真正的搜索时间为actual time，在0.010-0.011ms之间，因此可以判断这部分的主要事件并非搜索时间而是读取或输出的时间，造成总时间差异的也非搜索差异，为了验证我们已经成功的创建索引，使用\d taxi_trips，验证了索引已经创建

据之前查询语句得到的信息，返回的行数基本上包括了整张表，这也许说明了查询的过程可能与其说是一次B树扫描，
不如说是一次seqscan全表扫描，而非indexscan，没有体现出B树搜索的高效性！

E2.2这次为了让totaltime更好反映Btree索引，我们使用了另一个range，使数据库返回更少的行数，来记录真正的搜索时间： EXPLAIN ANALYZE SELECT * FROM taxi_trips < 1.5; 可以看到actualtime为0.0004，这里面的因为搜索返回的函数更少，时间更快，因此我们可以

image.png

可以看到总体时间也小了很多，虽然还存在总体时间远大于搜索时间的状况

进行原理上的分析，我们发现对B树这种数据结构，在按照范围搜索的时候因为这个范围太大会每次都寻找父节点下最大的子节点而非所有节点，因此本来B树深度只有logn级别，每层只找一个子节点的方式自然会让查询操作十分迅速！因此一个dbms系统的查询速度是显著高于没有构建B树索引的C++的！

在C++中我们无需构建B树，直接进行查询并且对比速度：

MAIN EXP E3.哈希查询 1实验过程：构建哈希表的查询：除了构建B树，数据库还提供了构建哈希表等查询方式，对应着select语句中的 where a like "%a%"的语法，先试一下不创建哈希索引的数据库查询操作： image.png image.png

接下来使用语句 create index idx_pay on taxi_trips using hash(payment_type);创建哈希
发现这句语句的执行花费了非常非常长的时间：timingon之后运行结果一直没结束，等待了一个半小时之后忍无可忍，只好决定检查一下正在运行的操作：
image.png 即使数据库构建了哈希之后查询会很快，但是构建哈希的时间哪怕只有1000万数据都要至少花上1h31min，对于非极高极其频繁的查询
显然是直接查询速度更快。因此在当前的场景我们认为构建这个哈希查询没有意义，暂停实验。

2！！原理分析：之所以哈希构建如此缓慢关系到了一个参数maintenance——work_mem,即单个维护操作的最大可用内存量
在创建索引时，通常需要对数据进行排序。如果要排序的数据量能够完全放入 maintenance_work_mem 定义的内存中，排序操作将在内存中快速完成。如果内存不足，数据库就需要将中间结果写入磁盘临时文件，这会带来大量的磁盘I/O，严重拖慢索引创建的速度,更大的内存设置可以显著加速大规模索引的构建image.png
这里的哈希索引创建的这么慢，我认为跟maintenance_work_mem十分相关

3 补充：哈希锁测试：此时为了体现数据库的哈希创建带来的消极影响，我们并发开一个进程，在创建哈希运行的同时创建一个B树索引 create index concurrently idx_payment on taxi_trips(payment_types)；发现B树索引迟迟未创建： image.png 终止了哈希之后重新创建B树用时极快：image.png

这说明了哈希锁确实阻止了对于数组的操作！

分析背后的原理，哈希值一旦对象改变就会发生变化，因此数据库在创建哈希的时候会加上排他锁，其他的进程不能进行对数据的读写，
从这个意义上比较，数据库和一个哈希，B树构建操作完善的C++文件相比，缺少了在索引创建上的灵活性！

4.优化哈希创建速度：由于之前哈希索引的创建十分缓慢，分析原理之后，将尝试一次调大maintenance_work_mem之后的哈希创建测试
首先对maintenance_work_mem这一项进行查询，image.png
结论惊人！！maintenance_work_mem只有16MV，数据库能把index存在这么小的缓存里面才怪！！所以为了真正检验数据库的创建哈希的性能
我们应该在创建前调整这一参数：set maintenance_work_mem = '2GB'; 调整之后，尝试重新创建哈希：

结论CONCLUSION:对比数据库构建了哈希的查询为何比C++正常查询快，快了多少：发现构建哈希的时间过长，反而会导致对于非高频操作总共耗时大于
C++普通的查询； 启示INSPIRATION:在使用数据库执行大型的操作的时候应该先提升maintenance_work_mem这一参数，防止索引创建时频繁的磁盘I/O（索引写入磁盘临时文件）；

MAIN EXP E4.改写操作对比：C++缺点在此无限放大，因为C++的改写需要I/O操作，每次都需要先读取然后再进行一次保存，但是对于DBMS
数据本身就按照其格式被保存在系统之中，无需考虑这样的问题，因此相对于普通的文件操作数据库主要的优势是自行管理缓存
无需读取或再整理数据，因此会在I/O上性能远超C++ 数据库操作下的改写：
在进行对比实验之前我们先进行了一个简单的尝试感知了一下数据库的改写操作， 我们先尝试了一个贴近真实情况，又有较大的改写量，需要对于一列的所有元素进行改写的语句的，语句如下：

-- 场景：将所有现金支付（CASH）订单增加10%附加费 UPDATE taxi_trips SET surcharge = Fare_Amt * 0.1, Total_Amt = Total_Amt + (Fare_Amt * 0.1) WHERE Payment_Type = 'CASH' image.png 可发现后三次运行时间在2400-3000ms，平均运行时间为2550ms，第一次运行花费了3092ms，可以反应冷启动和热启动带来的差别 不过即使是冷启动，数据库的效率也远超C++程序

对比试验，基础的改写： 执行一个简单的C++程序易于执行的SQL语句，explain(analyze,buffers) update taxi_Trips set vendor_name = 'VTS' where vendor_name = 'VTT'; 得到结果如下：

image.png 可以看到 C++操作的改写： C++的改写：设计一个C++的程序，寻找改写对象index的过程加上改写过程记录为writingtime，而仅仅改写的过程记录为purewritingtime： 实验结果如下： image.png

结论与分析： 可以看到writingtimeC++用时平均16000ms左右，效率反而高于了数据库，这是因为C++的改写直接使用了等于赋值，而数据库中改写需要先标记
原来的元素的OID从xmin变化为xmax，然后插入新的对象并且创建它的oid及一系列相关记录，数据库的改写虽然牺牲了一定的时间，但是对于多类型的处理 远比简单C++更加完善，并且具有可回滚的robust性质。

Part2 对比OPENGAUSS POSTGRESQL

SECTION1简介和原则：对比两个数据库可以从以下方面入手： Performance Bottleneck: The processing speed of the CPU and the bandwidth of the memory. We refer to such cases as CPU-bound operations.
Efficiency of the Buffer Pool: This directly demonstrates the critical im- portance of the **database buffer pool (Buffer Pool / Shared Buffers). It is a carefully designed and managed memory area — the lifeline of database perfor- mance.
Importance of Algorithmic Efficiency: Once the I/O bottleneck is eliminated, the efficiency of the algorithms executed by the CPU becomes the dominant factor. If the string matching algorithm is poorly implemented, the execution time will remain long even under a warm start.
主要的对比依然会集中于时间对比之上，当然这里面我们为了更好的深入机理，也会对内存i/o rollback等机制进行讨论

SECTION2分析工具选择：

postgres自带了十分便捷的analyze系统可以很好的记录时间和缓存
对于opengauss进行buffer或cache的分析比较麻烦，需要如下的语句： -- 开启扩展（需管理员权限） CREATE EXTENSION pg_buffercache; SELECT datname, blks_hit AS 缓存命中块数, blks_read AS 磁盘读取块数, round(blks_hit * 100.0 / (blks_hit + blks_read), 2) AS 命中率百分比 FROM pg_stat_database WHERE datname = current_database();
SELECT * FROM pg_stat_bgwriter;

SECTION3.数据集的选择，实验环境搭建： 我们选择了yellow——taxi——trips数据集，共有18列，包含13380122条数据，数据量较大，可以较好的实现对于大量数据查询的对比：image.png
为了公平比较性能，数据库的使用者应该会

SECTION4！！.一些关于原理的调查和可能的验证方案：其实opengauss主要沿用了postgres的架构，但是在如下点有一些优化： POINT1 行存储和列存储：image.png opengauss由于可以按列存储，因此在列方向上内存连续，可以更好的支持取一个列的平均值之类的按列操作，这点
我们会在后续实验验证， 按行连续存储可以更好的处理频繁的增删修改，这点opengauss也实现了，也就是说openguass同时支持行存储和列存储

POINT2 事务id的优化：当数据库修改数据时不会修改原始数据，而是先在存储时产生事务idxmin，xmax，当变化为新数据的时候把旧数据的事务
id从xmin切换到xmax， 读数据的时候数据库只能看到xmax > 当前事务id的数据内容
postgres的32位事务id可能导致意外，如耗尽rollback，但是opegauss优化到了64位

POINT3：关于buffer：***补充：buffer的指标和mechanisim： buffer是指数据库为了避免从磁盘和OSpagecache里面频繁取数据创建的一个自己的内存管理区域
流程图如：image.png

buffers_alloc 是指缓冲区分配计数，也就是从数据库启动开始累计分配新槽位的次数， image.png

buffers_backend - 后端进程直接写入的数量，尽量为0

在暴力的不创建索引的查询中大概率buffer_alloc指标会暴涨，这点将在下面的蛮力搜索中观测,顺便验证一下冷热启动的差别

POINT4 处理大量并发请求：postgres使用多进程模型，而opengauss使用线程池，线程池即在空闲时预先创建线程，新连接到来
再取出线程取回应，结束后回收进入池供再次利用，比postgres的暴力开操作系统进程的方式应该高效不少，这点也可以验证一下： image.png

POINT5 指令优化器：opengauss对于parser的优化在于对于简单指令不做复杂的优化搜索而是直接执行，并且用模式识别直接进入fushion算子
image.png 这点我们可以通过指令优化耗时parsetime对于这个优化器进行分析

这里我深入调查了fushion算子这一概念，直白的讲，opengauss可以把复杂的查询操作在数学公式层面上直接转化为更简单的算子操作
这样会让opengauss的查询更加高效，这一点不是对于优化器耗时的优化，而是对优化器效果的优化 ！

经过调查，opFUSHION主要针对的情况是对于简单的INSERT,DELETE,UPDATE，不支持复杂的多表join查询，opFUSHINON出现的关键词是：
在查询plan中出现子节[byPASS]，便说明opengauss使用了独有的fushion算子优化，依据这个调查，接下来的实验会考验opengauss在
有fushion优化和无fushion优化时的不同表现

5.实验前的检查和控制变量:
实验中将对齐的几个重要参数包括了：shared_buffers,work_mem image.png
先记录一下opengauss在容器中运行的各项参数

使用docker运行opengauss的时候还需要检查一下docker里面opengauss容器的状态：
image.png
补充一下CPU状态：image.png

再检查一下postgres的状态：
image.png
可以看到shared_buffers, work_mem在两者之间存在差异，为了更公平的对比，我们通过SET语句进行对齐

alter system set shared_buffers = '1GB';alter system set work_mem = '64MB';

后续的实验将对齐这两个标准。
另外，注意到还有random_page_cost这个代表了从磁盘读取数据成本的参数，经过查询两个系统的这个参数都是4

MAIN EXP E2.对比实验:蛮力搜索 实验目的:蛮力搜索的主要目的是测试数据库的I/O性能，即从磁盘读取数据再到建立缓存再到搜索并取出数据的用时
方法设计：select count(*) from taxi_trips where vendor_name = 'DDS';
为了不仅对比数据库的时间性能差异，更深入分析造成时间差异的原因，我们将记录buffer_alloc等内存使用指标和情况， 记录shared_hit记录热启动后是否正确命中数据库管理的缓存，以更深入的对比两个DBMS系统

实验发现与思考：这次的查询中我们使用analyze仔细分析queryplan，发现实际上操作经历了两个阶段，第一个是优化器的planning
第二个是即时编译阶段JIT；另外，最初的分析中我们使用了数据库默认的workmem和sharebuffer，但是后期为了更深的分析性能 将两个DBMS系统的参数进行了对齐!

E2.1暴力io搜索 oppengauss实验结果：其实这里的实验结果和上方的是共享的，但是为了实验的严谨性我们重新运行多次 select count(*) from taxi_trips;

image.png

这里不同的点是要使用postgres进行一次暴力查询，postgres实验结果：
POSTGRES.1:冷启动：image.png
这一波冷启动操作总耗时2627ms，优化耗时仅2.695ms，不难看出sharedhit极少只有2000多，大部分数据从磁盘read
了出来

POSTRGRES.2 : 热启动：image.png
显然热启动耗时更短，最大耗时521ms平均耗时460ms，不过我们可以看到最后shared_hit 仍然只有2000多，
远小于read，即从磁盘读取的数量（260000），既然I/O操作的耗费时间没有变化那么为什么时间变快了呢？

原来postgres的read统计无法区分从磁盘和从OS页面缓存读取的差别，image.png
参考图片，其实热启动和冷启动的差别在这里变成了从物理硬盘读和OS读取的差别，在OS读取时并行的worker无需同时发起I/O请求
之后排队等候，所以变快了！

image.png
通过图片可以先看到刚才的postgres
系统中sharedbuffers和workmem 的数量极小不能存储大量数据，这可能会是热启动仍需大量IO的原因，此时我们还没有遵循实验中约定的对齐sharedbuffer
和workmem的要求，因此我们使postgres的这两个参数对齐opengauss之后重新运行一次实验（buffer = 1GB，workmem = 64mb），发现：
image.png image.png image.png image.png 出于同样的原因，后几次实验变快其实是缓存从磁盘取出到缓存从ospagecahce取出带来的差别

E1实验结论： 总体上postgres的性能优于opengauss，原因分析： 在暴力的IO搜索中两者都经历了读取数据（read而非sharedhit）的取数据过程，但是postgres系统可能更好的把数据存在了OSPAGECACHE,OPENGAUSS则可能一直从
内存读取数据；另外，从queryplan可以看到聚合函数aggregatecuont（*）的操作计时极短基本上对总体时间没影响，差距就在查询中：postgres使用了并行查询
的方式进行优化，而opengauss中好像没有显示这一点，两者的plannigtime都在0，1ms左右也对时间几乎无影响，所以可以得到结论：

postgres在简单搜索上实现了更好的并行处理，并且有可能在和docker中的linux系统交互时实现了更好的pagecache处理。另外，对于数据量在1000万条的数据集
可能postgres默认的sharebuffers设置更佳；不过虽然sharedbuffer和workmem变化了，但是queryplan中的hit read比例几乎没变，因此查询时间的变化大概率 和内存无关吧image.png

E2.2 关键词暴力搜索 实验方法：对于taxitrips表使用语句 select * from taxi_trips where vendor_name = 'DDS'; E2.2OPENGAUSS

实验结果：
冷启动：image.png 热启动的情况下：image.png

E2.2postgres

冷启动：image.png
热启动的情况下： image.png

E2.2实验结论：仍然postgres的性能优于opengauss，如上，主要的性能差距的原因很可能是postgres查询的并行处理优化，另外opengauss对于
查询指令和查询操作的优化并没有在这里被明显体现出来。

MAIN EXP3.对比实验:索引查询 简介：索引查询是通过B树对于comparable的元素建立大小排列以便查询，同时这里我们也将尝试哈希查询的方式：先建立一个哈希索引，然后进行查询会使对于特定 字段和文本的查询速度大大加快

原理分析MECHANISIM:这里建立索引后的查询方式发生变化，从indexscan转化为bitmapscan，seqscan，为了充分的对比分析索引查询性能，
我们将通过变化查询语句在两个数据库中对这两种查询方式的性能进行验证，机制差别如图：
image.png

OPENGAUSS实验过程：

part1查询尝试与原理分析： 我们先使用create index idx_trips on taxi_trips(trip_distance);创建索引，之后对于数据进行查询发现速度并没有想象中那么快：image.png 之后我们重复了几次（认为数据没有被正确存储到缓存）： image.png 发现时间仍然不够快，但是比起之前的5000多ms的时间快了许多，到了3000左右的水准，大概是数据已经被加载到了缓存之中

此时注意到时间中真正的搜索时间为actual time，在0.010-0.011ms之间，因此可以判断这部分的主要事件并非搜索时间而是读取或输出的时间，造成总时间差异的也非搜索差异，为了验证我们已经成功的创建索引，使用\d taxi_trips 查询，发现image.png
索引已经创建

据之前查询语句得到的信息，返回的行数基本上包括了整张表，因此这次为了让totaltime更接近对索引查询，我们使用了另一个range，使数据库返回更少的行数，来记录真正的搜索时间：image.png 可以看到actualtime为0.0004，直接可以搜索到整个树里面所有的行数 EXPLAIN ANALYZE SELECT * FROM taxi_trips WHERE trip_distance > 50; image.png

可以看到总体时间也小了很多，虽然还存在总体时间远大于搜索时间的状况

进行原理上的分析，我们发现对B树这种数据结构，在按照范围搜索的时候因为这个范围太大会每次都寻找父节点下最大的子节点而非所有节点，因此本来B树深度只有logn级别，每层只找一个子节点的方式自然会让查询操作十分迅速！

当然这样的现象也可以引发一个另一个猜想：数据库其实在查询的时候的核心耗时不只是寻找数值，有一部分耗时是把寻找的的值对应的索引储存起来

part2 对比实验： BITMAPSCAN:
EXPLAIN ANALYZE SELECT * FROM taxi_trips WHERE trip_distance > 5.0 image.png opengauss在这里使用bimapscan扫描，前几次无法把数据正确读取到缓存之中十分缓慢，查询时间平均约46000ms；
image.png 存储到ospagecache缓存之后，查询速度指数级下降：
image.png
完全预热之后，查询的平均时间约1945ms

SEQSCAN: image.png secscan经过预热之后，查询时间仍然在4600ms

POSTGRESS实验过程 对于postgres进行分析和实验： 先运行EXPLAIN ANALYZE SELECT * FROM taxi_trips WHERE trip_distance > 5.0； image.png
发现查询采用了BITMAPSCAN的方式并且相对缓慢，分析原因发现其实是数据没有被存进缓存，大部分都是sharedread的结果 我们尝试用select * from taxi_trips where trip_distance < 100;因为大部分的数据都满足此条件因此触发了全表扫描
由于前几次的全表扫描已经读取数据到磁盘，全表扫描竟然快于部分扫描：image.png

但是这并没有公平体现两种查询方式的差异。因此我们预热后重新进行>5.0的bitmapscan查询，发现： image.png
说明创建B树确实有效加速了查询过程，bitmapindexscan只用了0.01ms，heapscan占据了主要的查询时间。约1000ms 整体的executiontime约1145ms

E2.CONCLUSION对比试验结果： 在创建B树index的查询上，postgres的bitmapscan速度高于opengauss，全表scan的seqscan因为缓存管理等原因也优于opengauss，平均时间约2700ms 为opengauss扫描seqscan时间的一半。 总体上讲，在创建索引或者依据索引的查询上postgres性能更佳！

MAIN EXP 4.对比实验:快速改写 两个数据库改写其中元素的能力可以通过批量改写进行对比，在真正的业务场景中，往往改写不会针对所有元素或者需要对于列的数量有一定的限制，但是为了对比 两个数据库的性能同样我们也可以大量改写

原理分析：其实改写分了几个部分，首先第一部分是找到改写的目标，这里考验的主要是数据库的查询能力，其次是改写之后对于产生的数据暂时保存于
缓存之中，形成了DIRTY PAGE，之后通过bgwriter再写入磁盘，每个部分对于数据库都对应了不同的部分的性能，工作原理如图：image.png。
当然这里面应该不会考验到bgwriter的能力，因为bgwriter会默默的不断的寻找空闲的缓冲区，真正的改写进程总是能找到可用的干净页，因此这点上
除非执行十分大规模的超大改写我猜测应该无法考验数据库的改写能力，为了探索一下我打算使用：SELECT * FROM pg_stat_bgwriter;进行监控

因为opengauss在这点声称了有如下的优化：

并行刷脏页 多线程同时写入不同脏页（bgwriter_thread_num参数） 目的：充分利用多核CPU和SSD并发能力
智能跳过热页 跳过最近被访问的脏页
避免把即将用到的数据误删改
NUMA感知 优先清理当前NUMA节点的脏页 减少跨节点内存访问开销 这几点优化都比较难以分析，因此主要还是基础的对比，对于buffer的探索仅仅是附加项
E1 ： 最简单的快速改写： explain (analyze ,buffers) update taxi_trips set vendor_name = 'DDD' where vendor_name = 'DDS'; explain (analyze, buffers) update taxi_trips set vendor_name = 'DDS' where vendor_name = 'DDD';

POSTGRES 实验结果： image.png

OPENGAUSS实验结果： image.png

对比与实验结论：可以看到整体执行时间上postgres的平均时间约4000ms远快于opengauss大于20000ms的平均时间，检查两者的queryplan之后发现 疑似两者的sharedhit sharedread总和时长不同。 进一步分析，发现opengauss的seqscan时间远长于postgres，并且opengauss的改写操作时间也远大于postgres，这里面我们可以进一步得到
postgres的脏页数量远小于opengauss，一个约40000，一个约10万，这可能是两者在改写时间上的巨大差异的原因！
另外，两者的性能仍然因为seqscan的速度差距明显产生了差别。 所以整体上得到结论：postgres的改写性能优于opengauss

E2 : 我们先尝试了一个贴近真实情况，又有较大的改写量，需要对于一列的所有元素进行改写的语句的，语句如下：

-- 场景：将所有现金支付（CASH）订单增加10%附加费 UPDATE taxi_trips SET surcharge = Fare_Amt * 0.1, Total_Amt = Total_Amt + (Fare_Amt * 0.1) WHERE Payment_Type = 'CASH';

OPENGAUSS运行结果： opengauss运行三次的平均执行时间是122755ms

image.png

postgres运行结果： image.png image.png
postgres三次运行的平均执行时间是18824ms

实验结论：在改写语句的性能上postgres远超opengauss，运行时间几乎只有opengauss的十分之一，我们认为有如下原因： 第一是检索改写目标时postgres使用seqscan进行扫描的速度远高于opengauss的seqscan，但是 postgres和opengauss的dirty和written参数完全相同，这证明了在实际进行改写的时候两者的策略是接近或相同的，因此改写操作时间
存在差距可能和两个DBMS系统写入数据的方式并无关联，opengauss和postgres在纯粹的写入上的性能对比中，虽然通过减去scan时间得到的
纯粹改写时间仍然是opengauss更长，但是两者的修改方式和缓存管理方式是一样的。

MAIN EXP E5.对比实验：聚合查询和内嵌函数 E1：aggregate function 聚合函数查询：因为聚合操作比较复杂我们可以在运行这条指令的时候顺便看一下两个DBMS的parser能否 优化查询命令 E1.1:查询语句： 首先创建一个附表 create table vendors(
vendor id SERIAL PRIMARY KEY

explain analyze
select vendor_name ,count(*) as total_trips,sum(total_amt) as total_revenue,
avg(trip_distance) as avg_distance, avg(total_amt) as avg_fare
from taxi_trips group by(vendor_name);

OPENGAUSS实验结果：image.png 结果可以发现是全表扫描占了主要的时间：总体平均runtime约11532ms，实际上哈希聚合的部操作时间仅为0.01ms级别
这样的结果可能启示我们需要通过更复杂的聚合函数考验DBMS系统在聚合查询上的能力

POSTGRES实验结果：explain (analyze,buffers)
select vendor_name ,count(*) as total_trips,sum(total_amt) as total_revenue,
avg(trip_distance) as avg_distance, avg(total_amt) as avg_fare
from taxi_trips group by(vendor_name);

image.png
image.png image.png 经过冷启动预热之后的聚合查询操作花费的时间约1491ms 可以看到，聚合查询的37.8%的时间463ms消耗在了全表扫描上，另外JIT编译消耗了200ms的时间，其他时间则为哈希聚合和进行排序的时间

E1.2 我们使用更复杂的聚合查询语句，并且更细致的查看查询操作内容：EXPLAIN (ANALYZE, BUFFERS, COSTS, TIMING, VERBOSE, FORMAT JSON)
SELECT vendor_name, COUNT(*) as total_trips, SUM(total_amt) as total_revenue, AVG(trip_distance) as avg_distance, AVG(total_amt) as avg_fare FROM taxi_trips
GROUP BY vendor_name;

OPENGAUSS实验结果： image.png
image.png 这里我们可以看到其实opengauss也进行了并行查询，据详细的输出显示，actualtotaltime仅2822.682ms，实际总运行时长为11009.682ms，
但是输出并没有显示具体的耗时信息

opengauss的查询计划则更是简单： 先是哈希聚合，之后就是全表扫描，而且没有并行进程

POSTGRES实验结果：image.png
image.png
image.png
image.png
image.png image.png
从这里可以看到POSTGRES十分详细的查询计划，先是全表扫描，然后是聚合aggregate根据vendor_name进行分组
接下来的子计划是sort（by quicksort），之后再有aggregate进行均值计算等操作，
最后使用gathermerge进行合并
根据每个子plan的运行实际时间actual time分析，我们可以看到如哈希聚合操作，quicksort操作
实际上都十分迅速， 花费时间较长的操作可能还是seqscan对于整张表的扫描，里面实际总运行时长为3749ms，运行时远长小于opengauss因此无需多次实验对比性能

得出实验结论：
postgres在聚合函数查询的方向上优于opengauss，可能的原因是postgres的查询计划和并行方式比较opengauss更为先进，postgres使用并行分组查询后融合的效率远高于 opengauss，并且gather merge，quicksort等操作十分高效，让postgres用opengauss的30%时间完成了复杂的查询操作
E2 join创建虚拟表的查询

E2.1： 查询语句：
EXPLAIN (ANALYZE,BUFFERS) SELECT t1.vendor_name FROM taxi_trips t1 INNER JOIN taxi_trips t2 ON t1.trip_distance = t2.trip_distance; E2.2:
查询语句sql： explain analyze
select tp.avg_distance, vt.total_amt from taxi_trips tp join
(select vendor_name,total_amt from taxi_trips) vt;

MAINEXP E6 并发场景测试：

E6.1锁竞争测试： 数据库为了确保单线程的操作正确会使用锁锁住正在操作的数据，有三种级别的锁：行级锁，表级锁，数据库锁
在并发的请求中冲突的锁操作（两个线程争夺一个锁）会导致锁竞争，导致进程阻塞

opengauss：

CONCLUSION AND INSPIRATION

OPENGAUSS 与 POSTGRES 对比总结：

在查询的方案上可以看到postgres的查询，聚合方式优于opengauss，通过使用并行查询，先分开查再排序再聚合再merge的聚合queryplan在查询和聚合上的性能高于
opengauss，opengauss声明的而解析器优化（如fushion)算子等并没有在如上的较简单的查询场景中被触发； 在改写上同样是postgres更加快速， postgres使用的
脏页数量明显小于opengauss，体现了postgres在内存管理上的优势（这里只能说明，和docker中的虚拟LINUX环境postgres相对于opengauss达成了更佳的优化效果
如果在鸿蒙操作系统中运行据调查我认为opengauss可能反而会表现更好） opengauss行列双存储的优势并没有体现出来，因为我们的改写语句，查询语句数量范围限制不能直接下结论opengauss的行列存储对于这些函数毫无优化，但是无论是opengauss
还是postgres在内存连续性上应该都有较好的优化（比简单的c++)更强大 opengauss的oid是64位可能会拖慢opengauss对于数据的查询改写等操作，但是这点确实能为反复修改的数据库提供回滚机制安全性保障
在创建索引的查询上postgres依旧性能优于opengauss，但两者在索引创建的速度上大致相近，查询的时间差距并非按数据结构寻找索引，而是在其他的方面体现，
因此这里可以认为postgres和opengauss在查询性能上接近。

在查询方案相同的情况下，postgres的运行速度仍然高于opengauss，这可能体现了postgres在取数据，改写数据上简单的操作的高效性，尤其全表扫描seqscan这一操作上，
两者巨大的时间差距奠定了postgres在本实验绝大部分场景运行速度高于opengauss的基础。

整体上的操作体验可以参考对于数据库总体性能和历史记录的查询：

postgres：image.png
从图中数据blkshit可以看出大部分的操作都是从缓存中取出数据而非磁盘，说明查询语句和内存管理整体处于健康状态；并且回滚数量少，说明数据库状态健康

opengauss：image.png
可以看到blkhit正常，缓存管理健康，opengauss的postgres在此处的数据极为相似，这证明了两者在实验中的性能差距可能出现在别的方面，如查询计划，
并行能力等元素。

补充：一些生活场景中看到的数据库应用问题：

高铁订票平台12306在春节抢票的时候有时候会卡，我认为是从数据库里面请求每个人的订票信息和身份信息应该是存在一定难度的，即便把（人，身份证）存成了 哈希表，登录的时候可能会更多的加载一些数据信息；多个用户同时登录一个人的账户也经常会导致问题，例如微信出现了在电脑上发和接受消息在手机上是否同步 的选项，这个也可以侧面证明微信应该是不会记录发出消息的客户端
老的微信聊天记录推测是会被从数据库里面移出去的，否则应该微信会提供恢复聊天记录的功能，但是微信聊天记录的搜索为什么会非常快始终让我不解。。？可以 尝试一下在微信了发几百万条聊天记录看看微信数据库系统还能不能快速搜索到。
