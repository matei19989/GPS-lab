#!/usr/bin/env python3
"""
Generate BMP textures for the skybox scene:
- Bottom.bmp: grass/ground texture
- Top.bmp: sky texture with sunset glow
- Back.bmp: sky + mountains + sunset/sun
- Front.bmp, Left.bmp, Right.bmp: sky + mountains horizon
- Terrain.bmp: grass/rock/dirt terrain texture
"""
import struct
import math
import random
import os

random.seed(77)

SIZE = 512
TEXDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Textures")


def write_bmp(filename, pixels, width, height):
    row_size = (width * 3 + 3) & ~3
    pixel_data_size = row_size * height
    file_size = 54 + pixel_data_size
    with open(filename, 'wb') as f:
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(struct.pack('<HH', 0, 0))
        f.write(struct.pack('<I', 54))
        f.write(struct.pack('<I', 40))
        f.write(struct.pack('<i', width))
        f.write(struct.pack('<i', height))
        f.write(struct.pack('<HH', 1, 24))
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', pixel_data_size))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<i', 2835))
        f.write(struct.pack('<I', 0))
        f.write(struct.pack('<I', 0))
        for y in range(height):
            row = bytearray()
            for x in range(width):
                r, g, b = pixels[y * width + x]
                row.extend([b, g, r])
            while len(row) % 4 != 0:
                row.append(0)
            f.write(row)


def clamp(v, lo=0, hi=255):
    return max(lo, min(hi, int(v)))


def lerp(a, b, t):
    return a + (b - a) * t


def noise2d(x, y, seed=0):
    n = x + y * 57 + seed * 131
    n = (n << 13) ^ n
    return 1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0


def smooth_noise(x, y, seed=0):
    corners = (noise2d(x-1, y-1, seed) + noise2d(x+1, y-1, seed) +
               noise2d(x-1, y+1, seed) + noise2d(x+1, y+1, seed)) / 16.0
    sides = (noise2d(x-1, y, seed) + noise2d(x+1, y, seed) +
             noise2d(x, y-1, seed) + noise2d(x, y+1, seed)) / 8.0
    center = noise2d(x, y, seed) / 4.0
    return corners + sides + center


def interpolate(a, b, x):
    f = (1.0 - math.cos(x * math.pi)) * 0.5
    return a * (1.0 - f) + b * f


def interpolated_noise(x, y, seed=0):
    ix, iy = int(x), int(y)
    fx, fy = x - ix, y - iy
    v1 = smooth_noise(ix, iy, seed)
    v2 = smooth_noise(ix + 1, iy, seed)
    v3 = smooth_noise(ix, iy + 1, seed)
    v4 = smooth_noise(ix + 1, iy + 1, seed)
    return interpolate(interpolate(v1, v2, fx), interpolate(v3, v4, fx), fy)


def perlin2d(x, y, octaves=4, persistence=0.5, seed=0):
    total = 0.0
    freq = 1.0
    amp = 1.0
    max_val = 0.0
    for _ in range(octaves):
        total += interpolated_noise(x * freq, y * freq, seed) * amp
        max_val += amp
        amp *= persistence
        freq *= 2.0
    return total / max_val


# ---- Sky color helpers ----

def sky_color(ny, nx=0.5, sunset_strength=0.0, seed_offset=0):
    """Return (r,g,b) for sky at normalized y (0=horizon, 1=zenith).
       sunset_strength: 0=none, 1=full sunset glow near horizon.
       nx: 0..1 horizontal position, used for sun glow placement."""

    # Base sky gradient: warm horizon to deep blue zenith
    # Horizon color (warm white-blue)
    hr, hg, hb = 185, 205, 230
    # Zenith color (deep blue)
    zr, zg, zb = 40, 80, 170

    t = min(1.0, ny * 1.2)  # Faster falloff
    t = t * t  # Quadratic for smoother gradient

    r = lerp(hr, zr, t)
    g = lerp(hg, zg, t)
    b = lerp(hb, zb, t)

    # Sunset glow near horizon
    if sunset_strength > 0 and ny < 0.5:
        glow_t = (1.0 - ny / 0.5) * sunset_strength
        glow_t *= glow_t
        # Sun position horizontal influence
        sun_cx = 0.5  # center
        sun_spread = max(0, 1.0 - abs(nx - sun_cx) * 2.5)
        glow_t *= (0.4 + 0.6 * sun_spread)
        # Orange/pink glow
        r = lerp(r, 255, glow_t * 0.7)
        g = lerp(g, 160, glow_t * 0.5)
        b = lerp(b, 90, glow_t * 0.3)

    return r, g, b


