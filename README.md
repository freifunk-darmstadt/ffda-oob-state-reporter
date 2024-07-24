# ffda-oob-state-reporter

## Description

This programm is used to receive Device-State information from GL.iNet Puli devices
used at Freifunk Darmstadt or providing means of out-of-band access.

In these devices, SIM cards from an IoT Enabler is installed. These provide VPN-like
infrastructure, where you can contact all card via fixed IP-Adresses from a OpenVPN endpoint.
This allows remote on-demand access to the devices for a cheap one-time fee for 10 years of connectivity.

The devices are attached to routers / switches with an USB-UART converter.

This daemon reports battery, charging-state and temperature i regular intervals. Additionally, updates are
always sent for the following conditions.

 - Charging state changed
 - State-of-charge changed while on battery

## Installation

Pre-compiled binaries are available on the  [Releases](https://github.com/freifunk-darmstadt/ffda-oob-state-reporter) page.

This tool requires a server-component which also be found on [GitHub](https://github.com/freifunk-darmstadt/ffda-oob-state-collector).

## License

This programm is provided under the GPLv2 license.
