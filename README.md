Vapoursynth 用　簡易マルチタップアルゴリズム式ダブルレートインターレース解除フィルタです。

ESRGAN＋こちらのインタレース解除を使用することをおすすめします。
ntsc素材に最適。

使い方；


```

import vapoursynth as vs



core = vs.core


clip = video_in



clip1 = core.std.AssumeFPS(clip   , fpsnum=60000, fpsden=1001)


deinterlaced = core.sd.StableDeinterlacer(clip1,tff=1)


deinterlaced .set_output()

```

かならずFPSを明示してください。

ボブのみのデインターレースですので、MPV Playerにて


```

 vf toggle fps=ntsc;vf toggle vapoursynth="~~/vs/Deint.vpy"


```
と事前にシングルレートFPS値も指定してください。
