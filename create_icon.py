from PIL import Image, ImageDraw

def create_md_icon():
    size = 256
    supersample = 4
    canvas_size = size * supersample
    img = Image.new('RGBA', (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    def scale(value):
        return round(value * supersample)

    def point(x, y):
        return (scale(x), scale(y))

    margin = scale(max(1, size // 16))
    border_width = scale(max(3, size // 24))
    corner_radius = scale(max(2, size // 7))
    bg_color = (52, 152, 219, 255)
    border_color = (30, 95, 145, 255)

    # Draw border (outer rounded rectangle)
    draw.rounded_rectangle(
        [margin, margin, canvas_size - margin, canvas_size - margin],
        radius=corner_radius,
        fill=border_color
    )
    
    # Draw inner background (slightly smaller for border effect)
    inner_margin = margin + border_width
    draw.rounded_rectangle(
        [inner_margin, inner_margin, canvas_size - inner_margin, canvas_size - inner_margin],
        radius=max(1, corner_radius - border_width // 2),
        fill=bg_color
    )

    mark_color = (255, 255, 255, 255)
    stroke = scale(11)

    def line(points):
        try:
            draw.line([point(x, y) for x, y in points], fill=mark_color, width=stroke, joint='curve')
        except TypeError:
            draw.line([point(x, y) for x, y in points], fill=mark_color, width=stroke)

    # Draw a custom mark rather than font text so the right M stroke flows into
    # the download arrow, matching the exported reference icon.
    line([
        (83, 148),
        (83, 51),
        (128, 148),
        (173, 51),
        (173, 166),
        (128, 166),
        (128, 191),
    ])

    # Pointed arrow head with straight edges; antialiasing comes from the
    # supersampled canvas rather than rounded geometry.
    draw.polygon(
        [
            point(108, 190),
            point(148, 190),
            point(128, 212),
        ],
        fill=mark_color
    )

    try:
        resample = Image.Resampling.LANCZOS
    except AttributeError:
        resample = 1

    img = img.resize((size, size), resample)

    # Save ICO with multiple sizes - let PIL downscale
    ico_sizes = [(16,16), (24,24), (32,32), (48,48), (64,64), (128,128), (256,256)]
    img.save('icon.ico', format='ICO', sizes=ico_sizes)
    img.save('icon.png', format='PNG')
    
    # Create a 96x96 version specifically for the welcome page (smooth scaling)
    img_96 = img.resize((96, 96), resample)
    img_96.save('icon_96.png', format='PNG')
    
    import os
    print(f"Created icon.ico ({os.path.getsize('icon.ico')} bytes) and icon.png ({os.path.getsize('icon.png')} bytes)")

if __name__ == "__main__":
    create_md_icon()
