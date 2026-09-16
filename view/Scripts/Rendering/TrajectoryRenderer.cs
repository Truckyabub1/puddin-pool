using System;
using System.Collections.Generic;
using PuddinPool.Native;

namespace PuddinPool.Rendering
{
    public struct TrajectoryLinePoint
    {
        public float x;
        public float y;
        public float alpha; // Dotted line alpha falloff
    }

    public class TrajectoryRenderer
    {
        public List<TrajectoryLinePoint> CueLinePoints { get; private set; }
        public List<TrajectoryLinePoint> TargetLinePoints { get; private set; }
        public bool HasTargetHit { get; private set; }
        public int TargetBallId { get; private set; }

        public TrajectoryRenderer()
        {
            CueLinePoints = new List<TrajectoryLinePoint>();
            TargetLinePoints = new List<TrajectoryLinePoint>();
        }

        public void UpdateTrajectory(TrajectoryBuffer buffer)
        {
            CueLinePoints.Clear();
            TargetLinePoints.Clear();

            TargetBallId = buffer.target_ball_id;
            HasTargetHit = buffer.target_ball_id > 0;

            // 1. Cue Ball preview line
            if (buffer.cue_points != null && buffer.cue_count > 0)
            {
                for (int i = 0; i < buffer.cue_count; ++i)
                {
                    float t = (float)i / Math.Max(1, buffer.cue_count - 1);
                    float alpha = 1.0f - 0.4f * t; // Subtle distance fade
                    CueLinePoints.Add(new TrajectoryLinePoint
                    {
                        x = buffer.cue_points[i].x,
                        y = buffer.cue_points[i].y,
                        alpha = alpha
                    });
                }
            }

            // 2. Target Ball preview line
            if (buffer.target_points != null && buffer.target_count > 0)
            {
                for (int i = 0; i < buffer.target_count; ++i)
                {
                    float t = (float)i / Math.Max(1, buffer.target_count - 1);
                    float alpha = 0.9f - 0.5f * t;
                    TargetLinePoints.Add(new TrajectoryLinePoint
                    {
                        x = buffer.target_points[i].x,
                        y = buffer.target_points[i].y,
                        alpha = alpha
                    });
                }
            }
        }
    }
}
