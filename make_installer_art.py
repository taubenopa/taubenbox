"""Erzeugt die Tauben-Grafiken für den Installer (benötigt Pillow)."""
import math, os, random
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "assets")
os.makedirs(OUT, exist_ok=True)
S = 3  # Supersampling


def bez(p0, p1, p2, p3, n=24):
    pts = []
    for i in range(n + 1):
        t = i / n
        u = 1 - t
        pts.append((u**3*p0[0] + 3*u*u*t*p1[0] + 3*u*t*t*p2[0] + t**3*p3[0],
                    u**3*p0[1] + 3*u*u*t*p1[1] + 3*u*t*t*p2[1] + t**3*p3[1]))
    return pts


WING = bez((0, 0), (-4, -14), (-14, -30), (-26, -40)) + bez((-26, -40), (-24, -22), (-26, -10), (-20, 0))[1:]


def ellipse_pts(cx, cy, rx, ry, n=40):
    return [(cx + rx*math.cos(2*math.pi*i/n), cy + ry*math.sin(2*math.pi*i/n)) for i in range(n)]


def draw_pigeon(img, x, y, scale, flip, flap, alpha=1.0):
    layer = Image.new("RGBA", img.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    sc = scale * S

    def T(p):
        return (x*S + flip*sc*p[0], y*S + sc*p[1])

    def poly(pts, colour):
        d.polygon([T(p) for p in pts], fill=colour)

    def wing(ws, ang, pivot, colour):
        ca, sa = math.cos(ang), math.sin(ang)
        pts = []
        for px, py in WING:
            px, py = px*ws, py*ws
            pts.append((px*ca - py*sa + pivot[0], px*sa + py*ca + pivot[1]))
        poly(pts, colour)

    wing(0.9, -0.9 + (-flap + 1) * 0.55, (2, -6), (77, 86, 111, 255))
    poly([(-16, -4), (-42, -8), (-46, 0), (-42, 8), (-16, 5)], (98, 108, 136, 255))
    poly(ellipse_pts(0, 0, 24, 11), (148, 157, 184, 255))
    poly(ellipse_pts(24, -9, 7.5, 7.5), (164, 173, 198, 255))
    poly(ellipse_pts(15.5, -6, 7.5, 7), (96, 200, 200, 255))
    poly(ellipse_pts(18, -4, 4.5, 4), (150, 120, 230, 255))
    poly([(30, -11), (38, -8.5), (30, -6.5)], (233, 200, 160, 255))
    poly(ellipse_pts(26.5, -10.5, 1.6, 1.6), (255, 169, 77, 255))
    wing(1.0, -0.9 + (flap + 1) * 0.55, (4, -6), (122, 133, 163, 255))

    if alpha < 1.0:
        layer.putalpha(layer.getchannel("A").point(lambda v: int(v * alpha)))
    img.alpha_composite(layer)


def gradient(w, h, top, bottom):
    img = Image.new("RGBA", (w, h))
    px = img.load()
    for yy in range(h):
        t = yy / max(1, h - 1)
        c = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3)) + (255,)
        for xx in range(w):
            px[xx, yy] = c
    return img


def font(size):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
              "C:/Windows/Fonts/segoeuib.ttf", "C:/Windows/Fonts/arialbd.ttf"):
        if os.path.exists(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


def banner(W=328, H=628):
    img = gradient(W*S, H*S, (13, 16, 32), (44, 60, 110))
    d = ImageDraw.Draw(img)
    rnd = random.Random(3)
    for _ in range(70):
        sx, sy = rnd.randint(0, W*S), rnd.randint(0, int(H*S*0.6))
        r = rnd.choice([1, 1, 2])
        d.ellipse([sx-r, sy-r, sx+r, sy+r], fill=(220, 230, 255, rnd.randint(80, 200)))
    d.ellipse([220*S, 40*S, 290*S, 110*S], fill=(240, 244, 255, 255))
    d.ellipse([234*S, 34*S, 300*S, 100*S], fill=(26, 34, 66, 255))
    flock = [(70, 170, 1.6, 1, 0.6), (230, 250, 1.1, -1, -0.5), (110, 330, 0.8, 1, 0.1),
             (250, 380, 0.65, -1, 0.7), (60, 440, 0.55, 1, -0.4), (190, 120, 0.5, 1, 0.3)]
    for x, y, s, f, fl in flock:
        draw_pigeon(img, x, y, s, f, fl, alpha=min(1.0, 0.45 + s*0.35))
    d = ImageDraw.Draw(img)
    d.text((22*S, 500*S), "TAUBEN", font=font(40*S), fill=(255, 255, 255, 255))
    d.text((22*S, 546*S), "BOX", font=font(40*S), fill=(82, 227, 194, 255))
    d.text((24*S, 596*S), "by taubenopa", font=font(15*S), fill=(178, 107, 255, 255))
    return img.convert("RGB").resize((W, H), Image.LANCZOS)


def small(W=110, H=116):
    img = gradient(W*S, H*S, (20, 26, 52), (44, 60, 110))
    draw_pigeon(img, 60, 70, 0.95, 1, 0.5)
    return img.convert("RGB").resize((W, H), Image.LANCZOS)


def icon(N=256):
    global S
    old = S
    S = 2
    img = gradient(N*S, N*S, (20, 26, 52), (52, 70, 130))
    draw_pigeon(img, 150, 150, 2.4, 1, 0.4)
    S = old
    return img.convert("RGBA").resize((N, N), Image.LANCZOS)


if __name__ == "__main__":
    banner().save(os.path.join(OUT, "wizard_large.bmp"))
    small().save(os.path.join(OUT, "wizard_small.bmp"))
    icon().save(os.path.join(OUT, "taubenbox.ico"), sizes=[(256, 256), (64, 64), (48, 48), (32, 32), (16, 16)])
    print("Grafiken erzeugt in", OUT)
