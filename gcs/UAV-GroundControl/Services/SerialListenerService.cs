using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.Hosting;
using SharpDX.DirectInput;
using UAV_GroundControl.Hubs;

public class SerialListenerService : BackgroundService
{
    private readonly IHubContext<TelemetryHub> _hubContext;

    private SerialPort? _serialPort;

    private readonly object _serialPortLock = new object();

    private readonly List<byte> _serialReceiveBuffer =
        new List<byte>();


    // =========================================================
    // SERIAL
    // =========================================================

    private const string SERIAL_PORT = "COM5";
    private const int BAUD_RATE = 115200;


    // =========================================================
    // JOYSTICK PACKET
    // =========================================================

    private const int JOYSTICK_PACKET_SIZE = 8;

    private const byte JOYSTICK_HEADER_1 = 0x25;
    private const byte JOYSTICK_HEADER_2 = 0x28;


    // =========================================================
    // TELEMETRY PACKET
    // =========================================================

    private const int TELEMETRY_PACKET_SIZE = 18;

    private const byte TELEMETRY_HEADER_1 = 0x35;
    private const byte TELEMETRY_HEADER_2 = 0x48;


    // =========================================================
    // TIMING
    // =========================================================

    private const int JOYSTICK_SEND_INTERVAL = 100;

    private const int JOYSTICK_READ_INTERVAL = 20;


    // =========================================================
    // DIRECTINPUT
    // =========================================================

    private DirectInput? _directInput;

    private Joystick? _joystick;

    private bool _joystickConnected = false;


    // =========================================================
    // JOYSTICK DATA
    //
    // ESP32'ye gönderilen değerler
    //
    // Pitch     : 0 - 180
    // Roll      : 0 - 180
    // Yaw       : 0 - 180
    // Throttle  : 0 - 255
    // FlightMode: 0 - 255
    // =========================================================

    private byte _pitch = 90;

    private byte _roll = 90;

    private byte _yaw = 90;

    private byte _throttle = 0;

    private byte _flightMode = 0;


    // =========================================================
    // TELEMETRY DATA
    // =========================================================

    private byte _telemetrySatellites = 0;

    private double _telemetryLatitude = 0;

    private double _telemetryLongitude = 0;

    private bool _isConnected = false;

    private double _imuPitch = 0.0;

    private double _imuRoll = 0.0;


    // =========================================================
    // CONSTRUCTOR
    // =========================================================

    public SerialListenerService(
        IHubContext<TelemetryHub> hubContext)
    {
        _hubContext = hubContext;
    }


    // =========================================================
    // EXECUTE
    // =========================================================

    protected override async Task ExecuteAsync(
        CancellationToken stoppingToken)
    {
        TryOpenSerialPort();

        InitializeJoystick();


        var serialTask =
            SerialReadLoop(stoppingToken);

        var joystickReadTask =
            JoystickReadLoop(stoppingToken);

        var joystickSendTask =
            JoystickSendLoop(stoppingToken);

        var telemetryTask =
            TelemetryBroadcastLoop(stoppingToken);

        var watchdogTask =
            ReconnectWatchdogLoop(stoppingToken);


        try
        {
            await Task.WhenAll(
                serialTask,
                joystickReadTask,
                joystickSendTask,
                telemetryTask,
                watchdogTask
            );
        }
        catch (OperationCanceledException)
        {
        }
    }


    // =========================================================
    // SERIAL PORT
    // =========================================================

    private void TryOpenSerialPort()
    {
        lock (_serialPortLock)
        {
            try
            {
                if (_serialPort != null &&
                    _serialPort.IsOpen)
                {
                    return;
                }


                _serialPort?.Dispose();


                _serialPort = new SerialPort(
                    SERIAL_PORT,
                    BAUD_RATE,
                    Parity.None,
                    8,
                    StopBits.One
                );


                _serialPort.ReadTimeout = 200;

                _serialPort.WriteTimeout = 200;

                _serialPort.DtrEnable = true;

                _serialPort.RtsEnable = true;


                _serialPort.Open();


                _isConnected = true;


                Console.WriteLine(
                    $"[SERIAL] Connected: " +
                    $"{SERIAL_PORT} @ {BAUD_RATE}"
                );
            }
            catch (Exception ex)
            {
                _isConnected = false;


                Console.WriteLine(
                    $"[SERIAL] Connection failed: " +
                    $"{ex.Message}"
                );
            }
        }
    }


