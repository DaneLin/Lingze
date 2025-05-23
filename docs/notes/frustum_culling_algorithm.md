# Frustum Culling 算法数学原理与实现

## 概述

Frustum Culling（视锥体剔除）是3D渲染中的一种重要优化技术，用于剔除不在相机视锥体内的几何体，从而减少不必要的渲染计算。本文档详细分析了当前项目中实现的Frustum Culling算法的数学原理和每一步计算过程。

## 1. 视锥体的数学表示

### 1.1 投影矩阵与视锥体平面

在透视投影中，视锥体由6个平面组成：近平面、远平面、左平面、右平面、上平面、下平面。

对于对称的透视投影矩阵：
```
P = [P00  0   0   0 ]
    [0   P11  0   0 ]
    [0    0  P22 P23]
    [0    0  -1   0 ]
```

其中：
- `P00 = 2 * near / (right - left) = 2 * near / width`
- `P11 = 2 * near / (top - bottom) = 2 * near / height`

### 1.2 视锥体平面方程的提取

从投影矩阵中提取视锥体平面的标准方法是使用Gribb-Hartmann方法。对于投影矩阵P，视锥体平面可以通过以下方式计算：

```cpp
// 代码实现：src/application/MeshShading/MeshShadingRenderer.cpp:120-123
glm::mat4 proj_matrix_t = glm::transpose(proj_matrix);
glm::vec4 frustum_x = glm::normalize(proj_matrix_t[3] + proj_matrix_t[0]);
glm::vec4 frustum_y = glm::normalize(proj_matrix_t[3] + proj_matrix_t[1]);
```

**数学推导：**

对于齐次坐标系中的点 `(x, y, z, w)`，投影后的标准化设备坐标(NDC)为：
```
x_ndc = x/w
y_ndc = y/w
z_ndc = z/w
```

视锥体的边界条件为：
- 左平面：`x_ndc >= -1` → `x + w >= 0`
- 右平面：`x_ndc <= 1` → `x - w <= 0` → `-x + w >= 0`
- 下平面：`y_ndc >= -1` → `y + w >= 0`
- 上平面：`y_ndc <= 1` → `y - w <= 0` → `-y + w >= 0`

在投影变换后，这些条件变为：
- 左/右平面：`P_row3 ± P_row0`
- 下/上平面：`P_row3 ± P_row1`

### 1.3 优化的对称视锥体表示

由于使用对称投影，可以利用对称性同时测试左右平面和上下平面：

```glsl
// 代码实现：shaders/glsl/GpuDriven/Culling.comp:69-70
is_visible = is_visible && center.z * cull_data.frustum[1] - abs(center.x) * cull_data.frustum[0] > -radius;
is_visible = is_visible && center.z * cull_data.frustum[3] - abs(center.y) * cull_data.frustum[2] > -radius;
```

其中：
- `frustum[0] = frustum_x.x`，`frustum[1] = frustum_x.z`
- `frustum[2] = frustum_y.y`，`frustum[3] = frustum_y.z`

**数学原理：**

对于对称视锥体，左右平面的法向量为 `(±a, 0, c)`，上下平面的法向量为 `(0, ±b, c)`。

点到平面的距离公式为：`d = ax + by + cz + d`

对于球体，如果球心到平面的距离大于负半径，则球体可能与视锥体相交：
```
distance_to_plane > -radius
```

利用对称性，可以写成：
```
|center.x| * frustum[0] - center.z * frustum[1] < radius
```

重新整理得到：
```
center.z * frustum[1] - |center.x| * frustum[0] > -radius
```

## 2. 球体-视锥体相交测试

### 2.1 坐标变换

首先将物体从模型空间变换到视图空间：

```glsl
// 代码实现：shaders/glsl/GpuDriven/Culling.comp:52-57
vec3 sphere_center = mesh.sphere_bound.xyz;
float sphere_radius = mesh.sphere_bound.w;
vec3 world_center = (mesh_draw.model_matrix * vec4(sphere_center, 1.0)).xyz;
vec3 view_center = (cull_data.view_matrix * vec4(world_center, 1.0)).xyz;
```

**变换过程：**
1. **模型空间 → 世界空间：** `world_center = M * model_center`
2. **世界空间 → 视图空间：** `view_center = V * world_center`

其中 `V = inverse(camera_transform_matrix)`

### 2.2 缩放处理

考虑非均匀缩放对包围球半径的影响：

```glsl
// 代码实现：shaders/glsl/GpuDriven/Culling.comp:59-64
float scale_x = length(vec3(mesh_draw.model_matrix[0][0], mesh_draw.model_matrix[0][1], mesh_draw.model_matrix[0][2]));
float scale_y = length(vec3(mesh_draw.model_matrix[1][0], mesh_draw.model_matrix[1][1], mesh_draw.model_matrix[1][2]));
float scale_z = length(vec3(mesh_draw.model_matrix[2][0], mesh_draw.model_matrix[2][1], mesh_draw.model_matrix[2][2]));
float max_scale = max(max(scale_x, scale_y), scale_z);
float transformed_radius = sphere_radius * max_scale;
```

**数学原理：**

对于变换矩阵 `M`，其列向量的长度表示沿各轴的缩放因子：
```
scale_x = ||M_column0||
scale_y = ||M_column1||
scale_z = ||M_column2||
```

为了保证包围球能够完全包含变换后的几何体，使用最大缩放因子：
```
transformed_radius = original_radius * max(scale_x, scale_y, scale_z)
```

### 2.3 视锥体平面测试

#### 2.3.1 左右平面测试

```glsl
center.z * cull_data.frustum[1] - abs(center.x) * cull_data.frustum[0] > -radius
```

