/*
 * CSR shader verification harness.
 *
 * Executes the CSR compute shaders on a Vulkan device and diffs the result
 * against csr/ref/golden/*.json, the same golden vectors the C# port is tested
 * against. Runs on llvmpipe (software Vulkan), so it needs no GPU and works in
 * CI.
 *
 * What this proves: the maths as written in the HLSL is correct.
 * What it does not prove: that fxc compiles the same source to cs_5_0, or that a
 * particular GPU's rcp/rsqrt approximations land inside tolerance. Both need
 * Windows and real hardware.
 *
 * Build: cc -O2 -o verify verify.c -lvulkan -lm
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vulkan/vulkan.h>

#define CHECK(x) do { VkResult _r = (x); if (_r != VK_SUCCESS) { \
    fprintf(stderr, "%s:%d: %s failed (%d)\n", __FILE__, __LINE__, #x, _r); \
    exit(2); } } while (0)

typedef struct { int w, h; float *rgb; } Plane;

/* ---- minimal JSON extraction -------------------------------------------- */
/* The golden files have a fixed shape emitted by export_golden.py, so a scan
 * for the named key is sufficient and avoids a JSON dependency in CI. */

static char *slurp(const char *path, long *len) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(2); }
    fseek(f, 0, SEEK_END); *len = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc(*len + 1);
    if (fread(buf, 1, *len, f) != (size_t)*len) { fprintf(stderr, "short read\n"); exit(2); }
    buf[*len] = 0; fclose(f);
    return buf;
}

static int find_int_after(const char *s, const char *key) {
    const char *p = strstr(s, key);
    if (!p) return -1;
    p += strlen(key);
    while (*p && (*p == ':' || *p == ' ' || *p == '"')) p++;
    return atoi(p);
}

/* Reads object `name` -> {width, height, rgb:[...]}. */
static Plane read_plane(const char *json, const char *name) {
    char key[64];
    snprintf(key, sizeof key, "\"%s\"", name);
    const char *obj = strstr(json, key);
    if (!obj) { fprintf(stderr, "key %s missing\n", name); exit(2); }

    Plane pl;
    pl.w = find_int_after(obj, "\"width\"");
    pl.h = find_int_after(obj, "\"height\"");
    if (pl.w <= 0 || pl.h <= 0) { fprintf(stderr, "bad dims for %s\n", name); exit(2); }

    const char *arr = strstr(obj, "\"rgb\"");
    if (!arr) { fprintf(stderr, "rgb missing for %s\n", name); exit(2); }
    arr = strchr(arr, '[');
    if (!arr) { fprintf(stderr, "rgb not an array\n"); exit(2); }
    arr++;

    long n = (long)pl.w * pl.h * 3;
    pl.rgb = malloc(sizeof(float) * n);
    char *cur = (char *)arr;
    for (long i = 0; i < n; i++) {
        pl.rgb[i] = strtof(cur, &cur);
        while (*cur == ',' || *cur == ' ' || *cur == '\n') cur++;
    }
    return pl;
}

/* ---- Vulkan ------------------------------------------------------------- */

typedef struct {
    VkInstance inst;
    VkPhysicalDevice phys;
    VkDevice dev;
    VkQueue queue;
    uint32_t qfam;
    VkCommandPool pool;
    char device_name[256];
} Ctx;

static void ctx_init(Ctx *c) {
    VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "csr-verify", .apiVersion = VK_API_VERSION_1_0 };
    VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app };
    CHECK(vkCreateInstance(&ici, NULL, &c->inst));

    uint32_t n = 0;
    CHECK(vkEnumeratePhysicalDevices(c->inst, &n, NULL));
    if (!n) { fprintf(stderr, "no Vulkan device\n"); exit(2); }
    VkPhysicalDevice *devs = malloc(sizeof(*devs) * n);
    CHECK(vkEnumeratePhysicalDevices(c->inst, &n, devs));
    c->phys = devs[0];
    free(devs);

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(c->phys, &props);
    snprintf(c->device_name, sizeof c->device_name, "%s", props.deviceName);

    uint32_t qn = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(c->phys, &qn, NULL);
    VkQueueFamilyProperties *qs = malloc(sizeof(*qs) * qn);
    vkGetPhysicalDeviceQueueFamilyProperties(c->phys, &qn, qs);
    c->qfam = UINT32_MAX;
    for (uint32_t i = 0; i < qn; i++)
        if (qs[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { c->qfam = i; break; }
    free(qs);
    if (c->qfam == UINT32_MAX) { fprintf(stderr, "no compute queue\n"); exit(2); }

    float prio = 1.0f;
    VkDeviceQueueCreateInfo dq = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = c->qfam, .queueCount = 1, .pQueuePriorities = &prio };
    VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1, .pQueueCreateInfos = &dq };
    CHECK(vkCreateDevice(c->phys, &dci, NULL, &c->dev));
    vkGetDeviceQueue(c->dev, c->qfam, 0, &c->queue);

    VkCommandPoolCreateInfo pci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = c->qfam };
    CHECK(vkCreateCommandPool(c->dev, &pci, NULL, &c->pool));
}

