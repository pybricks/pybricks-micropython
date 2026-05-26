try:
    import sys
except ImportError:
    sys = None

from pybricks.tools import run_task


import struct
import math
from pybricks.messaging import RFCOMMSocket, local_address
from pybricks.tools import wait, multitask, StopWatch

# Protocol Constants (1 byte commands)
CMD_ECHO = 1
CMD_BLCK = 2
CMD_DROP = 3
CMD_EXIT = 4

# Header format: [CMD:1][LEN:2] (Big Endian)
HEADER_FMT = ">BH"
HEADER_SIZE = 3
MTU = 1024


def debug_print(prefix, cmd, data):
    # C-style escaping for payload
    escaped = ""
    for b in data[:20]:
        if 32 <= b <= 126 and b != 92:  # printable and not backslash
            escaped += chr(b)
        else:
            escaped += f"\\x{b:02x}"
    # print(f"{prefix} CMD:{cmd} LEN:{len(data)} PL:\"{escaped}\"")


async def send_packet(sock, cmd, data=b""):
    debug_print("TX", cmd, data)
    payload_len = len(data)
    header = struct.pack(HEADER_FMT, cmd, payload_len)
    await sock.write(header + data)


async def read_packet(sock):
    header_data = await sock.read(HEADER_SIZE)
    if len(header_data) != HEADER_SIZE:
        raise OSError("Incomplete header")

    cmd, payload_len = struct.unpack(HEADER_FMT, header_data)

    if payload_len > 0:
        data = await sock.read(payload_len)
        if len(data) != payload_len:
            raise OSError("Incomplete payload")
    else:
        data = b""

    debug_print("RX", cmd, data)
    return cmd, data


def calculate_stats(samples):
    n = len(samples)
    if n == 0:
        return 0, 0
    mean = sum(samples) / n
    if n > 1:
        variance = sum((x - mean) ** 2 for x in samples) / (n - 1)
        stdev = math.sqrt(variance)
        # 90% confidence interval: t-score for df=39 is approx 1.685
        ci_90 = 1.685 * (stdev / math.sqrt(n))
    else:
        ci_90 = 0
    return mean, ci_90


async def follower():
    print("Follower starting.")
    print(f"My Address: {local_address()}")

    with RFCOMMSocket() as sock:
        print("Listening for connection...")
        try:
            await sock.listen()
            print("Client connected.")

            while True:
                try:
                    cmd, data = await read_packet(sock)
                except Exception as e:
                    print(f"Read error or disconnect: {e}")
                    break

                if cmd == CMD_ECHO:
                    await sock.write(data)

                elif cmd == CMD_BLCK:
                    # payload is duration in ms (4 bytes, big endian)
                    if len(data) >= 4:
                        msg_duration = int.from_bytes(data[:4], "big")
                    else:
                        msg_duration = 0

                    print(f"Blocking for {msg_duration}ms...")
                    await wait(msg_duration)

                elif cmd == CMD_DROP:
                    pass

                elif cmd == CMD_EXIT:
                    print("Received EXIT command.")
                    break

                else:
                    print(f"Unknown command: {cmd}")

        except Exception as e:
            print(f"Follower exception: {e}")
        print("Follower finished.")