    // =========================================================
    // JOYSTICK INITIALIZE
    // =========================================================

    private void InitializeJoystick()
    {
        try
        {
            _directInput?.Dispose();

            _directInput =
                new DirectInput();


            var devices =
                _directInput.GetDevices(
                    DeviceType.Gamepad,
                    DeviceEnumerationFlags.AttachedOnly
                );


            if (devices.Count == 0)
            {
                devices =
                    _directInput.GetDevices(
                        DeviceType.Joystick,
                        DeviceEnumerationFlags.AttachedOnly
                    );
            }


            if (devices.Count == 0)
            {
                _joystickConnected = false;
                return;
            }


            var device =
                devices[0];


            Console.WriteLine(
                $"[JOYSTICK] Device: " +
                $"{device.ProductName}"
            );


            _joystick =
                new Joystick(
                    _directInput,
                    device.InstanceGuid
                );


            _joystick.Properties.BufferSize = 128;

            _joystick.Acquire();

            _joystickConnected = true;


            Console.WriteLine(
                "[JOYSTICK] Connected."
            );
        }
        catch
        {
            _joystickConnected = false;
        }
    }


    // =========================================================
    // JOYSTICK READ LOOP
    // =========================================================

    private async Task JoystickReadLoop(
        CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                if (!_joystickConnected)
                {
                    TryReinitializeJoystick();
                }
                else
                {
                    ReadJoystick();
                }
            }
            catch
            {
                _joystickConnected = false;

                SetJoystickData(
                    90,
                    90,
                    90,
                    0,
                    _flightMode
                );
            }


