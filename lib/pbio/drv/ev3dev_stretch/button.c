#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>

#include <pbio/button.h>
#include <pbio/error.h>
#include <pbio/port.h>

#define EV3KEY_UP (103)
#define EV3KEY_CENTER (28)
#define EV3KEY_DOWN (108)
#define EV3KEY_RIGHT (106)
#define EV3KEY_LEFT (105)
#define EV3KEY_BACK (14)

uint8_t f_btn; // Button file descriptor

void _pbdrv_button_init(void) {
    f_btn = open("/dev/input/by-path/platform-gpio_keys-event", O_RDONLY);
}

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_button_deinit(void) {
    close(f_btn); 
 }
#endif

bool check(uint8_t *buffer, uint8_t key){
    return buffer[key>>3] & (1 << (key % 8));
}

pbio_error_t pbdrv_button_is_pressed(pbio_port_t port, pbio_button_flags_t *pressed) {
    if (port != PBIO_PORT_SELF) {
        return PBIO_ERROR_INVALID_PORT;
    }

    uint8_t buffer[(KEY_CNT + 8 - 1 ) / 8];
    ioctl(f_btn, EVIOCGKEY(sizeof(buffer)), &buffer);
    
    if(check(buffer, EV3KEY_UP)) {
        *pressed = PBIO_BUTTON_UP;
    }
    else if(check(buffer, EV3KEY_CENTER)) {
        *pressed = PBIO_BUTTON_CENTER;
    }    
    else if(check(buffer, EV3KEY_DOWN)) {
        *pressed = PBIO_BUTTON_DOWN;
    }  
    else if(check(buffer, EV3KEY_RIGHT)) {
        *pressed = PBIO_BUTTON_RIGHT;
    }  
    else if(check(buffer, EV3KEY_LEFT)) {
        *pressed = PBIO_BUTTON_LEFT;
    }  
    else if(check(buffer, EV3KEY_BACK)) {
        *pressed = PBIO_BUTTON_STOP;
    } 
    return PBIO_SUCCESS;
}
