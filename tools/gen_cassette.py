#!/usr/bin/env python3
"""Generate the cassette artwork shown on the Ready, Recording, Stopped and Playback screens.

Reads a cassette illustration (PNG), crops and scales it to the 266 px tall
hero image, blanks the paper label (the firmware draws the label text at
runtime), fills both reel wells with a flat navy disc, and renders 24 frames
of a white six-spoke hub. Each hub frame is drawn procedurally around the exact
sprite centre, so cycling the frames spins the hub in place without the
off-centre wobble you get from rotating a cropped bitmap.

Outputs two LVGL 8 image sources (LV_IMG_CF_TRUE_COLOR_ALPHA, RGB565 + alpha):

  ui_img_cassette_body.c   cassette_body
  ui_img_cassette_reels.c  cassette_reels[24], cassette_reel_count

The last line printed ("LVGL_GEOM lcx=.. rcx=.. cy=.. reel=..") gives the reel
centres and sprite size; keep HERO_LCX / HERO_RCX / HERO_CY / HERO_REEL in
legacytape_arduino/ui_widgets.c in sync with it.

Requires Pillow and NumPy:  pip install pillow numpy

Usage:
  python3 tools/gen_cassette.py SOURCE.png [--out-dir legacytape_arduino]
                                [--preview-dir DIR]
"""
import argparse
import os

import numpy as np
from PIL import Image, ImageDraw


def parse_args():
    p = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    p.add_argument("source", help="cassette artwork PNG")
    p.add_argument("--out-dir", default="legacytape_arduino",
                   help="directory that receives the two .c files (default: %(default)s)")
    p.add_argument("--preview-dir", default=None,
                   help="optional: write cassette_preview_0.png / _4.png here for a visual check")
    return p.parse_args()


args = parse_args()
SRC = args.source
OUT = args.out_dir
os.makedirs(OUT, exist_ok=True)

im = Image.open(SRC).convert("RGBA"); a = np.asarray(im); H, W = a.shape[:2]; rgb = a[:, :, :3].astype(int)
corners = np.vstack([rgb[0:20,0:20].reshape(-1,3), rgb[0:20,-20:].reshape(-1,3), rgb[-20:,0:20].reshape(-1,3), rgb[-20:,-20:].reshape(-1,3)])
bg = corners.mean(0); nonbg = (np.abs(rgb-bg).sum(2) > 70)
col = nonbg.sum(0); row = nonbg.sum(1)
xs = np.where(col > H*0.05)[0]; x0, x1 = xs.min(), xs.max()
ys = np.where(row > W*0.05)[0]; y0 = ys.min(); y1 = ys.max()
for y in range(int(H*0.55), H):
    if row[y] < W*0.02: y1 = y; break
cass = im.crop((max(0,x0-6), max(0,y0-6), min(W,x1+6), min(H,y1+6)))
TH = 266; TW = int(round(cass.size[0]*TH/cass.size[1])); cass = cass.resize((TW, TH), Image.LANCZOS)
c = np.asarray(cass)[:, :, :3].astype(int); mid = TW // 2
white = (c[:,:,0] > 198) & (c[:,:,1] > 198) & (c[:,:,2] > 198); white[:int(TH*0.36), :] = False
def detect(xlo, xhi, ylo, yhi):
    m = white.copy(); m[:, :xlo]=False; m[:, xhi:]=False; m[:ylo, :]=False; m[yhi:, :]=False
    ys2, xs2 = np.where(m)
    return (int(xs2.min())+int(xs2.max()))//2, (int(ys2.min())+int(ys2.max()))//2, int(round(float(np.percentile(np.hypot(xs2-((int(xs2.min())+int(xs2.max()))//2), ys2-((int(ys2.min())+int(ys2.max()))//2)), 99))))
_, rcy0, rr0 = detect(mid+45, TW-20, int(TH*0.36), TH)
ylo, yhi = rcy0-rr0-12, rcy0+rr0+12
lcx, lcy, lr = detect(20, mid-30, ylo, yhi)
rcx, rcy, rr = detect(276, TW-20, ylo, yhi)
cy = (lcy+rcy)//2
yy, xx = np.mgrid[0:TH, 0:TW]
def white_radius(cx):
    dist = np.hypot(xx-cx, yy-cy)
    for r in range(max(lr, rr), 12, -1):
        ring = (dist >= r-1.5) & (dist <= r+1.5)
        if ring.any() and white[ring].mean() > 0.45: return r
    return max(lr, rr)
R = min(white_radius(lcx), white_radius(rcx))
# sample the clutch white colour (to match the cassette)
reg = np.asarray(cass)[cy-R:cy+R, lcx-R:lcx+R, :3].reshape(-1,3)
wpx = reg[reg.min(1) > 170]
WHITE = tuple(int(v) for v in (np.median(wpx,0) if len(wpx) else [232,234,238]))
print("centers L=%d R=%d cy=%d R=%d white=%s" % (lcx, rcx, cy, R, WHITE))

