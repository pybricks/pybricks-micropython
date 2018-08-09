from mymodule import gpios

def gpio_init_in(port, pin):
    gpios(256*0+(ord(port)-65)*16+pin)

def gpio_init_out(port, pin):
    gpios(256*1+(ord(port)-65)*16+pin)    

def gpio_read(port, pin):
    return gpios(256*2+(ord(port)-65)*16+pin)        

def gpio_low(port, pin):
    gpios(256*3+(ord(port)-65)*16+pin)    

def gpio_high(port, pin):
    gpios(256*4+(ord(port)-65)*16+pin)

def all_in(port):
    for i in range(16):
        gpio_init_in(port, i)

def read_all(port):
    return [gpio_read(port, pin) for pin in range(16)]

def off():
    gpio_low('B',11)