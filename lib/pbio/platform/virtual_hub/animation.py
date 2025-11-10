#!/usr/bin/env python3
import pygame
import socket
import threading
import queue
from pathlib import Path
import struct


HOST = "127.0.0.1"
PORT = 5002
SCREEN_WIDTH = 1400
SCREEN_HEIGHT = 1000
FPS = 25

PBIO_PYBRICKS_EVENT_STATUS_REPORT = 0
PBIO_PYBRICKS_EVENT_WRITE_STDOUT = 1
PBIO_PYBRICKS_EVENT_WRITE_APP_DATA = 2
PBIO_PYBRICKS_EVENT_WRITE_PORT_VIEW = 3

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
        data, _ = sock.recvfrom(1024)
        data_queue.put(data)


def blit_rotate_at_center(screen, image, position, angle):
    """Rotate image around a specific center point"""
    rotated_image = pygame.transform.rotate(image, angle)
    rotated_rect = rotated_image.get_rect(center=position)
    screen.blit(rotated_image, rotated_rect)


# Define the base path for images
img_path = Path("lib/pbio/test/animator/img")


def load_and_scale_image(filename, scale=0.25):
    """Load an image and scale it by the given scale factor."""
    image = pygame.image.load(img_path / filename)
    original_size = image.get_size()
    scaled_size = (int(original_size[0] * scale), int(original_size[1] * scale))
    return pygame.transform.smoothscale(image, scaled_size)


# Hub state
angles = [0] * 6


def update_state():
    while not data_queue.empty():
        data = data_queue.get_nowait()
        if not isinstance(data, bytes) or len(data) < 2:
            continue
        event = data[0]
        payload = data[1:]

        if event == PBIO_PYBRICKS_EVENT_WRITE_STDOUT:
            try:
                print(payload.decode(), end="")
            except UnicodeDecodeError:
                print(payload)
        elif event == PBIO_PYBRICKS_EVENT_WRITE_PORT_VIEW:
            if len(payload) == 6:
                type_id, index, angle = struct.unpack("<bbi", payload[0:6])
                if type_id == 96:
                    angles[index] = angle


def main():
    # Initialize Pygame
    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Virtual Hub Animator")
    clock = pygame.time.Clock()

    # Load images
    hub = load_and_scale_image("main-model.png")
    wheel_left = load_and_scale_image("wheel-left.png")
    wheel_right = load_and_scale_image("wheel-right.png")
    shaft = load_and_scale_image("shaft.png")
    stall = load_and_scale_image("stall.png")
    gear_drive = load_and_scale_image("gear-drive.png")
    gear_follow = load_and_scale_image("gear-follow.png")

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
        blit_rotate_at_center(screen, wheel_left, (1242, 156), -angles[0])
        screen.blit(hub, (0, 0))
        blit_rotate_at_center(screen, wheel_right, (1067, 331), angles[1])
        blit_rotate_at_center(screen, shaft, (247, 712), angles[4])
        blit_rotate_at_center(screen, stall, (245, 189), angles[2])
        blit_rotate_at_center(screen, gear_drive, (943, 694), -angles[5])
        blit_rotate_at_center(screen, gear_follow, (1037, 694), angles[5] / 3)

        # Update display
        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()


if __name__ == "__main__":
    main()
