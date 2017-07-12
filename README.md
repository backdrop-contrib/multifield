# Multifield

This project seeks to provide a true compound field solution for Backdrop.

![Multifield](https://github.com/backdrop-contrib/multifield/blob/1.x-1.x/images/multifield-screenshot.png "Multifield")

## Limitations

 - The sub-fields inside the multifield are limited to one value. You can have a multiple value multifield, but the individual fields in in the multifield can only have one value.
 - You cannot have a multifield inside another multifield.
 - I've tried to test this with all the core field types, but more advanced fields may cause issues. I haven't run into any yet, but just be aware and test what you want to use.
 - I will be the first to admit, this solution is doing all kinds of unholy things to Backdrop's field and entity APIs, so there may be unexpected dragons laying around that I haven't discovered.

## Installation

 - Install this module using the official 
  [Backdrop CMS instructions](https://backdropcms.org/guide/modules)

## Issues

Bugs and Feature requests should be reported in the 
[Issue Queue](https://github.com/backdrop-contrib/multifield/issues)

## Current Maintainers

 - [Laryn Kragt Bakker](https://github.com/laryn) - [CEDC.org](https://cedc.org)
 - Help welcome!

## Credits

- Ported to Backdrop CMS by [Laryn Kragt Bakker](https://github.com/laryn) - [CEDC.org](https://cedc.org).
- Creator/Maintainer for Drupal: [Dave Reid](https://www.drupal.org/u/dave-reid) with support from [Lullabot](https://www.lullabot.com/).

## License

This project is GPL v2 software. See the [LICENSE.txt](https://github.com/backdrop-contrib/multifield/blob/1.x-1.x/LICENSE.txt) 
file in this directory for complete text.
