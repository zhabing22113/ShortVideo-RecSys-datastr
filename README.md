# 短视频推荐系统 - 数据可视化平台

## 项目概述

本项目是一个基于用户兴趣向量和视频特征向量的短视频推荐系统可视化平台。

## 功能需求与实现状态

| 功能编号 | 功能描述 | 实现状态 | 所在文件 |
|----------|----------|----------|----------|
| F1 | 通过网页采集或模拟短视频信息，构建不少于十万的视频信息库 | ✅ 已实现 | B站爬虫采集，数据在 `data/processed/videos.csv` |
| F2 | 模拟用户的点击观看行为，模拟不少于1万的用户 | ✅ 已实现 | 数据已生成在 `data/simulated/` |
| F3 | 针对某特定用户，分析和该用户具有相似兴趣的用户群体 | ✅ 已实现 | `app.py` |
| F4 | 针对某特定用户，根据其过去历史的浏览记录，为其推荐相关视频 | ✅ 已实现 | `app.py` |
| F5 | 针对某特定视频，根据其过去观看的历史，预测该视频未来被观看的热度变化 | ❌ 未实现 | `app.py` 或 `models/heat_prediction.py` (建议) |
| F6 | 给视频进行聚类分析，将具有相似观看用户的视频聚成一团 | ✅ 已实现 | `app.py` |
| F7 | 给用户进行聚类分析，将具有相似观看兴趣的用户聚成一团 | ✅ 已实现 | `app.py` |

---

## 数据文件结构

```
data/
├── outputs/                          # 向量输出目录
│   ├── user_interest_vectors.csv      # 用户兴趣向量 (12维) - 1万+用户
│   ├── video_feature_vectors.csv     # 视频特征向量 (26维) - 十万+视频
│   └── video_audience_vectors.csv   # 视频受众向量
├── processed/                        # 处理后的视频数据
│   └── videos.csv                    # 视频基础信息
└── simulated/                        # 模拟数据
    ├── users.csv                     # 用户信息
    └── events.csv                    # 用户行为事件
```

### 向量格式说明

#### 1. 用户兴趣向量 (user_interest_vectors.csv)
| 字段 | 说明 |
|------|------|
| user_id | 用户ID |
| group_id | 所属群体ID (0-11) |
| event_count | 行为事件数 |
| interest_vector | 12维兴趣向量，分号分隔 |

**兴趣向量维度对应关系：**
```
维度0 → 影视剪辑
维度1 → 科技数码
维度2 → 野生技能协会
维度3 → 美食制作
维度4 → 日常
维度5 → 动画综合
维度6 → 亲子
维度7 → 出行
维度8 → 音乐综合
维度9 → 健身
维度10 → 校园学习
维度11 → 小剧场
```

#### 2. 视频特征向量 (video_feature_vectors.csv)
| 字段 | 说明 |
|------|------|
| video_id | 视频ID |
| topic_id | 所属Topic ID (0-11) |
| event_count | 事件数 |
| has_audience | 是否有受众数据 |
| feature_vector | 26维特征向量，分号分隔 |

**特征向量维度说明：**
- 前12维：Topic向量，与用户兴趣向量维度对应
- 后14维：扩展特征（受众统计等）

---

## 已实现功能详解

### F3: 相似用户分析 (👥 相似用户分析)

#### 功能描述
针对某特定用户，分析和该用户具有相似兴趣的用户群体

#### 所在文件
`app.py` - 第189-237行

#### 数据依赖
- `data/outputs/user_interest_vectors.csv` - 用户兴趣向量
- `data/simulated/users.csv` - 用户基础信息

#### 算法设计

**核心算法：余弦相似度 (Cosine Similarity)**

```python
def cosine_similarity(a, b):
    a = np.array(a)
    b = np.array(b)
    norm_a = np.linalg.norm(a)
    norm_b = np.linalg.norm(b)
    if norm_a == 0 or norm_b == 0:
        return 0.0
    return np.dot(a, b) / (norm_a * norm_b)
```

**实现过程：**

1. **输入**：目标用户ID (user_id)
2. **获取目标用户向量**：从 `user_interest_vectors.csv` 读取目标用户的12维兴趣向量
3. **遍历计算相似度**：
   - 对每个其他用户，计算其兴趣向量与目标用户兴趣向量的余弦相似度
   - 公式：`cosine_similarity = dot(u, v) / (||u|| * ||v||)`
4. **排序返回**：按相似度降序排列，返回Top-K个最相似用户

