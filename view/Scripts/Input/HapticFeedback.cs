using System;
using System.Runtime.InteropServices;

namespace PuddinPool.Input
{
    public static class HapticFeedback
    {
        public static Action<string> OnHapticEvent; // Test / debug hook

#if UNITY_IOS && !UNITY_EDITOR
        [DllImport("__Internal")]
        private static extern void _triggerSelectionFeedback();

        [DllImport("__Internal")]
        private static extern void _triggerImpactFeedback(int style);
#endif

        public static void TriggerTick()
        {
            OnHapticEvent?.Invoke("TICK");

#if UNITY_IOS && !UNITY_EDITOR
            _triggerSelectionFeedback();
#elif UNITY_ANDROID && !UNITY_EDITOR
            try
            {
                using (var unityPlayer = new UnityEngine.AndroidJavaClass("com.unity3d.player.UnityPlayer"))
                using (var currentActivity = unityPlayer.GetStatic<UnityEngine.AndroidJavaObject>("currentActivity"))
                using (var vibrator = currentActivity.Call<UnityEngine.AndroidJavaObject>("getSystemService", "vibrator"))
                using (var vibrationEffect = new UnityEngine.AndroidJavaClass("android.os.VibrationEffect"))
                {
                    int effectTick = vibrationEffect.GetStatic<int>("EFFECT_TICK");
                    using (var effect = vibrationEffect.CallStatic<UnityEngine.AndroidJavaObject>("createPredefined", effectTick))
                    {
                        vibrator.Call("vibrate", effect);
                    }
                }
            }
            catch { }
#endif
        }

        public static void TriggerImpact(float intensity)
        {
            OnHapticEvent?.Invoke($"IMPACT_{intensity:F2}");

#if UNITY_IOS && !UNITY_EDITOR
            int style = (intensity > 0.6f) ? 2 : (intensity > 0.3f ? 1 : 0);
            _triggerImpactFeedback(style);
#endif
        }
    }
}