async def leader(peer_addr):
    print("Leader starting.")
    print(f"My Address: {local_address()}")

    if not peer_addr:
        print("No peer address provided.")
        return

    # We intentionally do not use a context manager here so we can also
    # test the close() method.
    sock = RFCOMMSocket()
    print(f"Connecting to {peer_addr}...")
    try:
        await sock.connect(peer_addr)
        print("Connected.")

        # --- Test 1: Message Order ---
        print("\nTest 1: Message Order")
        payloads = [bytes(f"Message {i}", "utf-8") for i in range(1, 6)]

        print(f"  Sending {len(payloads)} messages...")
        for p in payloads:
            await send_packet(sock, CMD_ECHO, p)

        print("  Reading messages...")
        received = []
        for i, p in enumerate(payloads):
            # We expect ECHO replies
            data = await sock.read(len(p))
            if data != p:
                print(f"  Packet {i + 1}: FAIL (Expected {p}, got {data})")
                success = False
            else:
                print(f"  Packet {i + 1}: OK")

        # --- Test 2: Throughput ---
        print("\nTest 2: Throughput")
        payload_size = MTU - HEADER_SIZE
        payload_mtu = b"x" * payload_size
        print(f"  Packet size: {MTU} (Header: {HEADER_SIZE} + Payload: {payload_size})")

        # Shared state container
        state = {"sent": 0, "received": 0, "sending": True}

        async def sender():
            sw = StopWatch()
            while sw.time() < 1000:
                await send_packet(sock, CMD_ECHO, payload_mtu)
                state["sent"] += 1
            state["sending"] = False

        async def receiver():
            # Read until sender is done AND we have received everything sent.
            while state["sending"] or state["received"] < state["sent"]:
                data = await sock.read(payload_size)
                if data == payload_mtu:
                    state["received"] += 1
                else:
                    print("  Throughput RX Error: mismatch or wrong cmd")
                    break

        # Run both concurrently
        await multitask(sender(), receiver())

        print(
            f"  Result: {state['received']} KB/s (Sent: {state['sent']}, Recv: {state['received']})"
        )

        # --- Test 3: RTT ---
        print("\nTest 3: RTT (40 samples)")
        payload_rtt = b"ping"
        samples = []
        sw = StopWatch()

        for _ in range(40):
            sw.reset()
            await send_packet(sock, CMD_ECHO, payload_rtt)
            await sock.read(len(payload_rtt))  # Read header + payload
            samples.append(sw.time())
            await wait(10)

        mean, ci = calculate_stats(samples)
        print(f"  RTT Mean: {mean:.2f} ms")
        print(f"  RTT 90% CI: +/- {ci:.2f} ms")

        # --- Test 4: Flow Control ---
        print("\nTest 4: Flow Control")
        block_ms = 500
        print(f"  Sending BLOCK for {block_ms}ms")

        # Send ONE block command
        await send_packet(sock, CMD_BLCK, block_ms.to_bytes(4, "big"))

        # Flood with DROP commands
        flood_payload_len = MTU - HEADER_SIZE
        flood_data = b"x" * flood_payload_len  # Just payload

        sent_bytes = 0
        sw.reset()

        while sw.time() < 500:
            try:
                await send_packet(sock, CMD_DROP, flood_data)
                if sw.time() < 500:
                    sent_bytes += HEADER_SIZE + len(flood_data)
            except Exception as e:
                print(f"  Send interrupted: {e}")
                break

        print(f"  Bytes sent during 500ms block window: {sent_bytes}")

        if sent_bytes <= 5 * 1024:
            print("  Flow Control: PASS")
        else:
            print(f"  Flow Control: FAIL (Sent {sent_bytes} > 5120 bytes)")

        await wait(1000)  # Allow time for the remote block to finish.

        # --- Test 5: in_waiting and read_all --    -
        print("\nTest 5: waiting and read_all")
        payload_5 = bytes(range(42))
        await send_packet(sock, CMD_ECHO, payload_5)

        # Wait for in_waiting() to be 42
        async def sock_waiting_len(n):
            while sock.waiting() < n:
                await wait(1)
            return True

        (res, _) = await multitask(
            sock_waiting_len(len(payload_5)), wait(1000), race=True
        )

        if res:
            print("  waiting: PASS")
        else:
            print(f"  waiting: FAIL (Expected {len(payload_5)}, got {sock.waiting()})")
            return
        data = sock.read_all()
        if len(data) == 42 and data == payload_5:
            print("  read_all: PASS")
        else:
            print(f"  read_all: FAIL (Expected 42 bytes, got {len(data)})")
            return

        # --- Test 6: wait_until ---
        print("\nTest 6: wait_until")
        payload_6 = b"bar" * 33
        payload_6 = payload_6[:-3] + b"foo"
        await send_packet(sock, CMD_ECHO, payload_6)

        async def wait_until_true(msg):
            await sock.wait_until(msg)
            return True

        (res, _) = await multitask(wait_until_true(b"foo"), wait(1000), race=True)
        if res:
            print("  wait_until: PASS")
        else:
            print(f"  wait_until: FAIL (never received 'foo')")
            return

        # --- Test 7: clear ---
        print("\nTest 7: clear")
        payload_7 = b"short"
        await send_packet(sock, CMD_ECHO, payload_7)
        (res, _) = await multitask(
            sock_waiting_len(len(payload_7)), wait(1000), race=True
        )
        if not res:
            print(
                f"  clear: FAIL (Expected {len(payload_7)} waiting, got {sock.waiting()})"
            )
            return
        sock.clear()
        if sock.waiting() == 0:
            print("  clear: PASS")
        else:
            print(f"  clear: FAIL (Expected 0 waiting, got {sock.waiting()})")

    except Exception as e:
        print(f"Leader Error: {e}")
    finally:
        sock.close()
    print("Leader finished.")


# Get peer address from args if available
peer_addr = "24:71:89:5A:03:23"
if sys and len(sys.argv) > 1:
    peer_addr = sys.argv[1]

run_task(leader(peer_addr))
