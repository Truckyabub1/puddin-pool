using System;
using System.Runtime.InteropServices;

namespace PuddinPool.Native
{
    public static class PuddinWisdomBridge
    {
        private const string LibName = "libpuddinpool";

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr get_coach_puddin_advice(
            int context_type,
            float cut_angle_deg,
            float distance,
            [MarshalAs(UnmanagedType.I1)] bool was_scratch
        );

        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int evaluate_coach_puddin_emote(
            int context_type,
            int cushions_hit,
            float cut_angle_deg,
            [MarshalAs(UnmanagedType.I1)] bool was_scratch,
            [MarshalAs(UnmanagedType.I1)] bool ball_pocketed
        );

        public static string GetShotAdvice(int contextType, float cutAngleDeg, float distance, bool wasScratch)
        {
            IntPtr ptr = get_coach_puddin_advice(contextType, cutAngleDeg, distance, wasScratch);
            if (ptr != IntPtr.Zero)
            {
                return Marshal.PtrToStringAnsi(ptr);
            }
            return "Smooth stroke, quiet mind.";
        }
    }
}