def add_clouds(r, g, b, x, y, density=0.35, seed_offset=0):
    """Add cloud wisps to sky color."""
    c1 = perlin2d(x / 60.0, y / 30.0, 4, 0.55, seed=70 + seed_offset)
    c2 = perlin2d(x / 120.0, y / 80.0, 3, 0.45, seed=80 + seed_offset) * 0.5
    cloud = c1 + c2
    if cloud > density:
        amt = min(1.0, (cloud - density) / 0.5)
        amt = amt * amt  # Softer edges
        # Clouds are warm white
        r = lerp(r, 250, amt * 0.7)
        g = lerp(g, 248, amt * 0.65)
        b = lerp(b, 240, amt * 0.55)
    return r, g, b


# ---- Texture generators ----

def generate_grass_ground():
    """Lush grass with patches of wildflowers and bare dirt."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            n1 = perlin2d(x / 25.0, y / 25.0, 5, 0.6, seed=1)
            n2 = perlin2d(x / 6.0, y / 6.0, 3, 0.5, seed=2)
            n3 = perlin2d(x / 50.0, y / 50.0, 3, 0.4, seed=3)

            # Base grass with variation
            base_g = 95 + n1 * 45 + n2 * 20
            base_r = 40 + n1 * 20 + n2 * 12
            base_b = 22 + n1 * 12 + n2 * 8

            # Dirt patches
            dirt = (n3 + 1.0) * 0.5
            if dirt > 0.65:
                dirt_amt = min(1.0, (dirt - 0.65) / 0.25)
                base_r = lerp(base_r, 135 + n2 * 20, dirt_amt)
                base_g = lerp(base_g, 100 + n2 * 15, dirt_amt)
                base_b = lerp(base_b, 60 + n2 * 10, dirt_amt)

            # Tiny yellow wildflower specks
            flower = perlin2d(x / 2.5, y / 2.5, 2, 0.7, seed=99)
            if flower > 0.7 and dirt < 0.6:
                f_amt = (flower - 0.7) / 0.3
                base_r = lerp(base_r, 230, f_amt * 0.5)
                base_g = lerp(base_g, 210, f_amt * 0.4)
                base_b = lerp(base_b, 50, f_amt * 0.3)

            pixels.append((clamp(base_r), clamp(base_g), clamp(base_b)))
    return pixels


def generate_terrain_texture():
    """Terrain with grass, rocky patches, and dirt trails."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            n1 = perlin2d(x / 35.0, y / 35.0, 5, 0.55, seed=10)
            n2 = perlin2d(x / 8.0, y / 8.0, 3, 0.5, seed=11)
            n3 = perlin2d(x / 70.0, y / 70.0, 4, 0.5, seed=12)
            n4 = perlin2d(x / 3.0, y / 3.0, 2, 0.6, seed=13)

            blend = (n3 + 1.0) * 0.5  # 0 to 1

            # Lush grass
            gr, gg, gb = 55 + n2 * 18, 115 + n2 * 30, 30 + n2 * 12
            # Rocky ground
            rr, rg, rb = 130 + n4 * 20, 120 + n4 * 18, 100 + n4 * 15
            # Sandy dirt
            dr, dg, db = 160 + n2 * 15, 130 + n2 * 12, 85 + n2 * 10

            if blend < 0.4:
                # Pure grass
                r, g, b = gr, gg, gb
            elif blend < 0.55:
                t = (blend - 0.4) / 0.15
                r = lerp(gr, rr, t)
                g = lerp(gg, rg, t)
                b = lerp(gb, rb, t)
            elif blend < 0.75:
                # Rocky
                r, g, b = rr, rg, rb
            else:
                t = min(1.0, (blend - 0.75) / 0.2)
                r = lerp(rr, dr, t)
                g = lerp(rg, dg, t)
                b = lerp(rb, db, t)

            # Add fine grass blades effect on grass areas
            if blend < 0.5:
                blade = perlin2d(x / 1.5, y / 1.5, 2, 0.7, seed=14)
                r += blade * 8
                g += blade * 15
                b += blade * 5

            pixels.append((clamp(r), clamp(g), clamp(b)))
    return pixels


