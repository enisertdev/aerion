using Microsoft.AspNetCore.SignalR;

namespace UAV_GroundControl.Hubs
{
    public class TelemetryHub : Hub
    {
        private readonly SerialListenerService _serialService;

        public TelemetryHub(SerialListenerService serialService)
        {
            _serialService = serialService;
        }

        // Tarayıcı bağlandığında
        public override async Task OnConnectedAsync()
        {
            await base.OnConnectedAsync();
        }

        // =========================================================
        // IMU LEVEL CALIBRATION
        // =========================================================

        public void CalibrateImu()
        {
            _serialService.CalibrateImu();
        }
    }
}