using System;
using System.Collections.Generic;
using PuddinPool.Native;

namespace PuddinPool.Rendering
{
    public struct ShadowQuad
    {
        public float x;
        public float y;
        public float radius;
        public float alpha;
    }

    public class ContactShadowRenderer
    {
        public List<ShadowQuad> ActiveShadows { get; private set; }
        public float BaseShadowRadius { get; set; } = 0.032f; // Slightly larger than ball radius (0.028575m)
        public float BaseAlpha { get; set; } = 0.65f;

        public ContactShadowRenderer()
        {
            ActiveShadows = new List<ShadowQuad>();
        }

        public void UpdateShadows(BallTransform[] transforms, int count)
        {
            ActiveShadows.Clear();
            for (int i = 0; i < count; ++i)
            {
                if (transforms[i].is_pocketed) continue;

                ActiveShadows.Add(new ShadowQuad
                {
                    x = transforms[i].x,
                    y = transforms[i].y,
                    radius = BaseShadowRadius,
                    alpha = BaseAlpha
                });
            }
        }
    }
}
