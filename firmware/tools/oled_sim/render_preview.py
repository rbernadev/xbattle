#!/usr/bin/env python3
"""Legge frames.jsonl (prodotto da ./sim) e crea anteprima_oled.png con i frame chiave.
Font approssimati (DejaVu): le posizioni sono quelle reali del firmware, i caratteri no.
Uso:  python3 render_preview.py [frames.jsonl] [uscita.png]
"""
import json, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont

src = sys.argv[1] if len(sys.argv) > 1 else "frames.jsonl"
out = sys.argv[2] if len(sys.argv) > 2 else "anteprima_oled.png"
fr = [json.loads(l) for l in open(src)]
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"
LABEL = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
fonts = {1: ImageFont.truetype(FONT, 8), 2: ImageFont.truetype(FONT, 13), 3: ImageFont.truetype(FONT, 20)}

def render(frame):
    buf = np.zeros((32, 128), dtype=bool); col = 1
    for c in frame["c"]:
        m = Image.new("1", (128, 32), 0); d = ImageDraw.Draw(m); d.fontmode = "1"; o = c["o"]
        if o == "col": col = c["c"]; continue
        if o == "box": d.rectangle([c["x"], c["y"], c["x"]+c["w"]-1, c["y"]+c["h"]-1], fill=1)
        elif o == "frame": d.rectangle([c["x"], c["y"], c["x"]+c["w"]-1, c["y"]+c["h"]-1], outline=1)
        elif o == "circle": d.ellipse([c["x"]-c["r"], c["y"]-c["r"], c["x"]+c["r"], c["y"]+c["r"]], outline=1)
        elif o == "disc": d.ellipse([c["x"]-c["r"], c["y"]-c["r"], c["x"]+c["r"], c["y"]+c["r"]], fill=1)
        elif o == "tri": p = c["p"]; d.polygon([(p[0], p[1]), (p[2], p[3]), (p[4], p[5])], fill=1)
        elif o == "str": d.text((c["x"], c["y"]), c["s"], font=fonts[c["f"]], fill=1, anchor="ls")
        a = np.array(m, dtype=bool)
        if col == 1: buf |= a
        elif col == 0: buf &= ~a
        elif col == 2: buf ^= a
    return buf

def to_img(buf, s=5):
    im = Image.new("RGB", (128, 32), (8, 10, 14)); px = im.load()
    for y in range(32):
        for x in range(128):
            if buf[y, x]: px[x, y] = (120, 220, 255)
    return im.resize((128*s, 32*s), Image.NEAREST)

pick = lambda t: min(fr, key=lambda f: abs(f["t"]-t))
shots = [(250, "1 Boot: il logo entra"), (1200, "2 Boot: il bey accelera"), (2000, "3 Attesa, nessun lancio"),
         (2800, "4 Banner connessione BLE"), (4900, "5 Lancio live: barra + RPM"), (6700, "6 Risultato: conteggio animato"),
         (7150, "7 Lampo al picco (negativo)"), (8500, "8 Risultato fermo"), (12750, "9 Record: NEW BEST!"),
         (14500, "10 Sync offline"), (15100, "11 Sync offline"), (16000, "12 Ultimo lancio (resta a schermo)")]
S = 5; W = 128*S+20; H = 32*S+40
sheet = Image.new("RGB", (W*3, H*4), (24, 26, 32)); dr = ImageDraw.Draw(sheet)
lab = ImageFont.truetype(LABEL, 14)
for i, (t, name) in enumerate(shots):
    f = pick(t); x = (i % 3)*W+10; y = (i//3)*H+30
    sheet.paste(to_img(render(f), S), (x, y)); dr.text((x, y-22), f"{name}  (t={f['t']/1000:.2f}s)", font=lab, fill=(230, 230, 230))
sheet.save(out); print("scritto", out)
