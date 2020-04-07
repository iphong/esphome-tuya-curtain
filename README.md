# ESPHome firmware for Tuya Curtain

So i got my hands on one of these Tuya Curtain. But the Home Assistant integration was terrible. So i decided to write my own custom firmware that talk to the Tuya MCU, and here is what i've goot so far.

- Set curtain to any position
- Realtime position feedback (even pull by hand)
- Report following positions: open, openning, closed, closing
- Hass Service for setting motor direction
- Hass service for sending custom command