def generate_sky_top():
    """Sky dome with soft clouds and a slight warm tint."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            dx = (x - SIZE / 2) / (SIZE / 2)
            dy = (y - SIZE / 2) / (SIZE / 2)
            dist = math.sqrt(dx * dx + dy * dy)
            dist = min(dist, 1.0)

            # Center is zenith (deep blue), edges are lighter (horizon)
            t = 1.0 - dist
            r = lerp(150, 45, t * t)
            g = lerp(185, 85, t * t)
            b = lerp(225, 175, t * t)

            # Clouds
            r, g, b = add_clouds(r, g, b, x, y, density=0.3, seed_offset=500)

            pixels.append((clamp(r), clamp(g), clamp(b)))
    return pixels


def generate_horizon_side(seed_offset=0, has_sun=False):
    """Sky + layered mountains with atmospheric perspective."""
    pixels = []

    # Generate jagged mountain profiles with ridges
    profiles = []
    for layer in range(4):
        heights = []
        base_h = [0.35, 0.28, 0.20, 0.12][layer]
        amplitude = [0.18, 0.14, 0.10, 0.06][layer]
        freq_scale = [45.0, 30.0, 20.0, 12.0][layer]
        for x in range(SIZE):
            h = perlin2d(x / freq_scale, 0, 5, 0.55, seed=200 + seed_offset + layer * 50)
            # Add sharp ridges
            ridge = perlin2d(x / (freq_scale * 0.4), 0, 3, 0.6, seed=300 + seed_offset + layer * 50)
            ridge = abs(ridge) * 0.4
            heights.append(base_h + h * amplitude + ridge * amplitude * 0.5)
        profiles.append(heights)

    # Mountain colors per layer (back to front): bluish-gray -> green-brown
    mt_colors = [
        # (base_r, base_g, base_b) - far layer, very atmospheric/blue
        (140, 150, 175),
        # Mid-far layer
        (100, 115, 130),
        # Mid-near layer
        (70, 90, 75),
        # Near layer, darkest green-brown
        (50, 65, 45),
    ]

    # Sun position for the back face
    sun_x = SIZE * 0.6
    sun_y = SIZE * 0.62

    for y in range(SIZE):
        for x in range(SIZE):
            ny = y / float(SIZE)
            nx = x / float(SIZE)

            # Determine which layer we're in (check from back/highest to front/lowest)
            in_mountain = False
            for layer in range(4):
                if ny < profiles[layer][x]:
                    # We're inside this mountain layer
                    n = perlin2d(x / (10.0 + layer * 5), y / (10.0 + layer * 5),
                                 3, 0.5, seed=400 + seed_offset + layer * 30)

                    br, bg, bb = mt_colors[layer]
                    r = br + n * 25
                    g = bg + n * 25
                    b = bb + n * 20

                    # Snow on peaks for back two layers
                    peak_h = profiles[layer][x]
                    snow_line = peak_h - 0.04 - layer * 0.01
                    if ny > snow_line and layer < 2:
                        snow_t = min(1.0, (ny - snow_line) / 0.03)
                        snow_t *= snow_t
                        n_snow = perlin2d(x / 5.0, y / 5.0, 2, 0.5, seed=450 + seed_offset) * 0.15
                        r = lerp(r, 230 + n_snow * 20, snow_t)
                        g = lerp(g, 235 + n_snow * 15, snow_t)
                        b = lerp(b, 240 + n_snow * 10, snow_t)

                    # Slight shading on slopes
                    if x > 0 and x < SIZE - 1:
                        slope = profiles[layer][x+1] - profiles[layer][x-1]
                        shade = max(-1, min(1, slope * 15))
                        r += shade * 12
                        g += shade * 12
                        b += shade * 10

                    pixels.append((clamp(r), clamp(g), clamp(b)))
                    in_mountain = True
                    break

            if not in_mountain:
                # Sky
                sky_ny = (ny - profiles[0][x]) / max(1.0 - profiles[0][x], 0.01)
                sky_ny = max(0, sky_ny)
                sunset = 0.6 if has_sun else 0.15
                r, g, b = sky_color(sky_ny, nx, sunset_strength=sunset, seed_offset=seed_offset)

                # Sun glow (only on back face)
                if has_sun:
                    dx = (x - sun_x) / SIZE
                    dy = (y - sun_y) / SIZE
                    sun_dist = math.sqrt(dx * dx + dy * dy)
                    if sun_dist < 0.25:
                        # Bright sun core
                        if sun_dist < 0.03:
                            sun_t = 1.0 - sun_dist / 0.03
                            r = lerp(r, 255, sun_t)
                            g = lerp(g, 252, sun_t * 0.95)
                            b = lerp(b, 220, sun_t * 0.8)
                        else:
                            # Sun glow halo
                            glow = 1.0 - (sun_dist - 0.03) / 0.22
                            glow = glow * glow * glow
                            r = lerp(r, 255, glow * 0.5)
                            g = lerp(g, 220, glow * 0.35)
                            b = lerp(b, 150, glow * 0.2)

                # Clouds
                r, g, b = add_clouds(r, g, b, x, y, density=0.32, seed_offset=seed_offset)

                pixels.append((clamp(r), clamp(g), clamp(b)))
    return pixels


def generate_detail_texture():
    """Fine detail texture for terrain close-up."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            n1 = perlin2d(x / 3.0, y / 3.0, 3, 0.65, seed=100)
            n2 = perlin2d(x / 8.0, y / 8.0, 2, 0.5, seed=101)
            v = 128 + n1 * 50 + n2 * 20
            # Slight color variation (not pure gray)
            r = clamp(v + 3)
            g = clamp(v + 5)
            b = clamp(v - 2)
            pixels.append((r, g, b))
    return pixels