static uint32_t mem_type(Ctx *c, uint32_t bits, VkMemoryPropertyFlags want) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(c->phys, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want)
            return i;
    fprintf(stderr, "no suitable memory type\n"); exit(2);
}

typedef struct { VkImage img; VkDeviceMemory mem; VkImageView view; int w, h; } Img;

static Img make_image(Ctx *c, int w, int h, VkImageUsageFlags usage) {
    Img im = { .w = w, .h = h };
    VkImageCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .extent = { w, h, 1 }, .mipLevels = 1, .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
    CHECK(vkCreateImage(c->dev, &ici, NULL, &im.img));

    VkMemoryRequirements mr;
    vkGetImageMemoryRequirements(c->dev, im.img, &mr);
    VkMemoryAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mr.size,
        .memoryTypeIndex = mem_type(c, mr.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) };
    CHECK(vkAllocateMemory(c->dev, &ai, NULL, &im.mem));
    CHECK(vkBindImageMemory(c->dev, im.img, im.mem, 0));

    VkImageViewCreateInfo vi = { .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = im.img, .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    CHECK(vkCreateImageView(c->dev, &vi, NULL, &im.view));
    return im;
}

typedef struct { VkBuffer buf; VkDeviceMemory mem; VkDeviceSize size; } Buf;

static Buf make_buffer(Ctx *c, VkDeviceSize size, VkBufferUsageFlags usage) {
    Buf b = { .size = size };
    VkBufferCreateInfo bi = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size, .usage = usage };
    CHECK(vkCreateBuffer(c->dev, &bi, NULL, &b.buf));
    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(c->dev, b.buf, &mr);
    VkMemoryAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mr.size,
        .memoryTypeIndex = mem_type(c, mr.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };
    CHECK(vkAllocateMemory(c->dev, &ai, NULL, &b.mem));
    CHECK(vkBindBufferMemory(c->dev, b.buf, b.mem, 0));
    return b;
}

static VkShaderModule load_spv(Ctx *c, const char *path) {
    long len;
    char *code = slurp(path, &len);
    VkShaderModuleCreateInfo si = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)len, .pCode = (const uint32_t *)code };
    VkShaderModule m;
    CHECK(vkCreateShaderModule(c->dev, &si, NULL, &m));
    free(code);
    return m;
}

/* Constant buffer layout, matching CsrGpuPipeline.WriteConstants exactly. */
typedef struct {
    uint32_t srcW, srcH, dstW, dstH;
    float scaleX, scaleY, sharpnessLin, adaptive;
} Constants;

