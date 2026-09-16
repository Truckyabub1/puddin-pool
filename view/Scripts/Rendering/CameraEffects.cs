using System;

namespace PuddinPool.Rendering
{
    public class CameraEffects
    {
        public float Trauma { get; private set; } = 0.0f;
        public float ShakeDecay { get; set; } = 2.5f; // Per second decay
        public float MaxShakeOffset { get; set; } = 0.025f; // In meters

        public float CurrentOffsetX { get; private set; } = 0.0f;
        public float CurrentOffsetY { get; private set; } = 0.0f;

        private float _timeAccumulator = 0.0f;

        // Trigger micro-shake on break shot scaled by power parameter
        public void OnBreakStrike(float power)
        {
            const float breakThreshold = 2.0f;
            if (power > breakThreshold)
            {
                float addTrauma = Math.Min(1.0f, (power - breakThreshold) / 3.0f);
                Trauma = Math.Min(1.0f, Trauma + addTrauma);
            }
        }

        public void Update(float deltaTime)
        {
            if (Trauma <= 0.0f)
            {
                CurrentOffsetX = 0.0f;
                CurrentOffsetY = 0.0f;
                return;
            }

            _timeAccumulator += deltaTime;

            // Non-linear shake intensity: Shake = Trauma^2
            float shake = Trauma * Trauma * MaxShakeOffset;

            // Pseudo-random harmonic jitter
            CurrentOffsetX = (float)Math.Sin(_timeAccumulator * 45.0f) * shake;
            CurrentOffsetY = (float)Math.Cos(_timeAccumulator * 37.0f) * shake;

            Trauma = Math.Max(0.0f, Trauma - ShakeDecay * deltaTime);
        }
    }
}