**几何意义：**
- `frustum[0]` 和 `frustum[1]` 来自归一化的平面方程 `(a, 0, c)`
- `abs(center.x)` 利用对称性同时测试左右平面
- 条件成立表示球体至少部分在左右平面之间

#### 2.3.2 上下平面测试

```glsl
center.z * cull_data.frustum[3] - abs(center.y) * cull_data.frustum[2] > -radius
```

**几何意义：**
- `frustum[2]` 和 `frustum[3]` 来自归一化的平面方程 `(0, b, c)`
- `abs(center.y)` 利用对称性同时测试上下平面
- 条件成立表示球体至少部分在上下平面之间

#### 2.3.3 近远平面测试

```glsl
center.z + radius > cull_data.znear && center.z - radius < cull_data.zfar
```

**几何意义：**
- 在视图空间中，Z轴指向相机后方（右手坐标系）
- `center.z + radius > znear`：球体的最近点在近平面之后
- `center.z - radius < zfar`：球体的最远点在远平面之前

## 3. 高级剔除技术

### 3.1 层次Z缓冲剔除（Hi-Z Culling）

项目中还实现了基于深度金字塔的遮挡剔除：

```glsl
// 代码实现：shaders/glsl/MeshShading/drawcull_late.comp:75-85
if (project_sphere(center, radius, cull_data.znear, cull_data.P00, cull_data.P11, aabb))
{
    float width = (aabb.z - aabb.x) * cull_data.depth_pyramid_width;
    float height = (aabb.y - aabb.w) * cull_data.depth_pyramid_height;
    float level = floor(log2(max(width, height)));
    float depth = textureLod(depth_pyramid, (aabb.xy + aabb.zw) * 0.5, level).x;
    float depthSphere = 1.0 - cull_data.znear / (center.z - radius);
    is_visible = is_visible && depthSphere <= depth;
}
```

### 3.2 球体投影算法

`project_sphere` 函数实现了Mara & McGuire 2013年论文中的算法：

```glsl
// 代码实现：shaders/glsl/common_math.h:25-42
bool project_sphere(vec3 C, float r, float znear, float P00, float P11, out vec4 aabb)
{
    if (C.z < r + znear) return false;
    
    vec2 cx = -C.xz;
    vec2 vx = vec2(sqrt(dot(cx, cx) - r * r), r) / length(cx);
    vec2 minx = mat2(vx.x, vx.y, -vx.y, vx.x) * cx;
    vec2 maxx = mat2(vx.x, -vx.y, vx.y, vx.x) * cx;
    
    vec2 cy = -C.yz;
    vec2 vy = vec2(sqrt(dot(cy, cy) - r * r), r) / length(cy);
    vec2 miny = mat2(vy.x, -vy.y, vy.y, vy.x) * cy;
    vec2 maxy = mat2(vy.x, vy.y, -vy.y, vy.x) * cy;
    
    aabb = vec4(minx.x / minx.y * P00, miny.x / miny.y * P11, 
                maxx.x / maxx.y * P00, maxy.x / maxy.y * P11) 
           * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f);
    
    return true;
}
```

**数学原理：**

1. **切线计算：** 对于球心 `C` 和半径 `r`，计算从原点到球体的切线
2. **投影边界：** 将3D球体的切线投影到2D屏幕空间
3. **AABB生成：** 生成包围投影球体的轴对齐包围盒

### 3.3 锥体剔除（Cone Culling）

对于meshlet级别的剔除，还实现了基于法向量锥的剔除：

```glsl
// 代码实现：shaders/glsl/common_math.h:18-21
bool cone_cull(vec3 center, float radius, vec3 cone_axis, float cone_cutoff, vec3 camera_position)
{
    return dot(center - camera_position, cone_axis) >= cone_cutoff * length(center - camera_position) + radius;
}
```

**几何意义：**
- 如果meshlet的法向量锥背向相机，则可以被剔除
- `cone_axis`：锥体轴向（平均法向量）
- `cone_cutoff`：锥体的cos(半角)

## 4. 性能优化

### 4.1 GPU并行计算

所有剔除计算都在GPU上并行执行：

```glsl
layout(local_size_x = COMPUTE_WGSIZE, local_size_y = 1, local_size_z = 1) in;
```

每个工作组处理多个绘制命令，充分利用GPU的并行计算能力。

### 4.2 早期退出优化

```glsl
bool is_visible = true;
is_visible = is_visible && /* 第一个测试 */;
is_visible = is_visible && /* 第二个测试 */;
is_visible = is_visible && /* 第三个测试 */;
```

使用短路求值，一旦某个测试失败就停止后续计算。

### 4.3 对称性利用

通过使用 `abs(center.x)` 和 `abs(center.y)`，同时测试对称的平面对，减少了计算量。

## 5. 总结

本项目实现的Frustum Culling算法具有以下特点：

1. **数学严谨性：** 基于标准的平面方程和球体-平面相交测试
2. **性能优化：** 利用对称性、GPU并行计算和早期退出
3. **扩展性：** 支持Hi-Z剔除和锥体剔除等高级技术
4. **实用性：** 处理非均匀缩放、多级剔除等实际渲染需求

该算法在保证正确性的同时，通过多种优化技术显著提升了渲染性能，是现代GPU驱动渲染管线的重要组成部分。

## 参考文献

1. Gribb, G., & Hartmann, K. (2001). "Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix"
2. Mara, M., & McGuire, M. (2013). "2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere"
3. Wihlidal, G. (2018). "Optimizing the Graphics Pipeline with Compute" - Advanced Culling Techniques 