def generate_road_texture():
    """Asphalt road with a dashed center line and white edge lines."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            # Base asphalt: dark grey with fine noise
            n = perlin2d(x / 4.0, y / 4.0, 3, 0.5, seed=200)
            base = 55 + n * 18
            r, g, b = clamp(base - 2), clamp(base), clamp(base + 2)

            # White edge lines (10 px wide, at top and bottom of texture)
            if y < 12 or y > SIZE - 13:
                r, g, b = 230, 230, 225

            # Dashed center line (yellow, 10 px wide, 40 px on / 40 px off)
            center_band = abs(x - SIZE // 2) < 6
            dash_on = (y // 40) % 2 == 0
            if center_band and dash_on:
                r, g, b = 240, 215, 30

            pixels.append((r, g, b))
    return pixels


def generate_building_wall():
    """Concrete/brick building facade with rows of windows."""
    pixels = []
    brick_h, brick_w = 24, 36
    mortar = 3
    win_cols, win_rows = 4, 6
    win_w = SIZE // win_cols
    win_h = SIZE // win_rows

    for y in range(SIZE):
        for x in range(SIZE):
            # Brick pattern
            row = y // brick_h
            col = (x + (row % 2) * (brick_w // 2)) // brick_w
            bx = (x + (row % 2) * (brick_w // 2)) % brick_w
            by = y % brick_h
            in_mortar = bx < mortar or by < mortar

            if in_mortar:
                r, g, b = 185, 180, 172
            else:
                n = perlin2d(x / 8.0, y / 8.0, 2, 0.5, seed=300 + row * 7 + col * 3)
                r = clamp(175 + n * 20)
                g = clamp(145 + n * 18)
                b = clamp(120 + n * 15)

            # Windows: dark blue-grey glass
            wx = x % win_w
            wy = y % win_h
            margin_x = win_w // 5
            margin_y = win_h // 5
            if margin_x < wx < win_w - margin_x and margin_y < wy < win_h - margin_y:
                n2 = perlin2d(x / 12.0, y / 12.0, 2, 0.4, seed=350)
                r = clamp(60 + n2 * 15)
                g = clamp(80 + n2 * 15)
                b = clamp(110 + n2 * 20)

            pixels.append((r, g, b))
    return pixels


def generate_building_roof():
    """Flat concrete roof with subtle texture and tar-paper lines."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            n = perlin2d(x / 15.0, y / 15.0, 3, 0.5, seed=400)
            base = 110 + n * 22
            r, g, b = clamp(base - 5), clamp(base - 3), clamp(base)
            # Horizontal tar strips every 64 px
            if (y % 64) < 4:
                r, g, b = clamp(r - 25), clamp(g - 25), clamp(b - 20)
            pixels.append((r, g, b))
    return pixels


