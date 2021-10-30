# AirGradient Air Sensor featuring a webserver

This is code for the Air Sensor built by following
[these wonderful instructions](https://www.airgradient.com/diy/).

I've modified the code a bit to make it more useful to me. Instead of pushing
the sensor data to a server, it allows other entities to query it when the data
is needed.

## The web server

There are three endpoints `/`, `/api` and `/metrics`. All of them are running
on port 80.

### `/` endpoint

Visiting this endpoint in the browser displays a nice table containing the
latest sensor data.

### `/api` endpoint

Querying this endpoint returns latest JSON formatted sensor data.

```
$ http <air sensor IP or hostname>/api
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 65
Content-Type: application/json
Keep-Alive: timeout=2000

{
    "co2": 803,
    "humidity": 36,
    "pm2": 32,
    "temperature": 25.7,
    "wifi": -80
}
```

### `/metrics` endpoint

This is a
[Prometheus metrics compatible endpoint](https://prometheus.io/docs/instrumenting/exposition_formats/).

```
$ http <air sensor IP or hostname>/metrics
HTTP/1.1 200 OK
Connection: keep-alive
Content-Length: 348
Content-Type: text/plain;charset=UTF-8
Keep-Alive: timeout=2000

# HELP wifi Current WiFi signal strength, in dB
# TYPE wifi gauge
wifi -82

# HELP pm2 Particulate Matter PM2.5 value, in μg/m3
# TYPE pm2 gauge
pm2 32

# HELP co2 CO2 value, in ppm
# TYPE co2 gauge
co2 811

# HELP temp Temperature, in degrees Celsius
# TYPE temp gauge
temp 25.70

# HELP hum Relative humidity, in percent
# TYPE hum gauge
hum 37
```

## Setup

### Connecting to the network

When you flash the software for the first time, a WiFi hotspot will be created.
Connect to it and specify the WiFi details that the sensors should use. After
that it should connect to the specified WiFi and the hotspot should not be
created anymore. This will be persisted across reboots.

### Get the device IP or Hostname

In order to query the AirSensor you will need to know its IP or hostname. This
can be done by logging onto your router and inspecting the DHCP leases. You
should then add a mapping for the device MAC and assign it a static IP and/or a
friendly hostname.

## To-do:

- Display the IP or hostname on the screen after WiFi connection is made
- Add `hostname` input to the WiFi setup screen

