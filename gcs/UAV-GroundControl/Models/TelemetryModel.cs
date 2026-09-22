namespace UAV_GroundControl.Models
{
    public class TelemetryModel
    {
        // Tutum (Attitude) Verileri
        public int Pitch { get; set; }
        public int Roll { get; set; }
        public int Yaw { get; set; }
        public int Throttle { get; set; }
        public int FlightMode { get; set; }

        // Sinyal ve Güç Verileri
        public int UavDbm { get; set; }
        public int GcsDbm { get; set; }
        public double Battery { get; set; }

        // GPS Verileri (TBS M10Q)
        public double Latitude { get; set; }
        public double Longitude { get; set; }
        public double Altitude { get; set; }
        public double Speed { get; set; }
        public int Satellites { get; set; }

        // Durum Verileri
        public bool IsConnected { get; set; }
        public DateTime LastSeen { get; set; } = DateTime.Now;
    }
}