**基于向量库的实现原理：**
- 向量库提供了用户的12维兴趣向量表示
- 每个用户的兴趣通过12个Topic维度的权重来表示
- 通过计算向量之间的余弦相似度，可以量化用户之间的兴趣相似程度
- 相似度范围：[0, 1]，越接近1表示兴趣越相似

**时间复杂度**：O(N)，N为用户数量

---

### F4: 视频推荐 (🎥 视频推荐)

#### 功能描述
针对某特定用户，根据其过去历史的浏览记录，为其推荐相关视频

#### 所在文件
`app.py` - 第239-294行

#### 数据依赖
- `data/outputs/user_interest_vectors.csv` - 用户兴趣向量
- `data/outputs/video_feature_vectors.csv` - 视频特征向量
- `data/simulated/users.csv` - 用户基础信息
- `data/processed/videos.csv` - 视频标题信息

#### 算法设计

**核心算法：混合推荐 (Hybrid Recommendation)**

```python
def recommend_videos(user_id, user_vectors_df, video_vectors_df, users_df, top_n=20):
    # 获取用户向量
    target_vector = user_vectors_df[user_vectors_df['user_id'] == user_id]['interest_vector']
    
    # 获取用户群体
    user_group = users_df[users_df['user_id'] == user_id]['group_id']
    
    scores = []
    for each video in video_vectors_df:
        # 取视频特征向量的前12维(与用户向量维度对齐)
        video_topic_vector = video_feature_vector[:12]
        
        # 计算兴趣匹配得分 (点积)
        interest_score = dot_product(target_vector, video_topic_vector)
        
        # 群体加成：同群体视频加0.1分
        group_bonus = 0.1 if video.topic_id == user_group else 0.0
        
        # 总得分 = 兴趣得分 + 群体加成
        total_score = interest_score + group_bonus
        
        scores.append((video_id, total_score, topic_id, event_count))
    
    # 排序返回Top-N
    return sorted(scores, key=lambda x: x[1], reverse=True)[:top_n]
```

**推荐得分公式：**

```
Score = dot_product(user_interest_vector[12D], video_topic_vector[12D]) + group_bonus

其中：
- user_interest_vector: 用户12维兴趣向量（从历史浏览记录生成）
- video_topic_vector: 视频26维特征向量的前12维（Topic向量）
- group_bonus = 0.1 (如果用户群体ID == 视频Topic ID)
```

**基于向量库的实现原理：**
- 用户兴趣向量是根据用户历史浏览记录生成的12维向量
- 视频特征向量的前12维表示视频的Topic分布
- 通过点积运算衡量用户兴趣与视频内容的匹配程度
- 点积越大，表示用户对该视频的兴趣越高

**维度对齐问题解决：**
- 用户兴趣向量：12维（仅Topic分布）
- 视频特征向量：26维（Topic分布 + 扩展特征）
- **解决方案**：只取视频特征向量的前12维与用户向量进行计算

**时间复杂度**：O(M)，M为视频数量

---

### F6: 视频聚类分析 (🎯 视频聚类分析)

#### 功能描述
给视频进行聚类分析，将具有相似观看用户的视频聚成一团

#### 所在文件
`app.py` - 第385-481行

#### 数据依赖
- `data/outputs/video_feature_vectors.csv` - 视频特征向量
- `data/processed/videos.csv` - 视频标题信息

#### 算法设计

**基于向量库的实现原理：**
- 视频特征向量的前12维表示视频的Topic分布
- 通过topic_id字段实现预聚类（K=12）
- 同一Topic的视频具有相似的内容特征
- 用户观看行为的相似性通过Topic分布间接体现

**功能实现：**

1. **Topic分布统计**：按topic_id分组统计各Topic的视频数量
2. **受众覆盖分析**：统计有/无受众数据的视频分布
3. **Topic详情表**：展示各Topic的视频数、平均事件数、总事件数
4. **视频向量查询**：选择视频查看其26维特征向量可视化
5. **Topic代表分析**：选择Topic查看该Topic代表视频的特征分布

---

### F7: 用户聚类分析 (📈 用户聚类分析)

#### 功能描述
给用户进行聚类分析，将具有相似观看兴趣的用户聚成一团

#### 所在文件
`app.py` - 第296-383行

#### 数据依赖
- `data/simulated/users.csv` - 用户基础信息
- `data/outputs/user_interest_vectors.csv` - 用户兴趣向量

#### 算法设计

**基于向量库的实现原理：**
- 用户兴趣向量的12维表示用户在各Topic上的兴趣分布
- 通过group_id字段实现预聚类（K=12）
- 同一群体的用户具有相似的兴趣偏好

