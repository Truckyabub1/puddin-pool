using System;
using PuddinPool.Native;

namespace PuddinPool.Input
{
    public class InputController
    {
        public float AimAngleRad { get; set; } = 0.0f;
        public float CoarseSensitivity { get; set; } = 0.005f; // Radians per pixel
        public const float FineWheelUnitDegrees = 0.05f;       // 0.05 degrees per touch unit

        public float PullTension { get; private set; } = 0.0f; // [0.0, 1.0]
        public float MaxImpulse { get; set; } = 5.0f;           // Max strike power

        public float SpinX { get; private set; } = 0.0f;       // [-1.0, 1.0] English
        public float SpinY { get; private set; } = 0.0f;       // [-1.0, 1.0] Follow/Draw

        private int _lastHapticTensionTier = 0;

        public Action<float, float, float, float> OnStrikeExecuted; // (angle, power, spinX, spinY)

        // 1. Coarse Aim Pan
        public void OnCoarseAimPan(float deltaPixelsX)
        {
            AimAngleRad += deltaPixelsX * CoarseSensitivity;
            NormalizeAngle();
        }

        // 2. Fine Aim Wheel
        public void OnFineAimWheel(float deltaUnits)
        {
            float deltaRad = deltaUnits * (FineWheelUnitDegrees * (float)Math.PI / 180.0f);
            AimAngleRad += deltaRad;
            NormalizeAngle();
        }

        // 3. Spin HUD Selection
        public void SetSpin(float x, float y)
        {
            float dist = (float)Math.Sqrt(x * x + y * y);
            if (dist > 1.0f)
            {
                SpinX = x / dist;
                SpinY = y / dist;
            }
            else
            {
                SpinX = x;
                SpinY = y;
            }
        }

        // 4. Cue Stick Slider Pull
        public void OnCuePull(float normalizedTension)
        {
            PullTension = Math.Max(0.0f, Math.Min(1.0f, normalizedTension));

            // Fire a haptic tick on every 10% tension threshold
            int currentTier = (int)(PullTension * 10.0f);
            if (currentTier > _lastHapticTensionTier)
            {
                for (int t = _lastHapticTensionTier + 1; t <= currentTier; ++t)
                {
                    HapticFeedback.TriggerTick();
                }
            }
            _lastHapticTensionTier = currentTier;
        }

        // 5. Cue Stick Slider Release -> Execute Strike
        public void OnCueRelease()
        {
            if (PullTension > 0.02f)
            {
                float power = PullTension * MaxImpulse;
                PuddinPoolBridge.execute_strike(AimAngleRad, power, SpinX, SpinY);
                OnStrikeExecuted?.Invoke(AimAngleRad, power, SpinX, SpinY);
            }

            PullTension = 0.0f;
            _lastHapticTensionTier = 0;
        }

        private void NormalizeAngle()
        {
            const float twoPi = (float)(2.0 * Math.PI);
            while (AimAngleRad < 0.0f) AimAngleRad += twoPi;
            while (AimAngleRad >= twoPi) AimAngleRad -= twoPi;
        }
    }
}
