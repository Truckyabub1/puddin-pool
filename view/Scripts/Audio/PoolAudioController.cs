using System;

namespace PuddinPool.Audio
{
    public struct AudioPlaybackCommand
    {
        public string ClipName;
        public float Volume;
        public float Pitch;
        public float LowPassCutoffHz;
    }

    public class PoolAudioController
    {
        public Action<AudioPlaybackCommand> OnAudioPlayed; // Hook for audio output / tests

        public float MasterVolume { get; set; } = 1.0f;
        public float RollingLoopVolume { get; private set; } = 0.0f;

        // 1. Procedural Ball-to-Ball Impact Sound
        // Volume and pitch modulated proportional to collision impulse force J = dv * m
        public void OnBallBallCollision(float impulse)
        {
            const float maxImpulse = 1.5f;
            float normImpulse = Math.Min(1.0f, Math.Max(0.05f, impulse / maxImpulse));

            // Volume scales with impulse
            float volume = normImpulse * MasterVolume;

            // Pitch shifts slightly higher on harder hits
            float pitch = 0.90f + 0.25f * normImpulse;

            OnAudioPlayed?.Invoke(new AudioPlaybackCommand
            {
                ClipName = "ball_impact",
                Volume = volume,
                Pitch = pitch,
                LowPassCutoffHz = 22000.0f
            });
        }

        // 2. Cushion Impact Sound
        // Low-pass muffled thud matching angle and velocity
        public void OnCushionImpact(float normalVelocity, float incidentAngleRad)
        {
            float speed = Math.Abs(normalVelocity);
            float volume = Math.Min(1.0f, Math.Max(0.05f, speed / 2.5f)) * MasterVolume;

            // Steeper angles and lower speeds have stronger low-pass muffling
            float lowPassHz = 1500.0f + 4000.0f * (float)Math.Sin(Math.Abs(incidentAngleRad));

            OnAudioPlayed?.Invoke(new AudioPlaybackCommand
            {
                ClipName = "cushion_thud",
                Volume = volume,
                Pitch = 0.85f + 0.15f * Math.Min(1.0f, speed / 3.0f),
                LowPassCutoffHz = lowPassHz
            });
        }

        // 3. Ball Rolling Layer (subtle looping audio tied to aggregate linear velocity)
        public void UpdateRollingLayer(float aggregateSpeed)
        {
            const float maxSpeed = 4.0f;
            float normSpeed = Math.Min(1.0f, Math.Max(0.0f, aggregateSpeed / maxSpeed));
            RollingLoopVolume = normSpeed * 0.35f * MasterVolume;
        }

        // 4. Pocket Drop Sound (distinct hollow drop sound on ball sink events)
        public void OnBallPocketed(int ballId)
        {
            OnAudioPlayed?.Invoke(new AudioPlaybackCommand
            {
                ClipName = "pocket_drop",
                Volume = 0.9f * MasterVolume,
                Pitch = 1.0f,
                LowPassCutoffHz = 12000.0f
            });
        }
    }
}
