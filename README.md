Reading data from a [Morningstar Sunsaver MPPT] [ssmppt] and storing it in Redis.

Since inception, many improvements have been made to this project. It pulls data every 5 seconds, stores it to Redis, then can send it to a remote server as JSON to be stored and analyzed.

This runs on a Raspberry Pi. Requires [hiredis] [hired] and [libmodbus] [libmod]. Required is a USB to RS232 converter which connects to the [Morningstar PC Meterbus adapter] [meterbus].

Basis of the modbus from [this blog post] [tom-blog].

To setup a Raspberry Pi with Arch Linux see [rpi-arch-setup] [arch-setup]

Work to improve this:

- ~~send to statsd/graphite~~
- Visualize with d3 (realtime and archived)
- Clean up collect.c
  - DRY up data accessing and number conversions

[A few (older) images of the setup] [flikr-set]

[tom-blog]: http://westyd1982.wordpress.com/2010/03/26/linux-and-mac-os-x-software-to-read-data-from-the-sunsaver-mppt-using-modbus/

[hired]: https://github.com/redis/hiredis

[libmod]: https://github.com/stephane/libmodbus

[flikr-set]: http://www.flickr.com/photos/bfosh/sets/72157637640405973/

[my-blog]: http://www.gingilipino.com/brian/solar-data-collection.html

[solar-collector]: https://github.com/crakalakin/solar-collector

[arch-setup]: https://github.com/crakalakin/modbus-redis/blob/master/rpi-arch-setup.md

[ssmppt]: http://www.morningstarcorp.com/products/sunsaver-mppt/

[meterbus]: http://www.morningstarcorp.com/products/pc-meterbus-adapter/
