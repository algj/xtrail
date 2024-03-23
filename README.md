# xtrail

xtrail is a lightweight program designed to render trails behind the mouse cursor on X11-based systems. Unlike similar tools, xtrail doesn't require a compositor and utilizes dithering techniques for rendering with a minimalistic 1-bit "transparency" depth.

## Features

- Renders trails behind the mouse cursor with multiple render types.
- Supports customizable trail length and thickness.
- Adjustable mouse position history count.
- Configurable refresh rates for both rendering and mouse tracking.

## Installation

To build xtrail, simply use the provided Makefile by running:
```
make
```

This will generate a binary named `xtrail`.

## Usage

Once built, you can run xtrail with various options:

```
Usage: xtrail [OPTIONS]
Options:
--help Display this help message
--trail-length <length> Set trail length to <length>
--trail-thickness <px> Set trail thickness to <px>
--color <hex> Set color to <hex> (e.g. 0x7F7F7F)
--mouse-hcount <count> Set position history count to <count>
--refresh-rate <fps> Set refresh rate count to <fps>
--mouse-refresh-rate <hz> Set mouse refresh rate count to <hz> (e.g. 240.00)
--no-dither Disable dithering
--mouse-smooth-factor Enable synchronous rendering and mouse pooling
Render type options:
--trail Render "trail" type
--dots Render "dots" type
```

## License

xtrail is licensed under the BSD 2-Clause License. See the `LICENSE` file for more details.

## Disclaimer

This project is provided as-is and may not receive regular updates or extensive support. Use it at your own discretion. Expect high CPU usage when running this.

## Seeking for a job

Looking for work. Let's chat!