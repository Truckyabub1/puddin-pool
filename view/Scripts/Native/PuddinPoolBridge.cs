using System;
using System.Runtime.InteropServices;

namespace PuddinPool.Native
{
    [StructLayout(LayoutKind.Sequential)]
    public struct NativeVector2
    {
        public float x;
        public float y;

        public NativeVector2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public override string ToString() => $"({x:F3}, {y:F3})";
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct BallTransform
    {
        public int id;
        public float x;
        public float y;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
        public float[] rot_quaternion; // [x, y, z, w]

        [MarshalAs(UnmanagedType.I1)]
        public bool is_pocketed;

        public override string ToString() =>
            $"Ball #{id}: pos=({x:F3}, {y:F3}), rot=[{rot_quaternion?[0]:F2}, {rot_quaternion?[1]:F2}, {rot_quaternion?[2]:F2}, {rot_quaternion?[3]:F2}], sunk={is_pocketed}";
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TrajectoryBuffer
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public NativeVector2[] cue_points;
        public int cue_count;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16)]
        public NativeVector2[] target_points;
        public int target_count;

        public int target_ball_id;

        public static TrajectoryBuffer Create()
        {
            return new TrajectoryBuffer
            {
                cue_points = new NativeVector2[16],
                cue_count = 0,
                target_points = new NativeVector2[16],
                target_count = 0,
                target_ball_id = 0
            };
        }
    }

    public static class PuddinPoolBridge
    {
        private const string LibName = "libpuddinpool";

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void init_table(float width, float height);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void reset_rack(int game_mode);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void get_aim_trajectory(
            float cue_x,
            float cue_y,
            float angle_rad,
            float power,
            float spin_x,
            float spin_y,
            ref TrajectoryBuffer out_buffer
        );

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void execute_strike(float angle_rad, float power, float spin_x, float spin_y);

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool step_simulation(
            float dt,
            [In, Out] BallTransform[] out_transforms,
            int max_balls,
            out int out_active_count
        );

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool is_simulation_at_rest();
    }
}
