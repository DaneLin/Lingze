#ifndef CONFIG_H
#define CONFIG_H

// Workgroup size for task shader; each task shader thread produces up to one meshlet
#define TASK_WGSIZE 32

// Workgroup size for mesh shader; mesh shader workgroup processes the entire meshlet in parallel
#define MESH_WGSIZE 32

#define MESHLET_MAX_VERTICES 64
#define MESHLET_MAX_TRIANGLES 124        // we use 4 bytes to store indices, so the max triangle count is 124 that is divisible by 4
#define MESHLET_CONE_WEIGHT 0.5f

#define BINDLESS_SET_ID 1
#define BINDLESS_TEXTURE_BINDING 11
#define BINDLESS_RESOURCE_COUNT 1024

#define COMMON_RESOURCE_COUNT 1024

#endif        // CONFIG_H
