# RSEye
*A simple [RSI](http://www.nhs.uk/conditions/repetitive-strain-injury/Pages/Introduction.aspx) and eyes defender*

[NAME](#NAME)  
[SYNOPSIS](#SYNOPSIS)  
[DESCRIPTION](#DESCRIPTION)  
[FILES](#FILES)  
[OPTIONS](#OPTIONS)  
[COMMANDS](#COMMANDS)  
[CUSTOMIZATION](#CUSTOMIZATION)  
[AUTHOR](#AUTHOR)  

* * *

## NAME<a name="NAME"></a>

**rseye** − help prevent RSI and protect your eyes

## SYNOPSIS<a name="SYNOPSIS"></a>

**rseye** [**-w** _worktime_] [**-s** _smallbreak_] [**-l** _largebreak_] [**-o** _logfile_] [**-m** _maxworktime_]

## DESCRIPTION<a name="DESCRIPTION"></a>

**rseye** can hopefully prevent RSI and protect your eyes.

**rseye** is inspired by [_workrave_](http://www.workrave.org/), [_slock_](http://tools.suckless.org/slock/), and [_i3lock_](https://i3wm.org/i3lock/). _Workrave_ does not work on my Linux box many times, and _i3lock_ and _slock_ does not have the timer that I want.

**rseye** freezes screen _smallbreak_ (seconds) every _worktime_ (minutes) and _largebreak_ (minutes) on the _n-th smallbreak_ such that _n*worktime <= maxworktime < (n+1)*worktime._ Here, _maxworktime_ is the maximum total _worktime_ between two consecutive _largebreaks._

If the system has suspended for longer than _largebreak_, **rseye** assumes that you just had a large break after system waking up.

Only _one_ instance of **rseye** can run at a time. When you start **rseye** if there is an instance of **rseye** already running, you have the following three options (_requiring keyboard input_, _4 second timeout_):


| Key Pressed | Action |
| --- | --- |
| | (_default_) kill the already running process and continue with the current process |
| **k** | kill the current process only |
| **a** | kill both the already running and the current processes |


## FILES<a name="FILES"></a>

_/etc/rseyerc.sample_

* Sample config file.

_$HOME/.rseyerc_

* This file is _optinal_. If it exists, **rseye** will load it at start, but values from this config file will be overrode by command line options. However, you can reload config file during breaks.

## OPTIONS<a name="OPTIONS"></a>

**-w** _worktime_

* This is the length (in minutes) of your working period. It will be rounded up to the nearest multiple of 5\. If _worktime_ is larger than 30, it will be set to 30\. Default value is 20 minutes.

**-s** _smallbreak_

* This is the length (in seconds) of a small break between working periods. If it is larger than 120, it will be set to 120\. If it is smaller than 20, it will be set to 20\. Default value is 60 seconds.

**-l** _largebreak_

* This is the length (in minutes) of a large break. If it is smaller than 5, it will be set to 5\. Default value is 8 minutes

**-o** _logfile_

* This file logs starting and ending times for work periods and breaks. Default value is `/tmp/rseye.log`. If everything fails, it falls back to `stderr`.

**-m** _maxworktime_

* This is the allowable maximum value (in minutes) for the accumulative sum of worktime between two consecutive largebreaks. If it is larger than 180 (which is 3 hours), it will be set to 180\. If it is smaller than `min(worktime, 60)`, it is set to `min(worktime, 60)`. Default value is 60 minutes.

## COMMANDS<a name="COMMANDS"></a>

During a break, you can do the followings:


| Key Pressed | Action |
| --- | --- |
| **Esc** | end the current break (_only after 20 seconds_) |
| **q** | end the current break (_only after 20 seconds_) |
| **r** | reload config file |

## CUSTOMIZATION<a name="CUSTOMIZATION"></a>

**rseye** can be customized by modifying `config.h` and (re)compiling the source code.

The background color is chosen to have temperature of about 1000K. You can change this color in `config.h`. For your reference, conversion from color temperature to RGB can be found [here](http://www.vendian.org/mncharity/dir3/blackbody/UnstableURLs/bbr_color.html).

## AUTHOR<a name="AUTHOR"></a>

Hoang-Ngan Nguyen

* * *
