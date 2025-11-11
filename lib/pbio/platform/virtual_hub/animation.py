#!/usr/bin/env python3
import pygame
import socket
import threading
import queue
from pathlib import Path
import struct

IMG_PATH = Path("lib/pbio/test/animator/img")
HOST = "127.0.0.1"
PORT = 5002
SCREEN_WIDTH = 1400
SCREEN_HEIGHT = 1000
FPS = 25

# Virtual display settings
DISPLAY_WIDTH = 178
DISPLAY_HEIGHT = 128
DISPLAY_SCALE = 2  # Scale factor for display on screen
DISPLAY_POS = (450, 50)  # Position on main screen

PBIO_PYBRICKS_EVENT_STATUS_REPORT = 0
PBIO_PYBRICKS_EVENT_WRITE_STDOUT = 1
PBIO_PYBRICKS_EVENT_WRITE_APP_DATA = 2
PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY = 3

# Incoming events (stdout, status, app data, port view)
data_queue = queue.Queue()


# Handles socket communication. Accepts new connection when client disconnects.
# This means it can stay open at all times even when restarting pybricks-micropython
# for fast debugging.
def socket_listener_thread():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, PORT))
    print(f"Listening on {HOST}:{PORT}")
    while True:
        data, _ = sock.recvfrom(DISPLAY_WIDTH * DISPLAY_HEIGHT)
        data_queue.put(data)


# Hub state
angles = [0] * 6
virtual_display = pygame.Surface((DISPLAY_WIDTH, DISPLAY_HEIGHT))
display_color = [0x90, 0xC5, 0xAD]
virtual_display.fill(display_color)


def update_display(data):
    virtual_display.fill(display_color)
    for y in range(DISPLAY_HEIGHT):
        for x in range(DISPLAY_WIDTH):
            value = data[y * DISPLAY_WIDTH + x]
            if value:
                brightness = (100, 75, 25, 0)[value]
                color = [c * brightness // 100 for c in display_color]
                virtual_display.set_at((x, y), color)


def process_telemetry(payload):
    # Only supports motors for now.
    if len(payload) == 6:
        type_id, index, angle = struct.unpack("<bbi", payload[0:6])
        if type_id == 96:
            angles[index] = angle


def update_state():
    while not data_queue.empty():
        data = data_queue.get_nowait()

        # Revisit: we should use the telemetry protocol for the display, but
        # this is not hooked up yet. For now, just assume that a huge payload
        # is a display buffer.
        if len(data) > 6000:
            update_display(data)
            continue

        # Expecting a notification event code with payload
        if not isinstance(data, bytes) or len(data) < 2:
            continue
        event = data[0]
        payload = data[1:]

        # Stdout goes to animation terminal.
        if event == PBIO_PYBRICKS_EVENT_WRITE_STDOUT:
            try:
                print(payload.decode(), end="")
            except UnicodeDecodeError:
                print(payload)
        # Telemetry is used to update the visual state.
        elif event == PBIO_PYBRICKS_EVENT_WRITE_TELEMETRY:
            process_telemetry(payload)


def main():
    # Initialize Pygame
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Virtual Hub Animator")
    clock = pygame.time.Clock()

    def blit_rotate_at_center(image, position, angle):
        rotated_image = pygame.transform.rotate(image, angle)
        rotated_rect = rotated_image.get_rect(center=position)
        screen.blit(rotated_image, rotated_rect)

    def load_and_scale_image(filename, scale=0.25):
        image = pygame.image.load(IMG_PATH / filename)
        original_size = image.get_size()
        scaled_size = (int(original_size[0] * scale), int(original_size[1] * scale))
        return pygame.transform.smoothscale(image, scaled_size)

    # Load images
    hub = load_and_scale_image("main-model.png")
    wheel_left = load_and_scale_image("wheel-left.png")
    wheel_right = load_and_scale_image("wheel-right.png")
    shaft = load_and_scale_image("shaft.png")
    stall = load_and_scale_image("stall.png")
    gear_drive = load_and_scale_image("gear-drive.png")
    gear_follow = load_and_scale_image("gear-follow.png")

    display_border_rect = pygame.Rect(
        DISPLAY_POS[0] - 2,
        DISPLAY_POS[1] - 2,
        DISPLAY_WIDTH * DISPLAY_SCALE + 4,
        DISPLAY_HEIGHT * DISPLAY_SCALE + 4,
    )

    # Start the socket listener thread
    threading.Thread(target=socket_listener_thread, daemon=True).start()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        # Parse queued incoming data.
        update_state()

        # Clear screen.
        screen.fill((161, 168, 175))

        # Draw and rotate components, with left wheel behind hub.
        blit_rotate_at_center(wheel_left, (1242, 156), -angles[0])
        screen.blit(hub, (0, 0))
        blit_rotate_at_center(wheel_right, (1067, 331), angles[1])
        blit_rotate_at_center(shaft, (247, 712), angles[4])
        blit_rotate_at_center(stall, (245, 189), angles[2])
        blit_rotate_at_center(gear_drive, (943, 694), -angles[5])
        blit_rotate_at_center(gear_follow, (1037, 694), angles[5] / 3)

        # Show virtual display.
        pygame.draw.rect(screen, (50, 50, 50), display_border_rect)
        scaled_surface = pygame.transform.scale(
            virtual_display,
            (DISPLAY_WIDTH * DISPLAY_SCALE, DISPLAY_HEIGHT * DISPLAY_SCALE),
        )
        screen.blit(scaled_surface, DISPLAY_POS)

        # Update display
        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()


if __name__ == "__main__":
    main()
