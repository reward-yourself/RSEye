# RSEye
*A simple [RSI](http://www.nhs.uk/conditions/repetitive-strain-injury/Pages/Introduction.aspx) and eyes defender*

Freeze screen `smallBreak`(seconds) every `workTime`(minutes) and `largeBreak`(minutes) on the third smallBreak. My code is inspired by [workrave](http://www.workrave.org/), [slock](http://tools.suckless.org/slock/), and [i3lock](https://i3wm.org/i3lock/). The reason for this program are that `workrave` does not work on my computer many times and that `slock` and `i3lock` do not have the timer that I want.

    Usage:
        rseye -w worktime -s smallbreak -l largebreak -o logfile
        -w  worktime    The time in minutes between consecutive breaks. Default value is 20 minutes.
        -s  smallbreak  The length in seconds of a small break. Default value is 60 seconds.
        -l  largebreak  The length in minutes of a large break. Default value is 8 minutes.
        -o  logfile     Record starting and ending time for working times and breaks. Default values is /tmp/rseye.log
## Notes:
1. If the system has suspended for longer than largebreak, `rseye` assumes that you just had a largebreak after system waking up.
2. The background color is chosen to have temperature of about 1000K. You can change this color in the `config.h`. For your reference, conversion from color temperature to RGB can be found [here](http://www.vendian.org/mncharity/dir3/blackbody/UnstableURLs/bbr_color.html).
3. During break times, there is a countdown `binary` clock in the middle of the screen. Since you are not supposed to stare at the screen during break time, I have no intension to add any decoration to the screen.
