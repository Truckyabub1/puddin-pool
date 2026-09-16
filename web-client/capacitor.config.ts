import { CapacitorConfig } from "@capacitor/cli";

const config: CapacitorConfig = {
  appId: "com.puddinpool.game",
  appName: "Puddin's Pool",
  webDir: "dist",
  server: {
    androidScheme: "https",
  },
  plugins: {
    SplashScreen: {
      launchShowDuration: 1500,
      backgroundColor: "#0f141c",
    },
    StatusBar: {
      style: "DARK",
      overlaysWebView: true,
    },
  },
};

export default config;
