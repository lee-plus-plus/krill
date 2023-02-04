int sleep_count;
int sleep(void) {
    int x;
    x = 0;
    while (x < sleep_count) {
        x = x + 1;
    }
    return x;
}

int set_sleep_count(int ms) {
    sleep_count = ms * 1310;
    return sleep_count;
}

int sleep_ms(int ms) {
    set_sleep_count(ms);
    sleep();
}

/* Helpers for LED, tested. */
int set_led_value(int value) {
  $(0xfffffc60) = value;
  return value;
}

int get_led_value(void) {
  int x;
  x = $(0xfffffc60);
  return x;
}

/* Helpers for switch, tested. */
int get_switch_value(void) {
  int x;
  x = $(0XFFFFFC70);
  return x;
}

/* (Tested) Helper for keyboard: 0-D for 0~D. E for star, F for # */
int extract(int x, int d) {
    return (x >> d) & 1;
}
int keyboard_is_pressed(int required) {
  int x;
  int retval;
  retval = 0;
  x = $(0xFFFFFC10);
  if (required == 0) {
    retval = extract(x, 8);
  }
  if (required == 1) {
    retval = extract(x, 15);
  }
  if (required == 2) {
    retval = extract(x, 11);
  }
  if (required == 3) {
    retval = extract(x, 7);
  }
  if (required == 4) {
    retval = extract(x, 14);
  }
  if (required == 5) {
    retval = extract(x, 10);
  }
  if (required == 6) {
    retval = extract(x, 6);
  }
  if (required == 7) {
    retval = extract(x, 13);
  }
  if (required == 8) {
    retval = extract(x, 9);
  }
  if (required == 9) {
    retval = extract(x, 5);
  }
  if (required == 10) {
    retval = extract(x, 3);
  }
  if (required == 11) {
    retval = extract(x, 2);
  }
  if (required == 12) {
    retval = extract(x, 1);
  }
  if (required == 13) {
    retval = extract(x, 0);
  }
  if (required == 14) {
    retval = extract(x, 12);
  }
  if (required == 15) {
    retval = extract(x, 4);
  }
  return retval;
}

/* Helper for buzzer, input param `freq` is linear to real frequency */
int set_buzzer_freq(int freq) {
  int cnt;
  cnt = 0x1400000 / freq;
  $(0xFFFFFD10) = cnt;
}

int main(void) {
    int i;
    int j;
    int z;
    j = 0;
    i = 0;
    set_sleep_count(3000);
    while (1) {
        j = j + 1;
        i = get_switch_value();
        set_buzzer_freq(i * 200);
        i = keyboard_is_pressed(i);
        z = i + j << 1;
        set_led_value(z);
        sleep();
    }
    return 0;
}