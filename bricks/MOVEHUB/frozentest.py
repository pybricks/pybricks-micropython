from mymodule import gpios

def gpio_init_in_up(port, pins):
    for pin in pins:
        gpios(256*0+(ord(port)-65)*16+pin) 

def gpio_init_out(port, pins):
    for pin in pins:
        gpios(256*1+(ord(port)-65)*16+pin)    

def gpio_read(port, pins):
    return [gpios(256*2+(ord(port)-65)*16+pin) for pin in pins]

def gpio_low(port, pins):
    for pin in pins:
        gpios(256*3+(ord(port)-65)*16+pin)    

def gpio_high(port, pins):
    for pin in pins:
        gpios(256*4+(ord(port)-65)*16+pin)

def gpio_init_in_down(port, pins):
    for pin in pins:
        gpios(256*5+(ord(port)-65)*16+pin)   

def off():
    gpio_low('B', (11,))