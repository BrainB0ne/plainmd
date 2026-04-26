from PIL import Image, ImageDraw, ImageFont

def create_md_icon():
    size = 256
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    margin = max(1, size // 16)
    border_width = max(3, size // 24)
    corner_radius = max(2, size // 8)
    bg_color = (52, 152, 219, 255)
    border_color = (30, 90, 140, 255)  # Darker blue border for contrast
    
    # Draw border (outer rounded rectangle)
    draw.rounded_rectangle(
        [margin, margin, size - margin, size - margin],
        radius=corner_radius,
        fill=border_color
    )
    
    # Draw inner background (slightly smaller for border effect)
    inner_margin = margin + border_width
    draw.rounded_rectangle(
        [inner_margin, inner_margin, size - inner_margin, size - inner_margin],
        radius=max(1, corner_radius - border_width // 2),
        fill=bg_color
    )
    
    text_color = (255, 255, 255, 255)
    font_size = int(size * 0.55)
    
    try:
        font = ImageFont.truetype("segoeui.ttf", font_size)
    except:
        try:
            font = ImageFont.truetype("arial.ttf", font_size)
        except:
            font = ImageFont.load_default()
    
    text = "M"
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    x = (size - text_width) // 2
    y = (size - text_height) // 2 - bbox[1] // 2
    
    draw.text((x, y), text, font=font, fill=text_color)
    
    # Save ICO with multiple sizes - let PIL downscale
    ico_sizes = [(16,16), (24,24), (32,32), (48,48), (64,64), (128,128), (256,256)]
    img.save('icon.ico', format='ICO', sizes=ico_sizes)
    img.save('icon.png', format='PNG')
    
    # Create a 96x96 version specifically for the welcome page (smooth scaling)
    img_96 = img.resize((96, 96), Image.LANCZOS)
    img_96.save('icon_96.png', format='PNG')
    
    import os
    print(f"Created icon.ico ({os.path.getsize('icon.ico')} bytes) and icon.png ({os.path.getsize('icon.png')} bytes)")

if __name__ == "__main__":
    create_md_icon()
