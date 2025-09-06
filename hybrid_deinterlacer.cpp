#include <VapourSynth4.h>
#include <VSHelper4.h>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cmath>

typedef struct DeinterlaceData {
    VSNode *node;
    VSVideoInfo vi;
    int tff;
} DeinterlaceData;

// 高品質4タップ3次補間 (Catmull-Rom)
static inline uint8_t cubic_interpolate(int p0, int p1, int p2, int p3) {
    double result = -0.0625 * p0 + 0.5625 * p1 + 0.5625 * p2 - 0.0625 * p3;
    return static_cast<uint8_t>(std::clamp(result + 0.5, 0.0, 255.0));
}

static const VSFrame *VS_CC deinterlaceGetFrame(
    int n, int activationReason, void *instanceData, void **,
    VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    DeinterlaceData *d = (DeinterlaceData *)instanceData;

    const int src_n = n / 2;

    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(src_n, d->node, frameCtx);
        return NULL;
    }

    if (activationReason == arAllFramesReady) {
        const VSFrame *cur_f = vsapi->getFrameFilter(src_n, d->node, frameCtx);
        const VSVideoFormat *fi = &d->vi.format;
        VSFrame *dst = vsapi->newVideoFrame(fi, d->vi.width, d->vi.height, cur_f, core);

        for (int plane = 0; plane < fi->numPlanes; plane++) {
            const int h = vsapi->getFrameHeight(cur_f, plane);
            const int w = vsapi->getFrameWidth(cur_f, plane);
            const ptrdiff_t stride = vsapi->getStride(cur_f, plane);
            const uint8_t *rp_c = vsapi->getReadPtr(cur_f, plane);
            uint8_t *wp = vsapi->getWritePtr(dst, plane);

            for (int y = 0; y < h; y++) {
                const bool line_is_top_field = (y % 2 == 0);
                const int field = n % 2;
                const bool output_is_based_on_top_field = d->tff ? (field == 0) : (field == 1);

                if ((line_is_top_field && output_is_based_on_top_field) || (!line_is_top_field && !output_is_based_on_top_field)) {
                    // 主体フィールドの行は、ソースからそのままコピー
                    for (int x = 0; x < w; x++) wp[y * stride + x] = rp_c[y * stride + x];
                } else {
                    // 補間が必要な行
                    for (int x = 0; x < w; x++) {
                        // ★★★ これが、クラッシュを根絶するための、絶対的な安全境界チェックです ★★★
                        //
                        // 4タップ補間には、補間したい行yの上下に2ラインずつ、合計4つの参照ラインが必要です。
                        // (y-3), (y-1), (y+1), (y+3)
                        // この条件を安全に満たせるのは、yが3からh-4までの範囲のみです。
                        //
                        if (y >= 3 && y < h - 3) {
                            // 【最高品質モード】フレームの中央部分、安全に参照ラインが確保できる領域
                            int p0 = rp_c[(y - 3) * stride + x];
                            int p1 = rp_c[(y - 1) * stride + x];
                            int p2 = rp_c[(y + 1) * stride + x];
                            int p3 = rp_c[(y + 3) * stride + x];
                            wp[y * stride + x] = cubic_interpolate(p0, p1, p2, p3);
                        } else {
                            // 【安全フォールバックモード】フレームの上下端、参照ラインが足りない領域
                            // この領域では、よりシンプルな線形補間（2タップ）を行います。
                            // この線形補間自体も、y=0とy=h-1では動作しないため、さらに安全策を講じます。
                            if (y == 0) {
                                // 一番上の行は、下のラインをコピーするしかない
                                wp[y * stride + x] = rp_c[(y + 1) * stride + x];
                            } else if (y == h - 1) {
                                // 一番下の行は、上のラインをコピーするしかない
                                wp[y * stride + x] = rp_c[(y - 1) * stride + x];
                            } else {
                                // y=1, 2, h-2, h-3 の場合は、安全に線形補間が可能
                                wp[y * stride + x] = (rp_c[(y - 1) * stride + x] + rp_c[(y + 1) * stride + x]) / 2;
                            }
                        }
                    }
                }
            }
        }
        vsapi->freeFrame(cur_f);
        return dst;
    }
    return NULL;
}

static void VS_CC deinterlaceFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
    DeinterlaceData *d = (DeinterlaceData *)instanceData;
    if (d->node) vsapi->freeNode(d->node);
    free(d);
}

static void VS_CC deinterlaceCreate(
    const VSMap *in, VSMap *out, void *userData,
    VSCore *core, const VSAPI *vsapi)
{
    int err = 0;
    DeinterlaceData *d = (DeinterlaceData *)malloc(sizeof(DeinterlaceData));
    d->node = vsapi->mapGetNode(in, "clip", 0, &err);
    if (err) {
        vsapi->mapSetError(out, "StableDeinterlacer: clip is required.");
        free(d);
        return;
    }
    d->vi = *vsapi->getVideoInfo(d->node);

    d->tff = (int)vsapi->mapGetInt(in, "tff", 0, &err);
    if (err) d->tff = 1;

    d->vi.fpsNum *= 2;
    d->vi.numFrames *= 2;

    VSFilterDependency deps[] = { { d->node, rpGeneral } };
    vsapi->createVideoFilter(out, "StableDeinterlacer", &d->vi,
                             deinterlaceGetFrame, deinterlaceFree,
                             fmParallel, deps, 1, d, core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(
    VSPlugin *plugin, const VSPLUGINAPI *vspapi)
{
    vspapi->configPlugin("com.example.stabledeinterlacer", "sd",
                         "Stable High Quality Deinterlacer (API v4)",
                         VS_MAKE_VERSION(10, 0), VAPOURSYNTH_API_VERSION,
                         0, plugin);
    vspapi->registerFunction("StableDeinterlacer",
                             "clip:vnode;tff:int:opt;",
                             "clip:vnode;",
                             deinterlaceCreate, NULL, plugin);
}