**功能实现：**

1. **群体分布统计**：按group_id分组统计各群体的用户数量
2. **行为分析**：各群体的平均行为数分布
3. **群体详情表**：展示各群体的用户数、平均活跃度、平均行为数
4. **用户向量查询**：选择用户查看其12维兴趣向量可视化
5. **群体代表分析**：选择群体查看该群体代表用户的兴趣分布

---

## 已实现功能补充说明

### F1: 构建视频信息库

**功能描述**：通过网页采集或模拟短视频信息，构建不少于十万的视频信息库

**实现方式**：使用爬虫从B站采集了10万+条视频数据，并进行了清洗处理

**数据位置**：`data/processed/videos.csv`

### F2: 模拟用户行为

**功能描述**：模拟用户的点击观看行为，模拟不少于1万的用户

**实现状态**：✅ 已通过模拟数据实现

**数据位置**：
- `data/simulated/users.csv` - 1万+用户信息
- `data/simulated/events.csv` - 用户行为事件
- `data/outputs/user_interest_vectors.csv` - 用户兴趣向量（从行为数据生成）

---

## 未实现功能

### F5: 视频热度预测

**功能描述**：针对某特定视频，根据其过去观看的历史，预测该视频未来被观看的热度变化

**建议实现位置**：`app.py` 或新建 `models/heat_prediction.py`

**实现思路**：
1. 收集视频的历史观看数据（按时间序列）
2. 使用时间序列预测模型（如ARIMA、Prophet）
3. 或使用机器学习模型（如LSTM）进行预测
4. 输入特征：历史观看数、当前热度、Topic分布等
5. 输出：未来一段时间的热度预测值

---

## 代码架构

### 核心函数

| 函数名 | 功能 | 位置 |
|--------|------|------|
| `load_user_interest_vectors()` | 加载用户兴趣向量 | app.py:42-48 |
| `load_video_feature_vectors()` | 加载视频特征向量 | app.py:50-57 |
| `load_users()` | 加载用户信息 | app.py:60-62 |
| `load_videos()` | 加载视频信息 | app.py:65-67 |
| `load_events()` | 加载行为事件 | app.py:70-72 |
| `load_video_title_map()` | 构建视频ID→标题映射 | app.py:74-77 |
| `cosine_similarity()` | 计算余弦相似度 | app.py:83-90 |
| `dot_product()` | 计算向量点积 | app.py:92-95 |
| `find_similar_users()` | 查找相似用户 (F3) | app.py:97-108 |
| `recommend_videos()` | 视频推荐 (F4) | app.py:110-128 |

### Streamlit页面结构

```
📊 数据概览
├── 用户/视频/行为数据统计
├── 向量数据示例
└── 分布图表

👥 相似用户分析 (F3)
├── 用户选择器
├── 相似用户列表
└── 用户向量可视化

🎥 视频推荐 (F4)
├── 用户选择器
├── 推荐结果表格
└── 推荐分布统计

📈 用户聚类分析 (F7)
├── 群体分布统计
├── 群体详情表
├── 用户向量查询
└── 群体代表分析

🎯 视频聚类分析 (F6)
├── Topic分布统计
├── Topic详情表
├── 视频向量查询
└── Topic代表分析
```

---

## 运行方式

```bash
# 安装依赖
pip install -r requirements.txt

# 运行Streamlit应用
streamlit run app.py
```

---

## ID映射表

### Topic ID → 中文名称

| Topic ID | 名称 |
|----------|------|
| 0 | 影视剪辑 |
| 1 | 科技数码 |
| 2 | 野生技能协会 |
| 3 | 美食制作 |
| 4 | 日常 |
| 5 | 动画综合 |
| 6 | 亲子 |
| 7 | 出行 |
| 8 | 音乐综合 |
| 9 | 健身 |
| 10 | 校园学习 |
| 11 | 小剧场 |

### Group ID → 用户群体名称

| Group ID | 群体名称 |
|----------|----------|
| 0 | 影视剪辑用户 |
| 1 | 科技数码用户 |
| 2 | 野生技能用户 |
| 3 | 美食制作用户 |
| 4 | 日常分享用户 |
| 5 | 动画综合用户 |
| 6 | 亲子教育用户 |
| 7 | 出行旅游用户 |
| 8 | 音乐欣赏用户 |
| 9 | 健身运动用户 |
| 10 | 校园学习用户 |
| 11 | 小剧场用户 |