/* Runs one compute pass: src image -> dst image. */
static void run_pass(Ctx *c, const char *spv, Img *src, Img *dst,
                     Constants k, const float *src_pixels, float *out_pixels) {
    VkShaderModule mod = load_spv(c, spv);

    VkDescriptorSetLayoutBinding binds[3] = {
        { 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
        { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
        { 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, NULL },
    };
    VkDescriptorSetLayoutCreateInfo dli = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 3, .pBindings = binds };
    VkDescriptorSetLayout dsl;
    CHECK(vkCreateDescriptorSetLayout(c->dev, &dli, NULL, &dsl));

    VkPipelineLayoutCreateInfo pli = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1, .pSetLayouts = &dsl };
    VkPipelineLayout pl;
    CHECK(vkCreatePipelineLayout(c->dev, &pli, NULL, &pl));

    VkComputePipelineCreateInfo cpi = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                   .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = mod, .pName = "main" },
        .layout = pl };
    VkPipeline pipe;
    CHECK(vkCreateComputePipelines(c->dev, VK_NULL_HANDLE, 1, &cpi, NULL, &pipe));

    VkDescriptorPoolSize ps[3] = {
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 } };
    VkDescriptorPoolCreateInfo dpi = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1, .poolSizeCount = 3, .pPoolSizes = ps };
    VkDescriptorPool dp;
    CHECK(vkCreateDescriptorPool(c->dev, &dpi, NULL, &dp));
    VkDescriptorSetAllocateInfo dai = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = dp, .descriptorSetCount = 1, .pSetLayouts = &dsl };
    VkDescriptorSet ds;
    CHECK(vkAllocateDescriptorSets(c->dev, &dai, &ds));

    /* Staging: upload source pixels, download result. */
    VkDeviceSize src_bytes = (VkDeviceSize)src->w * src->h * 4 * sizeof(float);
    VkDeviceSize dst_bytes = (VkDeviceSize)dst->w * dst->h * 4 * sizeof(float);
    Buf up = make_buffer(c, src_bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    Buf down = make_buffer(c, dst_bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    Buf ubo = make_buffer(c, sizeof(Constants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    void *p;
    CHECK(vkMapMemory(c->dev, up.mem, 0, src_bytes, 0, &p));
    float *fp = p;
    for (int i = 0; i < src->w * src->h; i++) {
        fp[i * 4 + 0] = src_pixels[i * 3 + 0];
        fp[i * 4 + 1] = src_pixels[i * 3 + 1];
        fp[i * 4 + 2] = src_pixels[i * 3 + 2];
        fp[i * 4 + 3] = 1.0f;
    }
    vkUnmapMemory(c->dev, up.mem);

    CHECK(vkMapMemory(c->dev, ubo.mem, 0, sizeof k, 0, &p));
    memcpy(p, &k, sizeof k);
    vkUnmapMemory(c->dev, ubo.mem);

    VkDescriptorImageInfo si_info = { .imageView = src->view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkDescriptorImageInfo di_info = { .imageView = dst->view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL };
    VkDescriptorBufferInfo bi_info = { .buffer = ubo.buf, .offset = 0, .range = sizeof k };
    VkWriteDescriptorSet w[3] = {
        { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .dstBinding = 0,
          .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
          .pImageInfo = &si_info },
        { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .dstBinding = 1,
          .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &bi_info },
        { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = ds, .dstBinding = 2,
          .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = &di_info } };
    vkUpdateDescriptorSets(c->dev, 3, w, 0, NULL);

    VkCommandBufferAllocateInfo cai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = c->pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
    VkCommandBuffer cb;
    CHECK(vkAllocateCommandBuffers(c->dev, &cai, &cb));
    VkCommandBufferBeginInfo bbi = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    CHECK(vkBeginCommandBuffer(cb, &bbi));

    VkImageMemoryBarrier to_dst = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = src->img, .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &to_dst);

    VkBufferImageCopy region = { .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent = { src->w, src->h, 1 } };
    vkCmdCopyBufferToImage(cb, up.buf, src->img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier to_read = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .image = src->img,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    VkImageMemoryBarrier to_general = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = dst->img, .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    VkImageMemoryBarrier pre[2] = { to_read, to_general };
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 2, pre);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pl, 0, 1, &ds, 0, NULL);
    vkCmdDispatch(cb, (dst->w + 7) / 8, (dst->h + 7) / 8, 1);

    VkImageMemoryBarrier to_src = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .image = dst->img, .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &to_src);

    VkBufferImageCopy back = { .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent = { dst->w, dst->h, 1 } };
    vkCmdCopyImageToBuffer(cb, dst->img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           down.buf, 1, &back);

    CHECK(vkEndCommandBuffer(cb));
    VkSubmitInfo sub = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1, .pCommandBuffers = &cb };
    CHECK(vkQueueSubmit(c->queue, 1, &sub, VK_NULL_HANDLE));
    CHECK(vkQueueWaitIdle(c->queue));

    CHECK(vkMapMemory(c->dev, down.mem, 0, dst_bytes, 0, &p));
    fp = p;
    for (int i = 0; i < dst->w * dst->h; i++) {
        out_pixels[i * 3 + 0] = fp[i * 4 + 0];
        out_pixels[i * 3 + 1] = fp[i * 4 + 1];
        out_pixels[i * 3 + 2] = fp[i * 4 + 2];
    }
    vkUnmapMemory(c->dev, down.mem);

    vkFreeCommandBuffers(c->dev, c->pool, 1, &cb);
    vkDestroyDescriptorPool(c->dev, dp, NULL);
    vkDestroyPipeline(c->dev, pipe, NULL);
    vkDestroyPipelineLayout(c->dev, pl, NULL);
    vkDestroyDescriptorSetLayout(c->dev, dsl, NULL);
    vkDestroyShaderModule(c->dev, mod, NULL);
    vkDestroyBuffer(c->dev, up.buf, NULL);   vkFreeMemory(c->dev, up.mem, NULL);
    vkDestroyBuffer(c->dev, down.buf, NULL); vkFreeMemory(c->dev, down.mem, NULL);
    vkDestroyBuffer(c->dev, ubo.buf, NULL);  vkFreeMemory(c->dev, ubo.mem, NULL);
}