# ---- label erase clamped to band edge ----
arr = np.asarray(cass); creamish = (arr[:,:,0] > 215) & (arr[:,:,1] > 205) & (arr[:,:,2] > 170)
rws = creamish.sum(1) > TW*0.30
by0 = int(np.argmax(rws)); by1 = by0
while by1+1 < TH and rws[by1+1]: by1 += 1
cc = np.where(creamish[by0:by1+1].sum(0) > (by1-by0)*0.30)[0]
lab_right = int(cc.max()) if len(cc) else TW-8
bandpix = arr[by0:by1+1, :int(TW*0.22), :3]
bxs = np.where(((bandpix < 95).all(2)).sum(0) > (by1-by0)*0.25)[0]
badge_right = int(min((bxs.max()+10) if len(bxs) else TW*0.10, TW*0.20))
cream = np.median(arr[by0:by1+1][creamish[by0:by1+1]], axis=0)[:3].astype(int)
ImageDraw.Draw(cass).rectangle([badge_right, by0+1, lab_right, by1-1], fill=tuple(int(v) for v in cream)+(255,))

# ---- fill body clutch wells with flat navy disc (static) ----
reg2 = np.asarray(cass)[cy-R:cy+R, lcx-R:lcx+R, :3].reshape(-1,3); dk = reg2[reg2.min(1) < 95]
navy = tuple(int(v) for v in (np.median(dk,0) if len(dk) else [33,38,62]))
cd = ImageDraw.Draw(cass)
for cx in (lcx, rcx): cd.ellipse([cx-R, cy-R, cx+R, cy+R], fill=navy+(255,))

# ---- procedural white hub frames (perfectly centered) ----
SS = 4; size = R*2; N = size*SS; ctr = N/2.0
gy, gx = np.mgrid[0:N, 0:N]; rr_ = np.hypot(gx-ctr, gy-ctr)/SS; th_ = np.arctan2(gy-ctr, gx-ctr)
rim = (rr_ >= R-8) & (rr_ <= R-2)
hubdisc = rr_ <= 11
hole = rr_ <= 4
NFR = 24
frames = []
for f in range(NFR):
    off = np.radians(f*360.0/NFR)
    spoke = np.zeros((N, N), bool)
    for k in range(6):
        ang = np.radians(k*60.0) + off
        dth = np.abs(((th_ - ang + np.pi) % (2*np.pi)) - np.pi)
        spoke |= (dth < np.radians(15)) & (rr_ >= 9) & (rr_ <= R-2)
    wm = (rim | spoke | hubdisc) & ~hole
    al = (wm*255).astype('uint8')
    rgbim = np.zeros((N, N, 3), 'uint8'); rgbim[:] = WHITE
    fr = Image.fromarray(np.dstack([rgbim, al])).resize((size, size), Image.LANCZOS)
    frames.append(fr)

# optional preview: frame 0 vs frame 4 (white should rotate, stay centered; navy static)
if args.preview_dir:
    os.makedirs(args.preview_dir, exist_ok=True)
    for fi in (0, 4):
        p = cass.copy()
        for cx in (lcx, rcx): p.alpha_composite(frames[fi], (cx-R, cy-R))
        Image.alpha_composite(Image.new("RGBA", p.size, (22,28,50,255)), p).convert("RGB").save(
            os.path.join(args.preview_dir, "cassette_preview_%d.png" % fi))

def tobytes(img):
    img = img.convert("RGBA"); px = img.load(); w, h = img.size; out = bytearray()
    for y in range(h):
        for x in range(w):
            r, g, b, al = px[x, y]; v = ((r>>3)<<11)|((g>>2)<<5)|(b>>3)
            out += bytes((v & 0xFF, (v>>8) & 0xFF, al))
    return out, w, h
def emit(out, name, w, h, static=False):
    pre = "static const" if static else "const"
    s = "%s LV_ATTRIBUTE_MEM_ALIGN uint8_t %s_map[] = {\n" % (pre, name)
    for i in range(0, len(out), 24): s += "  " + ",".join("0x%02X" % b for b in out[i:i+24]) + ",\n"
    s += "};\nconst lv_img_dsc_t %s = { .header.cf=LV_IMG_CF_TRUE_COLOR_ALPHA, .header.always_zero=0, .header.reserved=0, .header.w=%d, .header.h=%d, .data_size=%d, .data=%s_map };\n" % (name, w, h, w*h*3, name)
    return s
o,w,h = tobytes(cass)
open(os.path.join(OUT,"ui_img_cassette_body.c"),"w").write('#include "ui.h"\n\n#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n'+emit(o,"cassette_body",w,h))
with open(os.path.join(OUT,"ui_img_cassette_reels.c"),"w") as f:
    f.write('#include "ui.h"\n\n#ifndef LV_ATTRIBUTE_MEM_ALIGN\n#define LV_ATTRIBUTE_MEM_ALIGN\n#endif\n\n')
    for i,fr in enumerate(frames):
        o,w,h = tobytes(fr); f.write(emit(o,"cassette_reel_%d"%i,w,h,static=True)+"\n")
    f.write("const lv_img_dsc_t *cassette_reels[%d] = { %s };\n" % (NFR, ", ".join("&cassette_reel_%d"%i for i in range(NFR))))
    f.write("const int cassette_reel_count = %d;\n" % NFR)
print("LVGL_GEOM lcx=%d rcx=%d cy=%d reel=%d nfr=%d" % (lcx, rcx, cy, size, NFR))