            await Task.Delay(
                JOYSTICK_READ_INTERVAL,
                stoppingToken
            );
        }
    }


    // =========================================================
    // JOYSTICK READ
    // =========================================================

    private void ReadJoystick()
    {
        if (_joystick == null)
        {
            _joystickConnected = false;
            return;
        }


        try
        {
            _joystick.Poll();


            var state =
                _joystick.GetCurrentState();


            if (state == null)
            {
                throw new Exception();
            }


            int rawX = state.X;

            int rawY = state.Y;

            int rawZ = state.Z;

            int rawSlider0 =
                state.Sliders.Length > 0
                    ? state.Sliders[0]
                    : 0;


            byte roll =
                AxisTo180(rawX);


            byte pitch =
                AxisTo180(rawY);


            byte yaw =
                AxisTo180(rawZ);


            byte throttle =
                (byte)(
                    255 -
                    AxisTo255(rawSlider0)
                );


            SetJoystickData(
                pitch,
                roll,
                yaw,
                throttle,
                _flightMode
            );


            _joystickConnected = true;
        }
        catch
        {
            _joystickConnected = false;


            // Joystick bağlantısı kesildiğinde
            // güvenli değerler.

            SetJoystickData(
                90,
                90,
                90,
                0,
                _flightMode
            );


            TryReinitializeJoystick();
        }
    }


    // =========================================================
    // AXIS → 0-180
    // =========================================================

    private static byte AxisTo180(int value)
    {
        value =
            Math.Clamp(
                value,
                0,
                65535
            );


        double normalized =
            value / 65535.0;


        double result =
            normalized * 180.0;


        return (byte)
            Math.Clamp(
                (int)Math.Round(result),
                0,
                180
            );
    }


    // =========================================================
    // AXIS → 0-255
    // =========================================================

    private static byte AxisTo255(int value)
    {
        value =
            Math.Clamp(
                value,
                0,
                65535
            );


        double normalized =
            value / 65535.0;


        double result =
            normalized * 255.0;


        return (byte)
            Math.Clamp(
                (int)Math.Round(result),
                0,
                255
            );
    }


    // =========================================================
    // JOYSTICK REINITIALIZE
    // =========================================================

    private void TryReinitializeJoystick()
    {
        try
        {
            _joystick?.Unacquire();

            _joystick?.Dispose();

            _joystick = null;


            _joystickConnected = false;


            InitializeJoystick();
        }
        catch
        {
            _joystickConnected = false;
        }
    }


    // =========================================================
    // SERIAL READ LOOP
    // =========================================================

    private async Task SerialReadLoop(
        CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                lock (_serialPortLock)
                {
                    if (_serialPort != null &&
                        _serialPort.IsOpen)
                    {
                        int bytesToRead =
                            _serialPort.BytesToRead;


                        if (bytesToRead > 0)
                        {
                            byte[] buffer =
                                new byte[bytesToRead];


                            int read =
                                _serialPort.Read(
                                    buffer,
                                    0,
                                    buffer.Length
                                );


                            for (
                                int i = 0;
                                i < read;
                                i++)
                            {
                                _serialReceiveBuffer.Add(
                                    buffer[i]
                                );
                            }


                            ParseTelemetryBuffer();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                _isConnected = false;


                Console.WriteLine(
                    $"[SERIAL RX ERROR] " +
                    $"{ex.Message}"
                );
            }


            await Task.Delay(
                1,
                stoppingToken
            );
        }
    }


    // =========================================================
    // TELEMETRY BUFFER PARSER
    // =========================================================

    private void ParseTelemetryBuffer()
    {
        while (
            _serialReceiveBuffer.Count >=
            TELEMETRY_PACKET_SIZE)
        {
            int headerIndex = -1;


            for (
                int i = 0;
                i <=
                _serialReceiveBuffer.Count - 2;
                i++)
            {
                if (
                    _serialReceiveBuffer[i] ==
                        TELEMETRY_HEADER_1 &&
                    _serialReceiveBuffer[i + 1] ==
                        TELEMETRY_HEADER_2)
                {
                    headerIndex = i;
                    break;
                }
            }


            if (headerIndex < 0)
            {
                _serialReceiveBuffer.Clear();
                return;
            }


            if (headerIndex > 0)
            {
                _serialReceiveBuffer.RemoveRange(
                    0,
                    headerIndex
                );
            }


            if (
                _serialReceiveBuffer.Count <
                TELEMETRY_PACKET_SIZE)
            {
                return;
            }


            byte[] packet =
                _serialReceiveBuffer
                    .Take(TELEMETRY_PACKET_SIZE)
                    .ToArray();


            byte checksum = 0;


            for (int i = 2; i <= 16; i++)
            {
                checksum ^= packet[i];
            }


            if (checksum != packet[17])
            {
                Console.WriteLine(
                    "[TELEMETRY] CHECKSUM ERROR"
                );


                _serialReceiveBuffer.RemoveAt(0);

                continue;
            }


            ParseTelemetryPacket(packet);


            _serialReceiveBuffer.RemoveRange(
                0,
                TELEMETRY_PACKET_SIZE
            );
        }
    }


    // =========================================================
    // TELEMETRY PARSE
    // =========================================================

    private void ParseTelemetryPacket(
        byte[] packet)
    {
        if (
            packet.Length !=
            TELEMETRY_PACKET_SIZE)
        {
            return;
        }


        _telemetrySatellites =
            packet[2];


        int latitudeRaw =
            BitConverter.ToInt32(
                packet,
                3
            );


        _telemetryLatitude =
            latitudeRaw / 1000000.0;


        int longitudeRaw =
            BitConverter.ToInt32(
                packet,
                7
            );


        _telemetryLongitude =
            longitudeRaw / 1000000.0;


        short pitchRaw =
            BitConverter.ToInt16(
                packet,
                11
            );


        _imuPitch =
            pitchRaw / 100.0;


        short rollRaw =
            BitConverter.ToInt16(
                packet,
                13
            );


        _imuRoll =
            rollRaw / 100.0;
    }


    // =========================================================
    // JOYSTICK SEND LOOP
    // =========================================================

    private async Task JoystickSendLoop(
        CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                SendJoystickPacket();
            }
            catch (Exception ex)
            {
                Console.WriteLine(
                    $"[JOYSTICK SEND ERROR] " +
                    $"{ex.Message}"
                );
            }


            await Task.Delay(
                JOYSTICK_SEND_INTERVAL,
                stoppingToken
            );
        }
    }


    // =========================================================
    // JOYSTICK SEND
    // =========================================================

    private void SendJoystickPacket()
    {
        // Joystick yoksa paket gönderme.
        // Böylece ESP32 failsafe'e düşer.

        if (!_joystickConnected)
        {
            return;
        }


        byte[] packet =
            new byte[JOYSTICK_PACKET_SIZE];


        packet[0] =
            JOYSTICK_HEADER_1;


        packet[1] =
            JOYSTICK_HEADER_2;


        packet[2] =
            _pitch;


        packet[3] =
            _roll;


        packet[4] =
            _yaw;


        packet[5] =
            _throttle;


        packet[6] =
            _flightMode;


        packet[7] =
            (byte)(
                packet[2] ^
                packet[3] ^
                packet[4] ^
                packet[5] ^
                packet[6]
            );


        lock (_serialPortLock)
        {
            if (
                _serialPort == null ||
                !_serialPort.IsOpen)
            {
                return;
            }


            _serialPort.Write(
                packet,
                0,
                packet.Length
            );
        }
    }


    // =========================================================
    // TELEMETRY BROADCAST
    // =========================================================

    private async Task TelemetryBroadcastLoop(
        CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                await _hubContext.Clients.All.SendAsync(
                    "TelemetryUpdate",
                    new
                    {
                        IsConnected =
                            _isConnected,

                        JoystickConnected =
                            _joystickConnected,

                        Satellites =
                            _telemetrySatellites,

                        Latitude =
                            _telemetryLatitude,

                        Longitude =
                            _telemetryLongitude,

                        ImuPitch =
                            _imuPitch,

                        ImuRoll =
                            _imuRoll,

                        JoystickPitch =
                            _pitch,

                        JoystickRoll =
                            _roll,

                        JoystickYaw =
                            _yaw,

                        JoystickThrottle =
                            _throttle,

                        FlightMode =
                            _flightMode,

                        PacketRssi = 0
                    },
                    stoppingToken
                );
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (Exception ex)
            {
                Console.WriteLine(
                    $"[SIGNALR ERROR] " +
                    $"{ex.Message}"
                );
            }


            await Task.Delay(
                50,
                stoppingToken
            );
        }
    }


    // =========================================================
    // SERIAL RECONNECT WATCHDOG
    // =========================================================

    private async Task ReconnectWatchdogLoop(
        CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            try
            {
                bool connected;


                lock (_serialPortLock)
                {
                    connected =
                        _serialPort != null &&
                        _serialPort.IsOpen;
                }


                if (!connected)
                {
                    TryOpenSerialPort();
                }
            }
            catch
            {
            }


            await Task.Delay(
                500,
                stoppingToken
            );
        }
    }


    // =========================================================
    // CALIBRATE IMU
    // =========================================================

    public void CalibrateImu()
    {
        byte[] packet =
        {
            JOYSTICK_HEADER_1,
            JOYSTICK_HEADER_2,

            0,
            0,
            0,
            0,

            0xFA,

            0xFA
        };


        lock (_serialPortLock)
        {
            if (
                _serialPort == null ||
                !_serialPort.IsOpen)
            {
                Console.WriteLine(
                    "[IMU] Serial not connected."
                );

                return;
            }


            _serialPort.Write(
                packet,
                0,
                packet.Length
            );
        }


        Console.WriteLine(
            "[IMU] CALIBRATE command sent."
        );
    }


    // =========================================================
    // JOYSTICK SET
    // =========================================================

    public void SetJoystickData(
        byte pitch,
        byte roll,
        byte yaw,
        byte throttle,
        byte flightMode)
    {
        _pitch = pitch;

        _roll = roll;

        _yaw = yaw;

        _throttle = throttle;

        _flightMode = flightMode;
    }


    // =========================================================
    // GETTERS
    // =========================================================

    public byte GetPitch()
        => _pitch;


    public byte GetRoll()
        => _roll;


    public byte GetYaw()
        => _yaw;


    public byte GetThrottle()
        => _throttle;


    public byte GetFlightMode()
        => _flightMode;


    // =========================================================
    // DISPOSE
    // =========================================================

    public override void Dispose()
    {
        try
        {
            _joystick?.Unacquire();
        }
        catch
        {
        }


        try
        {
            _joystick?.Dispose();

            _directInput?.Dispose();
        }
        catch
        {
        }


        lock (_serialPortLock)
        {
            try
            {
                if (
                    _serialPort != null &&
                    _serialPort.IsOpen)
                {
                    _serialPort.Close();
                }
            }
            catch
            {
            }


            _serialPort?.Dispose();

            _serialPort = null;
        }


        base.Dispose();
    }
}