def generate_tree_bark():
    """Rough dark-brown bark with vertical streaks."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            n1 = perlin2d(x / 3.0, y / 12.0, 4, 0.6, seed=500)
            n2 = perlin2d(x / 8.0, y / 5.0, 3, 0.5, seed=510)
            r = clamp(90 + n1 * 30 + n2 * 12)
            g = clamp(58 + n1 * 22 + n2 * 8)
            b = clamp(28 + n1 * 12 + n2 * 5)
            pixels.append((r, g, b))
    return pixels


def generate_tree_leaves():
    """Dense green foliage with light/dark variation."""
    pixels = []
    for y in range(SIZE):
        for x in range(SIZE):
            n1 = perlin2d(x / 12.0, y / 12.0, 4, 0.55, seed=600)
            n2 = perlin2d(x / 4.0, y / 4.0, 3, 0.6, seed=610)
            r = clamp(30 + n1 * 20 + n2 * 8)
            g = clamp(90 + n1 * 40 + n2 * 20)
            b = clamp(18 + n1 * 10 + n2 * 5)
            pixels.append((r, g, b))
    return pixels


if __name__ == "__main__":
    os.makedirs(TEXDIR, exist_ok=True)

    print("Generating grass ground texture (Bottom.bmp)...")
    write_bmp(os.path.join(TEXDIR, "Bottom.bmp"), generate_grass_ground(), SIZE, SIZE)

    print("Generating sky top texture (Top.bmp)...")
    write_bmp(os.path.join(TEXDIR, "Top.bmp"), generate_sky_top(), SIZE, SIZE)

    print("Generating terrain texture (Terrain.bmp)...")
    write_bmp(os.path.join(TEXDIR, "Terrain.bmp"), generate_terrain_texture(), SIZE, SIZE)

    print("Generating detail texture (Detail.bmp)...")
    write_bmp(os.path.join(TEXDIR, "Detail.bmp"), generate_detail_texture(), SIZE, SIZE)

    # Back face gets the sun, others just mountains + sky
    sides = [
        ("Back.bmp",  0,   True),   # Sun on back face
        ("Front.bmp", 100, False),
        ("Left.bmp",  200, False),
        ("Right.bmp", 300, False),
    ]
    for name, seed_off, sun in sides:
        print(f"Generating horizon texture ({name})...")
        write_bmp(os.path.join(TEXDIR, name), generate_horizon_side(seed_off, has_sun=sun), SIZE, SIZE)

    print("Generating road texture (Road.bmp)...")
    write_bmp(os.path.join(TEXDIR, "Road.bmp"), generate_road_texture(), SIZE, SIZE)

    print("Generating building wall texture (BuildingWall.bmp)...")
    write_bmp(os.path.join(TEXDIR, "BuildingWall.bmp"), generate_building_wall(), SIZE, SIZE)

    print("Generating building roof texture (BuildingRoof.bmp)...")
    write_bmp(os.path.join(TEXDIR, "BuildingRoof.bmp"), generate_building_roof(), SIZE, SIZE)

    print("Generating tree bark texture (TreeBark.bmp)...")
    write_bmp(os.path.join(TEXDIR, "TreeBark.bmp"), generate_tree_bark(), SIZE, SIZE)

    print("Generating tree leaves texture (TreeLeaves.bmp)...")
    write_bmp(os.path.join(TEXDIR, "TreeLeaves.bmp"), generate_tree_leaves(), SIZE, SIZE)

    print("Done! All textures generated.")
