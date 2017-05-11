# RSEye
*A simple RSI and eyes defender*

Freeze screen `smallBreak`(seconds) every `workTime`(minutes) and `largeBreak`(minutes) on the third smallBreak. My code is inspired by [workrave](http://www.workrave.org/), [slock](http://tools.suckless.org/slock/), and [i3lock](https://i3wm.org/i3lock/). Just like `slock`, you can change the default values in the `config.h` file and recompile.

    Usage:
        rseye -k -w worktime -s smallbreak -l largebreak -o logfile
        -k    
            Kill all running instances of rseye or abort. This option cannot be combined with any other options.
        -w  worktime
            The time in minutes between consecutive breaks. Default value is 20 minutes.
        -s  smallbreak
            The length in seconds of a small break. Default value is 60 seconds.
        -l  largebreak
            The length in minutes of a large break. Default value is 8 minutes.
        -o  logfile
            Record starting and ending time for working times and breaks. There is no default value.
