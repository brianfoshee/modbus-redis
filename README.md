[Blog post] [my-blog]

Reading data from a Morningstar Sunsaver MPPT and storing it in Redis.

Starting point for what will be a system to monitor and view solar/charging data.

This runs on a Raspberry Pi. Requires [hiredis] [hired] and [libmodbus] [libmod].

Basis of the modbus from [this blog post] [tom-blog].

To setup a Raspberry Pi with Arch Linux see [rpi-arch-setup] [arch-setup]

Work to improve this:

- Send data to an app on Heroku (collecting every 5 seconds generates a bunch of Redis keys)
  - [Sinatra app for this here] [solar-collector]
- Visualize with D3 (realtime and archived)
- Clean up mod-red.c
- Refactor mod-red.c
  - Restructure Redis keys
  - DRY up data accessing and number conversions

[tom-blog]: http://westyd1982.wordpress.com/2010/03/26/linux-and-mac-os-x-software-to-read-data-from-the-sunsaver-mppt-using-modbus/

[hired]: https://github.com/redis/hiredis

[libmod]: https://github.com/stephane/libmodbus

[my-blog]: http://www.gingilipino.com/brian/solar-data-collection.html

[solar-collector]: https://github.com/crakalakin/solar-collector

[arch-setup]: https://github.com/crakalakin/modbus-redis/blob/master/rpi-arch-setup.md