static void destroy_image(Ctx *c, Img *im) {
    vkDestroyImageView(c->dev, im->view, NULL);
    vkDestroyImage(c->dev, im->img, NULL);
    vkFreeMemory(c->dev, im->mem, NULL);
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s <spv-dir> <golden-dir> <case> [case...]\n", argv[0]);
        return 2;
    }
    const char *spv_dir = argv[1];
    const char *golden_dir = argv[2];

    Ctx c;
    ctx_init(&c);
    printf("Vulkan device: %s\n\n", c.device_name);
    printf("%-24s %-10s %12s %12s  %s\n",
           "case", "pass", "worst dev", "tolerance", "result");
    printf("--------------------------------------------------------------------------\n");

    int failures = 0;
    char resolve_spv[512], sharpen_spv[512], json_path[512];
    snprintf(resolve_spv, sizeof resolve_spv, "%s/CsrResolve.spv", spv_dir);
    snprintf(sharpen_spv, sizeof sharpen_spv, "%s/CsrSharpen.spv", spv_dir);

    for (int a = 3; a < argc; a++) {
        snprintf(json_path, sizeof json_path, "%s/%s.json", golden_dir, argv[a]);
        long len;
        char *json = slurp(json_path, &len);

        Plane in = read_plane(json, "input");
        Plane want_resolve = read_plane(json, "resolve");
        Plane want_full = read_plane(json, "full");

        const char *tp = strstr(json, "\"tolerance\"");
        float tol = tp ? strtof(strchr(tp, ':') + 1, NULL) : 1.0f / 255.0f;

        /* Pass 1: resolve. */
        Img src = make_image(&c, in.w, in.h,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        Img mid = make_image(&c, want_resolve.w, want_resolve.h,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        float *got_resolve = malloc(sizeof(float) * want_resolve.w * want_resolve.h * 3);
        Constants k = { in.w, in.h, want_resolve.w, want_resolve.h,
                        (float)in.w / want_resolve.w, (float)in.h / want_resolve.h,
                        powf(2.0f, -0.25f), 1.0f };
        run_pass(&c, resolve_spv, &src, &mid, k, in.rgb, got_resolve);

        /* Pass 2: sharpen, at output resolution. */
        Img mid_in = make_image(&c, want_resolve.w, want_resolve.h,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        Img out = make_image(&c, want_full.w, want_full.h,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        float *got_full = malloc(sizeof(float) * want_full.w * want_full.h * 3);
        Constants k2 = { want_resolve.w, want_resolve.h, want_full.w, want_full.h,
                         1.0f, 1.0f, powf(2.0f, -0.25f), 1.0f };
        run_pass(&c, sharpen_spv, &mid_in, &out, k2, got_resolve, got_full);

        struct { const char *name; Plane *want; float *got; } checks[2] = {
            { "resolve", &want_resolve, got_resolve },
            { "full", &want_full, got_full } };

        for (int ci = 0; ci < 2; ci++) {
            long n = (long)checks[ci].want->w * checks[ci].want->h * 3;
            float worst = 0.0f; long worst_i = -1;
            for (long i = 0; i < n; i++) {
                float d = fabsf(checks[ci].want->rgb[i] - checks[ci].got[i]);
                if (d > worst) { worst = d; worst_i = i; }
            }
            int ok = worst <= tol;
            if (!ok) failures++;
            printf("%-24s %-10s %12.7f %12.7f  %s", argv[a], checks[ci].name,
                   worst, tol, ok ? "MATCH" : "MISMATCH");
            if (!ok) {
                long px = worst_i / 3;
                printf("  (pixel %ld,%ld ch%ld: want %.6f got %.6f)",
                       px % checks[ci].want->w, px / checks[ci].want->w, worst_i % 3,
                       checks[ci].want->rgb[worst_i], checks[ci].got[worst_i]);
            }
            printf("\n");
        }

        destroy_image(&c, &src); destroy_image(&c, &mid);
        destroy_image(&c, &mid_in); destroy_image(&c, &out);
        free(got_resolve); free(got_full);
        free(in.rgb); free(want_resolve.rgb); free(want_full.rgb);
        free(json);
    }

    printf("--------------------------------------------------------------------------\n");
    printf(failures ? "\n%d comparison(s) FAILED\n" : "\nall comparisons matched\n", failures);
    vkDestroyCommandPool(c.dev, c.pool, NULL);
    vkDestroyDevice(c.dev, NULL);
    vkDestroyInstance(c.inst, NULL);
    return failures ? 1 : 0;
}
