# Test classes used for EV3/NXT standard firmware compatibility

from pybricks.messaging import (
    EV3LogicMailbox, EV3NumericMailbox, EV3TextMailbox)


class MockConnection:
    _bricks = {}

    def __init__(self, brick):
        self._name = brick
        self._bricks[brick] = {}

    def send_to_mailbox(self, brick, mbox, payload):
        if isinstance(payload, str):
            payload = payload.encode()
        if brick is None:
            for k, v in self._bricks.items():
                if k != self._name:
                    v[mbox] = payload
        else:
            self._bricks[brick][mbox] = payload

    def read_from_mailbox(self, mbox):
        return self._bricks[self._name].get(mbox)

    def print_payload(self, mbox):
        print(self._bricks[self._name].get(mbox))


SERVER = 'server'
CLIENT = 'client'

LOGIC = 'logic'
NUMERIC = 'numeric'
TEXT = 'text'


server = MockConnection(SERVER)
server_logic = EV3LogicMailbox(LOGIC, server)
server_numeric = EV3NumericMailbox(NUMERIC, server)
server_text = EV3TextMailbox(TEXT, server)


client = MockConnection(CLIENT)
client_logic = EV3LogicMailbox(LOGIC, client)
client_numeric = EV3NumericMailbox(NUMERIC, client)
client_text = EV3TextMailbox(TEXT, client)


# mailbox that has not received a value returns None
print(client_logic.read())

server_logic.send(True)
# should be True
print(client_logic.read())
client.print_payload(LOGIC)

server_logic.send(None)
# should be False - None is falsy
print(client_logic.read())
client.print_payload(LOGIC)


# mailbox that has not received a value returns None
print(client_numeric.read())

server_numeric.send(1.2)
# should be 1.200000047683716 - does not have exact representation
print(client_numeric.read())
client.print_payload(NUMERIC)

server_numeric.send(5)
# should be 5.0 - int is converted to float
print(client_numeric.read())
client.print_payload(NUMERIC)


# mailbox that has not received a value returns None
print(client_text.read())

server_text.send('hi')
# should be hi
print(client_text.read())
client.print_payload(TEXT)

server_text.send(5)
# should be 5 - other types are converted to string
print(client_text.read())
client.print_payload(TEXT)
