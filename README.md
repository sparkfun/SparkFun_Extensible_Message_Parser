SparkFun Extensible Message Parser
========================================

The SparkFun Extensible Message Parser provides a base set of routines to construct serial stream parsers. On top of this are several GNSS protocol parsers for NMEA, RTCM, u-blox UBX, SPARTN, Septentrio SBF and Unicore. Some of SparkFun's RTK products use these parsers. Users may add protocol parse routines to enable the base routines to parse other protocols. Examples are provided for various parse configurations.

Examples
--------

The examples provided with library are primarily for testing however they do show:
* How to initialize the parsers
* How to feed data a character at a time to the parsers
* How to use multiple parsers to parse a single data stream where no message is interrupted
* How to use multiple parsers to parse a single data stream with interrupted messages

The following libraries have examples of GNSS receivers feeding data to the parsers:
* [SparkFun LG290P Quadband RTK GNSS Arduino Library](https://github.com/sparkfun/SparkFun_LG290P_GNSS_Arduino_Library)
* [SparkFun UM980 Triband RTK GNSS Arduino Library](https://github.com/sparkfun/SparkFun_Unicore_GNSS_Arduino_Library)

License Information
-------------------

This product is _**open source**_!

Please review the LICENSE.md file for license information.

If you have any questions or concerns on licensing, please contact technical support on our [SparkFun forums](https://forum.sparkfun.com/viewforum.php?f=152).

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
