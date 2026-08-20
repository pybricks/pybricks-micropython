from pybricks.tools import cobs_encode, cobs_decode

from urandom import randint

## Mimic a sender.

# Two example packets put on the wire on a single stream.
stream = cobs_encode(0, b"Hello, ") + cobs_encode(0, b"World!")


# Mimic data coming in chunks of arbitrary size, e.g. UART.
def chunked_stream(stream):
    while stream:
        chunk_size = randint(1, 8)
        yield stream[:chunk_size]
        stream = stream[chunk_size:]


## Receiver example

# User-provided fixed-size buffer holding incomplete messages.
buffer = bytearray(64)

for chunk in chunked_stream(stream):
    # Adds chunks to the buffer, yields complete packets
    # when they are reader.
    for prefix, payload in cobs_decode(buffer, chunk):
        print(prefix, payload)
